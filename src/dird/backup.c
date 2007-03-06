/*
 *
 *   Bacula Director -- backup.c -- responsible for doing backup jobs
 *
 *     Kern Sibbald, March MM
 *
 *  Basic tasks done here:
 *     Open DB and create records for this job.
 *     Open Message Channel with Storage daemon to tell him a job will be starting.
 *     Open connection with File daemon and pass him commands
 *	 to do the backup.
 *     When the File daemon finishes the job, update the DB.
 *
 *   Version $Id: backup.c,v 1.76 2004/11/21 08:53:21 kerns Exp $
 */

/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "dird.h"
#include "ua.h"

/* Commands sent to File daemon */
static char backupcmd[] = "backup\n";
static char storaddr[]  = "storage address=%s port=%d ssl=%d\n";

/* Responses received from File daemon */
static char OKbackup[]   = "2000 OK backup\n";
static char OKstore[]    = "2000 OK storage\n";
static char EndJob[]     = "2800 End Job TermCode=%d JobFiles=%u "
                           "ReadBytes=%" lld " JobBytes=%" lld " Errors=%u\n";


/* Forward referenced functions */
static void backup_cleanup(JCR *jcr, int TermCode, char *since, FILESET_DBR *fsr);

/* External functions */

/* 
 * Do a backup of the specified FileSet
 *    
 *  Returns:  0 on failure
 *	      1 on success
 */
int do_backup(JCR *jcr) 
{
   char since[MAXSTRING];
   int stat;
   BSOCK   *fd;
   POOL_DBR pr;
   FILESET_DBR fsr;
   STORE *store;

   since[0] = 0;

   if (!get_or_create_client_record(jcr)) {
      goto bail_out;
   }

   if (!get_or_create_fileset_record(jcr, &fsr)) {
      goto bail_out;
   }

   get_level_since_time(jcr, since, sizeof(since));

   jcr->fname = get_pool_memory(PM_FNAME);

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
	 goto bail_out;
      } else {
         Jmsg(jcr, M_INFO, 0, _("Pool %s created in database.\n"), pr.Name);
      }
   }
   jcr->PoolId = pr.PoolId;		  /****FIXME**** this can go away */
   jcr->jr.PoolId = pr.PoolId;


   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Backup JobId %u, Job=%s\n"),
	jcr->JobId, jcr->Job);

   set_jcr_job_status(jcr, JS_Running);
   Dmsg2(100, "JobId=%d JobLevel=%c\n", jcr->jr.JobId, jcr->jr.JobLevel);
   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      goto bail_out;
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
      goto bail_out;
   }
   /*
    * Now start a job with the Storage daemon
    */
   if (!start_storage_daemon_job(jcr)) {
      goto bail_out;
   }
   /*
    * Now start a Storage daemon message thread
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      goto bail_out;
   }
   Dmsg0(150, "Storage daemon connection OK\n");

   set_jcr_job_status(jcr, JS_WaitFD);
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      goto bail_out;
   }

   set_jcr_job_status(jcr, JS_Running);
   fd = jcr->file_bsock;

   if (!send_include_list(jcr)) {
      goto bail_out;
   }

   if (!send_exclude_list(jcr)) {
      goto bail_out;
   }

   if (!send_level_command(jcr)) {
      goto bail_out;
   }

   /* 
    * send Storage daemon address to the File daemon
    */
   store = (STORE *)jcr->storage[0]->first();
   if (store->SDDport == 0) {
      store->SDDport = store->SDport;
   }
   bnet_fsend(fd, storaddr, store->address, store->SDDport,
	      store->enable_ssl);
   if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
      goto bail_out;
   }


   if (!send_run_before_and_after_commands(jcr)) {
      goto bail_out;
   }

   /* Send backup command */
   bnet_fsend(fd, backupcmd);
   if (!response(jcr, fd, OKbackup, "backup", DISPLAY_ERROR)) {
      goto bail_out;
   }

   /* Pickup Job termination data */	    
   stat = wait_for_job_termination(jcr);
   backup_cleanup(jcr, stat, since, &fsr);
   return 1;

bail_out:
   backup_cleanup(jcr, JS_ErrorTerminated, since, &fsr);
   return 0;
}

/*
 * Here we wait for the File daemon to signal termination,
 *   then we wait for the Storage daemon.  When both
 *   are done, we return the job status.
 * Also used by restore.c 
 */
