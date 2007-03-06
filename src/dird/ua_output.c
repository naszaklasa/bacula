/*
 *
 *   Bacula Director -- User Agent Output Commands
 *     I.e. messages, listing database, showing resources, ...
 *
 *     Kern Sibbald, September MM
 *
 *   Version $Id: ua_output.c,v 1.48 2004/11/13 10:34:17 kerns Exp $
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

/* Imported subroutines */

/* Imported variables */
extern int r_first;
extern int r_last;
extern RES_TABLE resources[];
extern RES **res_head;
extern int console_msg_pending;
extern FILE *con_fd;
extern brwlock_t con_lock;

/* Imported functions */

/* Forward referenced functions */
static int do_list_cmd(UAContext *ua, const char *cmd, e_list_type llist);
static bool list_nextvol(UAContext *ua);

/*
 * Turn auto display of console messages on/off
 */
int autodisplay_cmd(UAContext *ua, const char *cmd)
{
   static const char *kw[] = {
      N_("on"), 
      N_("off"),
      NULL};

   switch (find_arg_keyword(ua, kw)) {
   case 0:
      ua->auto_display_messages = 1;
      break;
   case 1:
      ua->auto_display_messages = 0;
      break;
   default:
      bsendmsg(ua, _("ON or OFF keyword missing.\n"));
      break;
   }
   return 1; 
}

/*
 * Turn gui processing on/off
 */
int gui_cmd(UAContext *ua, const char *cmd)
{
   static const char *kw[] = {
      N_("on"), 
      N_("off"),
      NULL};

   switch (find_arg_keyword(ua, kw)) {
   case 0:
      ua->jcr->gui = true;
      break;
   case 1:
      ua->jcr->gui = false;
      break;
   default:
      bsendmsg(ua, _("ON or OFF keyword missing.\n"));
      break;
   }
   return 1; 
}



struct showstruct {const char *res_name; int type;};
static struct showstruct reses[] = {
   {N_("directors"),  R_DIRECTOR},
   {N_("clients"),    R_CLIENT},
   {N_("counters"),   R_COUNTER},
   {N_("jobs"),       R_JOB},
   {N_("storages"),   R_STORAGE},
   {N_("catalogs"),   R_CATALOG},
   {N_("schedules"),  R_SCHEDULE},
   {N_("filesets"),   R_FILESET},
   {N_("pools"),      R_POOL},
   {N_("messages"),   R_MSGS},
   {N_("all"),        -1},
   {N_("help"),       -2},
   {NULL,	    0}
};


/*
 *  Displays Resources
 *
 *  show all
 *  show <resource-keyword-name>  e.g. show directors
 *  show <resource-keyword-name>=<name> e.g. show director=HeadMan
 *
 */
int show_cmd(UAContext *ua, const char *cmd)
{
   int i, j, type, len; 
   int recurse;
   char *res_name;
   RES *res = NULL;

   Dmsg1(20, "show: %s\n", ua->UA_sock->msg);


   LockRes();
   for (i=1; i<ua->argc; i++) {
      type = 0;
      res_name = ua->argk[i]; 
      if (!ua->argv[i]) {	      /* was a name given? */
	 /* No name, dump all resources of specified type */
	 recurse = 1;
	 len = strlen(res_name);
	 for (j=0; reses[j].res_name; j++) {
	    if (strncasecmp(res_name, _(reses[j].res_name), len) == 0) {
	       type = reses[j].type;
	       if (type > 0) {
		  res = res_head[type-r_first];
//		  res = resources[type-r_first].res_head;
	       } else {
		  res = NULL;
	       }
	       break;
	    }
	 }

      } else {
	 /* Dump a single resource with specified name */
	 recurse = 0;
	 len = strlen(res_name);
	 for (j=0; reses[j].res_name; j++) {
	    if (strncasecmp(res_name, _(reses[j].res_name), len) == 0) {
	       type = reses[j].type;
	       res = (RES *)GetResWithName(type, ua->argv[i]);
	       if (!res) {
		  type = -3;
	       }
	       break;
	    }
	 }
      }

      switch (type) {
      case -1:				 /* all */
	 for (j=r_first; j<=r_last; j++) {
	    dump_resource(j, res_head[j-r_first], bsendmsg, ua);     
//	    dump_resource(j, resources[j-r_first].res_head, bsendmsg, ua);     
	 }
	 break;
      case -2:
         bsendmsg(ua, _("Keywords for the show command are:\n"));
	 for (j=0; reses[j].res_name; j++) {
            bsendmsg(ua, "%s\n", _(reses[j].res_name));
	 }
	 goto bail_out;
      case -3:
         bsendmsg(ua, _("%s resource %s not found.\n"), res_name, ua->argv[i]);
	 goto bail_out;
      case 0:
         bsendmsg(ua, _("Resource %s not found\n"), res_name);
	 goto bail_out;
      default:
	 dump_resource(recurse?type:-type, res, bsendmsg, ua);
	 break;
      }
   }
bail_out:
   UnlockRes();
   return 1;
}




