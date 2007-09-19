/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2007 Free Software Foundation Europe e.V.

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
 *  Routines to acquire and release a device for read/write
 *
 *   Kern Sibbald, August MMII
 *
 *   Version $Id: acquire.c 5552 2007-09-14 09:49:06Z kerns $
 */

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Forward referenced functions */
static void attach_dcr_to_dev(DCR *dcr);


/*********************************************************************
 * Acquire device for reading. 
 *  The drive should have previously been reserved by calling 
 *  reserve_device_for_read(). We read the Volume label from the block and
 *  leave the block pointers just after the label.
 *
 *  Returns: NULL if failed for any reason
 *           dcr  if successful
 */
bool acquire_device_for_read(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   bool ok = false;
   bool tape_previously_mounted;
   bool tape_initially_mounted;
   VOL_LIST *vol;
   bool try_autochanger = true;
   int i;
   int vol_label_status;
   int retry = 0;
   
   Dmsg1(950, "jcr->dcr=%p\n", jcr->dcr);
   dev->dblock(BST_DOING_ACQUIRE);

   if (dev->num_writers > 0) {
      Jmsg2(jcr, M_FATAL, 0, _("Acquire read: num_writers=%d not zero. Job %d canceled.\n"), 
         dev->num_writers, jcr->JobId);
      goto get_out;
   }

   /* Find next Volume, if any */
   vol = jcr->VolList;
   if (!vol) {
      char ed1[50];
      Jmsg(jcr, M_FATAL, 0, _("No volumes specified for reading. Job %s canceled.\n"), 
         edit_int64(jcr->JobId, ed1));
      goto get_out;
   }
   jcr->CurReadVolume++;
   for (i=1; i<jcr->CurReadVolume; i++) {
      vol = vol->next;
   }
   if (!vol) {
      Jmsg(jcr, M_FATAL, 0, _("Logic error: no next volume to read. Numvol=%d Curvol=%d\n"),
         jcr->NumReadVolumes, jcr->CurReadVolume);
      goto get_out;                   /* should not happen */   
   }
   bstrncpy(dcr->VolumeName, vol->VolumeName, sizeof(dcr->VolumeName));
   bstrncpy(dcr->media_type, vol->MediaType, sizeof(dcr->media_type));
   dcr->VolCatInfo.Slot = vol->Slot;
    
   /*
    * If the MediaType requested for this volume is not the
    *  same as the current drive, we attempt to find the same
    *  device that was used to write the orginal volume.  If
    *  found, we switch to using that device.
    *
    *  N.B. A lot of routines rely on the dcr pointer not changing
    *    read_records.c even has multiple dcrs cached, so we take care
    *    here to release all important parts of the dcr and re-acquire
    *    them such as the block pointer (size may change), but we do
    *    not release the dcr.
    */
   Dmsg2(50, "MediaType dcr=%s dev=%s\n", dcr->media_type, dev->device->media_type);
   if (dcr->media_type[0] && strcmp(dcr->media_type, dev->device->media_type) != 0) {
      RCTX rctx;
      DIRSTORE *store;
      int stat;

      Jmsg3(jcr, M_INFO, 0, _("Changing device. Want Media Type=\"%s\" have=\"%s\"\n"
                              "  device=%s\n"), 
            dcr->media_type, dev->device->media_type, dev->print_name());
      Dmsg3(50, "Changing device. Want Media Type=\"%s\" have=\"%s\"\n"
                              "  device=%s\n", 
            dcr->media_type, dev->device->media_type, dev->print_name());

      dev->dunblock(DEV_UNLOCKED);

      lock_reservations();
      memset(&rctx, 0, sizeof(RCTX));
      rctx.jcr = jcr;
      jcr->reserve_msgs = New(alist(10, not_owned_by_alist));
      rctx.any_drive = true;
      rctx.device_name = vol->device;
      store = new DIRSTORE;
      memset(store, 0, sizeof(DIRSTORE));
      store->name[0] = 0; /* No dir name */
      bstrncpy(store->media_type, vol->MediaType, sizeof(store->media_type));
      bstrncpy(store->pool_name, dcr->pool_name, sizeof(store->pool_name));
      bstrncpy(store->pool_type, dcr->pool_type, sizeof(store->pool_type));
      store->append = false;
      rctx.store = store;
      dcr->keep_dcr = true;                  /* do not free the dcr */
      release_device(dcr);
      dcr->keep_dcr = false;
      
      /*
       * Search for a new device
       */
      stat = search_res_for_device(rctx);
      release_reserve_messages(jcr);         /* release queued messages */
      unlock_reservations();

      if (stat == 1) {
         dev = dcr->dev;                     /* get new device pointer */
         dev->dblock(BST_DOING_ACQUIRE); 
         dcr->VolumeName[0] = 0;
         Jmsg(jcr, M_INFO, 0, _("Media Type change.  New device %s chosen.\n"),
            dev->print_name());
         Dmsg1(50, "Media Type change.  New device %s chosen.\n", dev->print_name());

         bstrncpy(dcr->VolumeName, vol->VolumeName, sizeof(dcr->VolumeName));
         bstrncpy(dcr->media_type, vol->MediaType, sizeof(dcr->media_type));
         dcr->VolCatInfo.Slot = vol->Slot;
         bstrncpy(dcr->pool_name, store->pool_name, sizeof(dcr->pool_name));
         bstrncpy(dcr->pool_type, store->pool_type, sizeof(dcr->pool_type));
      } else {
         /* error */
         Jmsg1(jcr, M_FATAL, 0, _("No suitable device found to read Volume \"%s\"\n"),
            vol->VolumeName);
         Dmsg1(50, "No suitable device found to read Volume \"%s\"\n", vol->VolumeName);
         goto get_out;
      }
   }


   init_device_wait_timers(dcr);

   tape_previously_mounted = dev->can_read() || dev->can_append() ||
                             dev->is_labeled();
   tape_initially_mounted = tape_previously_mounted;


   /* Volume info is always needed because of VolParts */
   Dmsg0(200, "dir_get_volume_info\n");
   if (!dir_get_volume_info(dcr, GET_VOL_INFO_FOR_READ)) {
      Jmsg1(jcr, M_WARNING, 0, "%s", jcr->errmsg);
   }
   
   for ( ;; ) {
      /* If not polling limit retries */
      if (!dev->poll && retry++ > 10) {
         break;
      }
      dev->clear_labeled();              /* force reread of label */
      if (job_canceled(jcr)) {
         char ed1[50];
         Mmsg1(dev->errmsg, _("Job %s canceled.\n"), edit_int64(jcr->JobId, ed1));
         Jmsg(jcr, M_INFO, 0, dev->errmsg);
         goto get_out;                /* error return */
      }

      autoload_device(dcr, 0, NULL);

      /*
       * This code ensures that the device is ready for
       * reading. If it is a file, it opens it.
       * If it is a tape, it checks the volume name
       */
      Dmsg1(100, "bstored: open vol=%s\n", dcr->VolumeName);
      if (dev->open(dcr, OPEN_READ_ONLY) < 0) {
        Jmsg3(jcr, M_WARNING, 0, _("Read open device %s Volume \"%s\" failed: ERR=%s\n"),
              dev->print_name(), dcr->VolumeName, dev->bstrerror());
         goto default_path;
      }
      Dmsg1(50, "opened dev %s OK\n", dev->print_name());
      
      /* Read Volume Label */
      Dmsg0(50, "calling read-vol-label\n");
      vol_label_status = read_dev_volume_label(dcr);
      switch (vol_label_status) {
      case VOL_OK:
         ok = true;
         memcpy(&dev->VolCatInfo, &dcr->VolCatInfo, sizeof(dev->VolCatInfo));
         break;                    /* got it */
      case VOL_IO_ERROR:
         /*
          * Send error message generated by read_dev_volume_label()
          *  only we really had a tape mounted. This supresses superfluous
          *  error messages when nothing is mounted.
          */
         if (tape_previously_mounted) {
            Jmsg(jcr, M_WARNING, 0, "%s", jcr->errmsg);
         }
         goto default_path;
      case VOL_NAME_ERROR:
         if (tape_initially_mounted) {
            tape_initially_mounted = false;
            goto default_path;
         }
         /* If polling and got a previous bad name, ignore it */
         if (dev->poll && strcmp(dev->BadVolName, dev->VolHdr.VolumeName) == 0) {
            goto default_path;
         } else {
             bstrncpy(dev->BadVolName, dev->VolHdr.VolumeName, sizeof(dev->BadVolName));
         }
         /* Fall through */
      default:
         Jmsg1(jcr, M_WARNING, 0, "%s", jcr->errmsg);
default_path:
         tape_previously_mounted = true;
         
         /*
          * If the device requires mount, close it, so the device can be ejected.
          */
         if (dev->requires_mount()) {
            dev->close();
         }
         
         /* Call autochanger only once unless ask_sysop called */
         if (try_autochanger) {
            int stat;
            Dmsg2(200, "calling autoload Vol=%s Slot=%d\n",
               dcr->VolumeName, dcr->VolCatInfo.Slot);
            stat = autoload_device(dcr, 0, NULL);
            if (stat > 0) {
               try_autochanger = false;
               continue;              /* try reading volume mounted */
            }
         }
         
         /* Mount a specific volume and no other */
         Dmsg0(200, "calling dir_ask_sysop\n");
         if (!dir_ask_sysop_to_mount_volume(dcr)) {
            goto get_out;             /* error return */
         }
         try_autochanger = true;      /* permit using autochanger again */
         continue;                    /* try reading again */
      } /* end switch */
      break;
   } /* end for loop */
   if (!ok) {
      Jmsg1(jcr, M_FATAL, 0, _("Too many errors trying to mount device %s for reading.\n"),
            dev->print_name());
      goto get_out;
   }

   dev->clear_append();
   dev->set_read();
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);
   Jmsg(jcr, M_INFO, 0, _("Ready to read from volume \"%s\" on device %s.\n"),
      dcr->VolumeName, dev->print_name());

