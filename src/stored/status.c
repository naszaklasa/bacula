/*
 *  This file handles the status command
 *
 *     Kern Sibbald, May MMIII
 *
 *   Version $Id: status.c,v 1.25.4.2 2005/02/15 11:51:04 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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
#include "stored.h"

/* Exported variables */

/* Imported variables */
extern BSOCK *filed_chan;
extern int r_first, r_last;
extern struct s_res resources[];
extern char my_name[];
extern time_t daemon_start_time;
extern int num_jobs_run;


/* Static variables */
static char qstatus[] = ".status %127s\n";

static char OKqstatus[]   = "3000 OK .status\n";
static char DotStatusJob[] = "JobId=%d JobStatus=%c JobErrors=%d\n";


/* Forward referenced functions */
static void send_blocked_status(JCR *jcr, DEVICE *dev);
static void list_terminated_jobs(void *arg);
static void list_running_jobs(BSOCK *user);
static void sendit(const char *msg, int len, void *arg);
static const char *level_to_str(int level);


/*
 * Status command from Director
 */
bool status_cmd(JCR *jcr)
{
   DEVRES *device;
   DEVICE *dev;
   BSOCK *user = jcr->dir_bsock;
   char dt[MAX_TIME_LENGTH];
   char b1[30], b2[30], b3[30];
   int bpb;

   bnet_fsend(user, "\n%s Version: " VERSION " (" BDATE ") %s %s %s\n", my_name,
	      HOST_OS, DISTNAME, DISTVER);
   bstrftime_nc(dt, sizeof(dt), daemon_start_time);
   bnet_fsend(user, _("Daemon started %s, %d Job%s run since started.\n"), dt, num_jobs_run,
        num_jobs_run == 1 ? "" : "s");
   if (debug_level > 0) {
      char b1[35], b2[35], b3[35], b4[35];
      bnet_fsend(user, _(" Heap: bytes=%s max_bytes=%s bufs=%s max_bufs=%s\n"),
	    edit_uint64_with_commas(sm_bytes, b1),
	    edit_uint64_with_commas(sm_max_bytes, b2),
	    edit_uint64_with_commas(sm_buffers, b3),
	    edit_uint64_with_commas(sm_max_buffers, b4));
   }

   /*
    * List running jobs
    */
   list_running_jobs(user);

   /*
    * List terminated jobs
    */
   list_terminated_jobs(user);

   /*
    * List devices
    */
   bnet_fsend(user, _("\nDevice status:\n"));
   LockRes();
   foreach_res(device, R_DEVICE) {
      dev = device->dev;
      if (dev && dev->is_open()) {
	 if (dev->is_labeled()) {
            bnet_fsend(user, _("Device \"%s\" is mounted with Volume \"%s\"\n"),
	       dev_name(dev), dev->VolHdr.VolName);
	 } else {
            bnet_fsend(user, _("Device \"%s\" open but no Bacula volume is mounted.\n"), dev_name(dev));
	 }
	 send_blocked_status(jcr, dev);
	 if (dev_state(dev, ST_APPEND)) {
	    bpb = dev->VolCatInfo.VolCatBlocks;
	    if (bpb <= 0) {
	       bpb = 1;
	    }
	    bpb = dev->VolCatInfo.VolCatBytes / bpb;
            bnet_fsend(user, _("    Total Bytes=%s Blocks=%s Bytes/block=%s\n"),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatBytes, b1),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatBlocks, b2),
	       edit_uint64_with_commas(bpb, b3));
	 } else {  /* reading */
	    bpb = dev->VolCatInfo.VolCatReads;
	    if (bpb <= 0) {
	       bpb = 1;
	    }
	    if (dev->VolCatInfo.VolCatRBytes > 0) {
	       bpb = dev->VolCatInfo.VolCatRBytes / bpb;
	    } else {
	       bpb = 0;
	    }
            bnet_fsend(user, _("    Total Bytes Read=%s Blocks Read=%s Bytes/block=%s\n"),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatRBytes, b1),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatReads, b2),
	       edit_uint64_with_commas(bpb, b3));
	 }
         bnet_fsend(user, _("    Positioned at File=%s Block=%s\n"),
	    edit_uint64_with_commas(dev->file, b1),
	    edit_uint64_with_commas(dev->block_num, b2));

      } else {
         bnet_fsend(user, _("Archive \"%s\" is not open or does not exist.\n"), device->hdr.name);
	 send_blocked_status(jcr, dev);
      }
   }
   UnlockRes();

#ifdef xxx
   if (debug_level > 0) {
      bnet_fsend(user, "====\n\n");
      dump_resource(R_DEVICE, resources[R_DEVICE-r_first].res_head, sendit, user);
   }
   bnet_fsend(user, "====\n\n");