/*
 *  List contents of database
 *
 *  list jobs		- lists all jobs run
 *  list jobid=nnn	- list job data for jobid
 *  list job=name	- list job data for job
 *  list jobmedia jobid=<nn>
 *  list jobmedia job=name
 *  list files jobid=<nn> - list files saved for job nn
 *  list files job=name
 *  list pools		- list pool records
 *  list jobtotals	- list totals for all jobs
 *  list media		- list media for given pool (deprecated)
 *  list volumes	- list Volumes
 *  list clients	- list clients
 *  list nextvol job=xx  - list the next vol to be used by job
 *  list nextvolume job=xx - same as above.
 *
 */

/* Do long or full listing */
int llist_cmd(UAContext *ua, const char *cmd)
{
   return do_list_cmd(ua, cmd, VERT_LIST);
}

/* Do short or summary listing */
int list_cmd(UAContext *ua, const char *cmd)
{
   return do_list_cmd(ua, cmd, HORZ_LIST);
}

static int do_list_cmd(UAContext *ua, const char *cmd, e_list_type llist)
{
   POOLMEM *VolumeName;
   int jobid, n;
   int i, j;
   JOB_DBR jr;
   POOL_DBR pr;
   MEDIA_DBR mr;

   if (!open_db(ua))
      return 1;

   memset(&jr, 0, sizeof(jr));
   memset(&pr, 0, sizeof(pr));
   memset(&mr, 0, sizeof(mr));

   Dmsg1(20, "list: %s\n", cmd);

   if (!ua->db) {
      bsendmsg(ua, _("Hey! DB is NULL\n"));
   }

   /* Scan arguments looking for things to do */
   for (i=1; i<ua->argc; i++) {
      /* List JOBS */
      if (strcasecmp(ua->argk[i], _("jobs")) == 0) {
	 db_list_job_records(ua->jcr, ua->db, &jr, prtit, ua, llist);

	 /* List JOBTOTALS */
      } else if (strcasecmp(ua->argk[i], _("jobtotals")) == 0) {
	 db_list_job_totals(ua->jcr, ua->db, &jr, prtit, ua);

      /* List JOBID */
      } else if (strcasecmp(ua->argk[i], _("jobid")) == 0) {
	 if (ua->argv[i]) {
	    jobid = atoi(ua->argv[i]);
	    if (jobid > 0) {
	       jr.JobId = jobid;
	       db_list_job_records(ua->jcr, ua->db, &jr, prtit, ua, llist);
	    }
	 }

      /* List JOB */
      } else if (strcasecmp(ua->argk[i], _("job")) == 0 && ua->argv[i]) {
	 bstrncpy(jr.Job, ua->argv[i], MAX_NAME_LENGTH);
	 jr.JobId = 0;
	 db_list_job_records(ua->jcr, ua->db, &jr, prtit, ua, llist);

      /* List FILES */
      } else if (strcasecmp(ua->argk[i], _("files")) == 0) {

	 for (j=i+1; j<ua->argc; j++) {
            if (strcasecmp(ua->argk[j], _("job")) == 0 && ua->argv[j]) {
	       bstrncpy(jr.Job, ua->argv[j], MAX_NAME_LENGTH);
	       jr.JobId = 0;
	       db_get_job_record(ua->jcr, ua->db, &jr);
	       jobid = jr.JobId;
            } else if (strcasecmp(ua->argk[j], _("jobid")) == 0 && ua->argv[j]) {
	       jobid = atoi(ua->argv[j]);
	    } else {
	       continue;
	    }
	    if (jobid > 0) {
	       db_list_files_for_job(ua->jcr, ua->db, jobid, prtit, ua);
	    }
	 }
      
      /* List JOBMEDIA */
      } else if (strcasecmp(ua->argk[i], _("jobmedia")) == 0) {
	 int done = FALSE;
	 for (j=i+1; j<ua->argc; j++) {
            if (strcasecmp(ua->argk[j], _("job")) == 0 && ua->argv[j]) {
	       bstrncpy(jr.Job, ua->argv[j], MAX_NAME_LENGTH);
	       jr.JobId = 0;
	       db_get_job_record(ua->jcr, ua->db, &jr);
	       jobid = jr.JobId;
            } else if (strcasecmp(ua->argk[j], _("jobid")) == 0 && ua->argv[j]) {
	       jobid = atoi(ua->argv[j]);
	    } else {
	       continue;
	    }
	    db_list_jobmedia_records(ua->jcr, ua->db, jobid, prtit, ua, llist);
	    done = TRUE;
	 }
	 if (!done) {
	    /* List for all jobs (jobid=0) */
	    db_list_jobmedia_records(ua->jcr, ua->db, 0, prtit, ua, llist);
	 }

      /* List POOLS */
      } else if (strcasecmp(ua->argk[i], _("pools")) == 0) {
	 db_list_pool_records(ua->jcr, ua->db, prtit, ua, llist);

      } else if (strcasecmp(ua->argk[i], _("clients")) == 0) {
	 db_list_client_records(ua->jcr, ua->db, prtit, ua, llist);


      /* List MEDIA or VOLUMES */
      } else if (strcasecmp(ua->argk[i], _("media")) == 0 ||
                 strcasecmp(ua->argk[i], _("volumes")) == 0) {
	 bool done = false;
	 for (j=i+1; j<ua->argc; j++) {
            if (strcasecmp(ua->argk[j], _("job")) == 0 && ua->argv[j]) {
	       bstrncpy(jr.Job, ua->argv[j], MAX_NAME_LENGTH);
	       jr.JobId = 0;
	       db_get_job_record(ua->jcr, ua->db, &jr);
	       jobid = jr.JobId;
            } else if (strcasecmp(ua->argk[j], _("jobid")) == 0 && ua->argv[j]) {
	       jobid = atoi(ua->argv[j]);
	    } else {
	       continue;
	    }
	    VolumeName = get_pool_memory(PM_FNAME);
	    n = db_get_job_volume_names(ua->jcr, ua->db, jobid, &VolumeName);
            bsendmsg(ua, _("Jobid %d used %d Volume(s): %s\n"), jobid, n, VolumeName);
	    free_pool_memory(VolumeName);
	    done = true;
	 }
	 /* if no job or jobid keyword found, then we list all media */
	 if (!done) {
	    int num_pools;
	    uint32_t *ids;
	    /* Is a specific pool wanted? */
	    for (i=1; i<ua->argc; i++) {
               if (strcasecmp(ua->argk[i], _("pool")) == 0) {
		  if (!get_pool_dbr(ua, &pr)) {
                     bsendmsg(ua, _("No Pool specified.\n"));
		     return 1;
		  }
		  mr.PoolId = pr.PoolId;
		  db_list_media_records(ua->jcr, ua->db, &mr, prtit, ua, llist);
		  return 1;
	       }
	    }
	    /* List Volumes in all pools */
	    if (!db_get_pool_ids(ua->jcr, ua->db, &num_pools, &ids)) {
               bsendmsg(ua, _("Error obtaining pool ids. ERR=%s\n"), 
			db_strerror(ua->db));
	       return 1;
	    }
	    if (num_pools <= 0) {
	       return 1;
	    }
	    for (i=0; i < num_pools; i++) {
	       pr.PoolId = ids[i];
	       if (db_get_pool_record(ua->jcr, ua->db, &pr)) {
                  bsendmsg(ua, _("Pool: %s\n"), pr.Name);
	       }
	       mr.PoolId = ids[i];
	       db_list_media_records(ua->jcr, ua->db, &mr, prtit, ua, llist);
	    }
	    free(ids);
	    return 1;
	 }
      /* List a specific volume */
      } else if (strcasecmp(ua->argk[i], _("volume")) == 0) {
	 if (!ua->argv[i]) {
            bsendmsg(ua, _("No Volume Name specified.\n"));
	    return 1;
	 }
	 bstrncpy(mr.VolumeName, ua->argv[i], sizeof(mr.VolumeName));
	 db_list_media_records(ua->jcr, ua->db, &mr, prtit, ua, llist);
	 return 1;
      /* List next volume */
      } else if (strcasecmp(ua->argk[i], _("nextvol")) == 0 || 
                 strcasecmp(ua->argk[i], _("nextvolume")) == 0) {
	 list_nextvol(ua);
      } else {
         bsendmsg(ua, _("Unknown list keyword: %s\n"), NPRT(ua->argk[i]));
      }
   }
   return 1;
}