get_out:
   dev->dlock();
   if (dcr && dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(50, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   /* 
    * Normally we are blocked, but in at least one error case above 
    *   we are not blocked because we unsuccessfully tried changing
    *   devices.  
    */
   if (dev->is_blocked()) {
      dev->dunblock(DEV_LOCKED);
   }
   Dmsg1(950, "jcr->dcr=%p\n", jcr->dcr);
   return ok;
}


/*
 * Acquire device for writing. We permit multiple writers.
 *  If this is the first one, we read the label.
 *
 *  Returns: NULL if failed for any reason
 *           dcr if successful.
 *   Note, normally reserve_device_for_append() is called
 *   before this routine.
 */
DCR *acquire_device_for_append(DCR *dcr)
{
   bool release = false;
   bool recycle = false;
   bool do_mount = false;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   init_device_wait_timers(dcr);

   dev->dblock(BST_DOING_ACQUIRE);
   Dmsg1(190, "acquire_append device is %s\n", dev->is_tape()?"tape":
        (dev->is_dvd()?"DVD":"disk"));

   /*
    * With the reservation system, this should not happen
    */
   if (dev->can_read()) {
      Jmsg1(jcr, M_FATAL, 0, _("Want to append, but device %s is busy reading.\n"), dev->print_name());
      Dmsg1(200, "Want to append but device %s is busy reading.\n", dev->print_name());
      goto get_out;
   }

   if (dev->can_append()) {
      Dmsg0(190, "device already in append.\n");
      /*
       * Device already in append mode
       *
       * Check if we have the right Volume mounted
       *   OK if current volume info OK
       *   OK if next volume matches current volume
       *   otherwise mount desired volume obtained from
       *    dir_find_next_appendable_volume
       *  dev->VolHdr.VolumeName is what is in the drive
       *  dcr->VolumeName is what we pass into the routines, or
       *    get back from the subroutines.
       */
      bstrncpy(dcr->VolumeName, dev->VolHdr.VolumeName, sizeof(dcr->VolumeName));
      if (!dir_get_volume_info(dcr, GET_VOL_INFO_FOR_WRITE) &&
          !(dir_find_next_appendable_volume(dcr) &&
            strcmp(dev->VolHdr.VolumeName, dcr->VolumeName) == 0)) { /* wrong tape mounted */
         Dmsg2(190, "Wrong tape mounted: %s. wants:%s\n", dev->VolHdr.VolumeName,
            dcr->VolumeName);
         /* Release volume reserved by dir_find_next_appendable_volume() */
         if (dcr->VolumeName[0]) {
            volume_unused(dcr);
         }
         if (dev->num_writers != 0) {
            Jmsg3(jcr, M_FATAL, 0, _("Wanted to append to Volume \"%s\", but device %s is busy writing on \"%s\" .\n"), 
                 dcr->VolumeName, dev->print_name(), dev->VolHdr.VolumeName);
            Dmsg3(200, "Wanted to append to Volume \"%s\", but device %s is busy writing on \"%s\" .\n",  
                 dcr->VolumeName, dev->print_name(), dev->VolHdr.VolumeName);
            goto get_out;
         }
         /* Wrong tape mounted, release it, then fall through to get correct one */
         Dmsg0(190, "Wrong tape mounted, release and try mount.\n");
         release = true;
         do_mount = true;
      } else {
         /*
          * At this point, the correct tape is already mounted, so
          *   we do not need to do mount_next_write_volume(), unless
          *   we need to recycle the tape.
          */
          recycle = strcmp(dcr->VolCatInfo.VolCatStatus, "Recycle") == 0;
          Dmsg1(190, "Correct tape mounted. recycle=%d\n", recycle);
          if (recycle && dev->num_writers != 0) {
             Jmsg(jcr, M_FATAL, 0, _("Cannot recycle volume \"%s\""
                  " on device %s because it is in use by another job.\n"),
                  dev->VolHdr.VolumeName, dev->print_name());
             goto get_out;
          }
          if (dev->num_writers == 0) {
             memcpy(&dev->VolCatInfo, &dcr->VolCatInfo, sizeof(dev->VolCatInfo));
          }

          /*
           *      Insanity check 
           *
           * Check to see if the tape position as defined by the OS is
           *  the same as our concept.  If it is not, we bail out, because
           *  it means the user has probably manually rewound the tape.
           * Note, we check only if num_writers == 0, but this code will
           *  also work fine for any number of writers. If num_writers > 0,
           *  we probably should cancel all jobs using this device, or 
           *  perhaps even abort the SD, or at a minimum, mark the tape
           *  in error.  Another strategy with num_writers == 0, would be
           *  to rewind the tape and do a new eod() request.
           */
          if (dev->is_tape() && dev->num_writers == 0) {
             int32_t file = dev->get_os_tape_file();
             if (file >= 0 && file != (int32_t)dev->get_file()) {
                Jmsg(jcr, M_FATAL, 0, _("Invalid tape position on volume \"%s\"" 
                     " on device %s. Expected %d, got %d\n"), 
                     dev->VolHdr.VolumeName, dev->print_name(), dev->get_file(), file);
                goto get_out;
             }
          }
      }
   } else {
      /* Not already in append mode, so mount the device */
      Dmsg0(190, "Not in append mode, try mount.\n");
      ASSERT(dev->num_writers == 0);
      do_mount = true;
   }

   if (do_mount || recycle) {
      Dmsg0(190, "Do mount_next_write_vol\n");
      bool mounted = mount_next_write_volume(dcr, release);
      if (!mounted) {
         if (!job_canceled(jcr)) {
            /* Reduce "noise" -- don't print if job canceled */
            Jmsg(jcr, M_FATAL, 0, _("Could not ready device %s for append.\n"),
               dev->print_name());
            Dmsg1(200, "Could not ready device %s for append.\n", 
               dev->print_name());
         }
         goto get_out;
      }
      Dmsg2(190, "Output pos=%u:%u\n", dcr->dev->file, dcr->dev->block_num);
   }

   dev->num_writers++;                /* we are now a writer */
   if (jcr->NumWriteVolumes == 0) {
      jcr->NumWriteVolumes = 1;
   }
   dev->VolCatInfo.VolCatJobs++;              /* increment number of jobs on vol */
   dir_update_volume_info(dcr, false);        /* send Volume info to Director */
   dev->dlock();
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   dev->dunblock(DEV_LOCKED);
   return dcr;

/*
 * Error return
 */
get_out:
   dev->dlock();
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   dev->dunblock(DEV_LOCKED);
   return NULL;
}


/*
 * This job is done, so release the device. From a Unix standpoint,
 *  the device remains open.
 *
 * Note, if we are spooling, we may enter with the device locked.
 * However, in all cases, unlock the device when leaving.
 *
 */
bool release_device(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   bool ok = true;

   /* lock only if not already locked by this thread */
   if (!dcr->is_dev_locked()) {
      dev->r_dlock();
   }
   Dmsg2(100, "release_device device %s is %s\n", dev->print_name(), dev->is_tape()?"tape":"disk");

   /* if device is reserved, job never started, so release the reserve here */
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }

   if (dev->can_read()) {
      dev->clear_read();              /* clear read bit */
      Dmsg0(100, "dir_update_vol_info. Release0\n");
      dir_update_volume_info(dcr, false); /* send Volume info to Director */

   } else if (dev->num_writers > 0) {
      /* 
       * Note if WEOT is set, we are at the end of the tape
       *   and may not be positioned correctly, so the
       *   job_media_record and update_vol_info have already been
       *   done, which means we skip them here.
       */
      dev->num_writers--;
      Dmsg1(100, "There are %d writers in release_device\n", dev->num_writers);
      if (dev->is_labeled()) {
         Dmsg0(100, "dir_create_jobmedia_record. Release\n");
         if (!dev->at_weot() && !dir_create_jobmedia_record(dcr)) {
            Jmsg(jcr, M_FATAL, 0, _("Could not create JobMedia record for Volume=\"%s\" Job=%s\n"),
               dcr->VolCatInfo.VolCatName, jcr->Job);
         }
         /* If no more writers, write an EOF */
         if (!dev->num_writers && dev->can_write() && dev->block_num > 0) {
            dev->weof(1);
            write_ansi_ibm_labels(dcr, ANSI_EOF_LABEL, dev->VolHdr.VolumeName);
         }
         if (!dev->at_weot()) {
            dev->VolCatInfo.VolCatFiles = dev->file;   /* set number of files */
            /* Note! do volume update before close, which zaps VolCatInfo */
            Dmsg0(100, "dir_update_vol_info. Release0\n");
            dir_update_volume_info(dcr, false); /* send Volume info to Director */
         }
      }

   } else {
      /*                
       * If we reach here, it is most likely because the job
       *   has failed, since the device is not in read mode and
       *   there are no writers. It was probably reserved.
       */
   }

   /* If no writers, close if file or !CAP_ALWAYS_OPEN */
   if (dev->num_writers == 0 && (!dev->is_tape() || !dev->has_cap(CAP_ALWAYSOPEN))) {
      dvd_remove_empty_part(dcr);        /* get rid of any empty spool part */
      dev->close();
   }

   /* Fire off Alert command and include any output */
   if (!job_canceled(jcr) && dcr->device->alert_command) {
      POOLMEM *alert;
      int status = 1;
      BPIPE *bpipe;
      char line[MAXSTRING];
      alert = get_pool_memory(PM_FNAME);
      alert = edit_device_codes(dcr, alert, dcr->device->alert_command, "");
      bpipe = open_bpipe(alert, 0, "r");
      if (bpipe) {
         while (fgets(line, sizeof(line), bpipe->rfd)) {
            Jmsg(jcr, M_ALERT, 0, _("Alert: %s"), line);
         }
         status = close_bpipe(bpipe);
      } else {
         status = errno;
      }
      if (status != 0) {
         berrno be;
         Jmsg(jcr, M_ALERT, 0, _("3997 Bad alert command: %s: ERR=%s.\n"),
              alert, be.bstrerror(status));
      }

      Dmsg1(400, "alert status=%d\n", status);
      free_pool_memory(alert);
   }
   pthread_cond_broadcast(&dev->wait_next_vol);
   Dmsg1(100, "JobId=%u broadcast wait_device_release\n", (uint32_t)jcr->JobId);
   pthread_cond_broadcast(&wait_device_release);
   dev->dunlock();
   if (dcr->keep_dcr) {
      detach_dcr_from_dev(dcr);
   } else {
      if (jcr->read_dcr == dcr) {
         jcr->read_dcr = NULL;
      }
      if (jcr->dcr == dcr) {
         jcr->dcr = NULL;
      }
      free_dcr(dcr);
   }
   Dmsg2(100, "===== Device %s released by JobId=%u\n", dev->print_name(),
         (uint32_t)jcr->JobId);
   return ok;
}

