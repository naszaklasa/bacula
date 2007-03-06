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
 *       to do the backup.
 *     When the File daemon finishes the job, update the DB.
 *
 *   Version $Id: backup.c,v 1.93.2.5 2006/05/02 14:48:14 kerns Exp $
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
#include "ua.h"

/* Commands sent to File daemon */
static char backupcmd[] = "backup\n";
static char storaddr[]  = "storage address=%s port=%d ssl=%d\n";

/* Responses received from File daemon */
static char OKbackup[]   = "2000 OK backup\n";
static char OKstore[]    = "2000 OK storage\n";
static char EndJob[]     = "2800 End Job TermCode=%d JobFiles=%u "
                           "ReadBytes=%lld JobBytes=%lld Errors=%u\n";

/* 
 * Called here before the job is run to do the job
 *   specific setup.
 */
bool do_backup_init(JCR *jcr)
{
   POOL_DBR pr;

   if (!get_or_create_fileset_record(jcr)) {
      return false;
   }

   /* 
    * Get definitive Job level and since time
    */
   get_level_since_time(jcr, jcr->since, sizeof(jcr->since));

   /*
    * Apply any level related Pool selections
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

   if (!db_get_pool_record(jcr, jcr->db, &pr)) { /* get by Name */
      /* Try to create the pool */
      if (create_pool(jcr, jcr->db, jcr->pool, POOL_OP_CREATE) < 0) {
         Jmsg(jcr, M_FATAL, 0, _("Pool %s not in database. %s"), pr.Name,
            db_strerror(jcr->db));
         return false;
      } else {
         Jmsg(jcr, M_INFO, 0, _("Pool %s created in database.\n"), pr.Name);
         if (!db_get_pool_record(jcr, jcr->db, &pr)) { /* get by Name */
            Jmsg(jcr, M_FATAL, 0, _("Pool %s not in database. %s"), pr.Name,
               db_strerror(jcr->db));
            return false;
         }
      }
   }
   jcr->PoolId = pr.PoolId;
   jcr->jr.PoolId = pr.PoolId;

   /*
    * Fire off any clone jobs (run directives)
    */
   Dmsg2(900, "cloned=%d run_cmds=%p\n", jcr->cloned, jcr->job->run_cmds);
   if (!jcr->cloned && jcr->job->run_cmds) {
      char *runcmd;
      JOB *job = jcr->job;
      POOLMEM *cmd = get_pool_memory(PM_FNAME);
      UAContext *ua = new_ua_context(jcr);
      ua->batch = true;
      foreach_alist(runcmd, job->run_cmds) {
         cmd = edit_job_codes(jcr, cmd, runcmd, "");              
         Mmsg(ua->cmd, "run %s cloned=yes", cmd);
         Dmsg1(900, "=============== Clone cmd=%s\n", ua->cmd);
         parse_ua_args(ua);                 /* parse command */
         int stat = run_cmd(ua, ua->cmd);
         if (stat == 0) {
            Jmsg(jcr, M_ERROR, 0, _("Could not start clone job.\n"));
         } else {
            Jmsg(jcr, M_INFO, 0, _("Clone JobId %d started.\n"), stat);
         }
      }
      free_ua_context(ua);
      free_pool_memory(cmd);
   }

   return true;
}

/*
 * Do a backup of the specified FileSet
 *
 *  Returns:  false on failure
 *            true  on success
 */
bool do_backup(JCR *jcr)
{
   int stat;
   int tls_need = BNET_TLS_NONE;
   BSOCK   *fd;
   STORE *store;
   char ed1[100];


   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Backup JobId %s, Job=%s\n"),
        edit_uint64(jcr->JobId, ed1), jcr->Job);

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
   if (!start_storage_daemon_job(jcr, NULL, jcr->storage)) {
      return false;
   }
   /*
    * Now start a Storage daemon message thread.  Note,
    *   this thread is used to provide the catalog services
    *   for the backup job, including inserting the attributes
    *   into the catalog.  See catalog_update() in catreq.c
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      return false;
   }
   Dmsg0(150, "Storage daemon connection OK\n");

   if (!bnet_fsend(jcr->store_bsock, "run")) {
      goto bail_out;
   }

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
   store = jcr->store;
   if (store->SDDport == 0) {
      store->SDDport = store->SDport;
   }

   /* TLS Requirement */
   if (store->tls_enable) {
      if (store->tls_require) {
         tls_need = BNET_TLS_REQUIRED;
      } else {
         tls_need = BNET_TLS_OK;
      }
   }

   bnet_fsend(fd, storaddr, store->address, store->SDDport,
              tls_need);
   if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
      goto bail_out;
   }


   if (!send_run_before_and_after_commands(jcr)) {
      goto bail_out;
   }

   /*    
    * We re-update the job start record so that the start
    *  time is set after the run before job.  This avoids 
    *  that any files created by the run before job will
    *  be saved twice.  They will be backed up in the current
    *  job, but not in the next one unless they are changed.
    *  Without this, they will be backed up in this job and
    *  in the next job run because in that case, their date 
    *   is after the start of this run.
    */
   jcr->start_time = time(NULL);
   jcr->jr.StartTime = jcr->start_time;
   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
   }

   /* Send backup command */
   bnet_fsend(fd, backupcmd);
   if (!response(jcr, fd, OKbackup, "backup", DISPLAY_ERROR)) {
      goto bail_out;
   }

   /* Pickup Job termination data */
   stat = wait_for_job_termination(jcr);
   if (stat == JS_Terminated) {
      backup_cleanup(jcr, stat);
      return true;
   }     
   return false;

