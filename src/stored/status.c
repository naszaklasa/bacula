/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2007 Free Software Foundation Europe e.V.

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
 *  This file handles the status command
 *
 *     Kern Sibbald, May MMIII
 *
 *   Version $Id: status.c 5144 2007-07-12 07:49:21Z kerns $
 *
 */

#include "bacula.h"
#include "stored.h"

/* Exported variables */

/* Imported variables */
extern BSOCK *filed_chan;
extern int r_first, r_last;
extern struct s_res resources[];
extern void *start_heap;

/* Static variables */
static char qstatus[] = ".status %127s\n";

static char OKqstatus[]   = "3000 OK .status\n";
static char DotStatusJob[] = "JobId=%d JobStatus=%c JobErrors=%d\n";


/* Forward referenced functions */
static void send_blocked_status(DEVICE *dev, void sendit(const char *msg, int len, void *sarg), void *arg);
static void list_terminated_jobs(void sendit(const char *msg, int len, void *sarg), void *arg);
static void list_running_jobs(void sendit(const char *msg, int len, void *sarg), void *arg);
static void list_jobs_waiting_on_reservation(void sendit(const char *msg, int len, void *sarg), void *arg);

static const char *level_to_str(int level);

/*
 * Status command from Director
 */
void output_status(void sendit(const char *msg, int len, void *sarg), void *arg)
{
   DEVRES *device;
   AUTOCHANGER *changer;
   DEVICE *dev;
   char dt[MAX_TIME_LENGTH];
   char b1[35], b2[35], b3[35], b4[35], b5[35];
   POOLMEM *msg;
   int bpb;
   int len;

   msg = get_pool_memory(PM_MESSAGE);

   len = Mmsg(msg, _("%s Version: %s (%s) %s %s %s\n"), 
              my_name, VERSION, BDATE, HOST_OS, DISTNAME, DISTVER);
   sendit(msg, len, arg);

   bstrftime_nc(dt, sizeof(dt), daemon_start_time);


   len = Mmsg(msg, _("Daemon started %s, %d Job%s run since started.\n"),
        dt, num_jobs_run, num_jobs_run == 1 ? "" : "s");
   sendit(msg, len, arg);

   len = Mmsg(msg, _(" Heap: heap=%s smbytes=%s max_bytes=%s bufs=%s max_bufs=%s\n"),
         edit_uint64_with_commas((char *)sbrk(0)-(char *)start_heap, b1),
         edit_uint64_with_commas(sm_bytes, b2),
         edit_uint64_with_commas(sm_max_bytes, b3),
         edit_uint64_with_commas(sm_buffers, b4),
         edit_uint64_with_commas(sm_max_buffers, b5));
   sendit(msg, len, arg);
   len = Mmsg(msg, "Sizes: boffset_t=%d size_t=%d int32_t=%d int64_t=%d\n", 
         (int)sizeof(boffset_t), (int)sizeof(size_t), (int)sizeof(int32_t),
         (int)sizeof(int64_t));
   sendit(msg, len, arg);

   /*
    * List running jobs
    */
   list_running_jobs(sendit, arg);

   /*
    * List jobs stuck in reservation system
    */
   list_jobs_waiting_on_reservation(sendit, arg);

   /*
    * List terminated jobs
    */
   list_terminated_jobs(sendit, arg);

   /*
    * List devices
    */
   len = Mmsg(msg, _("\nDevice status:\n"));
   sendit(msg, len, arg);

   foreach_res(changer, R_AUTOCHANGER) {
      len = Mmsg(msg, _("Autochanger \"%s\" with devices:\n"),
         changer->hdr.name);
      sendit(msg, len, arg);

      foreach_alist(device, changer->device) {
         if (device->dev) {
            len = Mmsg(msg, "   %s\n", device->dev->print_name());
            sendit(msg, len, arg);
         } else {
            len = Mmsg(msg, "   %s\n", device->hdr.name);
            sendit(msg, len, arg);
         }
      }
   }
   foreach_res(device, R_DEVICE) {
      dev = device->dev;
      if (dev && dev->is_open()) {
         if (dev->is_labeled()) {
            len = Mmsg(msg, _("Device %s is mounted with:\n"
                              "    Volume:      %s\n"
                              "    Pool:        %s\n"
                              "    Media type:  %s\n"),
               dev->print_name(), 
               dev->VolHdr.VolumeName, 
               dev->pool_name[0]?dev->pool_name:"*unknown*",
               dev->device->media_type);
            sendit(msg, len, arg);
         } else {
            len = Mmsg(msg, _("Device %s open but no Bacula volume is currently mounted.\n"), 
               dev->print_name());
            sendit(msg, len, arg);
         }
         send_blocked_status(dev, sendit, arg);
         if (dev->can_append()) {
            bpb = dev->VolCatInfo.VolCatBlocks;
            if (bpb <= 0) {
               bpb = 1;
            }
            bpb = dev->VolCatInfo.VolCatBytes / bpb;
            len = Mmsg(msg, _("    Total Bytes=%s Blocks=%s Bytes/block=%s\n"),
               edit_uint64_with_commas(dev->VolCatInfo.VolCatBytes, b1),
               edit_uint64_with_commas(dev->VolCatInfo.VolCatBlocks, b2),
               edit_uint64_with_commas(bpb, b3));
            sendit(msg, len, arg);
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
            len = Mmsg(msg, _("    Total Bytes Read=%s Blocks Read=%s Bytes/block=%s\n"),
               edit_uint64_with_commas(dev->VolCatInfo.VolCatRBytes, b1),
               edit_uint64_with_commas(dev->VolCatInfo.VolCatReads, b2),
               edit_uint64_with_commas(bpb, b3));
            sendit(msg, len, arg);
         }
         len = Mmsg(msg, _("    Positioned at File=%s Block=%s\n"),
            edit_uint64_with_commas(dev->file, b1),
            edit_uint64_with_commas(dev->block_num, b2));
         sendit(msg, len, arg);

      } else {
         if (dev) {
            len = Mmsg(msg, _("Device %s is not open.\n"), dev->print_name());
            sendit(msg, len, arg);
            send_blocked_status(dev, sendit, arg);
        } else {
            len = Mmsg(msg, _("Device \"%s\" is not open or does not exist.\n"), device->hdr.name);
            sendit(msg, len, arg);
         }
      }
   }
   sendit("====\n\n", 6, arg);
   len = Mmsg(msg, _("In Use Volume status:\n"));
   sendit(msg, len, arg);
   list_volumes(sendit, arg);
   sendit("====\n\n", 6, arg);

#ifdef xxx
   if (debug_level > 10) {
      user->fsend(_("====\n\n"));
      dump_resource(R_DEVICE, resources[R_DEVICE-r_first].res_head, sendit, user);
      user->fsend(_("====\n\n"));
   }
#endif

   list_spool_stats(sendit, arg);

   free_pool_memory(msg);
}