/*
 * Create a new Device Control Record and attach
 *   it to the device (if this is a real job).
 * Note, this has been updated so that it can be called first 
 *   without a DEVICE, then a second or third time with a DEVICE,
 *   and each time, it should cleanup and point to the new device.
 *   This should facilitate switching devices.
 * Note, each dcr must point to the controlling job (jcr).  However,
 *   a job can have multiple dcrs, so we must not store in the jcr's
 *   structure as previously. The higher level routine must store
 *   this dcr in the right place
 *
 */
DCR *new_dcr(JCR *jcr, DCR *dcr, DEVICE *dev)
{
   if (!dcr) {
      dcr = (DCR *)malloc(sizeof(DCR));
      memset(dcr, 0, sizeof(DCR));
      dcr->tid = pthread_self();
      dcr->spool_fd = -1;
   }
   dcr->jcr = jcr;                 /* point back to jcr */
   /* Set device information, possibly change device */
   if (dev) {
      if (dcr->block) {
         free_block(dcr->block);
      }
      dcr->block = new_block(dev);
      if (dcr->rec) {
         free_record(dcr->rec);
      }
      dcr->rec = new_record();
      if (dcr->attached_to_dev) {
         detach_dcr_from_dev(dcr);
      }
      dcr->max_job_spool_size = dev->device->max_job_spool_size;
      dcr->device = dev->device;
      dcr->dev = dev;
      attach_dcr_to_dev(dcr);
   }
   return dcr;
}

