/*
 *
 *   Bacula Director -- mac.c -- responsible for doing
 *     migration, archive, and copy jobs.
 *
 *     Kern Sibbald, September MMIV
 *
 *  Basic tasks done here:
 *     Open DB and create records for this job.
 *     Open Message Channel with Storage daemon to tell him a job will be starting.
 *     Open connection with File daemon and pass him commands
 *       to do the backup.
 *     When the File daemon finishes the job, update the DB.
 *
 *   Version $Id: mac.c,v 1.10.2.2 2006/03/14 21:41:40 kerns Exp $
 */
/*
   Copyright (C) 2004-2005 Kern Sibbald

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
#include "ua.h"

/* 
 * Called here before the job is run to do the job
 *   specific setup.
 */
bool do_mac_init(JCR *jcr)
{
   POOL_DBR pr;
   JOB_DBR jr;
   JobId_t input_jobid;
   char *Name;
   RESTORE_CTX rx;
   UAContext *ua;
   const char *Type;

   switch(jcr->JobType) {
   case JT_MIGRATE:
      Type = "Migration";
      break;
   case JT_ARCHIVE:
      Type = "Archive";
      break;
   case JT_COPY:
      Type = "Copy";
      break;
   default:
      Type = "Unknown";
      break;
   }

   if (!get_or_create_fileset_record(jcr)) {
      return false;
   }

   /*
    * Find JobId of last job that ran.
    */
   memcpy(&jr, &jcr->jr, sizeof(jr));
   Name = jcr->job->migration_job->hdr.name;
   Dmsg1(100, "find last jobid for: %s\n", NPRT(Name));
   if (!db_find_last_jobid(jcr, jcr->db, Name, &jr)) {
      Jmsg(jcr, M_FATAL, 0, _(
           _("Unable to find JobId of previous Job for this client.\n")));
      return false;
   }
   input_jobid = jr.JobId;
   Dmsg1(100, "Last jobid=%d\n", input_jobid);

   jcr->previous_jr.JobId = input_jobid;
   if (!db_get_job_record(jcr, jcr->db, &jcr->previous_jr)) {
      Jmsg(jcr, M_FATAL, 0, _("Could not get job record for previous Job. ERR=%s"),
           db_strerror(jcr->db));
      return false;
   }
   if (jcr->previous_jr.JobStatus != 'T') {
      Jmsg(jcr, M_FATAL, 0, _("Last Job %d did not terminate normally. JobStatus=%c\n"),
         input_jobid, jcr->previous_jr.JobStatus);
      return false;
   }
   Jmsg(jcr, M_INFO, 0, _("%s using JobId=%d Job=%s\n"),
      Type, jcr->previous_jr.JobId, jcr->previous_jr.Job);


   /*
    * Get the Pool record -- first apply any level defined pools
    */
   switch (jcr->JobLevel) {
   case L_FULL:
      if (jcr->full_pool) {
         jcr->pool = jcr->full_pool;
      }
      break;
   case L_INCREMENTAL:
      if (jcr->inc_pool) {
         jcr->pool = jcr->inc_pool;
      }
      break;
   case L_DIFFERENTIAL:
      if (jcr->dif_pool) {
         jcr->pool = jcr->dif_pool;
      }
      break;
   }
   memset(&pr, 0, sizeof(pr));
   bstrncpy(pr.Name, jcr->pool->hdr.name, sizeof(pr.Name));

   while (!db_get_pool_record(jcr, jcr->db, &pr)) { /* get by Name */
      /* Try to create the pool */
      if (create_pool(jcr, jcr->db, jcr->pool, POOL_OP_CREATE) < 0) {
         Jmsg(jcr, M_FATAL, 0, _("Pool %s not in database. %s"), pr.Name,
            db_strerror(jcr->db));
         return false;
      } else {
         Jmsg(jcr, M_INFO, 0, _("Pool %s created in database.\n"), pr.Name);
      }
   }
   jcr->PoolId = pr.PoolId;               /****FIXME**** this can go away */
   jcr->jr.PoolId = pr.PoolId;

   memset(&rx, 0, sizeof(rx));
   rx.bsr = new_bsr();
   rx.JobIds = "";                       
   rx.bsr->JobId = jcr->previous_jr.JobId;
   ua = new_ua_context(jcr);
   complete_bsr(ua, rx.bsr);
   rx.bsr->fi = new_findex();
   rx.bsr->fi->findex = 1;
   rx.bsr->fi->findex2 = jcr->previous_jr.JobFiles;
   jcr->ExpectedFiles = write_bsr_file(ua, rx);
   if (jcr->ExpectedFiles == 0) {
      free_ua_context(ua);
      free_bsr(rx.bsr);
      return false;
   }
   free_ua_context(ua);
   free_bsr(rx.bsr);

   jcr->needs_sd = true;
   return true;
}

/*
 * Do a Migration, Archive, or Copy of a previous job
 *
 *  Returns:  false on failure
 *            true  on success
 */
