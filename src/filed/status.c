/*
 *  Bacula File Daemon Status routines
 *
 *    Kern Sibbald, August MMI
 *
 *   Version $Id: status.c,v 1.43 2004/10/13 16:10:09 kerns Exp $
 *
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
#include "filed.h"

extern char my_name[];
extern int num_jobs_run;
extern time_t daemon_start_time;
extern bool get_trace(void);

/* Forward referenced functions */
static void  list_terminated_jobs(void sendit(const char *msg, int len, void *sarg), void *arg);
static void bsock_sendit(const char *msg, int len, void *arg);
static const char *level_to_str(int level);

/* Static variables */
static char qstatus[] = ".status %s\n";

static char OKqstatus[]   = "2000 OK .status\n";
static char DotStatusJob[] = "JobId=%d JobStatus=%c JobErrors=%d\n";

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
static int privs = 0;
#endif

/*
 * General status generator
 */
static void do_status(void sendit(const char *msg, int len, void *sarg), void *arg) 
{
   int sec, bps;
   char *msg, b1[32], b2[32], b3[32], b4[32];
   int found, len;
   JCR *njcr;
   char dt[MAX_TIME_LENGTH];

   msg = (char *)get_pool_memory(PM_MESSAGE);
   found = 0;
   len = Mmsg(msg, "%s Version: " VERSION " (" BDATE ") %s %s %s\n", my_name,
	      HOST_OS, DISTNAME, DISTVER);
   sendit(msg, len, arg);
   bstrftime_nc(dt, sizeof(dt), daemon_start_time);
   len = Mmsg(msg, _("Daemon started %s, %d Job%s run since started.\n"), 
        dt, num_jobs_run, num_jobs_run == 1 ? "" : "s");
   sendit(msg, len, arg);
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
   if (debug_level > 0) {
      if (!privs) {
	 privs = enable_backup_privileges(NULL, 1);
      }
      len = Mmsg(msg, 
         _(" Priv 0x%x APIs=%sOPT,%sATP,%sLPV,%sGFAE,%sBR,%sBW,%sSPSP\n"), privs,
         p_OpenProcessToken?"":"!",
         p_AdjustTokenPrivileges?"":"!",
         p_LookupPrivilegeValue?"":"!",
         p_GetFileAttributesEx?"":"!",
         p_BackupRead?"":"!",
         p_BackupWrite?"":"!",
         p_SetProcessShutdownParameters?"":"!");
      sendit(msg, len, arg);
   }
#endif
   if (debug_level > 0) {
      len = Mmsg(msg, _(" Heap: bytes=%s max_bytes=%s bufs=%s max_bufs=%s\n"),
	    edit_uint64_with_commas(sm_bytes, b1),
	    edit_uint64_with_commas(sm_max_bytes, b2),
	    edit_uint64_with_commas(sm_buffers, b3),
	    edit_uint64_with_commas(sm_max_buffers, b4));
      sendit(msg, len, arg);
      len = Mmsg(msg, _(" Sizeof: off_t=%d size_t=%d debug=%d trace=%d\n"), 
	    sizeof(off_t), sizeof(size_t), debug_level, get_trace());
      sendit(msg, len, arg);
   }

   list_terminated_jobs(sendit, arg);

   /*
    * List running jobs  
    */
   Dmsg0(1000, "Begin status jcr loop.\n");
   len = Mmsg(msg, _("Running Jobs:\n"));
   sendit(msg, len, arg);
   lock_jcr_chain();
   foreach_jcr(njcr) {
      bstrftime_nc(dt, sizeof(dt), njcr->start_time);
      if (njcr->JobId == 0) {
         len = Mmsg(msg, _("Director connected at: %s\n"), dt);
      } else {
         len = Mmsg(msg, _("JobId %d Job %s is running.\n"), 
		    njcr->JobId, njcr->Job);
	 sendit(msg, len, arg);
         len = Mmsg(msg, _("    %s Job started: %s\n"), 
		    job_type_to_str(njcr->JobType), dt);
      }
      sendit(msg, len, arg);
      if (njcr->JobId == 0) {
	 free_locked_jcr(njcr);
	 continue;
      }
      sec = time(NULL) - njcr->start_time;
      if (sec <= 0) {
	 sec = 1;
      }
      bps = (int)(njcr->JobBytes / sec);
      len = Mmsg(msg,  _("    Files=%s Bytes=%s Bytes/sec=%s\n"), 
	   edit_uint64_with_commas(njcr->JobFiles, b1),
	   edit_uint64_with_commas(njcr->JobBytes, b2),
	   edit_uint64_with_commas(bps, b3));
      sendit(msg, len, arg);
      len = Mmsg(msg, _("    Files Examined=%s\n"), 
	   edit_uint64_with_commas(njcr->num_files_examined, b1));
      sendit(msg, len, arg);
      if (njcr->JobFiles > 0) {
	 P(njcr->mutex);
         len = Mmsg(msg, _("    Processing file: %s\n"), njcr->last_fname);
	 V(njcr->mutex);
	 sendit(msg, len, arg);
      }

      found = 1;
      if (njcr->store_bsock) {
         len = Mmsg(msg, "    SDReadSeqNo=%" lld " fd=%d\n",
	     njcr->store_bsock->read_seqno, njcr->store_bsock->fd);
	 sendit(msg, len, arg);
      } else {
         len = Mmsg(msg, _("    SDSocket closed.\n"));
	 sendit(msg, len, arg);
      }
      free_locked_jcr(njcr);
   }
   unlock_jcr_chain();
   Dmsg0(1000, "Begin status jcr loop.\n");
   if (!found) {
      len = Mmsg(msg, _("No Jobs running.\n"));
      sendit(msg, len, arg);
   }
   len = Mmsg(msg, _("====\n"));
   sendit(msg, len, arg);
   free_pool_memory(msg);
}