/* Come here only after starting SD thread */
bail_out:
   set_jcr_job_status(jcr, JS_ErrorTerminated);
   Dmsg1(400, "wait for sd. use=%d\n", jcr->use_count());
   wait_for_storage_daemon_termination(jcr);
   Dmsg1(400, "after wait for sd. use=%d\n", jcr->use_count());
   return false;
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
void backup_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50], schedt[50];
   char ec1[30], ec2[30], ec3[30], ec4[30], ec5[30], compress[50];
   char ec6[30], ec7[30], ec8[30], elapsed[50];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   MEDIA_DBR mr;
   CLIENT_DBR cr;
   double kbps, compression;
   utime_t RunTime;

   Dmsg2(100, "Enter backup_cleanup %d %c\n", TermCode, TermCode);
   dequeue_messages(jcr);             /* display any queued messages */
   memset(&mr, 0, sizeof(mr));
   memset(&cr, 0, sizeof(cr));
   set_jcr_job_status(jcr, TermCode);

   update_job_end_record(jcr);        /* update database */

   if (!db_get_job_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting job record for stats: %s"),
         db_strerror(jcr->db));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   bstrncpy(cr.Name, jcr->client->hdr.name, sizeof(cr.Name));
   if (!db_get_client_record(jcr, jcr->db, &cr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting client record for stats: %s"),
         db_strerror(jcr->db));
   }

   bstrncpy(mr.VolumeName, jcr->VolumeName, sizeof(mr.VolumeName));
   if (!db_get_media_record(jcr, jcr->db, &mr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting Media record for Volume \"%s\": ERR=%s"),
         mr.VolumeName, db_strerror(jcr->db));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   update_bootstrap_file(jcr);

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
   bstrftimes(schedt, sizeof(schedt), jcr->jr.SchedTime);
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
"  Client:                 \"%s\" %s\n"
"  FileSet:                \"%s\" %s\n"
"  Pool:                   \"%s\"\n"
"  Storage:                \"%s\"\n"
"  Scheduled time:         %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Elapsed time:           %s\n"
"  Priority:               %d\n"
"  FD Files Written:       %s\n"
"  SD Files Written:       %s\n"
"  FD Bytes Written:       %s (%sB)\n"
"  SD Bytes Written:       %s (%sB)\n"
"  Rate:                   %.1f KB/s\n"
"  Software Compression:   %s\n"
"  Volume name(s):         %s\n"
"  Volume Session Id:      %d\n"
"  Volume Session Time:    %d\n"
"  Last Volume Bytes:      %s (%sB)\n"
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
        jcr->client->hdr.name, cr.Uname,
        jcr->fileset->hdr.name, jcr->FSCreateTime,
        jcr->pool->hdr.name,
        jcr->store->hdr.name,
        schedt,
        sdt,
        edt,
        edit_utime(RunTime, elapsed, sizeof(elapsed)),
        jcr->JobPriority,
        edit_uint64_with_commas(jcr->jr.JobFiles, ec1),
        edit_uint64_with_commas(jcr->SDJobFiles, ec2),
        edit_uint64_with_commas(jcr->jr.JobBytes, ec3),
        edit_uint64_with_suffix(jcr->jr.JobBytes, ec4),
        edit_uint64_with_commas(jcr->SDJobBytes, ec5),
        edit_uint64_with_suffix(jcr->SDJobBytes, ec6),
        (float)kbps,
        compress,
        jcr->VolumeName,
        jcr->VolSessionId,
        jcr->VolSessionTime,
        edit_uint64_with_commas(mr.VolBytes, ec7),
        edit_uint64_with_suffix(mr.VolBytes, ec8),
        jcr->Errors,
        jcr->SDErrors,
        fd_term_msg,
        sd_term_msg,
        term_msg);

   Dmsg0(100, "Leave backup_cleanup()\n");
}

void update_bootstrap_file(JCR *jcr)
{
   /* Now update the bootstrap file if any */
   if (jcr->JobStatus == JS_Terminated && jcr->jr.JobBytes &&
       jcr->job->WriteBootstrap) {
      FILE *fd;
      BPIPE *bpipe = NULL;
      int got_pipe = 0;
      char *fname = jcr->job->WriteBootstrap;
      VOL_PARAMS *VolParams = NULL;
      int VolCount;
      char edt[50];

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
         /* Start output with when and who wrote it */
         bstrftimes(edt, sizeof(edt), time(NULL));
         fprintf(fd, "# %s - %s - %s%s\n", edt, jcr->jr.Job,
                 level_to_str(jcr->JobLevel), jcr->since);
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
}