static void send_blocked_status(DEVICE *dev, void sendit(const char *msg, int len, void *sarg), void *arg)
{
   char *msg;
   int len;

   msg = (char *)get_pool_memory(PM_MESSAGE);

   if (!dev) {
      len = Mmsg(msg, _("No DEVICE structure.\n\n"));
      sendit(msg, len, arg);
      free_pool_memory(msg);
      return;
   }
   switch (dev->blocked()) {
   case BST_UNMOUNTED:
      len = Mmsg(msg, _("    Device is BLOCKED. User unmounted.\n"));
      sendit(msg, len, arg);
      break;
   case BST_UNMOUNTED_WAITING_FOR_SYSOP:
      len = Mmsg(msg, _("    Device is BLOCKED. User unmounted during wait for media/mount.\n"));
      sendit(msg, len, arg);
      break;
   case BST_WAITING_FOR_SYSOP:
      {
         dlist *dcrs = dev->attached_dcrs;
         bool found_jcr = false;

         if (dcrs != NULL) {
            DCR *dcr;
            for (dcr = (DCR *)dcrs->first(); dcr != NULL; dcr = (DCR *)dcrs->next(dcr)) {
               if (dcr->jcr->JobStatus == JS_WaitMount) {
                  len = Mmsg(msg, _("    Device is BLOCKED waiting for mount of volume \"%s\",\n"
                                    "       Pool:        %s\n"
                                    "       Media type:  %s\n"),
                             dcr->VolumeName,
                             dcr->pool_name,
                             dcr->media_type);
                  sendit(msg, len, arg);
                  found_jcr = true;
               } else if (dcr->jcr->JobStatus == JS_WaitMedia) {
                  len = Mmsg(msg, _("    Device is BLOCKED waiting to create a volume for:\n"
                                    "       Pool:        %s\n"
                                    "       Media type:  %s\n"),
                             dcr->pool_name,
                             dcr->media_type);
                  sendit(msg, len, arg);
                  found_jcr = true;
               }
            }
         }

         if (!found_jcr) {
            len = Mmsg(msg, _("    Device is BLOCKED waiting for media.\n"));
            sendit(msg, len, arg);
         }
      }
      break;
   case BST_DOING_ACQUIRE:
      len = Mmsg(msg, _("    Device is being initialized.\n"));
      sendit(msg, len, arg);
      break;
   case BST_WRITING_LABEL:
      len = Mmsg(msg, _("    Device is blocked labeling a Volume.\n"));
      sendit(msg, len, arg);
      break;
   default:
      break;
   }
   /* Send autochanger slot status */
   if (dev->is_autochanger()) {
      if (dev->Slot > 0) {
         len = Mmsg(msg, _("    Slot %d is loaded in drive %d.\n"), 
            dev->Slot, dev->drive_index);
         sendit(msg, len, arg);
      } else if (dev->Slot == 0) {
         len = Mmsg(msg, _("    Drive %d is not loaded.\n"), dev->drive_index);
         sendit(msg, len, arg);
      } else {
         len = Mmsg(msg, _("    Drive %d status unknown.\n"), dev->drive_index);
         sendit(msg, len, arg);
      }
   }
   if (debug_level > 1) {
      len = Mmsg(msg, _("Configured device capabilities:\n"));
      sendit(msg, len, arg);

      len = Mmsg(msg, "%sEOF %sBSR %sBSF %sFSR %sFSF %sEOM %sREM %sRACCESS %sAUTOMOUNT %sLABEL %sANONVOLS %sALWAYSOPEN\n",
         dev->capabilities & CAP_EOF ? "" : "!", 
         dev->capabilities & CAP_BSR ? "" : "!", 
         dev->capabilities & CAP_BSF ? "" : "!", 
         dev->capabilities & CAP_FSR ? "" : "!", 
         dev->capabilities & CAP_FSF ? "" : "!", 
         dev->capabilities & CAP_EOM ? "" : "!", 
         dev->capabilities & CAP_REM ? "" : "!", 
         dev->capabilities & CAP_RACCESS ? "" : "!",
         dev->capabilities & CAP_AUTOMOUNT ? "" : "!", 
         dev->capabilities & CAP_LABEL ? "" : "!", 
         dev->capabilities & CAP_ANONVOLS ? "" : "!", 
         dev->capabilities & CAP_ALWAYSOPEN ? "" : "!");
      sendit(msg, len, arg);

      len = Mmsg(msg, _("Device state:\n"));
      sendit(msg, len, arg);

      len = Mmsg(msg, "%sOPENED %sTAPE %sLABEL %sMALLOC %sAPPEND %sREAD %sEOT %sWEOT %sEOF %sNEXTVOL %sSHORT %sMOUNTED\n", 
         dev->is_open() ? "" : "!", 
         dev->is_tape() ? "" : "!", 
         dev->is_labeled() ? "" : "!", 
         dev->state & ST_MALLOC ? "" : "!", 
         dev->can_append() ? "" : "!", 
         dev->can_read() ? "" : "!", 
         dev->at_eot() ? "" : "!", 
         dev->state & ST_WEOT ? "" : "!", 
         dev->at_eof() ? "" : "!", 
         dev->state & ST_NEXTVOL ? "" : "!", 
         dev->state & ST_SHORT ? "" : "!", 
         dev->state & ST_MOUNTED ? "" : "!");
      sendit(msg, len, arg);

      len = Mmsg(msg, _("num_writers=%d block=%d\n\n"), dev->num_writers, dev->blocked());
      sendit(msg, len, arg);

      len = Mmsg(msg, _("Device parameters:\n"));
      sendit(msg, len, arg);

      len = Mmsg(msg, _("Archive name: %s Device name: %s\n"), dev->archive_name(),
         dev->name());
      sendit(msg, len, arg);

      len = Mmsg(msg, _("File=%u block=%u\n"), dev->file, dev->block_num);
      sendit(msg, len, arg);

      len = Mmsg(msg, _("Min block=%u Max block=%u\n"), dev->min_block_size, dev->max_block_size);
      sendit(msg, len, arg);
   }

   free_pool_memory(msg);
}

