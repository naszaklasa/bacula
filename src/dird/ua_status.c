/*
 *
 *   Bacula Director -- User Agent Status Command
 *
 *     Kern Sibbald, August MMI
 *
 *   Version $Id: ua_status.c,v 1.72.2.5 2006/03/14 21:41:40 kerns Exp $
 */
/*
   Copyright (C) 2001-2006 Kern Sibbald

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

extern char my_name[];
extern time_t daemon_start_time;
extern int num_jobs_run;

static void list_scheduled_jobs(UAContext *ua);
static void list_running_jobs(UAContext *ua);
static void list_terminated_jobs(UAContext *ua);
static void do_storage_status(UAContext *ua, STORE *store);
static void do_client_status(UAContext *ua, CLIENT *client);
static void do_director_status(UAContext *ua);
static void do_all_status(UAContext *ua);

static char OKqstatus[]   = "1000 OK .status\n";
static char DotStatusJob[] = "JobId=%s JobStatus=%c JobErrors=%d\n";

/*
 * .status command
 */
int qstatus_cmd(UAContext *ua, const char *cmd)
{
   JCR* njcr = NULL;
   s_last_job* job;
   char ed1[50];

   if (!open_db(ua)) {
      return 1;
   }
   Dmsg1(20, "status:%s:\n", cmd);

   if ((ua->argc != 3) || (strcasecmp(ua->argk[1], "dir"))) {
      bsendmsg(ua, "1900 Bad .status command, missing arguments.\n");
      return 1;
   }

   if (strcasecmp(ua->argk[2], "current") == 0) {
      bsendmsg(ua, OKqstatus, ua->argk[2]);
      foreach_jcr(njcr) {
         if (njcr->JobId != 0) {
            bsendmsg(ua, DotStatusJob, edit_int64(njcr->JobId, ed1), 
                     njcr->JobStatus, njcr->JobErrors);
         }
      }
      endeach_jcr(njcr);
   } else if (strcasecmp(ua->argk[2], "last") == 0) {
      bsendmsg(ua, OKqstatus, ua->argk[2]);
      if ((last_jobs) && (last_jobs->size() > 0)) {
         job = (s_last_job*)last_jobs->last();
         bsendmsg(ua, DotStatusJob, edit_int64(job->JobId, ed1), 
                  job->JobStatus, job->Errors);
      }
   } else {
      bsendmsg(ua, "1900 Bad .status command, wrong argument.\n");
      return 1;
   }

   return 1;
}

/*
 * status command
 */
int status_cmd(UAContext *ua, const char *cmd)
{
   STORE *store;
   CLIENT *client;
   int item, i;

   if (!open_db(ua)) {
      return 1;
   }
   Dmsg1(20, "status:%s:\n", cmd);

   for (i=1; i<ua->argc; i++) {
      if (strcasecmp(ua->argk[i], N_("all")) == 0) {
         do_all_status(ua);
         return 1;
      } else if (strcasecmp(ua->argk[i], N_("dir")) == 0 ||
                 strcasecmp(ua->argk[i], N_("director")) == 0) {
         do_director_status(ua);
         return 1;
      } else if (strcasecmp(ua->argk[i], N_("client")) == 0) {
         client = get_client_resource(ua);
         if (client) {
            do_client_status(ua, client);
         }
         return 1;
      } else {
         store = get_storage_resource(ua, false/*no default*/);
         if (store) {
            do_storage_status(ua, store);
         }
         return 1;
      }
   }
   /* If no args, ask for status type */
   if (ua->argc == 1) {
       char prmt[MAX_NAME_LENGTH];

      start_prompt(ua, _("Status available for:\n"));
      add_prompt(ua, N_("Director"));
      add_prompt(ua, N_("Storage"));
      add_prompt(ua, N_("Client"));
      add_prompt(ua, N_("All"));
      Dmsg0(20, "do_prompt: select daemon\n");
      if ((item=do_prompt(ua, "",  _("Select daemon type for status"), prmt, sizeof(prmt))) < 0) {
         return 1;
      }
      Dmsg1(20, "item=%d\n", item);
      switch (item) {
      case 0:                         /* Director */
         do_director_status(ua);
         break;
      case 1:
         store = select_storage_resource(ua);
         if (store) {
            do_storage_status(ua, store);
         }
         break;
      case 2:
         client = select_client_resource(ua);
         if (client) {
            do_client_status(ua, client);
         }
         break;
      case 3:
         do_all_status(ua);
         break;
      default:
         break;
      }
   }
   return 1;
}