static bool list_nextvol(UAContext *ua)
{
   JOB *job;
   JCR *jcr = ua->jcr;
   POOL *pool;
   RUN *run;
   time_t runtime;
   bool found = false;
   MEDIA_DBR mr;

   memset(&mr, 0, sizeof(mr));
   int i = find_arg_with_value(ua, "job");
   if (i <= 0) {
      if ((job = select_job_resource(ua)) == NULL) {
	 return false;
      }
   } else {
      job = (JOB *)GetResWithName(R_JOB, ua->argv[i]);
      if (!job) {
         Jmsg(jcr, M_ERROR, 0, _("%s is not a job name.\n"), ua->argv[i]);
	 if ((job = select_job_resource(ua)) == NULL) {
	    return false;
	 }
      }
   }
   for (run=NULL; (run = find_next_run(run, job, runtime)); ) {
      pool = run ? run->pool : NULL;
      if (!complete_jcr_for_job(jcr, job, pool)) {
	 return false;
      }
     
      if (!find_next_volume_for_append(jcr, &mr, 0)) {
         bsendmsg(ua, _("Could not find next Volume.\n"));
      } else {
         bsendmsg(ua, _("The next Volume to be used by Job \"%s\" will be %s\n"), 
	    job->hdr.name, mr.VolumeName);
	 found = true;
      }
      if (jcr->db && jcr->db != ua->db) {
	 db_close_database(jcr, jcr->db);
	 jcr->db = NULL;
      }
   }
   if (!found) {
      bsendmsg(ua, _("Could not find next Volume.\n"));
      return false;
   }
   return true;
}


