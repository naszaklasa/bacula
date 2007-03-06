/*
 *  Subroutines to handle Catalog reqests sent to the Director
 *   Reqests/commands from the Director are handled in dircmd.c
 *
 *   Kern Sibbald, December 2000
 *
 *   Version $Id: askdir.c,v 1.61.4.1 2005/02/14 10:02:27 kerns Exp $
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

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Requests sent to the Director */
static char Find_media[]   = "CatReq Job=%s FindMedia=%d\n";
static char Get_Vol_Info[] = "CatReq Job=%s GetVolInfo VolName=%s write=%d\n";
static char Update_media[] = "CatReq Job=%s UpdateMedia VolName=%s"
   " VolJobs=%u VolFiles=%u VolBlocks=%u VolBytes=%s VolMounts=%u"
   " VolErrors=%u VolWrites=%u MaxVolBytes=%s EndTime=%d VolStatus=%s"
   " Slot=%d relabel=%d InChanger=%d VolReadTime=%s VolWriteTime=%s\n";
static char Create_job_media[] = "CatReq Job=%s CreateJobMedia" 
   " FirstIndex=%u LastIndex=%u StartFile=%u EndFile=%u" 
   " StartBlock=%u EndBlock=%u\n";
static char FileAttributes[] = "UpdCat Job=%s FileAttributes ";
static char Job_status[]     = "Status Job=%s JobStatus=%d\n";


/* Responses received from the Director */
static char OK_media[] = "1000 OK VolName=%127s VolJobs=%u VolFiles=%u"
   " VolBlocks=%u VolBytes=%" lld " VolMounts=%u VolErrors=%u VolWrites=%u"
   " MaxVolBytes=%" lld " VolCapacityBytes=%" lld " VolStatus=%20s"
   " Slot=%d MaxVolJobs=%u MaxVolFiles=%u InChanger=%d"
   " VolReadTime=%" lld " VolWriteTime=%" lld " EndFile=%u EndBlock=%u";


static char OK_create[] = "1000 OK CreateJobMedia\n";

/* Forward referenced functions */
static int wait_for_sysop(DCR *dcr);

/*
 * Send current JobStatus to Director
 */
bool dir_send_job_status(JCR *jcr)
{
   return bnet_fsend(jcr->dir_bsock, Job_status, jcr->Job, jcr->JobStatus);
}

/*
 * Common routine for:
 *   dir_get_volume_info()
 * and
 *   dir_find_next_appendable_volume()
 * 
 *  Returns: true  on success and vol info in dcr->VolCatInfo
 *	     false on failure
 */
static bool do_get_volume_info(DCR *dcr)
{
    JCR *jcr = dcr->jcr;
    BSOCK *dir = jcr->dir_bsock;
    VOLUME_CAT_INFO vol;
    int n;
    int InChanger;

    dcr->VolumeName[0] = 0;	      /* No volume */
    if (bnet_recv(dir) <= 0) {
       Dmsg0(200, "getvolname error bnet_recv\n");
       Mmsg(jcr->errmsg, _("Network error on bnet_recv in req_vol_info.\n"));
       return false;
    }
    memset(&vol, 0, sizeof(vol));
    Dmsg1(300, "Get vol info=%s", dir->msg);
    n = sscanf(dir->msg, OK_media, vol.VolCatName, 
	       &vol.VolCatJobs, &vol.VolCatFiles,
	       &vol.VolCatBlocks, &vol.VolCatBytes,
	       &vol.VolCatMounts, &vol.VolCatErrors,
	       &vol.VolCatWrites, &vol.VolCatMaxBytes,
	       &vol.VolCatCapacityBytes, vol.VolCatStatus,
	       &vol.Slot, &vol.VolCatMaxJobs, &vol.VolCatMaxFiles,
	       &InChanger, &vol.VolReadTime, &vol.VolWriteTime,
	       &vol.EndFile, &vol.EndBlock);
    if (n != 19) {
       Dmsg2(100, "Bad response from Dir fields=%d: %s\n", n, dir->msg);
       Mmsg(jcr->errmsg, _("Error getting Volume info: %s\n"), dir->msg);
       return false;
    }
    vol.InChanger = InChanger;	      /* bool in structure */
    unbash_spaces(vol.VolCatName);
    bstrncpy(dcr->VolumeName, vol.VolCatName, sizeof(dcr->VolumeName));
    memcpy(&dcr->VolCatInfo, &vol, sizeof(dcr->VolCatInfo));
    
    Dmsg2(300, "do_reqest_vol_info got slot=%d Volume=%s\n", 
	  vol.Slot, vol.VolCatName);
    return true;
}