static void list_running_jobs(void sendit(const char *msg, int len, void *sarg), void *arg)
{
   bool found = false;
   int bps, sec;
   JCR *jcr;
   DCR *dcr, *rdcr;
   char JobName[MAX_NAME_LENGTH];
   char *msg, b1[30], b2[30], b3[30];
   int len;

   msg = (char *)get_pool_memory(PM_MESSAGE);

   len = Mmsg(msg, _("\nRunning Jobs:\n"));
   sendit(msg, len, arg);

   foreach_jcr(jcr) {
      if (jcr->JobStatus == JS_WaitFD) {
         len = Mmsg(msg, _("%s Job %s waiting for Client connection.\n"),
            job_type_to_str(jcr->JobType), jcr->Job);
         sendit(msg, len, arg);
      }
      dcr = jcr->dcr;
      rdcr = jcr->read_dcr;
      if ((dcr && dcr->device) || rdcr && rdcr->device) {
         bstrncpy(JobName, jcr->Job, sizeof(JobName));
         /* There are three periods after the Job name */
         char *p;
         for (int i=0; i<3; i++) {
            if ((p=strrchr(JobName, '.')) != NULL) {
               *p = 0;
            }
         }
         if (rdcr && rdcr->device) {
            len = Mmsg(msg, _("Reading: %s %s job %s JobId=%d Volume=\"%s\"\n"
                            "    pool=\"%s\" device=%s\n"),
                   job_level_to_str(jcr->JobLevel),
                   job_type_to_str(jcr->JobType),
                   JobName,
                   jcr->JobId,
                   rdcr->VolumeName,
                   rdcr->pool_name,
                   rdcr->dev?rdcr->dev->print_name(): 
                            rdcr->device->device_name);
            sendit(msg, len, arg);
         }
         if (dcr && dcr->device) {
            len = Mmsg(msg, _("Writing: %s %s job %s JobId=%d Volume=\"%s\"\n"
                            "    pool=\"%s\" device=%s\n"),
                   job_level_to_str(jcr->JobLevel),
                   job_type_to_str(jcr->JobType),
                   JobName,
                   jcr->JobId,
                   dcr->VolumeName,
                   dcr->pool_name,
                   dcr->dev?dcr->dev->print_name(): 
                            dcr->device->device_name);
            sendit(msg, len, arg);
            len= Mmsg(msg, _("    spooling=%d despooling=%d despool_wait=%d\n"),
                   dcr->spooling, dcr->despooling, dcr->despool_wait);
            sendit(msg, len, arg);
         }
         sec = time(NULL) - jcr->run_time;
         if (sec <= 0) {
            sec = 1;
         }
         bps = jcr->JobBytes / sec;
         len = Mmsg(msg, _("    Files=%s Bytes=%s Bytes/sec=%s\n"),
            edit_uint64_with_commas(jcr->JobFiles, b1),
            edit_uint64_with_commas(jcr->JobBytes, b2),
            edit_uint64_with_commas(bps, b3));
         sendit(msg, len, arg);
         found = true;
#ifdef DEBUG
         if (jcr->file_bsock) {
            len = Mmsg(msg, _("    FDReadSeqNo=%s in_msg=%u out_msg=%d fd=%d\n"),
               edit_uint64_with_commas(jcr->file_bsock->read_seqno, b1),
               jcr->file_bsock->in_msg_no, jcr->file_bsock->out_msg_no,
               jcr->file_bsock->m_fd);
            sendit(msg, len, arg);
         } else {
            len = Mmsg(msg, _("    FDSocket closed\n"));
            sendit(msg, len, arg);
         }
#endif
      }
   }
   endeach_jcr(jcr);

   if (!found) {
      len = Mmsg(msg, _("No Jobs running.\n"));
      sendit(msg, len, arg);
   }
   sendit("====\n", 5, arg);

   free_pool_memory(msg);
}

