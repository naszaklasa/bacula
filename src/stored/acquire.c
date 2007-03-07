/*
 *  Routines to acquire and release a device for read/write
 *
 *   Kern Sibbald, August MMII
 *
 *   Version $Id: acquire.c 4183 2007-02-15 18:57:55Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

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
   
   Dmsg1(50, "jcr->dcr=%p\n", jcr->dcr);
   dev->block(BST_DOING_ACQUIRE);

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
    */
   Dmsg2(100, "MediaType dcr=%s dev=%s\n", dcr->media_type, dev->device->media_type);
   if (dcr->media_type[0] && strcmp(dcr->media_type, dev->device->media_type) != 0) {
      RCTX rctx;
      DIRSTORE *store;
      int stat;
      DCR *dcr_save = jcr->dcr;

      lock_reservations();
      jcr->dcr = NULL;
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
      
      /*
       * Note, if search_for_device() succeeds, we get a new_dcr,
       *  which we do not use except for the dev info.
       */
      stat = search_res_for_device(rctx);
      release_msgs(jcr);              /* release queued messages */
      unlock_reservations();
      if (stat == 1) {
         DCR *new_dcr = jcr->read_dcr;
         dev->unblock();
         detach_dcr_from_dev(dcr);    /* release old device */
         /* Copy important info from the new dcr */
         dev = dcr->dev = new_dcr->dev; 
         jcr->read_dcr = dcr; 
         dcr->device = new_dcr->device;
         dcr->max_job_spool_size = dcr->device->max_job_spool_size;
         attach_dcr_to_dev(dcr);
         new_dcr->VolumeName[0] = 0;
         free_dcr(new_dcr);
         dev->block(BST_DOING_ACQUIRE); 
         Jmsg(jcr, M_INFO, 0, _("Media Type change.  New device %s chosen.\n"),
            dev->print_name());
         bstrncpy(dcr->VolumeName, vol->VolumeName, sizeof(dcr->VolumeName));
         bstrncpy(dcr->media_type, vol->MediaType, sizeof(dcr->media_type));
         dcr->VolCatInfo.Slot = vol->Slot;
         bstrncpy(dcr->pool_name, store->pool_name, sizeof(dcr->pool_name));
         bstrncpy(dcr->pool_type, store->pool_type, sizeof(dcr->pool_type));
      } else if (stat == 0) {   /* device busy */
         Pmsg1(000, "Device %s is busy.\n", vol->device);
      } else {
         /* error */
         Jmsg1(jcr, M_FATAL, 0, _("No suitable device found to read Volume \"%s\"\n"),
            vol->VolumeName);
         jcr->dcr = dcr_save;
         goto get_out;
      }
      jcr->dcr = dcr_save;
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
      Dmsg1(100, "opened dev %s OK\n", dev->print_name());
      
      /* Read Volume Label */
      
      Dmsg0(200, "calling read-vol-label\n");
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
   P(dev->mutex);
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   V(dev->mutex);
   dev->unblock();
   Dmsg1(50, "jcr->dcr=%p\n", jcr->dcr);
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

   dev->block(BST_DOING_ACQUIRE);
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
            free_unused_volume(dcr);
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
   P(dev->mutex);
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   V(dev->mutex);
   dev->unblock();
   return dcr;

/*
 * Error return
 */
get_out:
   P(dev->mutex);
   if (dcr->reserved_device) {
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
   }
   V(dev->mutex);
   dev->unblock();
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
   if (!dcr->dev_locked) {
      lock_device(dev);
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
//    dir_update_volume_info(dcr, false); /* send Volume info to Director */

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
              alert, be.strerror(status));
      }

      Dmsg1(400, "alert status=%d\n", status);
      free_pool_memory(alert);
   }
   dcr->dev_locked = false;              /* set no longer locked */
   unlock_device(dev);
   if (jcr->read_dcr == dcr) {
      jcr->read_dcr = NULL;
   }
   if (jcr->dcr == dcr) {
      jcr->dcr = NULL;
   }
   free_dcr(dcr);
   return ok;
}

/*
 * Create a new Device Control Record and attach
 *   it to the device (if this is a real job).
 */
DCR *new_dcr(JCR *jcr, DEVICE *dev)
{
   DCR *dcr = (DCR *)malloc(sizeof(DCR));
   memset(dcr, 0, sizeof(DCR));
   dcr->jcr = jcr;
   if (dev) {
      dcr->dev = dev;
      dcr->device = dev->device;
      dcr->block = new_block(dev);
      dcr->rec = new_record();
      dcr->max_job_spool_size = dev->device->max_job_spool_size;
      attach_dcr_to_dev(dcr);
   }
   dcr->spool_fd = -1;
   return dcr;
}

/*
 * Search the dcrs list for the given dcr. If it is found,
 *  as it should be, then remove it. Also zap the jcr pointer
 *  to the dcr if it is the same one.
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

   if (!dcr->attached_to_dev && dev->is_open() && jcr && jcr->JobType != JT_SYSTEM) {
      dev->attached_dcrs->append(dcr);  /* attach dcr to device */
      dcr->attached_to_dev = true;
   }
}

void detach_dcr_from_dev(DCR *dcr)
{
   DEVICE *dev = dcr->dev;

   if (dcr->reserved_device) {
      dcr->reserved_device = false;
      lock_device(dev);
      dev->reserved_device--;
      Dmsg2(100, "Dec reserve=%d dev=%s\n", dev->reserved_device, dev->print_name());
      dcr->reserved_device = false;
      /* If we set read mode in reserving, remove it */
      if (dev->can_read()) {
         dev->clear_read();
      }
      if (dev->num_writers < 0) {
         Jmsg1(dcr->jcr, M_ERROR, 0, _("Hey! num_writers=%d!!!!\n"), dev->num_writers);
         dev->num_writers = 0;
      }
      unlock_device(dev);
   }

   /* Detach this dcr only if attached */
   if (dcr->attached_to_dev) {
      dcr->dev->attached_dcrs->remove(dcr);  /* detach dcr from device */
      dcr->attached_to_dev = false;
//    remove_dcr_from_dcrs(dcr);      /* remove dcr from jcr list */
   }
   free_unused_volume(dcr);           /* free unused vols attached to this dcr */
   pthread_cond_broadcast(&dcr->dev->wait_next_vol);
   pthread_cond_broadcast(&wait_device_release);
}

/*
 * Free up all aspects of the given dcr -- i.e. dechain it,
 *  release allocated memory, zap pointers, ...
 */
void free_dcr(DCR *dcr)
{
   JCR *jcr = dcr->jcr;

   if (dcr->dev) {
      detach_dcr_from_dev(dcr);
   }

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