#endif

   list_spool_stats(user);

   bnet_sig(user, BNET_EOD);
   return true;
}

static void send_blocked_status(JCR *jcr, DEVICE *dev)
{
   BSOCK *user = jcr->dir_bsock;
   DCR *dcr = jcr->dcr;

   if (!dev) {
      return;
   }
   switch (dev->dev_blocked) {
   case BST_UNMOUNTED:
      bnet_fsend(user, _("    Device is BLOCKED. User unmounted.\n"));
      break;
   case BST_UNMOUNTED_WAITING_FOR_SYSOP:
      bnet_fsend(user, _("    Device is BLOCKED. User unmounted during wait for media/mount.\n"));
      break;
   case BST_WAITING_FOR_SYSOP:
      if (jcr->JobStatus == JS_WaitMount) {
         bnet_fsend(user, _("    Device is BLOCKED waiting for mount of volume \"%s\".\n"),
	    dcr->VolumeName);
      } else {
         bnet_fsend(user, _("    Device is BLOCKED waiting for appendable media.\n"));
      }
      break;
   case BST_DOING_ACQUIRE:
      bnet_fsend(user, _("    Device is being initialized.\n"));
      break;
   case BST_WRITING_LABEL:
      bnet_fsend(user, _("    Device is blocked labeling a Volume.\n"));
      break;
   default:
      break;
   }
   if (debug_level > 1) {
      bnet_fsend(user, _("Configured device capabilities:\n"));
      bnet_fsend(user, "%sEOF ", dev->capabilities & CAP_EOF ? "" : "!");
      bnet_fsend(user, "%sBSR ", dev->capabilities & CAP_BSR ? "" : "!");
      bnet_fsend(user, "%sBSF ", dev->capabilities & CAP_BSF ? "" : "!");
      bnet_fsend(user, "%sFSR ", dev->capabilities & CAP_FSR ? "" : "!");
      bnet_fsend(user, "%sFSF ", dev->capabilities & CAP_FSF ? "" : "!");
      bnet_fsend(user, "%sEOM ", dev->capabilities & CAP_EOM ? "" : "!");
      bnet_fsend(user, "%sREM ", dev->capabilities & CAP_REM ? "" : "!");
      bnet_fsend(user, "%sRACCESS ", dev->capabilities & CAP_RACCESS ? "" : "!");
      bnet_fsend(user, "%sAUTOMOUNT ", dev->capabilities & CAP_AUTOMOUNT ? "" : "!");
      bnet_fsend(user, "%sLABEL ", dev->capabilities & CAP_LABEL ? "" : "!");
      bnet_fsend(user, "%sANONVOLS ", dev->capabilities & CAP_ANONVOLS ? "" : "!");
      bnet_fsend(user, "%sALWAYSOPEN ", dev->capabilities & CAP_ALWAYSOPEN ? "" : "!");
      bnet_fsend(user, "\n");

      bnet_fsend(user, _("Device status:\n"));
      bnet_fsend(user, "%sOPENED ", dev->is_open() ? "" : "!");
      bnet_fsend(user, "%sTAPE ", dev->is_tape() ? "" : "!");
      bnet_fsend(user, "%sLABEL ", dev->is_labeled() ? "" : "!");
      bnet_fsend(user, "%sMALLOC ", dev->state & ST_MALLOC ? "" : "!");
      bnet_fsend(user, "%sAPPEND ", dev->can_append() ? "" : "!");
      bnet_fsend(user, "%sREAD ", dev->can_read() ? "" : "!");
      bnet_fsend(user, "%sEOT ", dev->at_eot() ? "" : "!");
      bnet_fsend(user, "%sWEOT ", dev->state & ST_WEOT ? "" : "!");
      bnet_fsend(user, "%sEOF ", dev->at_eof() ? "" : "!");
      bnet_fsend(user, "%sNEXTVOL ", dev->state & ST_NEXTVOL ? "" : "!");
      bnet_fsend(user, "%sSHORT ", dev->state & ST_SHORT ? "" : "!");
      bnet_fsend(user, "%sMOUNTED ", dev->state & ST_MOUNTED ? "" : "!");
      bnet_fsend(user, "\n");

      bnet_fsend(user, _("Device parameters:\n"));
      bnet_fsend(user, "Archive name: %s Device name: %s\n", dev->dev_name,
	 dev->device->hdr.name);
      bnet_fsend(user, "File=%u block=%u\n", dev->file, dev->block_num);
      bnet_fsend(user, "Min block=%u Max block=%u\n", dev->min_block_size, dev->max_block_size);
   }

}