static void list_jobs_waiting_on_reservation(void sendit(const char *msg, int len, void *sarg), void *arg)
{ 
   JCR *jcr;
   char *msg;

   msg = _("\nJobs waiting to reserve a drive:\n");
   sendit(msg, strlen(msg), arg);

   foreach_jcr(jcr) {
      if (!jcr->reserve_msgs) {
         continue;
      }
      send_drive_reserve_messages(jcr, sendit, arg);
   }
   endeach_jcr(jcr);

   sendit("====\n", 5, arg);
}


static void list_terminated_jobs(void sendit(const char *msg, int len, void *sarg), void *arg)
{
   char dt[MAX_TIME_LENGTH], b1[30], b2[30];
   char level[10];
   struct s_last_job *je;
   const char *msg;

   msg =  _("\nTerminated Jobs:\n");
   sendit(msg, strlen(msg), arg);
   if (last_jobs->size() == 0) {
      sendit("====\n", 5, arg);
      return;
   }
   lock_last_jobs_list();
   msg =  _(" JobId  Level    Files      Bytes   Status   Finished        Name \n");
   sendit(msg, strlen(msg), arg);
   msg =  _("===================================================================\n");
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
      bstrncpy(JobName, je->Job, sizeof(JobName));
      /* There are three periods after the Job name */
      char *p;
      for (int i=0; i<3; i++) {
         if ((p=strrchr(JobName, '.')) != NULL) {
            *p = 0;
         }
      }
      bsnprintf(buf, sizeof(buf), _("%6d  %-6s %8s %10s  %-7s  %-8s %s\n"),
         je->JobId,
         level,
         edit_uint64_with_commas(je->JobFiles, b1),
         edit_uint64_with_suffix(je->JobBytes, b2),
         termstat,
         dt, JobName);
      sendit(buf, strlen(buf), arg);
   }
   unlock_last_jobs_list();
   sendit("====\n", 5, arg);
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
static void dir_sendit(const char *msg, int len, void *arg)
{
   BSOCK *user = (BSOCK *)arg;

   memcpy(user->msg, msg, len+1);
   user->msglen = len+1;
   user->send();
}

