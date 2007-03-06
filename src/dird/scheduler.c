/*
 *
 *   Bacula scheduler
 *     It looks at what jobs are to be run and when
 *     and waits around until it is time to
 *     fire them up.
 *
 *     Kern Sibbald, May MM, major revision December MMIII
 *
 *   Version $Id: scheduler.c,v 1.33.2.1 2006/01/11 10:24:12 kerns Exp $
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

/* #define SCHED_DEBUG */


/* Local variables */
struct job_item {
   RUN *run;
   JOB *job;
   time_t runtime;
   int Priority;
   dlink link;                        /* link for list */
};

/* List of jobs to be run. They were scheduled in this hour or the next */
static dlist *jobs_to_run;               /* list of jobs to be run */

/* Time interval in secs to sleep if nothing to be run */
static int const NEXT_CHECK_SECS = 60;

/* Forward referenced subroutines */
static void find_runs();
static void add_job(JOB *job, RUN *run, time_t now, time_t runtime);
static void dump_job(job_item *ji, const char *msg);

/* Imported subroutines */

/* Imported variables */


/*********************************************************************
 *
 *         Main Bacula Scheduler
 *
 */
JCR *wait_for_next_job(char *one_shot_job_to_run)
{
   JCR *jcr;
   JOB *job;
   RUN *run;
   time_t now;
   static bool first = true;
   job_item *next_job = NULL;

   Dmsg0(200, "Enter wait_for_next_job\n");
   if (first) {
      first = false;
      /* Create scheduled jobs list */
      jobs_to_run = New(dlist(next_job, &next_job->link));
      if (one_shot_job_to_run) {            /* one shot */
         job = (JOB *)GetResWithName(R_JOB, one_shot_job_to_run);
         if (!job) {
            Emsg1(M_ABORT, 0, _("Job %s not found\n"), one_shot_job_to_run);
         }
         Dmsg1(5, "Found one_shot_job_to_run %s\n", one_shot_job_to_run);
         jcr = new_jcr(sizeof(JCR), dird_free_jcr);
         set_jcr_defaults(jcr, job);
         return jcr;
      }
   }
   /* Wait until we have something in the
    * next hour or so.
    */
again:
   while (jobs_to_run->empty()) {
      find_runs();
      if (!jobs_to_run->empty()) {
         break;
      }
      bmicrosleep(NEXT_CHECK_SECS, 0); /* recheck once per minute */
   }

#ifdef  list_chain
   job_item *je;
   foreach_dlist(je, jobs_to_run) {
      dump_job(je, _("Walk queue"));
   }
#endif
   /*
    * Pull the first job to run (already sorted by runtime and
    *  Priority, then wait around until it is time to run it.
    */
   next_job = (job_item *)jobs_to_run->first();
   jobs_to_run->remove(next_job);

   dump_job(next_job, _("Dequeued job"));

   if (!next_job) {                /* we really should have something now */
      Emsg0(M_ABORT, 0, _("Scheduler logic error\n"));
   }

   /* Now wait for the time to run the job */
   for (;;) {
      time_t twait;
      now = time(NULL);
      twait = next_job->runtime - now;
      if (twait <= 0) {               /* time to run it */
         break;
      }
      bmicrosleep(twait, 0);
   }
   run = next_job->run;               /* pick up needed values */
   job = next_job->job;
   if (job->enabled) {
      dump_job(next_job, _("Run job"));
   }
   free(next_job);
   if (!job->enabled) {
      goto again;                     /* ignore this job */
   }
   run->last_run = now;               /* mark as run now */

   jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   ASSERT(job);
   set_jcr_defaults(jcr, job);
   if (run->level) {
      jcr->JobLevel = run->level;     /* override run level */
   }
   if (run->pool) {
      jcr->pool = run->pool;          /* override pool */
   }
   if (run->full_pool) {
      jcr->full_pool = run->full_pool; /* override full pool */
   }
   if (run->inc_pool) {
      jcr->inc_pool = run->inc_pool;  /* override inc pool */
   }
   if (run->dif_pool) {
      jcr->dif_pool = run->dif_pool;  /* override dif pool */
   }
   if (run->storage) {
      set_storage(jcr, run->storage); /* override storage */
   }
   if (run->msgs) {
      jcr->messages = run->msgs;      /* override messages */
   }
   if (run->Priority) {
      jcr->JobPriority = run->Priority;
   }
   if (run->spool_data_set) {
      jcr->spool_data = run->spool_data;
   }
   if (run->write_part_after_job_set) {
      jcr->write_part_after_job = run->write_part_after_job;
   }
   Dmsg0(200, "Leave wait_for_next_job()\n");
   return jcr;
}