/*
 * Get Volume info for a specific volume from the Director's Database
 *
 * Returns: true  on success   (not Director guarantees that Pool and MediaType
 *			       are correct and VolStatus==Append or
 *			       VolStatus==Recycle)
 *	    false on failure
 *
 *	    Volume information returned in jcr
 */
bool dir_get_volume_info(DCR *dcr, enum get_vol_info_rw writing)
{
    JCR *jcr = dcr->jcr;
    BSOCK *dir = jcr->dir_bsock;

    bstrncpy(dcr->VolCatInfo.VolCatName, dcr->VolumeName, sizeof(dcr->VolCatInfo.VolCatName));
    Dmsg1(300, "dir_get_volume_info=%s\n", dcr->VolCatInfo.VolCatName);
    bash_spaces(dcr->VolCatInfo.VolCatName);
    bnet_fsend(dir, Get_Vol_Info, jcr->Job, dcr->VolCatInfo.VolCatName, 
       writing==GET_VOL_INFO_FOR_WRITE?1:0);
    return do_get_volume_info(dcr);
}



/*
 * Get info on the next appendable volume in the Director's database
 * Returns: true  on success
 *	    false on failure
 *
 *	    Volume information returned in dcr
 *
 */
bool dir_find_next_appendable_volume(DCR *dcr)
{
    JCR *jcr = dcr->jcr;
    BSOCK *dir = jcr->dir_bsock;
    JCR *njcr;

    Dmsg0(200, "dir_find_next_appendable_volume\n");
    /*
     * Try the three oldest or most available volumes.	Note,
     *	 the most available could already be mounted on another
     *	 drive, so we continue looking for a not in use Volume.
     */
    for (int vol_index=1;  vol_index < 3; vol_index++) {
       bnet_fsend(dir, Find_media, jcr->Job, vol_index);
       if (do_get_volume_info(dcr)) {
          Dmsg2(300, "JobId=%d got possible Vol=%s\n", jcr->JobId, dcr->VolumeName);
	  bool found = false;
	  /* 
	   * Walk through all jobs and see if the volume is   
	   *  already mounted. If so, try a different one.
	   * This would be better done by walking through  
	   *  all the devices.
	   */
	  lock_jcr_chain();
	  foreach_jcr(njcr) {
	     if (jcr == njcr) {
		free_locked_jcr(njcr);
		continue;	      /* us */
	     }
             Dmsg2(300, "Compare to JobId=%d using Vol=%s\n", njcr->JobId, njcr->dcr->VolumeName);
	     if (njcr->dcr && strcmp(dcr->VolumeName, njcr->dcr->VolumeName) == 0) {
		found = true;
                Dmsg1(400, "Vol in use by JobId=%u\n", njcr->JobId);
		free_locked_jcr(njcr);
		break;
	     }
	     free_locked_jcr(njcr);
	  }
	  unlock_jcr_chain();
	  if (!found) {
             Dmsg0(400, "dir_find_next_appendable_volume return true\n");
	     return true;	      /* Got good Volume */
	  }
       } else {
          Dmsg0(200, "No volume info, return false\n");
	  return false;
       }
    }
    Dmsg0(400, "dir_find_next_appendable_volume return true\n");
    return true; 
}

    
/*
 * After writing a Volume, send the updated statistics
 * back to the director. The information comes from the
 * dev record.	   
 */