/* 
 * For a given job, we examine all his run records
 *  to see if it is scheduled today or tomorrow.
 */
RUN *find_next_run(RUN *run, JOB *job, time_t &runtime)
{
   time_t now, tomorrow;
   SCHED *sched;
   struct tm tm;
   int mday, wday, month, wom, tmday, twday, tmonth, twom, i;
   int woy, twoy;
   int tod, tom;

   sched = job->schedule;
   if (sched == NULL) { 	   /* scheduled? */
      return NULL;		   /* no nothing to report */
   }
   /* Break down current time into components */ 
   now = time(NULL);
   localtime_r(&now, &tm);
   mday = tm.tm_mday - 1;
   wday = tm.tm_wday;
   month = tm.tm_mon;
   wom = mday / 7;
   woy = tm_woy(now);

   /* Break down tomorrow into components */
   tomorrow = now + 60 * 60 * 24;
   localtime_r(&tomorrow, &tm);
   tmday = tm.tm_mday - 1;
   twday = tm.tm_wday;
   tmonth = tm.tm_mon;
   twom = tmday / 7;
   twoy  = tm_woy(tomorrow);

   if (run == NULL) {
      run = sched->run;
   } else {
      run = run->next;
   }
   for ( ; run; run=run->next) {
      /* 
       * Find runs in next 24 hours
       */
      tod = bit_is_set(mday, run->mday) && bit_is_set(wday, run->wday) && 
	    bit_is_set(month, run->month) && bit_is_set(wom, run->wom) &&
	    bit_is_set(woy, run->woy);

      tom = bit_is_set(tmday, run->mday) && bit_is_set(twday, run->wday) &&
	    bit_is_set(tmonth, run->month) && bit_is_set(twom, run->wom) &&
	    bit_is_set(twoy, run->woy);

#ifdef xxx
      Dmsg2(000, "tod=%d tom=%d\n", tod, tom);
      Dmsg1(000, "bit_set_mday=%d\n", bit_is_set(mday, run->mday));
      Dmsg1(000, "bit_set_wday=%d\n", bit_is_set(wday, run->wday));
      Dmsg1(000, "bit_set_month=%d\n", bit_is_set(month, run->month));
      Dmsg1(000, "bit_set_wom=%d\n", bit_is_set(wom, run->wom));
      Dmsg1(000, "bit_set_woy=%d\n", bit_is_set(woy, run->woy));
#endif
      if (tod) {		   /* Jobs scheduled today (next 24 hours) */
#ifdef xxx
	 char buf[300], num[10];
         bsnprintf(buf, sizeof(buf), "tm.hour=%d hour=", tm.tm_hour);
	 for (i=0; i<24; i++) {
	    if (bit_is_set(i, run->hour)) {
               bsnprintf(num, sizeof(num), "%d ", i);
	       bstrncat(buf, num, sizeof(buf));
	    }
	 }
         bstrncat(buf, "\n", sizeof(buf));
         Dmsg1(000, "%s", buf);
#endif 
	 /* find time (time_t) job is to be run */
	 localtime_r(&now, &tm);
	 for (i=tm.tm_hour; i < 24; i++) {
	    if (bit_is_set(i, run->hour)) {
	       tm.tm_hour = i;
	       tm.tm_min = run->minute;
	       tm.tm_sec = 0;
	       runtime = mktime(&tm);
               Dmsg2(200, "now=%d runtime=%d\n", now, runtime);
	       if (runtime > now) {
                  Dmsg2(200, "Found it level=%d %c\n", run->level, run->level);
		  return run;	      /* found it, return run resource */
	       }
	    }
	 }
      }

//    Dmsg2(200, "runtime=%d now=%d\n", runtime, now);
      if (tom) {		/* look at jobs scheduled tomorrow */
	 localtime_r(&tomorrow, &tm);
	 for (i=0; i < 24; i++) {
	    if (bit_is_set(i, run->hour)) {
	       tm.tm_hour = i;
	       tm.tm_min = run->minute;
	       tm.tm_sec = 0;
	       runtime = mktime(&tm);
               Dmsg2(200, "now=%d runtime=%d\n", now, runtime);
	       if (runtime < tomorrow) {
                  Dmsg2(200, "Found it level=%d %c\n", run->level, run->level);
		  return run;	      /* found it, return run resource */
	       }
	    }
	 }
      }
   } /* end for loop over runs */ 
   /* Nothing found */
   return NULL;
}
/* 
 * Fill in the remaining fields of the jcr as if it
 *  is going to run the job.
 */
