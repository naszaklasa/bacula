/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula Director Job processing routines
 *
 *     Kern Sibbald, October MM
 *
 *    Version $Id: job.c 7744 2008-10-10 10:58:59Z ricozz $
 */

#include "bacula.h"
#include "dird.h"

/* Forward referenced subroutines */
static void *job_thread(void *arg);
static void job_monitor_watchdog(watchdog_t *self);
static void job_monitor_destructor(watchdog_t *self);
static bool job_check_maxwaittime(JCR *control_jcr, JCR *jcr);
static bool job_check_maxruntime(JCR *control_jcr, JCR *jcr);

/* Imported subroutines */
extern void term_scheduler();
extern void term_ua_server();

/* Imported variables */

jobq_t job_queue;

void init_job_server(int max_workers)
{
   int stat;
   watchdog_t *wd;

   if ((stat = jobq_init(&job_queue, max_workers, job_thread)) != 0) {
      berrno be;
      Emsg1(M_ABORT, 0, _("Could not init job queue: ERR=%s\n"), be.bstrerror(stat));
   }
   wd = new_watchdog();
   wd->callback = job_monitor_watchdog;
   wd->destructor = job_monitor_destructor;
   wd->one_shot = false;
   wd->interval = 60;
   wd->data = new_control_jcr("*JobMonitor*", JT_SYSTEM);
   register_watchdog(wd);
}

void term_job_server()
{
   jobq_destroy(&job_queue);          /* ignore any errors */
}

/*
 * Run a job -- typically called by the scheduler, but may also
 *              be called by the UA (Console program).
 *
 *  Returns: 0 on failure
 *           JobId on success
 *
 */
JobId_t run_job(JCR *jcr)
{
   int stat;
   if (setup_job(jcr)) {
      Dmsg0(200, "Add jrc to work queue\n");
      /* Queue the job to be run */
      if ((stat = jobq_add(&job_queue, jcr)) != 0) {
         berrno be;
         Jmsg(jcr, M_FATAL, 0, _("Could not add job queue: ERR=%s\n"), be.bstrerror(stat));
         return 0;
      }
      return jcr->JobId;
   }
   return 0;
}            

bool setup_job(JCR *jcr) 
{
   int errstat;

   jcr->lock();
   sm_check(__FILE__, __LINE__, true);
   init_msg(jcr, jcr->messages);

   /* Initialize termination condition variable */
   if ((errstat = pthread_cond_init(&jcr->term_wait, NULL)) != 0) {
      berrno be;
      Jmsg1(jcr, M_FATAL, 0, _("Unable to init job cond variable: ERR=%s\n"), be.bstrerror(errstat));
      goto bail_out;
   }
   jcr->term_wait_inited = true;

   create_unique_job_name(jcr, jcr->job->name());
   set_jcr_job_status(jcr, JS_Created);
   jcr->unlock();

   /*
    * Open database
    */
   Dmsg0(100, "Open database\n");
   jcr->db=db_init_database(jcr, jcr->catalog->db_name, jcr->catalog->db_user,
                            jcr->catalog->db_password, jcr->catalog->db_address,
                            jcr->catalog->db_port, jcr->catalog->db_socket,
                            jcr->catalog->mult_db_connections);
   if (!jcr->db || !db_open_database(jcr, jcr->db)) {
      Jmsg(jcr, M_FATAL, 0, _("Could not open database \"%s\".\n"),
                 jcr->catalog->db_name);
      if (jcr->db) {
         Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
         db_close_database(jcr, jcr->db);
      }
      goto bail_out;
   }
   Dmsg0(150, "DB opened\n");

   if (!jcr->fname) {
      jcr->fname = get_pool_memory(PM_FNAME);
   }
   if (!jcr->pool_source) {
      jcr->pool_source = get_pool_memory(PM_MESSAGE);
      pm_strcpy(jcr->pool_source, _("unknown source"));
   }
   Dmsg2(500, "pool=%s (From %s)\n", jcr->pool->name(), jcr->pool_source);
   if (jcr->JobType == JT_MIGRATE) {
      if (!jcr->rpool_source) {
         jcr->rpool_source = get_pool_memory(PM_MESSAGE);
         pm_strcpy(jcr->rpool_source, _("unknown source"));
      }
   }

   /*
    * Create Job record
    */
   init_jcr_job_record(jcr);
   if (!get_or_create_client_record(jcr)) {
      goto bail_out;
   }

   if (!db_create_job_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      goto bail_out;
   }
   jcr->JobId = jcr->jr.JobId;
   Dmsg4(100, "Created job record JobId=%d Name=%s Type=%c Level=%c\n",
       jcr->JobId, jcr->Job, jcr->jr.JobType, jcr->jr.JobLevel);

   generate_daemon_event(jcr, "JobStart");

   if (job_canceled(jcr)) {
      goto bail_out;
   }

   /*
    * Now, do pre-run stuff, like setting job level (Inc/diff, ...)
    *  this allows us to setup a proper job start record for restarting
    *  in case of later errors.
    */
   switch (jcr->JobType) {
   case JT_BACKUP:
      if (!do_backup_init(jcr)) {
         backup_cleanup(jcr, JS_ErrorTerminated);
      }
      break;
   case JT_VERIFY:
      if (!do_verify_init(jcr)) {
         verify_cleanup(jcr, JS_ErrorTerminated);
      }
      break;
   case JT_RESTORE:
      if (!do_restore_init(jcr)) {
         restore_cleanup(jcr, JS_ErrorTerminated);
      }
      break;
   case JT_ADMIN:
      if (!do_admin_init(jcr)) {
         admin_cleanup(jcr, JS_ErrorTerminated);
      }
      break;
   case JT_MIGRATE:
      if (!do_migration_init(jcr)) { 
         migration_cleanup(jcr, JS_ErrorTerminated);
      }
      break;
   default:
      Pmsg1(0, _("Unimplemented job type: %d\n"), jcr->JobType);
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      break;
   }

   generate_job_event(jcr, "JobInit");
   Dsm_check(1);
   return true;

bail_out:
   return false;
}