/*
 * Shutdown the scheduler
 */
void term_scheduler()
{
   if (jobs_to_run) {
      job_item *je;
      /* Release all queued job entries to be run */
      foreach_dlist(je, jobs_to_run) {
         free(je);
      }
      delete jobs_to_run;
   }
}

/*
 * Find all jobs to be run this hour and the next hour.
 */
static void find_runs()
{
   time_t now, next_hour, runtime;
   RUN *run;
   JOB *job;
   SCHED *sched;
   struct tm tm;
   int minute;
   int hour, mday, wday, month, wom, woy;
   /* Items corresponding to above at the next hour */
   int nh_hour, nh_mday, nh_wday, nh_month, nh_wom, nh_woy, nh_year;

   Dmsg0(1200, "enter find_runs()\n");


   /* compute values for time now */
   now = time(NULL);
   localtime_r(&now, &tm);
   hour = tm.tm_hour;
   minute = tm.tm_min;
   mday = tm.tm_mday - 1;
   wday = tm.tm_wday;
   month = tm.tm_mon;
   wom = mday / 7;
   woy = tm_woy(now);                     /* get week of year */

   /*
    * Compute values for next hour from now.
    * We do this to be sure we don't miss a job while
    * sleeping.
    */
   next_hour = now + 3600;
   localtime_r(&next_hour, &tm);
   nh_hour = tm.tm_hour;
   nh_mday = tm.tm_mday - 1;
   nh_wday = tm.tm_wday;
   nh_month = tm.tm_mon;
   nh_year  = tm.tm_year;
   nh_wom = nh_mday / 7;
   nh_woy = tm_woy(now);                     /* get week of year */

   /* Loop through all jobs */
   LockRes();
   foreach_res(job, R_JOB) {
      sched = job->schedule;
      if (sched == NULL || !job->enabled) { /* scheduled? or enabled? */
         continue;                    /* no, skip this job */
      }
      Dmsg1(1200, "Got job: %s\n", job->hdr.name);
      for (run=sched->run; run; run=run->next) {
         bool run_now, run_nh;
         /*
          * Find runs scheduled between now and the next hour.
          */
#ifdef xxxx
         Dmsg0(000, "\n");
         Dmsg6(000, "run h=%d m=%d md=%d wd=%d wom=%d woy=%d\n",
            hour, month, mday, wday, wom, woy);
         Dmsg6(000, "bitset bsh=%d bsm=%d bsmd=%d bswd=%d bswom=%d bswoy=%d\n",
            bit_is_set(hour, run->hour),
            bit_is_set(month, run->month),
            bit_is_set(mday, run->mday),
            bit_is_set(wday, run->wday),
            bit_is_set(wom, run->wom),
            bit_is_set(woy, run->woy));

         Dmsg6(000, "nh_run h=%d m=%d md=%d wd=%d wom=%d woy=%d\n",
            nh_hour, nh_month, nh_mday, nh_wday, nh_wom, nh_woy);
         Dmsg6(000, "nh_bitset bsh=%d bsm=%d bsmd=%d bswd=%d bswom=%d bswoy=%d\n",
            bit_is_set(nh_hour, run->hour),
            bit_is_set(nh_month, run->month),
            bit_is_set(nh_mday, run->mday),
            bit_is_set(nh_wday, run->wday),
            bit_is_set(nh_wom, run->wom),
            bit_is_set(nh_woy, run->woy));
#endif

         run_now = bit_is_set(hour, run->hour) &&
            bit_is_set(mday, run->mday) &&
            bit_is_set(wday, run->wday) &&
            bit_is_set(month, run->month) &&
            bit_is_set(wom, run->wom) &&
            bit_is_set(woy, run->woy);

         run_nh = bit_is_set(nh_hour, run->hour) &&
            bit_is_set(nh_mday, run->mday) &&
            bit_is_set(nh_wday, run->wday) &&
            bit_is_set(nh_month, run->month) &&
            bit_is_set(nh_wom, run->wom) &&
            bit_is_set(nh_woy, run->woy);

         Dmsg2(1200, "run_now=%d run_nh=%d\n", run_now, run_nh);

         /* find time (time_t) job is to be run */
         localtime_r(&now, &tm);      /* reset tm structure */
         tm.tm_min = run->minute;     /* set run minute */
         tm.tm_sec = 0;               /* zero secs */
         if (run_now) {
            runtime = mktime(&tm);
            add_job(job, run, now, runtime);
         }
         /* If job is to be run in the next hour schedule it */
         if (run_nh) {
            /* Set correct values */
            tm.tm_hour = nh_hour;
            tm.tm_mday = nh_mday + 1; /* fixup because we biased for tests above */
            tm.tm_mon = nh_month;
            tm.tm_year = nh_year;
            runtime = mktime(&tm);
            add_job(job, run, now, runtime);
         }
      }
   }
   UnlockRes();
   Dmsg0(1200, "Leave find_runs()\n");
}