int complete_jcr_for_job(JCR *jcr, JOB *job, POOL *pool)
{
   POOL_DBR pr;

   memset(&pr, 0, sizeof(POOL_DBR));   
   set_jcr_defaults(jcr, job);
   if (pool) {
      jcr->pool = pool; 	      /* override */
   }
   jcr->db = jcr->db=db_init_database(jcr, jcr->catalog->db_name, jcr->catalog->db_user,
		      jcr->catalog->db_password, jcr->catalog->db_address,
		      jcr->catalog->db_port, jcr->catalog->db_socket,	
		      jcr->catalog->mult_db_connections);
   if (!jcr->db || !db_open_database(jcr, jcr->db)) {
      Jmsg(jcr, M_FATAL, 0, _("Could not open database \"%s\".\n"),
		 jcr->catalog->db_name);
      if (jcr->db) {
         Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      }
      return 0;
   }
   bstrncpy(pr.Name, jcr->pool->hdr.name, sizeof(pr.Name));
   while (!db_get_pool_record(jcr, jcr->db, &pr)) { /* get by Name */
      /* Try to create the pool */
      if (create_pool(jcr, jcr->db, jcr->pool, POOL_OP_CREATE) < 0) {
         Jmsg(jcr, M_FATAL, 0, _("Pool %s not in database. %s"), pr.Name, 
	    db_strerror(jcr->db));
	 if (jcr->db) {
	    db_close_database(jcr, jcr->db);
	    jcr->db = NULL;
	 }
	 return 0;
      } else {
         Jmsg(jcr, M_INFO, 0, _("Pool %s created in database.\n"), pr.Name);
      }
   }
   jcr->PoolId = pr.PoolId; 
   jcr->jr.PoolId = pr.PoolId;
   return 1;
}