void update_job_end(JCR *jcr, int TermCode)
{
   dequeue_messages(jcr);             /* display any queued messages */
   set_jcr_job_status(jcr, TermCode);
   update_job_end_record(jcr);
}

/*
 * This is the engine called by jobq.c:jobq_add() when we were pulled
 *  from the work queue.
 *  At this point, we are running in our own thread and all
 *    necessary resources are allocated -- see jobq.c
 */
static void *job_thread(void *arg)
{
   JCR *jcr = (JCR *)arg;

   pthread_detach(pthread_self());
   Dsm_check(1);

   Dmsg0(200, "=====Start Job=========\n");
   set_jcr_job_status(jcr, JS_Running);   /* this will be set only if no error */
   jcr->start_time = time(NULL);      /* set the real start time */
   jcr->jr.StartTime = jcr->start_time;

   if (jcr->job->MaxStartDelay != 0 && jcr->job->MaxStartDelay <
       (utime_t)(jcr->start_time - jcr->sched_time)) {
      set_jcr_job_status(jcr, JS_Canceled);
      Jmsg(jcr, M_FATAL, 0, _("Job canceled because max start delay time exceeded.\n"));
   }

   /* TODO : check if it is used somewhere */
   if (jcr->job->RunScripts == NULL) {
      Dmsg0(200, "Warning, job->RunScripts is empty\n");
      jcr->job->RunScripts = New(alist(10, not_owned_by_alist));
   }

   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
   }

   /* Run any script BeforeJob on dird */
   run_scripts(jcr, jcr->job->RunScripts, "BeforeJob");

   if (job_canceled(jcr)) {
      update_job_end(jcr, jcr->JobStatus);

   } else {
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
      generate_job_event(jcr, "JobRun");

      switch (jcr->JobType) {
      case JT_BACKUP:
         if (do_backup(jcr)) {
            do_autoprune(jcr);
         } else {
            backup_cleanup(jcr, JS_ErrorTerminated);
         }
         break;
      case JT_VERIFY:
         if (do_verify(jcr)) {
            do_autoprune(jcr);
         } else {
            verify_cleanup(jcr, JS_ErrorTerminated);
         }
         break;
      case JT_RESTORE:
         if (do_restore(jcr)) {
            do_autoprune(jcr);
         } else {
            restore_cleanup(jcr, JS_ErrorTerminated);
         }
         break;
      case JT_ADMIN:
         if (do_admin(jcr)) {
            do_autoprune(jcr);
         } else {
            admin_cleanup(jcr, JS_ErrorTerminated);
         }
         break;
      case JT_MIGRATE:
      case JT_COPY:
      case JT_ARCHIVE:
         if (do_migration(jcr)) {
            do_autoprune(jcr);
         } else {
            migration_cleanup(jcr, JS_ErrorTerminated);
         }
         break;
      default:
         Pmsg1(0, _("Unimplemented job type: %d\n"), jcr->JobType);
         break;
      }
   }

   run_scripts(jcr, jcr->job->RunScripts, "AfterJob");

   /* Send off any queued messages */
   if (jcr->msg_queue && jcr->msg_queue->size() > 0) {
      dequeue_messages(jcr);
   }

   generate_daemon_event(jcr, "JobEnd");
   Dmsg1(50, "======== End Job stat=%c ==========\n", jcr->JobStatus);
   sm_check(__FILE__, __LINE__, true);
   return NULL;
}