bool dir_update_volume_info(DCR *dcr, bool label)
{
   JCR *jcr = dcr->jcr;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev = dcr->dev;
   time_t LastWritten = time(NULL);
   char ed1[50], ed2[50], ed3[50], ed4[50];
   VOLUME_CAT_INFO *vol = &dev->VolCatInfo;
   int InChanger;

   if (vol->VolCatName[0] == 0) {
      Jmsg0(jcr, M_FATAL, 0, _("NULL Volume name. This shouldn't happen!!!\n"));
      Dmsg0(000, "NULL Volume name. This shouldn't happen!!!\n");
      return false;
   }
   if (dev_state(dev, ST_READ)) {
      Jmsg0(jcr, M_FATAL, 0, _("Attempt to update_volume_info in read mode!!!\n"));
      Dmsg0(000, "Attempt to update_volume_info in read mode!!!\n");
      return false;
   }

   Dmsg1(300, "Update cat VolFiles=%d\n", dev->file);
   /* Just labeled or relabeled the tape */
   if (label) {
      bstrncpy(vol->VolCatStatus, "Append", sizeof(vol->VolCatStatus));
      vol->VolCatBytes = 1;	      /* indicates tape labeled */
   }
   bash_spaces(vol->VolCatName);
   InChanger = vol->InChanger;
   bnet_fsend(dir, Update_media, jcr->Job, 
      vol->VolCatName, vol->VolCatJobs, vol->VolCatFiles,
      vol->VolCatBlocks, edit_uint64(vol->VolCatBytes, ed1),
      vol->VolCatMounts, vol->VolCatErrors,
      vol->VolCatWrites, edit_uint64(vol->VolCatMaxBytes, ed2), 
      LastWritten, vol->VolCatStatus, vol->Slot, label,
      InChanger,		      /* bool in structure */
      edit_uint64(vol->VolReadTime, ed3), 
      edit_uint64(vol->VolWriteTime, ed4) );

   Dmsg1(300, "update_volume_info(): %s", dir->msg);
   unbash_spaces(vol->VolCatName);

   if (!do_get_volume_info(dcr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", jcr->errmsg);
      Dmsg1(000, "Didn't get vol info: %s", jcr->errmsg);
      return false;
   }
   Dmsg1(420, "get_volume_info(): %s", dir->msg);
   /* Update dev Volume info in case something changed (e.g. expired) */
   memcpy(&dev->VolCatInfo, &dcr->VolCatInfo, sizeof(dev->VolCatInfo));
   return true;
}

/*
 * After writing a Volume, create the JobMedia record.
 */
bool dir_create_jobmedia_record(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   BSOCK *dir = jcr->dir_bsock;

   if (!dcr->WroteVol) {
      return true;		      /* nothing written to tape */
   }

   dcr->WroteVol = false;
   bnet_fsend(dir, Create_job_media, jcr->Job, 
      dcr->VolFirstIndex, dcr->VolLastIndex,
      dcr->StartFile, dcr->EndFile,
      dcr->StartBlock, dcr->EndBlock);
   Dmsg1(400, "create_jobmedia(): %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      Dmsg0(190, "create_jobmedia error bnet_recv\n");
      Jmsg(jcr, M_FATAL, 0, _("Error creating JobMedia record: ERR=%s\n"), 
	   bnet_strerror(dir));
      return false;
   }
   Dmsg1(400, "Create_jobmedia: %s", dir->msg);
   if (strcmp(dir->msg, OK_create) != 0) {
      Dmsg1(130, "Bad response from Dir: %s\n", dir->msg);
      Jmsg(jcr, M_FATAL, 0, _("Error creating JobMedia record: %s\n"), dir->msg);
      return false;
   }
   return true;
}


/* 
 * Update File Attribute data
 */
bool dir_update_file_attributes(DCR *dcr, DEV_RECORD *rec)
{
   JCR *jcr = dcr->jcr;
   BSOCK *dir = jcr->dir_bsock;
   ser_declare;

   dir->msglen = sprintf(dir->msg, FileAttributes, jcr->Job);
   dir->msg = check_pool_memory_size(dir->msg, dir->msglen + 
		sizeof(DEV_RECORD) + rec->data_len);
   ser_begin(dir->msg + dir->msglen, 0);
   ser_uint32(rec->VolSessionId);
   ser_uint32(rec->VolSessionTime);
   ser_int32(rec->FileIndex);
   ser_int32(rec->Stream);
   ser_uint32(rec->data_len);
   ser_bytes(rec->data, rec->data_len);
   dir->msglen = ser_length(dir->msg);
   return bnet_send(dir);
}


/*
 *   Request the sysop to create an appendable volume
 *
 *   Entered with device blocked.
 *   Leaves with device blocked.
 *
 *   Returns: true  on success (operator issues a mount command)
 *	      false on failure
 *		Note, must create dev->errmsg on error return.
 *
 *    On success, dcr->VolumeName and dcr->VolCatInfo contain
 *	information on suggested volume, but this may not be the
 *	same as what is actually mounted.
 *
 *    When we return with success, the correct tape may or may not
 *	actually be mounted. The calling routine must read it and
 *	verify the label.
 */
bool dir_ask_sysop_to_create_appendable_volume(DCR *dcr)
{
   int stat = 0, jstat;
   bool unmounted;
   bool first = true;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   Dmsg0(400, "enter dir_ask_sysop_to_create_appendable_volume\n");
   ASSERT(dev->dev_blocked);
   for ( ;; ) {
      if (job_canceled(jcr)) {
	 Mmsg(dev->errmsg,
              _("Job %s canceled while waiting for mount on Storage Device \"%s\".\n"), 
	      jcr->Job, dcr->dev_name);
         Jmsg(jcr, M_INFO, 0, "%s", dev->errmsg);
	 return false;
      }
      /* First pass, we *know* there are no appendable volumes, so no need to call */
      if (!first && dir_find_next_appendable_volume(dcr)) { /* get suggested volume */
	 unmounted = (dev->dev_blocked == BST_UNMOUNTED) ||
		     (dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);
	 /*
	  * If we have a valid volume name and we are not
	  *   removable media, return now, or if we have a
	  *   Slot for an autochanger, otherwise wait
	  *   for the operator to mount the media.
	  */
	 if (!unmounted && ((dcr->VolumeName[0] && !dev_cap(dev, CAP_REM) && 
		dev_cap(dev, CAP_LABEL)) ||
		 (dcr->VolumeName[0] && dcr->VolCatInfo.Slot))) {
            Dmsg0(400, "Return 1 from mount without wait.\n");
	    return true;
	 }
	 jstat = JS_WaitMount;
	 if (!dev->poll) {
	    Jmsg(jcr, M_MOUNT, 0, _(
"Please mount Volume \"%s\" on Storage Device \"%s\" for Job %s\n"
"Use \"mount\" command to release Job.\n"),
	      dcr->VolumeName, dcr->dev_name, jcr->Job);
            Dmsg3(400, "Mount %s on %s for Job %s\n",
		  dcr->VolumeName, dcr->dev_name, jcr->Job);
	 }
      } else {
	 jstat = JS_WaitMedia;
	 if (!dev->poll) {
	    Jmsg(jcr, M_MOUNT, 0, _(
"Job %s waiting. Cannot find any appendable volumes.\n\
Please use the \"label\"  command to create a new Volume for:\n\
    Storage:      %s\n\
    Media type:   %s\n\
    Pool:         %s\n"),
	       jcr->Job, 
	       dcr->dev_name, 
	       dcr->media_type,
	       dcr->pool_name);
	 }
      }
      first = false;

      jcr->JobStatus = jstat;
      dir_send_job_status(jcr);

      stat = wait_for_sysop(dcr);
      if (dev->poll) {
         Dmsg1(400, "Poll timeout in create append vol on device %s\n", dev_name(dev));
	 continue;
      }

      if (stat == ETIMEDOUT) {
	 if (!double_dev_wait_time(dev)) {
            Mmsg(dev->errmsg, _("Max time exceeded waiting to mount Storage Device \"%s\" for Job %s\n"), 
	       dev_name(dev), jcr->Job);
            Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
            Dmsg1(400, "Gave up waiting on device %s\n", dev_name(dev));
	    return false;	      /* exceeded maximum waits */
	 }
	 continue;
      }
      if (stat == EINVAL) {
	 berrno be;
         Mmsg2(dev->errmsg, _("pthread error in mount_next_volume stat=%d ERR=%s\n"),
	       stat, be.strerror(stat));
         Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
	 return false;
      }
      if (stat != 0) {
	 berrno be;
         Jmsg(jcr, M_WARNING, 0, _("pthread error in mount_next_volume stat=%d ERR=%s\n"), stat,
	    be.strerror(stat));
      }
      Dmsg1(400, "Someone woke me for device %s\n", dev_name(dev));

      /* If no VolumeName, and cannot get one, try again */
      if (dcr->VolumeName[0] == 0 && !job_canceled(jcr) &&
	  !dir_find_next_appendable_volume(dcr)) {
	 Jmsg(jcr, M_MOUNT, 0, _(
"Someone woke me up, but I cannot find any appendable\n\
volumes for Job=%s.\n"), jcr->Job);
	 /* Restart wait counters after user interaction */
	 init_dev_wait_timers(dev);
	 continue;
      }       
      unmounted = (dev->dev_blocked == BST_UNMOUNTED) ||
		  (dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);
      if (unmounted) {
	 continue;		      /* continue to wait */
      }

      /*
       * Device mounted, we have a volume, break and return   
       */
      break;
   }
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);
   Dmsg0(400, "leave dir_ask_sysop_to_mount_create_appendable_volume\n");
   return true;
}