static void do_all_status(UAContext *ua)
{
   STORE *store, **unique_store;
   CLIENT *client, **unique_client;
   int i, j;
   bool found;

   do_director_status(ua);

   /* Count Storage items */
   LockRes();
   i = 0;
   foreach_res(store, R_STORAGE) {
      i++;
   }
   unique_store = (STORE **) malloc(i * sizeof(STORE));
   /* Find Unique Storage address/port */
   i = 0;
   foreach_res(store, R_STORAGE) {
      found = false;
      if (!acl_access_ok(ua, Storage_ACL, store->hdr.name)) {
         continue;
      }
      for (j=0; j<i; j++) {
         if (strcmp(unique_store[j]->address, store->address) == 0 &&
             unique_store[j]->SDport == store->SDport) {
            found = true;
            break;
         }
      }
      if (!found) {
         unique_store[i++] = store;
         Dmsg2(40, "Stuffing: %s:%d\n", store->address, store->SDport);
      }
   }
   UnlockRes();

   /* Call each unique Storage daemon */
   for (j=0; j<i; j++) {
      do_storage_status(ua, unique_store[j]);
   }
   free(unique_store);

   /* Count Client items */
   LockRes();
   i = 0;
   foreach_res(client, R_CLIENT) {
      i++;
   }
   unique_client = (CLIENT **)malloc(i * sizeof(CLIENT));
   /* Find Unique Client address/port */
   i = 0;
   foreach_res(client, R_CLIENT) {
      found = false;
      if (!acl_access_ok(ua, Client_ACL, client->hdr.name)) {
         continue;
      }
      for (j=0; j<i; j++) {
         if (strcmp(unique_client[j]->address, client->address) == 0 &&
             unique_client[j]->FDport == client->FDport) {
            found = true;
            break;
         }
      }
      if (!found) {
         unique_client[i++] = client;
         Dmsg2(40, "Stuffing: %s:%d\n", client->address, client->FDport);
      }
   }
   UnlockRes();

   /* Call each unique File daemon */
   for (j=0; j<i; j++) {
      do_client_status(ua, unique_client[j]);
   }
   free(unique_client);

}

static void do_director_status(UAContext *ua)
{
   char dt[MAX_TIME_LENGTH];

   bsendmsg(ua, _("%s Version: %s (%s) %s %s %s\n"), my_name, VERSION, BDATE,
            HOST_OS, DISTNAME, DISTVER);
   bstrftime_nc(dt, sizeof(dt), daemon_start_time);
   if (num_jobs_run == 1) {
      bsendmsg(ua, _("Daemon started %s, 1 Job run since started.\n"), dt);
   }
   else {
      bsendmsg(ua, _("Daemon started %s, %d Jobs run since started.\n"),
        dt, num_jobs_run);
   }
   if (debug_level > 0) {
      char b1[35], b2[35], b3[35], b4[35];
      bsendmsg(ua, _(" Heap: bytes=%s max_bytes=%s bufs=%s max_bufs=%s\n"),
            edit_uint64_with_commas(sm_bytes, b1),
            edit_uint64_with_commas(sm_max_bytes, b2),
            edit_uint64_with_commas(sm_buffers, b3),
            edit_uint64_with_commas(sm_max_buffers, b4));
   }
   /*
    * List scheduled Jobs
    */
   list_scheduled_jobs(ua);

   /*
    * List running jobs
    */
   list_running_jobs(ua);

   /*
    * List terminated jobs
    */
   list_terminated_jobs(ua);
   bsendmsg(ua, _("====\n"));
}

static void do_storage_status(UAContext *ua, STORE *store)
{
   BSOCK *sd;

   set_storage(ua->jcr, store);
   /* Try connecting for up to 15 seconds */
   bsendmsg(ua, _("Connecting to Storage daemon %s at %s:%d\n"),
      store->hdr.name, store->address, store->SDport);
   if (!connect_to_storage_daemon(ua->jcr, 1, 15, 0)) {
      bsendmsg(ua, _("\nFailed to connect to Storage daemon %s.\n====\n"),
         store->hdr.name);
      if (ua->jcr->store_bsock) {
         bnet_close(ua->jcr->store_bsock);
         ua->jcr->store_bsock = NULL;
      }
      return;
   }
   Dmsg0(20, _("Connected to storage daemon\n"));
   sd = ua->jcr->store_bsock;
   bnet_fsend(sd, "status");
   while (bnet_recv(sd) >= 0) {
      bsendmsg(ua, "%s", sd->msg);
   }
   bnet_sig(sd, BNET_TERMINATE);
   bnet_close(sd);
   ua->jcr->store_bsock = NULL;
   return;
}