static void list_running_jobs(BSOCK *user)
{
   bool found = false;
   int bps, sec;
   JCR *jcr;
   char JobName[MAX_NAME_LENGTH];
   char b1[30], b2[30], b3[30];

   bnet_fsend(user, _("\nRunning Jobs:\n"));
   lock_jcr_chain();
   foreach_jcr(jcr) {
      if (jcr->JobStatus == JS_WaitFD) {
         bnet_fsend(user, _("%s Job %s waiting for Client connection.\n"),
	    job_type_to_str(jcr->JobType), jcr->Job);
      }
      if (jcr->dcr && jcr->dcr->device) {
	 bstrncpy(JobName, jcr->Job, sizeof(JobName));
	 /* There are three periods after the Job name */
	 char *p;
	 for (int i=0; i<3; i++) {
            if ((p=strrchr(JobName, '.')) != NULL) {
	       *p = 0;
	    }
	 }
         bnet_fsend(user, _("%s %s job %s JobId=%d Volume=\"%s\" device=\"%s\"\n"),
		   job_level_to_str(jcr->JobLevel),
		   job_type_to_str(jcr->JobType),
		   JobName,
		   jcr->JobId,
		   jcr->dcr->VolumeName,
		   jcr->dcr->device->device_name);
	 sec = time(NULL) - jcr->run_time;
	 if (sec <= 0) {
	    sec = 1;
	 }
	 bps = jcr->JobBytes / sec;
         bnet_fsend(user, _("    Files=%s Bytes=%s Bytes/sec=%s\n"),
	    edit_uint64_with_commas(jcr->JobFiles, b1),
	    edit_uint64_with_commas(jcr->JobBytes, b2),
	    edit_uint64_with_commas(bps, b3));
	 found = true;
#ifdef DEBUG
	 if (jcr->file_bsock) {
            bnet_fsend(user, "    FDReadSeqNo=%s in_msg=%u out_msg=%d fd=%d\n",
	       edit_uint64_with_commas(jcr->file_bsock->read_seqno, b1),
	       jcr->file_bsock->in_msg_no, jcr->file_bsock->out_msg_no,
	       jcr->file_bsock->fd);
	 } else {
            bnet_fsend(user, "    FDSocket closed\n");
	 }
#endif
      }
      free_locked_jcr(jcr);
   }
   unlock_jcr_chain();
   if (!found) {
      bnet_fsend(user, _("No Jobs running.\n"));
   }
   bnet_fsend(user, "====\n");
}

static void list_terminated_jobs(void *arg)
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
   msg =  _("\nTerminated Jobs:\n");
   sendit(msg, strlen(msg), arg);
   msg =  _(" JobId  Level   Files          Bytes Status   Finished        Name \n");
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

/*
 * Send to Director
 */
static void sendit(const char *msg, int len, void *arg)
{
   BSOCK *user = (BSOCK *)arg;

   memcpy(user->msg, msg, len+1);
   user->msglen = len+1;
   bnet_send(user);
}

/*
 * .status command from Director
 */
bool qstatus_cmd(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   POOL_MEM time;
   JCR *njcr;
   s_last_job* job;

   if (sscanf(dir->msg, qstatus, time.c_str()) != 1) {
      pm_strcpy(jcr->errmsg, dir->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Bad .status command: %s\n"), jcr->errmsg);
      bnet_fsend(dir, "3900 Bad .status command, missing argument.\n");
      bnet_sig(dir, BNET_EOD);
      return false;
   }
   unbash_spaces(time);

   if (strcmp(time.c_str(), "current") == 0) {
      bnet_fsend(dir, OKqstatus, time.c_str());
      lock_jcr_chain();
      foreach_jcr(njcr) {
	 if (njcr->JobId != 0) {
	    bnet_fsend(dir, DotStatusJob, njcr->JobId, njcr->JobStatus, njcr->JobErrors);
	 }
	 free_locked_jcr(njcr);
      }
      unlock_jcr_chain();
   }
   else if (strcmp(time.c_str(), "last") == 0) {
      bnet_fsend(dir, OKqstatus, time.c_str());
      if ((last_jobs) && (last_jobs->size() > 0)) {
	 job = (s_last_job*)last_jobs->last();
	 bnet_fsend(dir, DotStatusJob, job->JobId, job->JobStatus, job->Errors);
      }
   }
   else {
      pm_strcpy(jcr->errmsg, dir->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Bad .status command: %s\n"), jcr->errmsg);
      bnet_fsend(dir, "3900 Bad .status command, wrong argument.\n");
      bnet_sig(dir, BNET_EOD);
      return false;
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}