/*
 *   Request to mount specific Volume
 *
 *   Entered with device blocked and dcr->VolumeName is desired
 *	volume.
 *   Leaves with device blocked.
 *
 *   Returns: true  on success (operator issues a mount command)
 *	      false on failure
 *		    Note, must create dev->errmsg on error return.
 *
 */
bool dir_ask_sysop_to_mount_volume(DCR *dcr)
{
   int stat = 0;
   const char *msg;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   Dmsg0(400, "enter dir_ask_sysop_to_mount_volume\n");
   if (!dcr->VolumeName[0]) {
      Mmsg0(dev->errmsg, _("Cannot request another volume: no volume name given.\n"));
      return false;
   }
   ASSERT(dev->dev_blocked);
   for ( ;; ) {
      if (job_canceled(jcr)) {
         Mmsg(dev->errmsg, _("Job %s canceled while waiting for mount on Storage Device \"%s\".\n"), 
	      jcr->Job, dcr->dev_name);
	 return false;
      }

      if (!dev->poll) {
         msg = _("Please mount");
         Jmsg(jcr, M_MOUNT, 0, _("%s Volume \"%s\" on Storage Device \"%s\" for Job %s\n"),
	      msg, dcr->VolumeName, dcr->dev_name, jcr->Job);
         Dmsg3(400, "Mount \"%s\" on device \"%s\" for Job %s\n",
	       dcr->VolumeName, dcr->dev_name, jcr->Job);
      }

      jcr->JobStatus = JS_WaitMount;
      dir_send_job_status(jcr);

      stat = wait_for_sysop(dcr);    ;	   /* wait on device */
      if (dev->poll) {
         Dmsg1(400, "Poll timeout in mount vol on device %s\n", dev_name(dev));
         Dmsg1(400, "Blocked=%s\n", edit_blocked_reason(dev));
	 return true;
      }

      if (stat == ETIMEDOUT) {
	 if (!double_dev_wait_time(dev)) {
            Mmsg(dev->errmsg, _("Max time exceeded waiting to mount Storage Device \"%s\" for Job %s\n"), 
	       dev_name(dev), jcr->Job);
            Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
            Dmsg1(400, "Gave up waiting on device %s\n", dev_name(dev));
	    return false;	      /* exceeded maximum waits */
	 }
	 continue;
      }
      if (stat == EINVAL) {
	 berrno be;
         Mmsg2(dev->errmsg, _("pthread error in mount_volume stat=%d ERR=%s\n"),
	       stat, be.strerror(stat));
         Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
	 return false;
      }
      if (stat != 0) {
	 berrno be;
         Jmsg(jcr, M_FATAL, 0, _("pthread error in mount_next_volume stat=%d ERR=%s\n"), stat,
	    be.strerror(stat));
      }
      Dmsg1(400, "Someone woke me for device %s\n", dev_name(dev));
      break;
   }
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);
   Dmsg0(400, "leave dir_ask_sysop_to_mount_volume\n");
   return true;
}