static void do_client_status(UAContext *ua, CLIENT *client)
{
   BSOCK *fd;

   /* Connect to File daemon */

   ua->jcr->client = client;
   /* Release any old dummy key */
   if (ua->jcr->sd_auth_key) {
      free(ua->jcr->sd_auth_key);
   }
   /* Create a new dummy SD auth key */
   ua->jcr->sd_auth_key = bstrdup("dummy");

   /* Try to connect for 15 seconds */
   bsendmsg(ua, _("Connecting to Client %s at %s:%d\n"),
      client->hdr.name, client->address, client->FDport);
   if (!connect_to_file_daemon(ua->jcr, 1, 15, 0)) {
      bsendmsg(ua, _("Failed to connect to Client %s.\n====\n"),
         client->hdr.name);
      if (ua->jcr->file_bsock) {
         bnet_close(ua->jcr->file_bsock);
         ua->jcr->file_bsock = NULL;
      }
      return;
   }
   Dmsg0(20, _("Connected to file daemon\n"));
   fd = ua->jcr->file_bsock;
   bnet_fsend(fd, "status");
   while (bnet_recv(fd) >= 0) {
      bsendmsg(ua, "%s", fd->msg);
   }
   bnet_sig(fd, BNET_TERMINATE);
   bnet_close(fd);
   ua->jcr->file_bsock = NULL;

   return;
}

static void prt_runhdr(UAContext *ua)
{
   bsendmsg(ua, _("\nScheduled Jobs:\n"));
   bsendmsg(ua, _("Level          Type     Pri  Scheduled          Name               Volume\n"));
   bsendmsg(ua, _("===================================================================================\n"));
}

/* Scheduling packet */
struct sched_pkt {
   dlink link;                        /* keep this as first item!!! */
   JOB *job;
   int level;
   int priority;
   time_t runtime;
   POOL *pool;
   STORE *store;
};

static void prt_runtime(UAContext *ua, sched_pkt *sp)
{
   char dt[MAX_TIME_LENGTH];
   const char *level_ptr;
   bool ok = false;
   bool close_db = false;
   JCR *jcr = ua->jcr;
   MEDIA_DBR mr;

   memset(&mr, 0, sizeof(mr));
   if (sp->job->JobType == JT_BACKUP) {
      jcr->db = NULL;
      ok = complete_jcr_for_job(jcr, sp->job, sp->pool);
      if (jcr->db) {
         close_db = true;             /* new db opened, remember to close it */
      }
      if (ok) {
         mr.PoolId = jcr->PoolId;
         mr.StorageId = sp->store->StorageId;
         ok = find_next_volume_for_append(jcr, &mr, 1, false/*no create*/);
      }
      if (!ok) {
         bstrncpy(mr.VolumeName, "*unknown*", sizeof(mr.VolumeName));
      }
   }
   bstrftime_nc(dt, sizeof(dt), sp->runtime);
   switch (sp->job->JobType) {
   case JT_ADMIN:
   case JT_RESTORE:
      level_ptr = " ";
      break;
   default:
      level_ptr = level_to_str(sp->level);
      break;
   }
   bsendmsg(ua, _("%-14s %-8s %3d  %-18s %-18s %s\n"),
      level_ptr, job_type_to_str(sp->job->JobType), sp->priority, dt,
      sp->job->hdr.name, mr.VolumeName);
   if (close_db) {
      db_close_database(jcr, jcr->db);
   }
   jcr->db = ua->db;                  /* restore ua db to jcr */

}

/*
 * Sort items by runtime, priority
 */
static int my_compare(void *item1, void *item2)
{
   sched_pkt *p1 = (sched_pkt *)item1;
   sched_pkt *p2 = (sched_pkt *)item2;
   if (p1->runtime < p2->runtime) {
      return -1;
   } else if (p1->runtime > p2->runtime) {
      return 1;
   }
   if (p1->priority < p2->priority) {
      return -1;
   } else if (p1->priority > p2->priority) {
      return 1;
   }
   return 0;
}

/*
 * Find all jobs to be run in roughly the
 *  next 24 hours.
 */