/*
 * Status command from Director
 */
bool status_cmd(JCR *jcr)
{
   BSOCK *user = jcr->dir_bsock;

   user->fsend("\n");
   output_status(dir_sendit, (void *)user);

   user->signal(BNET_EOD);
   return 1;
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
      dir->fsend(_("3900 Bad .status command, missing argument.\n"));
      dir->signal(BNET_EOD);
      return false;
   }
   unbash_spaces(time);

   if (strcmp(time.c_str(), "current") == 0) {
      dir->fsend(OKqstatus, time.c_str());
      foreach_jcr(njcr) {
         if (njcr->JobId != 0) {
            dir->fsend(DotStatusJob, njcr->JobId, njcr->JobStatus, njcr->JobErrors);
         }
      }
      endeach_jcr(njcr);
   } else if (strcmp(time.c_str(), "last") == 0) {
      dir->fsend(OKqstatus, time.c_str());
      if ((last_jobs) && (last_jobs->size() > 0)) {
         job = (s_last_job*)last_jobs->last();
         dir->fsend(DotStatusJob, job->JobId, job->JobStatus, job->Errors);
      }
   } else {
      pm_strcpy(jcr->errmsg, dir->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Bad .status command: %s\n"), jcr->errmsg);
      dir->fsend(_("3900 Bad .status command, wrong argument.\n"));
      dir->signal(BNET_EOD);
      return false;
   }
   dir->signal(BNET_EOD);
   return true;
}

#if defined(HAVE_WIN32)
int bacstat = 0;

char *bac_status(char *buf, int buf_len)
{
   JCR *njcr;
   const char *termstat = _("Bacula Storage: Idle");
   struct s_last_job *job;
   int stat = 0;                      /* Idle */

   if (!last_jobs) {
      goto done;
   }
   Dmsg0(1000, "Begin bac_status jcr loop.\n");
   foreach_jcr(njcr) {
      if (njcr->JobId != 0) {
         stat = JS_Running;
         termstat = _("Bacula Storage: Running");
         break;
      }
   }
   endeach_jcr(njcr);

   if (stat != 0) {
      goto done;
   }
   if (last_jobs->size() > 0) {
      job = (struct s_last_job *)last_jobs->last();
      stat = job->JobStatus;
      switch (job->JobStatus) {
      case JS_Canceled:
         termstat = _("Bacula Storage: Last Job Canceled");
         break;
      case JS_ErrorTerminated:
      case JS_FatalError:
         termstat = _("Bacula Storage: Last Job Failed");
         break;
      default:
         if (job->Errors) {
            termstat = _("Bacula Storage: Last Job had Warnings");
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

#endif /* HAVE_WIN32 */