/*
 * Search the dcrs list for the given dcr. If it is found,
 *  as it should be, then remove it. Also zap the jcr pointer
 *  to the dcr if it is the same one.
 *
 * Note, this code will be turned on when we can write to multiple
 *  dcrs at the same time.
 */
#ifdef needed
static void remove_dcr_from_dcrs(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   if (jcr->dcrs) {
      int i = 0;
      DCR *ldcr;
      int num = jcr->dcrs->size();
      for (i=0; i < num; i++) {
         ldcr = (DCR *)jcr->dcrs->get(i);
         if (ldcr == dcr) {
            jcr->dcrs->remove(i);
            if (jcr->dcr == dcr) {
               jcr->dcr = NULL;
            }
         }
      }
   }
}
#endif

static void attach_dcr_to_dev(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   if (jcr) Dmsg1(500, "JobId=%u enter attach_dcr_to_dev\n", (uint32_t)jcr->JobId);
   if (!dcr->attached_to_dev && dev->initiated && jcr && jcr->JobType != JT_SYSTEM) {
      dev->attached_dcrs->append(dcr);  /* attach dcr to device */
      dcr->attached_to_dev = true;
      Dmsg1(500, "JobId=%u attach_dcr_to_dev\n", (uint32_t)jcr->JobId);
   }
}

void detach_dcr_from_dev(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   Dmsg1(500, "JobId=%u enter detach_dcr_from_dev\n", (uint32_t)dcr->jcr->JobId);

   /* Detach this dcr only if attached */
   if (dcr->attached_to_dev && dev) {
      dev->dlock();
      unreserve_device(dcr);
      dcr->dev->attached_dcrs->remove(dcr);  /* detach dcr from device */
      dcr->attached_to_dev = false;
//    remove_dcr_from_dcrs(dcr);      /* remove dcr from jcr list */
      dev->dunlock();
   }
}

/*
 * Free up all aspects of the given dcr -- i.e. dechain it,
 *  release allocated memory, zap pointers, ...
 */
void free_dcr(DCR *dcr)
{
   JCR *jcr = dcr->jcr;

   detach_dcr_from_dev(dcr);

   if (dcr->block) {
      free_block(dcr->block);
   }
   if (dcr->rec) {
      free_record(dcr->rec);
   }
   if (jcr && jcr->dcr == dcr) {
      jcr->dcr = NULL;
   }
   free(dcr);
}