bool do_mac(JCR *jcr)
{
   int stat;
   const char *Type;
   char ed1[100];

   switch(jcr->JobType) {
   case JT_MIGRATE:
      Type = "Migration";
      break;
   case JT_ARCHIVE:
      Type = "Archive";
      break;
   case JT_COPY:
      Type = "Copy";
      break;
   default:
      Type = "Unknown";
      break;
   }


   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start %s JobId %s, Job=%s\n"),
        Type, edit_uint64(jcr->JobId, ed1), jcr->Job);

   set_jcr_job_status(jcr, JS_Running);
   Dmsg2(100, "JobId=%d JobLevel=%c\n", jcr->jr.JobId, jcr->jr.JobLevel);
   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      return false;
   }

   /*
    * Open a message channel connection with the Storage
    * daemon. This is to let him know that our client
    * will be contacting him for a backup  session.
    *
    */
   Dmsg0(110, "Open connection with storage daemon\n");
   set_jcr_job_status(jcr, JS_WaitSD);
   /*
    * Start conversation with Storage daemon
    */
   if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
      return false;
   }
   /*
    * Now start a job with the Storage daemon
    */
   if (!start_storage_daemon_job(jcr, jcr->storage, jcr->storage)) {
      return false;
   }
   /*
    * Now start a Storage daemon message thread
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      return false;
   }
   Dmsg0(150, "Storage daemon connection OK\n");

   /* Pickup Job termination data */
   set_jcr_job_status(jcr, JS_Running);

   /* Note, the SD stores in jcr->JobFiles/ReadBytes/JobBytes/Errors */
   wait_for_storage_daemon_termination(jcr);

   if (jcr->JobStatus != JS_Terminated) {
      stat = jcr->JobStatus;
   } else {
      stat = jcr->SDJobStatus;
   }
   if (stat == JS_Terminated) {
      mac_cleanup(jcr, stat);
      return true;
   }
   return false;
}


/*
 * Release resources allocated during backup.
 */
void mac_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50];
   char ec1[30], ec2[30], ec3[30], ec4[30], ec5[30], compress[50];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   MEDIA_DBR mr;
   double kbps, compression;
   utime_t RunTime;
   const char *Type;

   switch(jcr->JobType) {
   case JT_MIGRATE:
      Type = "Migration";
      break;
   case JT_ARCHIVE:
      Type = "Archive";
      break;
   case JT_COPY:
      Type = "Copy";
      break;
   default:
      Type = "Unknown";
      break;
   }

   Dmsg2(100, "Enter mac_cleanup %d %c\n", TermCode, TermCode);
   dequeue_messages(jcr);             /* display any queued messages */
   memset(&mr, 0, sizeof(mr));
   set_jcr_job_status(jcr, TermCode);

   update_job_end_record(jcr);        /* update database */

   if (!db_get_job_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting job record for stats: %s"),
         db_strerror(jcr->db));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   bstrncpy(mr.VolumeName, jcr->VolumeName, sizeof(mr.VolumeName));
   if (!db_get_media_record(jcr, jcr->db, &mr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting Media record for Volume \"%s\": ERR=%s"),
         mr.VolumeName, db_strerror(jcr->db));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   /* Now update the bootstrap file if any */
   if (jcr->JobStatus == JS_Terminated && jcr->jr.JobBytes &&
       jcr->job->WriteBootstrap) {
      FILE *fd;
      BPIPE *bpipe = NULL;
      int got_pipe = 0;
      char *fname = jcr->job->WriteBootstrap;
      VOL_PARAMS *VolParams = NULL;
      int VolCount;

      if (*fname == '|') {
         fname++;
         got_pipe = 1;
         bpipe = open_bpipe(fname, 0, "w");
         fd = bpipe ? bpipe->wfd : NULL;
      } else {
         /* ***FIXME*** handle BASE */
         fd = fopen(fname, jcr->JobLevel==L_FULL?"w+":"a+");
      }
      if (fd) {
         VolCount = db_get_job_volume_parameters(jcr, jcr->db, jcr->JobId,
                    &VolParams);
         if (VolCount == 0) {
            Jmsg(jcr, M_ERROR, 0, _("Could not get Job Volume Parameters to "
                 "update Bootstrap file. ERR=%s\n"), db_strerror(jcr->db));
             if (jcr->SDJobFiles != 0) {
                set_jcr_job_status(jcr, JS_ErrorTerminated);
             }

         }
         for (int i=0; i < VolCount; i++) {
            /* Write the record */
            fprintf(fd, "Volume=\"%s\"\n", VolParams[i].VolumeName);
            fprintf(fd, "MediaType=\"%s\"\n", VolParams[i].MediaType);
            fprintf(fd, "VolSessionId=%u\n", jcr->VolSessionId);
            fprintf(fd, "VolSessionTime=%u\n", jcr->VolSessionTime);
            fprintf(fd, "VolFile=%u-%u\n", VolParams[i].StartFile,
                         VolParams[i].EndFile);
            fprintf(fd, "VolBlock=%u-%u\n", VolParams[i].StartBlock,
                         VolParams[i].EndBlock);
            fprintf(fd, "FileIndex=%d-%d\n", VolParams[i].FirstIndex,
                         VolParams[i].LastIndex);
         }
         if (VolParams) {
            free(VolParams);
         }
         if (got_pipe) {
            close_bpipe(bpipe);
         } else {
            fclose(fd);
         }
      } else {
         berrno be;
         Jmsg(jcr, M_ERROR, 0, _("Could not open WriteBootstrap file:\n"
              "%s: ERR=%s\n"), fname, be.strerror());
         set_jcr_job_status(jcr, JS_ErrorTerminated);
      }
   }

   msg_type = M_INFO;                 /* by default INFO message */
   switch (jcr->JobStatus) {
      case JS_Terminated:
         if (jcr->Errors || jcr->SDErrors) {
            term_msg = _("Backup OK -- with warnings");
         } else {
            term_msg = _("Backup OK");
         }
         break;
      case JS_FatalError:
      case JS_ErrorTerminated:
         term_msg = _("*** Backup Error ***");
         msg_type = M_ERROR;          /* Generate error message */
         if (jcr->store_bsock) {
            bnet_sig(jcr->store_bsock, BNET_TERMINATE);
            if (jcr->SD_msg_chan) {
               pthread_cancel(jcr->SD_msg_chan);
            }
         }
         break;
      case JS_Canceled:
         term_msg = _("Backup Canceled");
         if (jcr->store_bsock) {
            bnet_sig(jcr->store_bsock, BNET_TERMINATE);
            if (jcr->SD_msg_chan) {
               pthread_cancel(jcr->SD_msg_chan);
            }
         }
         break;
      default:
         term_msg = term_code;
         sprintf(term_code, _("Inappropriate term code: %c\n"), jcr->JobStatus);
         break;
   }
   bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);
   RunTime = jcr->jr.EndTime - jcr->jr.StartTime;
   if (RunTime <= 0) {
      kbps = 0;
   } else {
      kbps = (double)jcr->jr.JobBytes / (1000 * RunTime);
   }
   if (!db_get_job_volume_names(jcr, jcr->db, jcr->jr.JobId, &jcr->VolumeName)) {
      /*
       * Note, if the job has erred, most likely it did not write any
       *  tape, so suppress this "error" message since in that case
       *  it is normal.  Or look at it the other way, only for a
       *  normal exit should we complain about this error.
       */
      if (jcr->JobStatus == JS_Terminated && jcr->jr.JobBytes) {
         Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
      }
      jcr->VolumeName[0] = 0;         /* none */
   }

   if (jcr->ReadBytes == 0) {
      bstrncpy(compress, "None", sizeof(compress));
   } else {
      compression = (double)100 - 100.0 * ((double)jcr->JobBytes / (double)jcr->ReadBytes);
      if (compression < 0.5) {
         bstrncpy(compress, "None", sizeof(compress));
      } else {
         bsnprintf(compress, sizeof(compress), "%.1f %%", (float)compression);
      }
   }
   jobstatus_to_ascii(jcr->FDJobStatus, fd_term_msg, sizeof(fd_term_msg));
   jobstatus_to_ascii(jcr->SDJobStatus, sd_term_msg, sizeof(sd_term_msg));