/*
 * Wait for SysOp to mount a tape
 */
static int wait_for_sysop(DCR *dcr)
{
   struct timeval tv;
   struct timezone tz;
   struct timespec timeout;
   time_t last_heartbeat = 0;
   time_t first_start = time(NULL);
   int stat = 0;
   int add_wait;
   bool unmounted;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   
   P(dev->mutex);
   unmounted = (dev->dev_blocked == BST_UNMOUNTED) ||
		(dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);

   dev->poll = false;
   /*
    * Wait requested time (dev->rem_wait_sec).	However, we also wake up every
    *	 HB_TIME seconds and send a heartbeat to the FD and the Director
    *	 to keep stateful firewalls from closing them down while waiting
    *	 for the operator.
    */
   add_wait = dev->rem_wait_sec;
   if (me->heartbeat_interval && add_wait > me->heartbeat_interval) {
      add_wait = me->heartbeat_interval;
   }
   /* If the user did not unmount the tape and we are polling, ensure
    *  that we poll at the correct interval. 
    */
   if (!unmounted && dev->vol_poll_interval && add_wait > dev->vol_poll_interval) {
      add_wait = dev->vol_poll_interval;
   }
   gettimeofday(&tv, &tz);
   timeout.tv_nsec = tv.tv_usec * 1000;
   timeout.tv_sec = tv.tv_sec + add_wait;

   if (!unmounted) {
      dev->dev_prev_blocked = dev->dev_blocked;
      dev->dev_blocked = BST_WAITING_FOR_SYSOP; /* indicate waiting for mount */
   }

   for ( ; !job_canceled(jcr); ) {
      time_t now, start;

      Dmsg3(400, "I'm going to sleep on device %s. HB=%d wait=%d\n", dev_name(dev),
	 (int)me->heartbeat_interval, dev->wait_sec);
      start = time(NULL);
      /* Wait required time */
      stat = pthread_cond_timedwait(&dev->wait_next_vol, &dev->mutex, &timeout);
      Dmsg1(400, "Wokeup from sleep on device stat=%d\n", stat);

      now = time(NULL);
      dev->rem_wait_sec -= (now - start);

      /* Note, this always triggers the first time. We want that. */
      if (me->heartbeat_interval) {
	 if (now - last_heartbeat >= me->heartbeat_interval) {
	    /* send heartbeats */
	    if (jcr->file_bsock) {
	       bnet_sig(jcr->file_bsock, BNET_HEARTBEAT);
               Dmsg0(400, "Send heartbeat to FD.\n");
	    }
	    if (jcr->dir_bsock) {
	       bnet_sig(jcr->dir_bsock, BNET_HEARTBEAT);
	    }
	    last_heartbeat = now;
	 }
      }

      /*
       * Check if user unmounted the device while we were waiting
       */
      unmounted = (dev->dev_blocked == BST_UNMOUNTED) ||
		   (dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);

      if (stat != ETIMEDOUT) {	   /* we blocked the device */
	 break; 		   /* on error return */
      }
      if (dev->rem_wait_sec <= 0) {  /* on exceeding wait time return */
         Dmsg0(400, "Exceed wait time.\n");
	 break;
      }
      
      if (!unmounted && dev->vol_poll_interval &&	
	  (now - first_start >= dev->vol_poll_interval)) {
         Dmsg1(400, "In wait blocked=%s\n", edit_blocked_reason(dev));
	 dev->poll = true;	      /* returning a poll event */
	 break;
      }
      /*
       * Check if user mounted the device while we were waiting
       */
      if (dev->dev_blocked == BST_MOUNT) {   /* mount request ? */
	 stat = 0;
	 break;
      }

      add_wait = dev->wait_sec - (now - start);
      if (add_wait < 0) {
	 add_wait = 0;
      }
      if (me->heartbeat_interval && add_wait > me->heartbeat_interval) {
	 add_wait = me->heartbeat_interval;
      }
      gettimeofday(&tv, &tz);
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec + add_wait; /* additional wait */
      Dmsg1(400, "Additional wait %d sec.\n", add_wait);
   }

   if (!unmounted) {
      dev->dev_blocked = dev->dev_prev_blocked;    /* restore entry state */
   }
   V(dev->mutex);
   return stat;
}