/*
 * Cancel a job -- typically called by the UA (Console program), but may also
 *              be called by the job watchdog.
 *
 *  Returns: true  if cancel appears to be successful
 *           false on failure. Message sent to ua->jcr.
 */
bool cancel_job(UAContext *ua, JCR *jcr)
{
   BSOCK *sd, *fd;
   char ed1[50];
   int32_t old_status = jcr->JobStatus;

   set_jcr_job_status(jcr, JS_Canceled);

   switch (old_status) {
   case JS_Created:
   case JS_WaitJobRes:
   case JS_WaitClientRes:
   case JS_WaitStoreRes:
   case JS_WaitPriority:
   case JS_WaitMaxJobs:
   case JS_WaitStartTime:
      ua->info_msg(_("JobId %s, Job %s marked to be canceled.\n"),
              edit_uint64(jcr->JobId, ed1), jcr->Job);
      jobq_remove(&job_queue, jcr); /* attempt to remove it from queue */
      return true;

   default:
      /* Cancel File daemon */
      if (jcr->file_bsock) {
         ua->jcr->client = jcr->client;
         if (!connect_to_file_daemon(ua->jcr, 10, FDConnectTimeout, 1)) {
            ua->error_msg(_("Failed to connect to File daemon.\n"));
            return 0;
         }
         Dmsg0(200, "Connected to file daemon\n");
         fd = ua->jcr->file_bsock;
         bnet_fsend(fd, "cancel Job=%s\n", jcr->Job);
         while (bnet_recv(fd) >= 0) {
            ua->send_msg("%s", fd->msg);
         }
         bnet_sig(fd, BNET_TERMINATE);
         bnet_close(fd);
         ua->jcr->file_bsock = NULL;
      }

      /* Cancel Storage daemon */
      if (jcr->store_bsock) {
         if (!ua->jcr->wstorage) {
            if (jcr->rstorage) {
               copy_wstorage(ua->jcr, jcr->rstorage, _("Job resource")); 
            } else {
               copy_wstorage(ua->jcr, jcr->wstorage, _("Job resource")); 
            }
         } else {
            USTORE store;
            if (jcr->rstorage) {
               store.store = jcr->rstore;
            } else {
               store.store = jcr->wstore;
            }
            set_wstorage(ua->jcr, &store);
         }

         if (!connect_to_storage_daemon(ua->jcr, 10, SDConnectTimeout, 1)) {
            ua->error_msg(_("Failed to connect to Storage daemon.\n"));
            return false;
         }
         Dmsg0(200, "Connected to storage daemon\n");
         sd = ua->jcr->store_bsock;
         sd->fsend("cancel Job=%s\n", jcr->Job);
         while (sd->recv() >= 0) {
            ua->send_msg("%s", sd->msg);
         }
         sd->signal(BNET_TERMINATE);
         sd->close();
         ua->jcr->store_bsock = NULL;
      }
   }

   return true;
}

void cancel_storage_daemon_job(JCR *jcr)
{
   UAContext *ua = new_ua_context(jcr);
   JCR *control_jcr = new_control_jcr("*JobCancel*", JT_SYSTEM);
   BSOCK *sd;

   ua->jcr = control_jcr;
   if (jcr->store_bsock) {
      if (!ua->jcr->wstorage) {
         if (jcr->rstorage) {
            copy_wstorage(ua->jcr, jcr->rstorage, _("Job resource")); 
         } else {
            copy_wstorage(ua->jcr, jcr->wstorage, _("Job resource")); 
         }
      } else {
         USTORE store;
         if (jcr->rstorage) {
            store.store = jcr->rstore;
         } else {
            store.store = jcr->wstore;
         }
         set_wstorage(ua->jcr, &store);
      }

      if (!connect_to_storage_daemon(ua->jcr, 10, SDConnectTimeout, 1)) {
         goto bail_out;
      }
      Dmsg0(200, "Connected to storage daemon\n");
      sd = ua->jcr->store_bsock;
      sd->fsend("cancel Job=%s\n", jcr->Job);
      while (sd->recv() >= 0) {
      }
      sd->signal(BNET_TERMINATE);
      sd->close();
      ua->jcr->store_bsock = NULL;
   }
bail_out:
   free_jcr(control_jcr);
   free_ua_context(ua);
}

static void job_monitor_destructor(watchdog_t *self)
{
   JCR *control_jcr = (JCR *)self->data;

   free_jcr(control_jcr);
}