static void add_job(JOB *job, RUN *run, time_t now, time_t runtime)
{
   job_item *ji;
   bool inserted = false;
   /*
    * Don't run any job that ran less than a minute ago, but
    *  do run any job scheduled less than a minute ago.
    */
   if (((runtime - run->last_run) < 61) || ((runtime+59) < now)) {
#ifdef SCHED_DEBUG
      char dt[50], dt1[50], dt2[50];
      bstrftime_nc(dt, sizeof(dt), runtime);
      bstrftime_nc(dt1, sizeof(dt1), run->last_run);
      bstrftime_nc(dt2, sizeof(dt2), now);
      Dmsg4(000, "Drop: Job=\"%s\" run=%s. last_run=%s. now=%s\n", job->hdr.name,
            dt, dt1, dt2);
      fflush(stdout);
#endif
      return;
   }
   /* accept to run this job */
   job_item *je = (job_item *)malloc(sizeof(job_item));
   je->run = run;
   je->job = job;
   je->runtime = runtime;
   if (run->Priority) {
      je->Priority = run->Priority;
   } else {
      je->Priority = job->Priority;
   }

   /* Add this job to the wait queue in runtime, priority sorted order */
   foreach_dlist(ji, jobs_to_run) {
      if (ji->runtime > je->runtime ||
          (ji->runtime == je->runtime && ji->Priority > je->Priority)) {
         jobs_to_run->insert_before(je, ji);
         dump_job(je, _("Inserted job"));
         inserted = true;
         break;
      }
   }
   /* If place not found in queue, append it */
   if (!inserted) {
      jobs_to_run->append(je);
      dump_job(je, _("Appended job"));
   }
#ifdef SCHED_DEBUG
   foreach_dlist(ji, jobs_to_run) {
      dump_job(ji, _("Run queue"));
   }
   Dmsg0(000, "End run queue\n");
#endif
}

static void dump_job(job_item *ji, const char *msg)
{
#ifdef SCHED_DEBUG
   char dt[MAX_TIME_LENGTH];
   int save_debug = debug_level;
   debug_level = 200;
   if (debug_level < 200) {
      return;
   }
   bstrftime_nc(dt, sizeof(dt), ji->runtime);
   Dmsg4(200, "%s: Job=%s priority=%d run %s\n", msg, ji->job->hdr.name,
      ji->Priority, dt);
   fflush(stdout);
   debug_level = save_debug;
#endif
}