static void  list_terminated_jobs(void sendit(const char *msg, int len, void *sarg), void *arg) 
{
   char dt[MAX_TIME_LENGTH], b1[30], b2[30];
   char level[10];
   struct s_last_job *je;
   const char *msg;

   if (last_jobs->size() == 0) {
      msg = _("No Terminated Jobs.\n"); 
      sendit(msg, strlen(msg), arg);
      return;
   }
   lock_last_jobs_list();
   sendit("\n", 1, arg);               /* send separately */
   msg =  _("Terminated Jobs:\n"); 
   sendit(msg, strlen(msg), arg);
   msg =  _(" JobId  Level     Files         Bytes  Status   Finished        Name \n");
   sendit(msg, strlen(msg), arg);
   msg = _("======================================================================\n"); 
   sendit(msg, strlen(msg), arg);
   foreach_dlist(je, last_jobs) {
      char JobName[MAX_NAME_LENGTH];
      const char *termstat;
      char buf[1000];

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
         termstat = "Created";
	 break;
      case JS_FatalError:
      case JS_ErrorTerminated:
         termstat = "Error";
	 break;
      case JS_Differences:
         termstat = "Diffs";
	 break;
      case JS_Canceled:
         termstat = "Cancel";
	 break;
      case JS_Terminated:
         termstat = "OK";
	 break;
      default:
         termstat = "Other";
	 break;
      }
      bstrncpy(JobName, je->Job, sizeof(JobName));
      /* There are three periods after the Job name */
      char *p;
      for (int i=0; i<3; i++) {
         if ((p=strrchr(JobName, '.')) != NULL) {
	    *p = 0;
	 }
      }
      bsnprintf(buf, sizeof(buf), _("%6d  %-6s %8s %14s %-7s  %-8s %s\n"), 
	 je->JobId,
	 level, 
	 edit_uint64_with_commas(je->JobFiles, b1),
	 edit_uint64_with_commas(je->JobBytes, b2), 
	 termstat,
	 dt, JobName);
      sendit(buf, strlen(buf), arg);
   }
   sendit("====\n", 5, arg);
   unlock_last_jobs_list();
}


/*
 * Send to bsock (Director or Console)
 */
static void bsock_sendit(const char *msg, int len, void *arg)
{
   BSOCK *user = (BSOCK *)arg;

   user->msg = check_pool_memory_size(user->msg, len+1);
   memcpy(user->msg, msg, len+1);
   user->msglen = len+1;
   bnet_send(user);
}
				   
/*
 * Status command from Director
 */
int status_cmd(JCR *jcr)
{
   BSOCK *user = jcr->dir_bsock;

   bnet_fsend(user, "\n");
   do_status(bsock_sendit, (void *)user);

   bnet_sig(user, BNET_EOD);
   return 1;
}

/*
 * .status command from Director
 */