static void job_monitor_watchdog(watchdog_t *self)
{
   JCR *control_jcr, *jcr;

   control_jcr = (JCR *)self->data;

   Dsm_check(1);
   Dmsg1(800, "job_monitor_watchdog %p called\n", self);

   foreach_jcr(jcr) {
      bool cancel = false;

      if (jcr->JobId == 0 || job_canceled(jcr)) {
         Dmsg2(800, "Skipping JCR=%p Job=%s\n", jcr, jcr->Job);
         continue;
      }

      /* check MaxWaitTime */
      if (job_check_maxwaittime(control_jcr, jcr)) {
         set_jcr_job_status(jcr, JS_Canceled);
         Qmsg(jcr, M_FATAL, 0, _("Max wait time exceeded. Job canceled.\n"));
         cancel = true;
      /* check MaxRunTime */
      } else if (job_check_maxruntime(control_jcr, jcr)) {
         set_jcr_job_status(jcr, JS_Canceled);
         Qmsg(jcr, M_FATAL, 0, _("Max run time exceeded. Job canceled.\n"));
         cancel = true;
      }

      if (cancel) {
         Dmsg3(800, "Cancelling JCR %p jobid %d (%s)\n", jcr, jcr->JobId, jcr->Job);
         UAContext *ua = new_ua_context(jcr);
         ua->jcr = control_jcr;
         cancel_job(ua, jcr);
         free_ua_context(ua);
         Dmsg2(800, "Have cancelled JCR %p Job=%d\n", jcr, jcr->JobId);
      }

   }
   /* Keep reference counts correct */
   endeach_jcr(jcr);
}

/*
 * Check if the maxwaittime has expired and it is possible
 *  to cancel the job.
 */
static bool job_check_maxwaittime(JCR *control_jcr, JCR *jcr)
{
   bool cancel = false;
   JOB *job = jcr->job;

   if (job_canceled(jcr)) {
      return false;                /* already canceled */
   }
   if (job->MaxWaitTime == 0 && job->FullMaxWaitTime == 0 &&
       job->IncMaxWaitTime == 0 && job->DiffMaxWaitTime == 0) {
      return false;
   } 
   if (jcr->JobLevel == L_FULL && job->FullMaxWaitTime != 0 &&
         (watchdog_time - jcr->start_time) >= job->FullMaxWaitTime) {
      cancel = true;
   } else if (jcr->JobLevel == L_DIFFERENTIAL && job->DiffMaxWaitTime != 0 &&
         (watchdog_time - jcr->start_time) >= job->DiffMaxWaitTime) {
      cancel = true;
   } else if (jcr->JobLevel == L_INCREMENTAL && job->IncMaxWaitTime != 0 &&
         (watchdog_time - jcr->start_time) >= job->IncMaxWaitTime) {
      cancel = true;
   } else if (job->MaxWaitTime != 0 &&
         (watchdog_time - jcr->start_time) >= job->MaxWaitTime) {
      cancel = true;
   }

   return cancel;
}

/*
 * Check if maxruntime has expired and if the job can be
 *   canceled.
 */
static bool job_check_maxruntime(JCR *control_jcr, JCR *jcr)
{
   if (jcr->job->MaxRunTime == 0 || job_canceled(jcr) || jcr->JobStatus == JS_Created) {
      return false;
   }
   if ((watchdog_time - jcr->start_time) < jcr->job->MaxRunTime) {
      Dmsg3(200, "Job %p (%s) with MaxRunTime %d not expired\n",
            jcr, jcr->Job, jcr->job->MaxRunTime);
      return false;
   }

   return true;
}

/*
 * Get or create a Pool record with the given name.
 * Returns: 0 on error
 *          poolid if OK
 */
DBId_t get_or_create_pool_record(JCR *jcr, char *pool_name)
{
   POOL_DBR pr;

   memset(&pr, 0, sizeof(pr));
   bstrncpy(pr.Name, pool_name, sizeof(pr.Name));
   Dmsg1(110, "get_or_create_pool=%s\n", pool_name);

   while (!db_get_pool_record(jcr, jcr->db, &pr)) { /* get by Name */
      /* Try to create the pool */
      if (create_pool(jcr, jcr->db, jcr->pool, POOL_OP_CREATE) < 0) {
         Jmsg(jcr, M_FATAL, 0, _("Pool %s not in database. %s"), pr.Name,
            db_strerror(jcr->db));
         return 0;
      } else {
         Jmsg(jcr, M_INFO, 0, _("Pool %s created in database.\n"), pr.Name);
      }
   }
   return pr.PoolId;
}

void apply_pool_overrides(JCR *jcr)
{
   if (jcr->run_pool_override) {
      pm_strcpy(jcr->pool_source, _("Run pool override"));
   }
   /*
    * Apply any level related Pool selections
    */
   switch (jcr->JobLevel) {
   case L_FULL:
      if (jcr->full_pool) {
         jcr->pool = jcr->full_pool;
         if (jcr->run_full_pool_override) {
            pm_strcpy(jcr->pool_source, _("Run FullPool override"));
         } else {
            pm_strcpy(jcr->pool_source, _("Job FullPool override"));
         }
      }
      break;
   case L_INCREMENTAL:
      if (jcr->inc_pool) {
         jcr->pool = jcr->inc_pool;
         if (jcr->run_inc_pool_override) {
            pm_strcpy(jcr->pool_source, _("Run IncPool override"));
         } else {
            pm_strcpy(jcr->pool_source, _("Job IncPool override"));
         }
      }
      break;
   case L_DIFFERENTIAL:
      if (jcr->diff_pool) {
         jcr->pool = jcr->diff_pool;
         if (jcr->run_diff_pool_override) {
            pm_strcpy(jcr->pool_source, _("Run DiffPool override"));
         } else {
            pm_strcpy(jcr->pool_source, _("Job DiffPool override"));
         }
      }
      break;
   }
}