static void list_scheduled_jobs(UAContext *ua)
{
   time_t runtime;
   RUN *run;
   JOB *job;
   STORE* store;
   int level, num_jobs = 0;
   int priority;
   bool hdr_printed = false;
   dlist sched;
   sched_pkt *sp;
   int days, i;

   Dmsg0(200, "enter list_sched_jobs()\n");

   days = 1;
   i = find_arg_with_value(ua, N_("days"));
   if (i >= 0) {
     days = atoi(ua->argv[i]);
     if ((days < 0) || (days > 50)) {
       bsendmsg(ua, _("Ignoring illegal value for days.\n"));
       days = 1;
     }
   }

   /* Loop through all jobs */
   LockRes();
   foreach_res(job, R_JOB) {
      if (!acl_access_ok(ua, Job_ACL, job->hdr.name) || !job->enabled) {
         continue;
      }
      for (run=NULL; (run = find_next_run(run, job, runtime, days)); ) {
         level = job->JobLevel;
         if (run->level) {
            level = run->level;
         }
         priority = job->Priority;
         if (run->Priority) {
            priority = run->Priority;
         }
         if (run->storage) {
            store = run->storage;
         } else {
            store = (STORE *)job->storage->first();
         }
         if (!hdr_printed) {
            prt_runhdr(ua);
            hdr_printed = true;
         }
         sp = (sched_pkt *)malloc(sizeof(sched_pkt));
         sp->job = job;
         sp->level = level;
         sp->priority = priority;
         sp->runtime = runtime;
         sp->pool = run->pool;
         sp->store = store;
         sched.binary_insert_multiple(sp, my_compare);
         num_jobs++;
      }
   } /* end for loop over resources */
   UnlockRes();
   foreach_dlist(sp, &sched) {
      prt_runtime(ua, sp);
   }
   if (num_jobs == 0) {
      bsendmsg(ua, _("No Scheduled Jobs.\n"));
   }
   bsendmsg(ua, _("====\n"));
   Dmsg0(200, "Leave list_sched_jobs_runs()\n");
}

static void list_running_jobs(UAContext *ua)
{
   JCR *jcr;
   int njobs = 0;
   const char *msg;
   char *emsg;                        /* edited message */
   char dt[MAX_TIME_LENGTH];
   char level[10];
   bool pool_mem = false;

   Dmsg0(200, "enter list_run_jobs()\n");
   bsendmsg(ua, _("\nRunning Jobs:\n"));
   foreach_jcr(jcr) {
      if (jcr->JobId == 0) {      /* this is us */
         /* this is a console or other control job. We only show console
          * jobs in the status output.
          */
         if (jcr->JobType == JT_CONSOLE) {
            bstrftime_nc(dt, sizeof(dt), jcr->start_time);
            bsendmsg(ua, _("Console connected at %s\n"), dt);
         }
         continue;
      }       
      njobs++;
   }
   endeach_jcr(jcr);

   if (njobs == 0) {
      /* Note the following message is used in regress -- don't change */
      bsendmsg(ua, _("No Jobs running.\n====\n"));
      Dmsg0(200, "leave list_run_jobs()\n");
      return;
   }
   njobs = 0;
   bsendmsg(ua, _(" JobId Level   Name                       Status\n"));
   bsendmsg(ua, _("======================================================================\n"));
   foreach_jcr(jcr) {
      if (jcr->JobId == 0 || !acl_access_ok(ua, Job_ACL, jcr->job->hdr.name)) {
         continue;
      }
      njobs++;
      switch (jcr->JobStatus) {
      case JS_Created:
         msg = _("is waiting execution");
         break;
      case JS_Running:
         msg = _("is running");
         break;
      case JS_Blocked:
         msg = _("is blocked");
         break;
      case JS_Terminated:
         msg = _("has terminated");
         break;
      case JS_ErrorTerminated:
         msg = _("has erred");
         break;
      case JS_Error:
         msg = _("has errors");
         break;
      case JS_FatalError:
         msg = _("has a fatal error");
         break;
      case JS_Differences:
         msg = _("has verify differences");
         break;
      case JS_Canceled:
         msg = _("has been canceled");
         break;
      case JS_WaitFD:
         emsg = (char *) get_pool_memory(PM_FNAME);
         Mmsg(emsg, _("is waiting on Client %s"), jcr->client->hdr.name);
         pool_mem = true;
         msg = emsg;
         break;
      case JS_WaitSD:
         emsg = (char *) get_pool_memory(PM_FNAME);
         Mmsg(emsg, _("is waiting on Storage %s"), jcr->store->hdr.name);
         pool_mem = true;
         msg = emsg;
         break;
      case JS_WaitStoreRes:
         msg = _("is waiting on max Storage jobs");
         break;
      case JS_WaitClientRes:
         msg = _("is waiting on max Client jobs");
         break;
      case JS_WaitJobRes:
         msg = _("is waiting on max Job jobs");
         break;
      case JS_WaitMaxJobs:
         msg = _("is waiting on max total jobs");
         break;
      case JS_WaitStartTime:
         msg = _("is waiting for its start time");
         break;
      case JS_WaitPriority:
         msg = _("is waiting for higher priority jobs to finish");
         break;

      default:
         emsg = (char *) get_pool_memory(PM_FNAME);
         Mmsg(emsg, _("is in unknown state %c"), jcr->JobStatus);
         pool_mem = true;
         msg = emsg;
         break;
      }
      /*
       * Now report Storage daemon status code
       */
      switch (jcr->SDJobStatus) {
      case JS_WaitMount:
         if (pool_mem) {
            free_pool_memory(emsg);
            pool_mem = false;
         }
         msg = _("is waiting for a mount request");
         break;
      case JS_WaitMedia:
         if (pool_mem) {
            free_pool_memory(emsg);
            pool_mem = false;
         }
         msg = _("is waiting for an appendable Volume");
         break;
      case JS_WaitFD:
         if (!pool_mem) {
            emsg = (char *) get_pool_memory(PM_FNAME);
            pool_mem = true;
         }
         Mmsg(emsg, _("is waiting for Client %s to connect to Storage %s"),
              jcr->client->hdr.name, jcr->store->hdr.name);
         msg = emsg;
         break;
      }
      switch (jcr->JobType) {
      case JT_ADMIN:
      case JT_RESTORE:
         bstrncpy(level, "      ", sizeof(level));
         break;
      default:
         bstrncpy(level, level_to_str(jcr->JobLevel), sizeof(level));
         level[7] = 0;
         break;
      }

      bsendmsg(ua, _("%6d %-6s  %-20s %s\n"),
         jcr->JobId,
         level,
         jcr->Job,
         msg);

      if (pool_mem) {
         free_pool_memory(emsg);
         pool_mem = false;
      }
   }
   endeach_jcr(jcr);
   bsendmsg(ua, _("====\n"));
   Dmsg0(200, "leave list_run_jobs()\n");
}