// bmicrosleep(15, 0);                /* for debugging SIGHUP */

   Jmsg(jcr, msg_type, 0, _("Bacula %s (%s): %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  Backup Level:           %s%s\n"
"  Client:                 %s\n"
"  FileSet:                \"%s\" %s\n"
"  Pool:                   \"%s\"\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  FD Files Written:       %s\n"
"  SD Files Written:       %s\n"
"  FD Bytes Written:       %s\n"
"  SD Bytes Written:       %s\n"
"  Rate:                   %.1f KB/s\n"
"  Software Compression:   %s\n"
"  Volume name(s):         %s\n"
"  Volume Session Id:      %d\n"
"  Volume Session Time:    %d\n"
"  Last Volume Bytes:      %s\n"
"  Non-fatal FD errors:    %d\n"
"  SD Errors:              %d\n"
"  FD termination status:  %s\n"
"  SD termination status:  %s\n"
"  Termination:            %s\n\n"),
   VERSION,
   LSMDATE,
        edt,
        jcr->jr.JobId,
        jcr->jr.Job,
        level_to_str(jcr->JobLevel), jcr->since,
        jcr->client->hdr.name,
        jcr->fileset->hdr.name, jcr->FSCreateTime,
        jcr->pool->hdr.name,
        sdt,
        edt,
        edit_uint64_with_commas(jcr->jr.JobFiles, ec1),
        edit_uint64_with_commas(jcr->SDJobFiles, ec4),
        edit_uint64_with_commas(jcr->jr.JobBytes, ec2),
        edit_uint64_with_commas(jcr->SDJobBytes, ec5),
        (float)kbps,
        compress,
        jcr->VolumeName,
        jcr->VolSessionId,
        jcr->VolSessionTime,
        edit_uint64_with_commas(mr.VolBytes, ec3),
        jcr->Errors,
        jcr->SDErrors,
        fd_term_msg,
        sd_term_msg,
        term_msg);

   Dmsg0(100, "Leave mac_cleanup()\n");
}