/*
 * Get or create a Client record for this Job
 */
bool get_or_create_client_record(JCR *jcr)
{
   CLIENT_DBR cr;

   memset(&cr, 0, sizeof(cr));
   bstrncpy(cr.Name, jcr->client->hdr.name, sizeof(cr.Name));
   cr.AutoPrune = jcr->client->AutoPrune;
   cr.FileRetention = jcr->client->FileRetention;
   cr.JobRetention = jcr->client->JobRetention;
   if (!jcr->client_name) {
      jcr->client_name = get_pool_memory(PM_NAME);
   }
   pm_strcpy(jcr->client_name, jcr->client->hdr.name);
   if (!db_create_client_record(jcr, jcr->db, &cr)) {
      Jmsg(jcr, M_FATAL, 0, _("Could not create Client record. ERR=%s\n"),
         db_strerror(jcr->db));
      return false;
   }
   jcr->jr.ClientId = cr.ClientId;
   if (cr.Uname[0]) {
      if (!jcr->client_uname) {
         jcr->client_uname = get_pool_memory(PM_NAME);
      }
      pm_strcpy(jcr->client_uname, cr.Uname);
   }
   Dmsg2(100, "Created Client %s record %d\n", jcr->client->hdr.name,
      jcr->jr.ClientId);
   return true;
}

bool get_or_create_fileset_record(JCR *jcr)
{
   FILESET_DBR fsr;
   /*
    * Get or Create FileSet record
    */
   memset(&fsr, 0, sizeof(FILESET_DBR));
   bstrncpy(fsr.FileSet, jcr->fileset->hdr.name, sizeof(fsr.FileSet));
   if (jcr->fileset->have_MD5) {
      struct MD5Context md5c;
      unsigned char digest[MD5HashSize];
      memcpy(&md5c, &jcr->fileset->md5c, sizeof(md5c));
      MD5Final(digest, &md5c);
      /*
       * Keep the flag (last arg) set to false otherwise old FileSets will
       * get new MD5 sums and the user will get Full backups on everything
       */
      bin_to_base64(fsr.MD5, sizeof(fsr.MD5), (char *)digest, MD5HashSize, false);
      bstrncpy(jcr->fileset->MD5, fsr.MD5, sizeof(jcr->fileset->MD5));
   } else {
      Jmsg(jcr, M_WARNING, 0, _("FileSet MD5 digest not found.\n"));
   }
   if (!jcr->fileset->ignore_fs_changes ||
       !db_get_fileset_record(jcr, jcr->db, &fsr)) {
      if (!db_create_fileset_record(jcr, jcr->db, &fsr)) {
         Jmsg(jcr, M_ERROR, 0, _("Could not create FileSet \"%s\" record. ERR=%s\n"),
            fsr.FileSet, db_strerror(jcr->db));
         return false;
      }
   }
   jcr->jr.FileSetId = fsr.FileSetId;
   bstrncpy(jcr->FSCreateTime, fsr.cCreateTime, sizeof(jcr->FSCreateTime));
   Dmsg2(119, "Created FileSet %s record %u\n", jcr->fileset->hdr.name,
      jcr->jr.FileSetId);
   return true;
}

void init_jcr_job_record(JCR *jcr)
{
   jcr->jr.SchedTime = jcr->sched_time;
   jcr->jr.StartTime = jcr->start_time;
   jcr->jr.EndTime = 0;               /* perhaps rescheduled, clear it */
   jcr->jr.JobType = jcr->JobType;
   jcr->jr.JobLevel = jcr->JobLevel;
   jcr->jr.JobStatus = jcr->JobStatus;
   jcr->jr.JobId = jcr->JobId;
   bstrncpy(jcr->jr.Name, jcr->job->name(), sizeof(jcr->jr.Name));
   bstrncpy(jcr->jr.Job, jcr->Job, sizeof(jcr->jr.Job));
}

/*
 * Write status and such in DB
 */