static void list_terminated_jobs(UAContext *ua)
{
   char dt[MAX_TIME_LENGTH], b1[30], b2[30];
   char level[10];

   if (last_jobs->empty()) {
      bsendmsg(ua, _("No Terminated Jobs.\n"));
      return;
   }
   lock_last_jobs_list();
   struct s_last_job *je;
   bsendmsg(ua, _("\nTerminated Jobs:\n"));
   bsendmsg(ua, _(" JobId  Level     Files      Bytes     Status   Finished        Name \n"));
   bsendmsg(ua, _("========================================================================\n"));
   foreach_dlist(je, last_jobs) {
      char JobName[MAX_NAME_LENGTH];
      const char *termstat;

      bstrncpy(JobName, je->Job, sizeof(JobName));
      /* There are three periods after the Job name */
      char *p;
      for (int i=0; i<3; i++) {
         if ((p=strrchr(JobName, '.')) != NULL) {
            *p = 0;
         }
      }

      if (!acl_access_ok(ua, Job_ACL, JobName)) {
         continue;
      }

      bstrftime_nc(dt, sizeof(dt), je->end_time);
      switch (je->JobType) {
      case JT_ADMIN:
      case JT_RESTORE:
         bstrncpy(level, "    ", sizeof(level));
         break;
      default:
         bstrncpy(level, level_to_str(je->JobLevel), sizeof(level));
         level[4] = 0;
         break;
      }
      switch (je->JobStatus) {
      case JS_Created:
         termstat = _("Created");
         break;
      case JS_FatalError:
      case JS_ErrorTerminated:
         termstat = _("Error");
         break;
      case JS_Differences:
         termstat = _("Diffs");
         break;
      case JS_Canceled:
         termstat = _("Cancel");
         break;
      case JS_Terminated:
         termstat = _("OK");
         break;
      default:
         termstat = _("Other");
         break;
      }
      bsendmsg(ua, _("%6d  %-6s %8s %14s %-7s  %-8s %s\n"),
         je->JobId,
         level,
         edit_uint64_with_commas(je->JobFiles, b1),
         edit_uint64_with_commas(je->JobBytes, b2),
         termstat,
         dt, JobName);
   }
   bsendmsg(ua, _("\n"));
   unlock_last_jobs_list();
}
