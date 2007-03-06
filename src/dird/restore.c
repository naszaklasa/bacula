/*
 *   Bacula Director -- restore.c -- responsible for restoring files
 *
 *     Kern Sibbald, November MM
 *
 *    This routine is run as a separate thread.
 *
 * Current implementation is Catalog verification only (i.e. no
 *  verification versus tape).
 *
 *  Basic tasks done here:
 *     Open DB
 *     Open Message Channel with Storage daemon to tell him a job will be starting.
 *     Open connection with File daemon and pass him commands
 *       to do the restore.
 *     Update the DB according to what files where restored????
 *
 *   Version $Id: restore.c,v 1.56.2.5 2006/06/04 12:24:39 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */


#include "bacula.h"
#include "dird.h"

/* Commands sent to File daemon */
static char restorecmd[]   = "restore replace=%c prelinks=%d where=%s\n";
static char storaddr[]     = "storage address=%s port=%d ssl=0\n";

/* Responses received from File daemon */
static char OKrestore[]   = "2000 OK restore\n";
static char OKstore[]     = "2000 OK storage\n";

/*
 * Do a restore of the specified files
 *
 *  Returns:  0 on failure
 *            1 on success
 */
bool do_restore(JCR *jcr)
{
   BSOCK   *fd;
   JOB_DBR rjr;                       /* restore job record */

   memset(&rjr, 0, sizeof(rjr));
   jcr->jr.JobLevel = L_FULL;         /* Full restore */
   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }
   Dmsg0(20, "Updated job start record\n");

   Dmsg1(20, "RestoreJobId=%d\n", jcr->job->RestoreJobId);

   if (!jcr->RestoreBootstrap) {
      Jmsg0(jcr, M_FATAL, 0, _("Cannot restore without bootstrap file.\n"));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }


   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Restore Job %s\n"), jcr->Job);

   /*
    * Open a message channel connection with the Storage
    * daemon. This is to let him know that our client
    * will be contacting him for a backup  session.
    *
    */
   Dmsg0(10, "Open connection with storage daemon\n");
   set_jcr_job_status(jcr, JS_WaitSD);
   /*
    * Start conversation with Storage daemon
    */
   if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }
   /*
    * Now start a job with the Storage daemon
    */
   if (!start_storage_daemon_job(jcr, jcr->storage, NULL)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }
   if (!bnet_fsend(jcr->store_bsock, "run")) {
      return false;
   }
   /*
    * Now start a Storage daemon message thread
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }
   Dmsg0(50, "Storage daemon connection OK\n");


   /*
    * Start conversation with File daemon
    */
   set_jcr_job_status(jcr, JS_WaitFD);
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }

   fd = jcr->file_bsock;
   set_jcr_job_status(jcr, JS_Running);

   /*
    * send Storage daemon address to the File daemon,
    *   then wait for File daemon to make connection
    *   with Storage daemon.
    */
   if (jcr->store->SDDport == 0) {
      jcr->store->SDDport = jcr->store->SDport;
   }
   bnet_fsend(fd, storaddr, jcr->store->address, jcr->store->SDDport);
   Dmsg1(6, "dird>filed: %s\n", fd->msg);
   if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }

   /*
    * Send the bootstrap file -- what Volumes/files to restore
    */
   if (!send_bootstrap_file(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }


   if (!send_run_before_and_after_commands(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }

   /* Send restore command */
   char replace, *where;
   char empty = '\0';

   if (jcr->replace != 0) {
      replace = jcr->replace;
   } else if (jcr->job->replace != 0) {
      replace = jcr->job->replace;
   } else {
      replace = REPLACE_ALWAYS;       /* always replace */
   }
   if (jcr->where) {
      where = jcr->where;             /* override */
   } else if (jcr->job->RestoreWhere) {
      where = jcr->job->RestoreWhere; /* no override take from job */
   } else {
      where = &empty;                 /* None */
   }
   jcr->prefix_links = jcr->job->PrefixLinks;
   bash_spaces(where);
   bnet_fsend(fd, restorecmd, replace, jcr->prefix_links, where);
   unbash_spaces(where);

   if (!response(jcr, fd, OKrestore, "Restore", DISPLAY_ERROR)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return false;
   }

   /* Wait for Job Termination */
   int stat = wait_for_job_termination(jcr);
   restore_cleanup(jcr, stat);

   return true;
}

bool do_restore_init(JCR *jcr) 
{
   return true;
}

/*
 * Release resources allocated during restore.
 *
 */
void restore_cleanup(JCR *jcr, int TermCode)
{
   char sdt[MAX_TIME_LENGTH], edt[MAX_TIME_LENGTH];
   char ec1[30], ec2[30], ec3[30];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   double kbps;

   Dmsg0(20, "In restore_cleanup\n");
   dequeue_messages(jcr);             /* display any queued messages */
   set_jcr_job_status(jcr, TermCode);

   if (jcr->unlink_bsr && jcr->RestoreBootstrap) {
      unlink(jcr->RestoreBootstrap);
      jcr->unlink_bsr = false;
   }

   update_job_end_record(jcr);

   msg_type = M_INFO;                 /* by default INFO message */
   switch (TermCode) {
   case JS_Terminated:
      if (jcr->ExpectedFiles > jcr->jr.JobFiles) {
         term_msg = _("Restore OK -- warning file count mismatch");
      } else {
         term_msg = _("Restore OK");
      }
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** Restore Error ***");
      msg_type = M_ERROR;          /* Generate error message */
      if (jcr->store_bsock) {
         bnet_sig(jcr->store_bsock, BNET_TERMINATE);
         if (jcr->SD_msg_chan) {
            pthread_cancel(jcr->SD_msg_chan);
         }
      }
      break;
   case JS_Canceled:
      term_msg = _("Restore Canceled");
      if (jcr->store_bsock) {
         bnet_sig(jcr->store_bsock, BNET_TERMINATE);
         if (jcr->SD_msg_chan) {
            pthread_cancel(jcr->SD_msg_chan);
         }
      }
      break;
   default:
      term_msg = term_code;
      sprintf(term_code, _("Inappropriate term code: %c\n"), TermCode);
      break;
   }
   bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);
   if (jcr->jr.EndTime - jcr->jr.StartTime > 0) {
      kbps = (double)jcr->jr.JobBytes / (1000 * (jcr->jr.EndTime - jcr->jr.StartTime));
   } else {
      kbps = 0;
   }
   if (kbps < 0.05) {
      kbps = 0;
   }

   jobstatus_to_ascii(jcr->FDJobStatus, fd_term_msg, sizeof(fd_term_msg));
   jobstatus_to_ascii(jcr->SDJobStatus, sd_term_msg, sizeof(sd_term_msg));

   Jmsg(jcr, msg_type, 0, _("Bacula %s (%s): %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  Client:                 %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Files Expected:         %s\n"
"  Files Restored:         %s\n"
"  Bytes Restored:         %s\n"
"  Rate:                   %.1f KB/s\n"
"  FD Errors:              %d\n"
"  FD termination status:  %s\n"
"  SD termination status:  %s\n"
"  Termination:            %s\n\n"),
        VERSION,
        LSMDATE,
        edt,
        jcr->jr.JobId,
        jcr->jr.Job,
        jcr->client->hdr.name,
        sdt,
        edt,
        edit_uint64_with_commas((uint64_t)jcr->ExpectedFiles, ec1),
        edit_uint64_with_commas((uint64_t)jcr->jr.JobFiles, ec2),
        edit_uint64_with_commas(jcr->jr.JobBytes, ec3),
        (float)kbps,
        jcr->Errors,
        fd_term_msg,
        sd_term_msg,
        term_msg);

   Dmsg0(20, "Leaving restore_cleanup\n");
}