void update_job_end_record(JCR *jcr)
{
   jcr->jr.EndTime = time(NULL);
   jcr->end_time = jcr->jr.EndTime;
   jcr->jr.JobId = jcr->JobId;
   jcr->jr.JobStatus = jcr->JobStatus;
   jcr->jr.JobFiles = jcr->JobFiles;
   jcr->jr.JobBytes = jcr->JobBytes;
   jcr->jr.VolSessionId = jcr->VolSessionId;
   jcr->jr.VolSessionTime = jcr->VolSessionTime;
   jcr->jr.JobErrors = jcr->Errors;
   if (!db_update_job_end_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error updating job record. %s"),
         db_strerror(jcr->db));
   }
}

/*
 * Takes base_name and appends (unique) current
 *   date and time to form unique job name.
 *
 *  Note, the seconds are actually a sequence number. This
 *   permits us to start a maximum fo 59 unique jobs a second, which
 *   should be sufficient.
 *
 *  Returns: unique job name in jcr->Job
 *    date/time in jcr->start_time
 */
void create_unique_job_name(JCR *jcr, const char *base_name)
{
   /* Job start mutex */
   static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
   static time_t last_start_time = 0;
   static int seq = 0;
   time_t now;
   struct tm tm;
   char dt[MAX_TIME_LENGTH];
   char name[MAX_NAME_LENGTH];
   char *p;

   /* Guarantee unique start time -- maximum one per second, and
    * thus unique Job Name
    */
   P(mutex);                          /* lock creation of jobs */
   now = time(NULL);
   seq++;
   if (seq > 59) {                    /* wrap as if it is seconds */
      seq = 0;
      while (now == last_start_time) {
         bmicrosleep(0, 500000);
         now = time(NULL);
      }
   }
   last_start_time = now;
   V(mutex);                          /* allow creation of jobs */
   jcr->start_time = now;
   /* Form Unique JobName */
   (void)localtime_r(&now, &tm);
   /* Use only characters that are permitted in Windows filenames */
   strftime(dt, sizeof(dt), "%Y-%m-%d_%H.%M", &tm);
   bstrncpy(name, base_name, sizeof(name));
   name[sizeof(name)-22] = 0;          /* truncate if too long */
   bsnprintf(jcr->Job, sizeof(jcr->Job), "%s.%s.%02d", name, dt, seq); /* add date & time */
   /* Convert spaces into underscores */
   for (p=jcr->Job; *p; p++) {
      if (*p == ' ') {
         *p = '_';
      }
   }
}

/* Called directly from job rescheduling */
void dird_free_jcr_pointers(JCR *jcr)
{
   if (jcr->sd_auth_key) {
      free(jcr->sd_auth_key);
      jcr->sd_auth_key = NULL;
   }
   if (jcr->where) {
      free(jcr->where);
      jcr->where = NULL;
   }
   if (jcr->file_bsock) {
      Dmsg0(200, "Close File bsock\n");
      bnet_close(jcr->file_bsock);
      jcr->file_bsock = NULL;
   }
   if (jcr->store_bsock) {
      Dmsg0(200, "Close Store bsock\n");
      bnet_close(jcr->store_bsock);
      jcr->store_bsock = NULL;
   }
   if (jcr->fname) {
      Dmsg0(200, "Free JCR fname\n");
      free_pool_memory(jcr->fname);
      jcr->fname = NULL;
   }
   if (jcr->RestoreBootstrap) {
      free(jcr->RestoreBootstrap);
      jcr->RestoreBootstrap = NULL;
   }
   if (jcr->client_uname) {
      free_pool_memory(jcr->client_uname);
      jcr->client_uname = NULL;
   }
   if (jcr->attr) {
      free_pool_memory(jcr->attr);
      jcr->attr = NULL;
   }
   if (jcr->ar) {
      free(jcr->ar);
      jcr->ar = NULL;
   }
}

/*
 * Free the Job Control Record if no one is still using it.
 *  Called from main free_jcr() routine in src/lib/jcr.c so
 *  that we can do our Director specific cleanup of the jcr.
 */
void dird_free_jcr(JCR *jcr)
{
   Dmsg0(200, "Start dird free_jcr\n");

   dird_free_jcr_pointers(jcr);
   if (jcr->term_wait_inited) {
      pthread_cond_destroy(&jcr->term_wait);
      jcr->term_wait_inited = false;
   }
   if (jcr->db_batch && jcr->db_batch != jcr->db) {
      db_close_database(jcr, jcr->db_batch);
   }
   jcr->db_batch = NULL;
   if (jcr->db) {
      db_close_database(jcr, jcr->db);
      jcr->db = NULL;
   }
   if (jcr->stime) {
      Dmsg0(200, "Free JCR stime\n");
      free_pool_memory(jcr->stime);
      jcr->stime = NULL;
   }
   if (jcr->fname) {
      Dmsg0(200, "Free JCR fname\n");
      free_pool_memory(jcr->fname);
      jcr->fname = NULL;
   }
   if (jcr->pool_source) {
      free_pool_memory(jcr->pool_source);
      jcr->pool_source = NULL;
   }
   if (jcr->rpool_source) {
      free_pool_memory(jcr->rpool_source);
      jcr->rpool_source = NULL;
   }
   if (jcr->wstore_source) {
      free_pool_memory(jcr->wstore_source);
      jcr->wstore_source = NULL;
   }
   if (jcr->rstore_source) {
      free_pool_memory(jcr->rstore_source);
      jcr->rstore_source = NULL;
   }

   /* Delete lists setup to hold storage pointers */
   free_rwstorage(jcr);

   jcr->job_end_push.destroy();
   Dmsg0(200, "End dird free_jcr\n");
}