static void con_lock_release(void *arg)
{
   Vw(con_lock);
}

void do_messages(UAContext *ua, const char *cmd)
{
   char msg[2000];
   int mlen; 
   int do_truncate = FALSE;

   Pw(con_lock);
   pthread_cleanup_push(con_lock_release, (void *)NULL);
   rewind(con_fd);
   while (fgets(msg, sizeof(msg), con_fd)) {
      mlen = strlen(msg);
      ua->UA_sock->msg = check_pool_memory_size(ua->UA_sock->msg, mlen+1);
      strcpy(ua->UA_sock->msg, msg);
      ua->UA_sock->msglen = mlen;
      bnet_send(ua->UA_sock);
      do_truncate = TRUE;
   }
   if (do_truncate) {
      ftruncate(fileno(con_fd), 0L);
   }
   console_msg_pending = FALSE;
   ua->user_notified_msg_pending = FALSE;
   pthread_cleanup_pop(0);
   Vw(con_lock);
}


int qmessagescmd(UAContext *ua, const char *cmd)
{
   if (console_msg_pending && ua->auto_display_messages) {
      do_messages(ua, cmd);
   }
   return 1;
}

int messagescmd(UAContext *ua, const char *cmd)
{
   if (console_msg_pending) {
      do_messages(ua, cmd);
   } else {
      bnet_fsend(ua->UA_sock, _("You have no messages.\n"));
   }
   return 1;
}

/*
 * Callback routine for "printing" database file listing
 */
void prtit(void *ctx, const char *msg)
{
   UAContext *ua = (UAContext *)ctx;
 
   bnet_fsend(ua->UA_sock, "%s", msg);
}

/* 
 * Format message and send to other end.  

 * If the UA_sock is NULL, it means that there is no user
 * agent, so we are being called from Bacula core. In
 * that case direct the messages to the Job.
 */
void bsendmsg(void *ctx, const char *fmt, ...)
{
   va_list arg_ptr;
   UAContext *ua = (UAContext *)ctx;
   BSOCK *bs = ua->UA_sock;
   int maxlen, len;
   POOLMEM *msg;

   if (bs) {
      msg = bs->msg;
   } else {
      msg = get_pool_memory(PM_EMSG);
   }

again:
   maxlen = sizeof_pool_memory(msg) - 1;
   va_start(arg_ptr, fmt);
   len = bvsnprintf(msg, maxlen, fmt, arg_ptr);
   va_end(arg_ptr);
   if (len < 0 || len >= maxlen) {
      msg = realloc_pool_memory(msg, maxlen + maxlen/2);
      goto again;
   }

   if (bs) {
      bs->msg = msg;
      bs->msglen = len;
      bnet_send(bs);
   } else {			      /* No UA, send to Job */
      Jmsg(ua->jcr, M_INFO, 0, "%s", msg);
      free_pool_memory(msg);
   }

}