int wait_for_job_termination(JCR *jcr)
{
   int32_t n = 0;
   BSOCK *fd = jcr->file_bsock;
   bool fd_ok = false;
   uint32_t JobFiles, Errors;
   uint64_t ReadBytes, JobBytes;

   set_jcr_job_status(jcr, JS_Running);
   /* Wait for Client to terminate */
   while ((n = bget_dirmsg(fd)) >= 0) {
      if (!fd_ok && sscanf(fd->msg, EndJob, &jcr->FDJobStatus, &JobFiles,
	  &ReadBytes, &JobBytes, &Errors) == 5) {
	 fd_ok = true;
	 set_jcr_job_status(jcr, jcr->FDJobStatus);
         Dmsg1(100, "FDStatus=%c\n", (char)jcr->JobStatus);
      } else {
         Jmsg(jcr, M_WARNING, 0, _("Unexpected Client Job message: %s\n"),
	    fd->msg);
      }
      if (job_canceled(jcr)) {
	 break;
      }
   }
   if (is_bnet_error(fd)) {
      Jmsg(jcr, M_FATAL, 0, _("Network error with FD during %s: ERR=%s\n"),
	  job_type_to_str(jcr->JobType), bnet_strerror(fd));
   }
   bnet_sig(fd, BNET_TERMINATE);   /* tell Client we are terminating */

   /* Note, the SD stores in jcr->JobFiles/ReadBytes/JobBytes/Errors */
   wait_for_storage_daemon_termination(jcr);


   /* Return values from FD */
   if (fd_ok) {
      jcr->JobFiles = JobFiles;
      jcr->Errors = Errors;
      jcr->ReadBytes = ReadBytes;
      jcr->JobBytes = JobBytes;
   } else {
      Jmsg(jcr, M_FATAL, 0, _("No Job status returned from FD.\n"));
   }

// Dmsg4(100, "fd_ok=%d FDJS=%d JS=%d SDJS=%d\n", fd_ok, jcr->FDJobStatus,
//   jcr->JobStatus, jcr->SDJobStatus);

   /* Return the first error status we find Dir, FD, or SD */
   if (!fd_ok || is_bnet_error(fd)) {			       
      jcr->FDJobStatus = JS_ErrorTerminated;
   }
   if (jcr->JobStatus != JS_Terminated) {
      return jcr->JobStatus;
   }
   if (jcr->FDJobStatus != JS_Terminated) {
      return jcr->FDJobStatus;
   }
   return jcr->SDJobStatus;
}

/*
 * Release resources allocated during backup.
 */
static void backup_cleanup(JCR *jcr, int TermCode, char *since, FILESET_DBR *fsr) 
{
   char sdt[50], edt[50];
   char ec1[30], ec2[30], ec3[30], ec4[30], ec5[30], compress[50];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   MEDIA_DBR mr;
   double kbps, compression;
   utime_t RunTime;

   Dmsg2(100, "Enter backup_cleanup %d %c\n", TermCode, TermCode);
   dequeue_messages(jcr);	      /* display any queued messages */
   memset(&mr, 0, sizeof(mr));
   set_jcr_job_status(jcr, TermCode);

   update_job_end_record(jcr);	      /* update database */
   
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

   msg_type = M_INFO;		      /* by default INFO message */
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
	 msg_type = M_ERROR;	      /* Generate error message */
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
      jcr->VolumeName[0] = 0;	      /* none */
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

// bmicrosleep(15, 0);		      /* for debugging SIGHUP */

   Jmsg(jcr, msg_type, 0, _("Bacula " VERSION " (" LSMDATE "): %s\n\
  JobId:                  %d\n\
  Job:                    %s\n\
  Backup Level:           %s%s\n\
  Client:                 %s\n\
  FileSet:                \"%s\" %s\n\
  Pool:                   \"%s\"\n\
  Storage:                \"%s\"\n\
  Start time:             %s\n\
  End time:               %s\n\
  FD Files Written:       %s\n\
  SD Files Written:       %s\n\
  FD Bytes Written:       %s\n\
  SD Bytes Written:       %s\n\
  Rate:                   %.1f KB/s\n\
  Software Compression:   %s\n\
  Volume name(s):         %s\n\
  Volume Session Id:      %d\n\
  Volume Session Time:    %d\n\
  Last Volume Bytes:      %s\n\
  Non-fatal FD errors:    %d\n\
  SD Errors:              %d\n\
  FD termination status:  %s\n\
  SD termination status:  %s\n\
  Termination:            %s\n\n"),
	edt,
	jcr->jr.JobId,
	jcr->jr.Job,
	level_to_str(jcr->JobLevel), since,
	jcr->client->hdr.name,
	jcr->fileset->hdr.name, fsr->cCreateTime,
	jcr->pool->hdr.name,
	jcr->store->hdr.name,
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

   Dmsg0(100, "Leave backup_cleanup()\n");
}