/* 
 * The Job storage definition must be either in the Job record
 *  or in the Pool record.  The Pool record overrides the Job 
 *  record.
 */
void get_job_storage(USTORE *store, JOB *job, RUN *run) 
{
   if (run && run->pool && run->pool->storage) {
      store->store = (STORE *)run->pool->storage->first();
      pm_strcpy(store->store_source, _("Run pool override"));
      return;
   }
   if (run && run->storage) {
      store->store = run->storage;
      pm_strcpy(store->store_source, _("Run storage override"));
      return;
   }
   if (job->pool->storage) {
      store->store = (STORE *)job->pool->storage->first();
      pm_strcpy(store->store_source, _("Pool resource"));
   } else {
      store->store = (STORE *)job->storage->first();
      pm_strcpy(store->store_source, _("Job resource"));
   }
}

/*
 * Set some defaults in the JCR necessary to
 * run. These items are pulled from the job
 * definition as defaults, but can be overridden
 * later either by the Run record in the Schedule resource,
 * or by the Console program.
 */
void set_jcr_defaults(JCR *jcr, JOB *job)
{
   jcr->job = job;
   jcr->JobType = job->JobType;
   jcr->JobStatus = JS_Created;
   switch (jcr->JobType) {
   case JT_ADMIN:
   case JT_RESTORE:
      jcr->JobLevel = L_NONE;
      break;
   case JT_MIGRATE:
      if (!jcr->rpool_source) {
         jcr->rpool_source = get_pool_memory(PM_MESSAGE);
         pm_strcpy(jcr->rpool_source, _("unknown source"));
      }
      /* Fall-through wanted */
   default:
      jcr->JobLevel = job->JobLevel;
      break;
   }
   if (!jcr->fname) {
      jcr->fname = get_pool_memory(PM_FNAME);
   }
   if (!jcr->pool_source) {
      jcr->pool_source = get_pool_memory(PM_MESSAGE);
      pm_strcpy(jcr->pool_source, _("unknown source"));
   }

   jcr->JobPriority = job->Priority;
   /* Copy storage definitions -- deleted in dir_free_jcr above */
   if (job->storage) {
      copy_rwstorage(jcr, job->storage, _("Job resource"));
   } else {
      copy_rwstorage(jcr, job->pool->storage, _("Pool resource"));
   }
   jcr->client = job->client;
   if (!jcr->client_name) {
      jcr->client_name = get_pool_memory(PM_NAME);
   }
   pm_strcpy(jcr->client_name, jcr->client->hdr.name);
   pm_strcpy(jcr->pool_source, _("Job resource"));
   jcr->pool = job->pool;
   jcr->full_pool = job->full_pool;
   jcr->inc_pool = job->inc_pool;
   jcr->diff_pool = job->diff_pool;
   jcr->catalog = job->client->catalog;
   jcr->fileset = job->fileset;
   jcr->messages = job->messages;
   jcr->spool_data = job->spool_data;
   jcr->write_part_after_job = job->write_part_after_job;
   if (jcr->RestoreBootstrap) {
      free(jcr->RestoreBootstrap);
      jcr->RestoreBootstrap = NULL;
   }
   /* This can be overridden by Console program */
   if (job->RestoreBootstrap) {
      jcr->RestoreBootstrap = bstrdup(job->RestoreBootstrap);
   }
   /* This can be overridden by Console program */
   jcr->verify_job = job->verify_job;
   /* If no default level given, set one */
   if (jcr->JobLevel == 0) {
      switch (jcr->JobType) {
      case JT_VERIFY:
         jcr->JobLevel = L_VERIFY_CATALOG;
         break;
      case JT_BACKUP:
         jcr->JobLevel = L_INCREMENTAL;
         break;
      case JT_RESTORE:
      case JT_ADMIN:
         jcr->JobLevel = L_NONE;
         break;
      default:
         jcr->JobLevel = L_FULL;
         break;
      }
   }
}

/* 
 * Copy the storage definitions from an alist to the JCR
 */
void copy_rwstorage(JCR *jcr, alist *storage, const char *where)
{
   switch(jcr->JobType) {
   case JT_RESTORE:
   case JT_VERIFY:
   case JT_MIGRATE:
      copy_rstorage(jcr, storage, where);
      break;
   default:
      copy_wstorage(jcr, storage, where);
      break;
   }
}