int qstatus_cmd(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   POOLMEM *time;
   JCR *njcr;
   s_last_job* job;

   time = get_memory(dir->msglen+1);
   
   if (sscanf(dir->msg, qstatus, time) != 1) {
      pm_strcpy(&jcr->errmsg, dir->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Bad .status command: %s\n"), jcr->errmsg);
      bnet_fsend(dir, "2900 Bad .status command, missing argument.\n");
      bnet_sig(dir, BNET_EOD);
      free_memory(time);
      return 0;
   }
   unbash_spaces(time);
   
   if (strcmp(time, "current") == 0) {
      bnet_fsend(dir, OKqstatus, time);
      lock_jcr_chain();
      foreach_jcr(njcr) {
	 if (njcr->JobId != 0) {
	    bnet_fsend(dir, DotStatusJob, njcr->JobId, njcr->JobStatus, njcr->JobErrors);
	 }
	 free_locked_jcr(njcr);
      }
      unlock_jcr_chain();
   }
   else if (strcmp(time, "last") == 0) {
      bnet_fsend(dir, OKqstatus, time);
      if ((last_jobs) && (last_jobs->size() > 0)) {
	 job = (s_last_job*)last_jobs->last();
	 bnet_fsend(dir, DotStatusJob, job->JobId, job->JobStatus, job->Errors);
      }
   }
   else {
      pm_strcpy(&jcr->errmsg, dir->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Bad .status command: %s\n"), jcr->errmsg);
      bnet_fsend(dir, "2900 Bad .status command, wrong argument.\n");
      bnet_sig(dir, BNET_EOD);
      free_memory(time);
      return 0;
   }
   
   bnet_sig(dir, BNET_EOD);
   free_memory(time);
   return 1;
}

/*
 * Convert Job Level into a string
 */
static const char *level_to_str(int level) 
{
   const char *str;

   switch (level) {
   case L_BASE:
      str = _("Base");
   case L_FULL:
      str = _("Full");
      break;
   case L_INCREMENTAL:
      str = _("Incremental");
      break;
   case L_DIFFERENTIAL:
      str = _("Differential");
      break;
   case L_SINCE:
      str = _("Since");
      break;
   case L_VERIFY_CATALOG:
      str = _("Verify Catalog");
      break;
   case L_VERIFY_INIT:
      str = _("Init Catalog");
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      str = _("Volume to Catalog");
      break;
   case L_VERIFY_DISK_TO_CATALOG:
      str = _("Disk to Catalog");
      break;
   case L_VERIFY_DATA:
      str = _("Data");
      break;
   case L_NONE:
      str = " ";
      break;
   default:
      str = _("Unknown Job Level");
      break;
   }
   return str;
}


#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
#include <windows.h>

int bacstat = 0;

struct s_win32_arg {
   HWND hwnd;
   int idlist;
};

/*
 * Put message in Window List Box
 */
static void win32_sendit(const char *msg, int len, void *marg)
{
   struct s_win32_arg *arg = (struct s_win32_arg *)marg;

   if (len > 0 && msg[len-1] == '\n') {
       // when compiling with visual studio some strings are read-only 
       // and cause access violations.	So we creat a tmp copy.
       char *_msg = (char *)alloca(len);
       bstrncpy(_msg, msg, len);
       msg = _msg;
   }
   SendDlgItemMessage(arg->hwnd, arg->idlist, LB_ADDSTRING, 0, (LONG)msg);
   
}

void FillStatusBox(HWND hwnd, int idlist)
{
   struct s_win32_arg arg;

   arg.hwnd = hwnd;
   arg.idlist = idlist;

   /* Empty box */
   for ( ; SendDlgItemMessage(hwnd, idlist, LB_DELETESTRING, 0, (LONG)0) > 0; )
      { }
   do_status(win32_sendit, (void *)&arg);
}

char *bac_status(char *buf, int buf_len)
{
   JCR *njcr;
   const char *termstat = _("Bacula Idle");
   struct s_last_job *job;
   int stat = 0;		      /* Idle */

   if (!last_jobs) {
      goto done;
   }
   Dmsg0(1000, "Begin bac_status jcr loop.\n");
   lock_jcr_chain();
   foreach_jcr(njcr) {
      if (njcr->JobId != 0) {
	 stat = JS_Running;
         termstat = _("Bacula Running");
	 free_locked_jcr(njcr);
	 break;
      }
      free_locked_jcr(njcr);
   }
   unlock_jcr_chain();
   if (stat != 0) {
      goto done;
   }
   if (last_jobs->size() > 0) {
      job = (struct s_last_job *)last_jobs->last();
      stat = job->JobStatus;
      switch (job->JobStatus) {
      case JS_Canceled:
         termstat = _("Last Job Canceled");
	 break;
      case JS_ErrorTerminated:
      case JS_FatalError:  
         termstat = _("Last Job Failed");
	 break;
      default:
	 if (job->Errors) {
            termstat = _("Last Job had Warnings");
	 }
	 break;
      }
   }
   Dmsg0(1000, "End bac_status jcr loop.\n");
done:
   bacstat = stat;
   if (buf) {
      bstrncpy(buf, termstat, buf_len);
   }
   return buf;
}

#endif /* HAVE_CYGWIN */