/* Set storage override.  Releases any previous storage definition */
void set_rwstorage(JCR *jcr, USTORE *store)
{
   if (!store) {
      Jmsg(jcr, M_FATAL, 0, _("No storage specified.\n"));
      return;
   }
   switch(jcr->JobType) {
   case JT_RESTORE:
   case JT_VERIFY:
   case JT_MIGRATE:
      set_rstorage(jcr, store);
      break;
   default:
      set_wstorage(jcr, store);
      break;
   }
}

void free_rwstorage(JCR *jcr)
{
   free_rstorage(jcr);
   free_wstorage(jcr);
}

/* 
 * Copy the storage definitions from an alist to the JCR
 */
void copy_rstorage(JCR *jcr, alist *storage, const char *where)
{
   if (storage) {
      STORE *st;
      if (jcr->rstorage) {
         delete jcr->rstorage;
      }
      jcr->rstorage = New(alist(10, not_owned_by_alist));
      foreach_alist(st, storage) {
         jcr->rstorage->append(st);
      }
      if (!jcr->rstore_source) {
         jcr->rstore_source = get_pool_memory(PM_MESSAGE);
      }
      pm_strcpy(jcr->rstore_source, where);
      if (jcr->rstorage) {
         jcr->rstore = (STORE *)jcr->rstorage->first();
      }
   }
}


/* Set storage override.  Remove all previous storage */
void set_rstorage(JCR *jcr, USTORE *store)
{
   STORE *storage;

   if (!store->store) {
      return;
   }
   if (jcr->rstorage) {
      free_rstorage(jcr);
   }
   if (!jcr->rstorage) {
      jcr->rstorage = New(alist(10, not_owned_by_alist));
   }
   jcr->rstore = store->store;
   if (!jcr->rstore_source) {
      jcr->rstore_source = get_pool_memory(PM_MESSAGE);
   }
   pm_strcpy(jcr->rstore_source, store->store_source);
   foreach_alist(storage, jcr->rstorage) {
      if (store->store == storage) {
         return;
      }
   }
   /* Store not in list, so add it */
   jcr->rstorage->prepend(store->store);
}

void free_rstorage(JCR *jcr)
{
   if (jcr->rstorage) {
      delete jcr->rstorage;
      jcr->rstorage = NULL;
   }
   jcr->rstore = NULL;
}

/* 
 * Copy the storage definitions from an alist to the JCR
 */
void copy_wstorage(JCR *jcr, alist *storage, const char *where)
{
   if (storage) {
      STORE *st;
      if (jcr->wstorage) {
         delete jcr->wstorage;
      }
      jcr->wstorage = New(alist(10, not_owned_by_alist));
      foreach_alist(st, storage) {
         Dmsg1(100, "wstorage=%s\n", st->name());
         jcr->wstorage->append(st);
      }
      if (!jcr->wstore_source) {
         jcr->wstore_source = get_pool_memory(PM_MESSAGE);
      }
      pm_strcpy(jcr->wstore_source, where);
      if (jcr->wstorage) {
         jcr->wstore = (STORE *)jcr->wstorage->first();
         Dmsg2(100, "wstore=%s where=%s\n", jcr->wstore->name(), jcr->wstore_source);
      }
   }
}


/* Set storage override. Remove all previous storage */
void set_wstorage(JCR *jcr, USTORE *store)
{
   STORE *storage;

   if (!store->store) {
      return;
   }
   if (jcr->wstorage) {
      free_wstorage(jcr);
   }
   if (!jcr->wstorage) {
      jcr->wstorage = New(alist(10, not_owned_by_alist));
   }
   jcr->wstore = store->store;
   if (!jcr->wstore_source) {
      jcr->wstore_source = get_pool_memory(PM_MESSAGE);
   }
   pm_strcpy(jcr->wstore_source, store->store_source);
   Dmsg2(50, "wstore=%s where=%s\n", jcr->wstore->name(), jcr->wstore_source);
   foreach_alist(storage, jcr->wstorage) {
      if (store->store == storage) {
         return;
      }
   }
   /* Store not in list, so add it */
   jcr->wstorage->prepend(store->store);
}

void free_wstorage(JCR *jcr)
{
   if (jcr->wstorage) {
      delete jcr->wstorage;
      jcr->wstorage = NULL;
   }
   jcr->wstore = NULL;
}

void create_clones(JCR *jcr)
{
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
}

bool create_restore_bootstrap_file(JCR *jcr)
{
   RESTORE_CTX rx;
   UAContext *ua;
   memset(&rx, 0, sizeof(rx));
   rx.bsr = new_bsr();
   rx.JobIds = (char *)"";                       
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
