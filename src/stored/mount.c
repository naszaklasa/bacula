/*
 *
 *  Routines for handling mounting tapes for reading and for
 *    writing.
 *
 *   Kern Sibbald, August MMII
 *
 *   Version $Id: mount.c,v 1.125.2.1 2007/01/25 11:26:03 kerns Exp $
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

static void mark_volume_not_inchanger(DCR *dcr);
static int try_autolabel(DCR *dcr, bool opened);

enum {
   try_next_vol = 1,
   try_read_vol,
   try_error,
   try_default
};

/*
 * If release is set, we rewind the current volume,
 * which we no longer want, and ask the user (console)
 * to mount the next volume.
 *
 *  Continue trying until we get it, and then ensure
 *  that we can write on it.
 *
 * This routine returns a 0 only if it is REALLY
 *  impossible to get the requested Volume.
 *
 */
bool mount_next_write_volume(DCR *dcr, bool release)
{
   int retry = 0;
   bool ask = false, recycle, autochanger;
   int vol_label_status;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   DEV_BLOCK *block = dcr->block;
   int mode;

   Dmsg2(150, "Enter mount_next_volume(release=%d) dev=%s\n", release,
      dev->print_name());

   init_device_wait_timers(dcr);

   /*
    * Attempt to mount the next volume. If something non-fatal goes
    *  wrong, we come back here to re-try (new op messages, re-read
    *  Volume, ...)
    */
mount_next_vol:
   Dmsg1(150, "mount_next_vol retry=%d\n", retry);
   /* Ignore retry if this is poll request */
   if (!dev->poll && retry++ > 4) {
      /* Last ditch effort before giving up, force operator to respond */
      dcr->VolCatInfo.Slot = 0;
      if (!dir_ask_sysop_to_mount_volume(dcr)) {
         Jmsg(jcr, M_FATAL, 0, _("Too many errors trying to mount device %s.\n"),
              dev->print_name());
         return false;
      }
   }
   if (job_canceled(jcr)) {
      Jmsg(jcr, M_FATAL, 0, _("Job %d canceled.\n"), jcr->JobId);
      return false;
   }
   recycle = false;
   if (release) {
      Dmsg0(150, "mount_next_volume release=1\n");
      release_volume(dcr);
      ask = true;                     /* ask operator to mount tape */
   }

   /*
    * Get Director's idea of what tape we should have mounted.
    *    in dcr->VolCatInfo
    */
   Dmsg0(200, "Before dir_find_next_appendable_volume.\n");
   while (!dir_find_next_appendable_volume(dcr)) {
       Dmsg0(200, "not dir_find_next\n");
       if (!dir_ask_sysop_to_create_appendable_volume(dcr)) {
         return false;
       }
       Dmsg0(200, "Again dir_find_next_append...\n");
   }
   if (job_canceled(jcr)) {
      return false;
   }
   Dmsg3(150, "After find_next_append. Vol=%s Slot=%d Parts=%d\n",
         dcr->VolCatInfo.VolCatName, dcr->VolCatInfo.Slot, dcr->VolCatInfo.VolCatParts);
   
   /*
    * Get next volume and ready it for append
    * This code ensures that the device is ready for
    * writing. We start from the assumption that there
    * may not be a tape mounted.
    *
    * If the device is a file, we create the output
    * file. If it is a tape, we check the volume name
    * and move the tape to the end of data.
    *
    */
   if (autoload_device(dcr, 1, NULL) > 0) {
      autochanger = true;
      ask = false;
   } else {
      autochanger = false;
      dcr->VolCatInfo.Slot = 0;
   }
   Dmsg1(200, "autoload_dev returns %d\n", autochanger);
   /*
    * If we autochanged to correct Volume or (we have not just
    *   released the Volume AND we can automount) we go ahead
    *   and read the label. If there is no tape in the drive,
    *   we will err, recurse and ask the operator the next time.
    */
   if (!release && dev->is_tape() && dev->has_cap(CAP_AUTOMOUNT)) {
      Dmsg0(150, "(1)Ask=0\n");
      ask = false;                 /* don't ask SYSOP this time */
   }
   /* Don't ask if not removable */
   if (!dev->is_removable()) {
      Dmsg0(150, "(2)Ask=0\n");
      ask = false;
   }
   Dmsg2(150, "Ask=%d autochanger=%d\n", ask, autochanger);
   release = true;                /* release next time if we "recurse" */

   if (ask && !dir_ask_sysop_to_mount_volume(dcr)) {
      Dmsg0(150, "Error return ask_sysop ...\n");
      return false;          /* error return */
   }
   if (job_canceled(jcr)) {
      return false;
   }
   Dmsg1(150, "want vol=%s\n", dcr->VolumeName);

   if (dev->poll && dev->has_cap(CAP_CLOSEONPOLL)) {
      dev->close();
   }

   /* Ensure the device is open */
   if (dev->has_cap(CAP_STREAM)) {
      mode = OPEN_WRITE_ONLY;
   } else {
      mode = OPEN_READ_WRITE;
   }
   /* Try autolabel if enabled */
   if (dev->open(dcr, mode) < 0) {
      try_autolabel(dcr, false);      /* try to create a new volume label */
   }
   while (dev->open(dcr, mode) < 0) {
      Dmsg1(150, "open_device failed: ERR=%s\n", dev->bstrerror());
      if ((dev->is_file() && dev->is_removable()) || dev->is_dvd()) {
         bool ok = true;
         Dmsg0(150, "call scan_dir_for_vol\n");
         if (dev->is_dvd()) {
            if (!dev->mount(0)) {
               ok = false;
            }
         }
         if (ok && dev->scan_dir_for_volume(dcr)) {
            if (dev->open(dcr, mode) >= 0) {
               break;                    /* got a valid volume */
            }
         }
         if (ok && dev->is_dvd()) {
            dev->unmount(0);
         }
      }
      if (try_autolabel(dcr, false) == try_read_vol) {
         break;                       /* created a new volume label */
      }
      /* If DVD, ignore the error, very often you cannot open the device
       * (when there is no DVD, or when the one inserted is a wrong one) */
      if (dev->poll || dev->is_dvd() || dev->is_removable()) {
         goto mount_next_vol;
      } else {
         Jmsg(jcr, M_ERROR, 0, _("Could not open device %s: ERR=%s\n"),
            dev->print_name(), dev->print_errmsg());
         return false;
      }
   }

   /*
    * Now make sure we have the right tape mounted
    */
read_volume:
   /*
    * If we are writing to a stream device, ASSUME the volume label
    *  is correct.
    */
   if (dev->has_cap(CAP_STREAM)) {
      vol_label_status = VOL_OK;
      create_volume_label(dev, dcr->VolumeName, "Default", false /* not DVD */);
      dev->VolHdr.LabelType = PRE_LABEL;
   } else {
      vol_label_status = read_dev_volume_label(dcr);
   }
   if (job_canceled(jcr)) {
      return false;
   }

   Dmsg2(150, "Want dirVol=%s dirStat=%s\n", dcr->VolumeName,
      dcr->VolCatInfo.VolCatStatus);
   /*
    * At this point, dev->VolCatInfo has what is in the drive, if anything,
    *          and   dcr->VolCatInfo has what the Director wants.
    */
   switch (vol_label_status) {
   case VOL_OK:
      Dmsg1(150, "Vol OK name=%s\n", dcr->VolumeName);
      dev->VolCatInfo = dcr->VolCatInfo;       /* structure assignment */
      recycle = strcmp(dev->VolCatInfo.VolCatStatus, "Recycle") == 0;
      break;                    /* got a Volume */
   case VOL_NAME_ERROR:
      VOLUME_CAT_INFO dcrVolCatInfo, devVolCatInfo;
      char VolumeName[MAX_NAME_LENGTH];

      /* If not removable, Volume is broken */
      if (!dev->is_removable()) {
         Jmsg(jcr, M_WARNING, 0, _("Volume \"%s\" not on device %s.\n"),
            dcr->VolumeName, dev->print_name());
         mark_volume_in_error(dcr);
         goto mount_next_vol;
      }

      Dmsg1(150, "Vol NAME Error Name=%s\n", dcr->VolumeName);
      /* If polling and got a previous bad name, ignore it */
      if (dev->poll && strcmp(dev->BadVolName, dev->VolHdr.VolumeName) == 0) {
         ask = true;
         Dmsg1(200, "Vol Name error supress due to poll. Name=%s\n", dcr->VolumeName);
         goto mount_next_vol;
      }
      /*
       * OK, we got a different volume mounted. First save the
       *  requested Volume info (dcr) structure, then query if
       *  this volume is really OK. If not, put back the desired
       *  volume name, mark it not in changer and continue.
       */
      dcrVolCatInfo = dcr->VolCatInfo;      /* structure assignment */
      devVolCatInfo = dev->VolCatInfo;      /* structure assignment */
      /* Check if this is a valid Volume in the pool */
      bstrncpy(VolumeName, dcr->VolumeName, sizeof(VolumeName));
      bstrncpy(dcr->VolumeName, dev->VolHdr.VolumeName, sizeof(dcr->VolumeName));
      if (!dir_get_volume_info(dcr, GET_VOL_INFO_FOR_WRITE)) {
         /* Restore desired volume name, note device info out of sync */
         /* This gets the info regardless of the Pool */
         bstrncpy(dcr->VolumeName, dev->VolHdr.VolumeName, sizeof(dcr->VolumeName));
         if (autochanger && !dir_get_volume_info(dcr, GET_VOL_INFO_FOR_READ)) {
            /*
             * If we get here, we know we cannot write on the Volume,
             *  and we know that we cannot read it either, so it 
             *  is not in the autochanger.
             */
            mark_volume_not_inchanger(dcr);
         }
         dev->VolCatInfo = devVolCatInfo;    /* structure assignment */
         bstrncpy(dev->BadVolName, dev->VolHdr.VolumeName, sizeof(dev->BadVolName));
         Jmsg(jcr, M_WARNING, 0, _("Director wanted Volume \"%s\".\n"
              "    Current Volume \"%s\" not acceptable because:\n"
              "    %s"),
             dcrVolCatInfo.VolCatName, dev->VolHdr.VolumeName,
             jcr->dir_bsock->msg);
         ask = true;
         /* Restore saved DCR before continuing */
         bstrncpy(dcr->VolumeName, VolumeName, sizeof(dcr->VolumeName));
         dcr->VolCatInfo = dcrVolCatInfo;  /* structure assignment */
         goto mount_next_vol;
      }
      /*
       * This was not the volume we expected, but it is OK with
       * the Director, so use it.
       */
      Dmsg1(150, "want new name=%s\n", dcr->VolumeName);
      dev->VolCatInfo = dcr->VolCatInfo;   /* structure assignment */
      recycle = strcmp(dev->VolCatInfo.VolCatStatus, "Recycle") == 0;
      break;                /* got a Volume */
   /*
    * At this point, we assume we have a blank tape mounted.
    */
   case VOL_IO_ERROR:
      if (dev->is_dvd()) {
         Jmsg(jcr, M_FATAL, 0, "%s", jcr->errmsg);
         mark_volume_in_error(dcr);
         return false;       /* we could not write on DVD */
      }
      /* Fall through wanted */
   case VOL_NO_LABEL:
      switch (try_autolabel(dcr, true)) {
      case try_next_vol:
         goto mount_next_vol;
      case try_read_vol:
         goto read_volume;
      case try_error:
         return false;
      case try_default:
         break;
      }

      /* NOTE! Fall-through wanted. */
   case VOL_NO_MEDIA:
   default:
      Dmsg0(200, "VOL_NO_MEDIA or default.\n");
      /* Send error message */
      if (!dev->poll) {
      } else {
         Dmsg1(200, "Msg suppressed by poll: %s\n", jcr->errmsg);
      }
      ask = true;
      /* Needed, so the medium can be changed */
      if (dev->requires_mount()) {
         dev->close();
      }
      goto mount_next_vol;
   }

   /*
    * See if we have a fresh tape or a tape with data.
    *
    * Note, if the LabelType is PRE_LABEL, it was labeled
    *  but never written. If so, rewrite the label but set as
    *  VOL_LABEL.  We rewind and return the label (reconstructed)
    *  in the block so that in the case of a new tape, data can
    *  be appended just after the block label.  If we are writing
    *  a second volume, the calling routine will write the label
    *  before writing the overflow block.
    *
    *  If the tape is marked as Recycle, we rewrite the label.
    */
   if (dev->VolHdr.LabelType == PRE_LABEL || recycle) {
      if (!rewrite_volume_label(dcr, recycle)) {
         mark_volume_in_error(dcr);
         goto mount_next_vol;
      }
   } else {
      /*
       * OK, at this point, we have a valid Bacula label, but
       * we need to position to the end of the volume, since we are
       * just now putting it into append mode.
       */
      Dmsg0(200, "Device previously written, moving to end of data\n");
      Jmsg(jcr, M_INFO, 0, _("Volume \"%s\" previously written, moving to end of data.\n"),
         dcr->VolumeName);
      if (!dev->eod(dcr)) {
         Jmsg(jcr, M_ERROR, 0, _("Unable to position to end of data on device %s: ERR=%s\n"),
            dev->print_name(), dev->bstrerror());
         mark_volume_in_error(dcr);
         goto mount_next_vol;
      }
      if (dev->is_dvd()) {
         char ed1[50], ed2[50];
         if (dev->VolCatInfo.VolCatBytes == dev->part_start + dev->part_size) {
            Jmsg(jcr, M_INFO, 0, _("Ready to append to end of Volume \"%s\""
                 " part=%d size=%s\n"), dcr->VolumeName, 
                 dev->part, edit_uint64(dev->VolCatInfo.VolCatBytes,ed1));
         } else {
            Jmsg(jcr, M_ERROR, 0, _("I cannot write on Volume \"%s\" because: "
                 "The sizes do not match! Volume=%s Catalog=%s\n"),
                 dcr->VolumeName,
                 edit_uint64(dev->part_start + dev->part_size, ed1),
                 edit_uint64(dev->VolCatInfo.VolCatBytes, ed2));
            mark_volume_in_error(dcr);
            goto mount_next_vol;
         }
      }
      /* *****FIXME**** we should do some checking for files too */
      if (dev->is_tape()) {
         /*
          * Check if we are positioned on the tape at the same place
          * that the database says we should be.
          */
         if (dev->VolCatInfo.VolCatFiles == dev->get_file()) {
            Jmsg(jcr, M_INFO, 0, _("Ready to append to end of Volume \"%s\" at file=%d.\n"),
                 dcr->VolumeName, dev->get_file());
         } else {
            Jmsg(jcr, M_ERROR, 0, _("I cannot write on Volume \"%s\" because:\n"
                 "The number of files mismatch! Volume=%u Catalog=%u\n"),
                 dcr->VolumeName, dev->get_file(), dev->VolCatInfo.VolCatFiles);
            mark_volume_in_error(dcr);
            goto mount_next_vol;
         }
      }
      dev->VolCatInfo.VolCatMounts++;      /* Update mounts */
      Dmsg1(150, "update volinfo mounts=%d\n", dev->VolCatInfo.VolCatMounts);
      if (!dir_update_volume_info(dcr, false)) {
         return false;
      }
      
      /*
       * DVD : check if the last part was removed or truncated, or if a written
       * part was overwritten.   
       * We need to do it after dir_update_volume_info, so we have the EndBlock
       * info. (nb: I don't understand why VolCatFiles is set (used to check
       * tape file number), but not EndBlock)
       * Maybe could it be changed "dev->is_file()" (would remove the fixme above)   
       *
       * Disabled: I had problems with this code... 
       * (maybe is it related to the seek bug ?)   
       */
#ifdef xxx
      if (dev->is_dvd()) {
         Dmsg2(150, "DVD/File sanity check addr=%u vs endblock=%u\n", (unsigned int)dev->file_addr, (unsigned int)dev->VolCatInfo.EndBlock);
         if (dev->file_addr == dev->VolCatInfo.EndBlock+1) {
            Jmsg(jcr, M_INFO, 0, _("Ready to append to end of Volume \"%s\" at file address=%u.\n"),
                 dcr->VolumeName, (unsigned int)dev->file_addr);
         }
         else {
            Jmsg(jcr, M_ERROR, 0, _("I cannot write on Volume \"%s\" because:\n"
                                    "The EOD file address is wrong: Volume file address=%u != Catalog Endblock=%u(+1)\n"
                                    "You probably removed DVD last part in spool directory.\n"),
                 dcr->VolumeName, (unsigned int)dev->file_addr, (unsigned int)dev->VolCatInfo.EndBlock);
            mark_volume_in_error(dcr);
            goto mount_next_vol;
         }
      }
#endif
      
      /* Return an empty block */
      empty_block(block);             /* we used it for reading so set for write */
   }
   dev->set_append();
   Dmsg1(150, "set APPEND, normal return from mount_next_write_volume. dev=%s\n",
      dev->print_name());

   return true;
}

/*
 * If permitted, we label the device, make sure we can do
 *   it by checking that the VolCatBytes is zero => not labeled,
 *   once the Volume is labeled we don't want to label another
 *   blank tape with the same name.  For disk, we go ahead and
 *   label it anyway, because the OS insures that there is only
 *   one Volume with that name.
 * As noted above, at this point dcr->VolCatInfo has what
 *   the Director wants and dev->VolCatInfo has info on the
 *   previous tape (or nothing).
 *
 * Return codes are:
 *   try_next_vol        label failed, look for another volume
 *   try_read_vol        labeled volume, now re-read the label
 *   try_error           hard error (catalog update)
 *   try_default         I couldn't do anything
 */
static int try_autolabel(DCR *dcr, bool opened)
{
   DEVICE *dev = dcr->dev;

   if (dev->poll && !dev->is_tape()) {
      return try_default;       /* if polling, don't try to create new labels */
   }
   /* For a tape require it to be opened and read before labeling */
   if (!opened && dev->is_tape()) {
      return try_default;
   }
   if (dev->has_cap(CAP_LABEL) && (dcr->VolCatInfo.VolCatBytes == 0 ||
         (!dev->is_tape() && strcmp(dcr->VolCatInfo.VolCatStatus,
                                "Recycle") == 0))) {
      Dmsg0(150, "Create volume label\n");
      /* Create a new Volume label and write it to the device */
      if (!write_new_volume_label_to_dev(dcr, dcr->VolumeName,
             dcr->pool_name, false, /* no relabel */ false /* defer DVD label */)) {
         Dmsg0(150, "!write_vol_label\n");
         if (opened) { 
            mark_volume_in_error(dcr);
         }
         return try_next_vol;
      }
      Dmsg0(150, "dir_update_vol_info. Set Append\n");
      /* Copy Director's info into the device info */
      dev->VolCatInfo = dcr->VolCatInfo;    /* structure assignment */
      if (!dir_update_volume_info(dcr, true)) {  /* indicate tape labeled */
         return try_error;
      }
      Jmsg(dcr->jcr, M_INFO, 0, _("Labeled new Volume \"%s\" on device %s.\n"),
         dcr->VolumeName, dev->print_name());
      return try_read_vol;   /* read label we just wrote */
   }
   if (!dev->has_cap(CAP_LABEL) && dcr->VolCatInfo.VolCatBytes == 0) {
      Jmsg(dcr->jcr, M_WARNING, 0, _("Device %s not configured to autolabel Volumes.\n"), 
         dev->print_name());
   }
   /* If not removable, Volume is broken */
   if (!dev->is_removable()) {
      Jmsg(dcr->jcr, M_WARNING, 0, _("Volume \"%s\" not on device %s.\n"),
         dcr->VolumeName, dev->print_name());
      mark_volume_in_error(dcr);
      return try_next_vol;
   }
   return try_default;
}


/*
 * Mark volume in error in catalog
 */
void mark_volume_in_error(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   Jmsg(dcr->jcr, M_INFO, 0, _("Marking Volume \"%s\" in Error in Catalog.\n"),
        dcr->VolumeName);
   dev->VolCatInfo = dcr->VolCatInfo;     /* structure assignment */
   bstrncpy(dev->VolCatInfo.VolCatStatus, "Error", sizeof(dev->VolCatInfo.VolCatStatus));
   Dmsg0(150, "dir_update_vol_info. Set Error.\n");
   dir_update_volume_info(dcr, false);
}

/*
 * The Volume is not in the correct slot, so mark this
 *   Volume as not being in the Changer.
 */
static void mark_volume_not_inchanger(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   Jmsg(jcr, M_ERROR, 0, _("Autochanger Volume \"%s\" not found in slot %d.\n"
"    Setting InChanger to zero in catalog.\n"),
        dcr->VolCatInfo.VolCatName, dcr->VolCatInfo.Slot);
   dev->VolCatInfo = dcr->VolCatInfo;    /* structure assignment */
   dcr->VolCatInfo.InChanger = false;
   dev->VolCatInfo.InChanger = false;
   Dmsg0(400, "update vol info in mount\n");
   dir_update_volume_info(dcr, true);  /* set new status */
}

/*
 * Either because we are going to hang a new volume, or because
 *  of explicit user request, we release the current volume.
 */
void release_volume(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   if (dcr->WroteVol) {
      Jmsg0(jcr, M_ERROR, 0, _("Hey!!!!! WroteVol non-zero !!!!!\n"));
      Dmsg0(190, "Hey!!!!! WroteVol non-zero !!!!!\n");
   }
   /*
    * First erase all memory of the current volume
    */
   dev->block_num = dev->file = 0;
   dev->EndBlock = dev->EndFile = 0;
   memset(&dev->VolCatInfo, 0, sizeof(dev->VolCatInfo));
   memset(&dcr->VolCatInfo, 0, sizeof(dcr->VolCatInfo));
   free_volume(dev);
   memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));
   /* Force re-read of label */
   dev->clear_labeled();
   dev->clear_read();
   dev->clear_append();
   dev->label_type = B_BACULA_LABEL;
   dcr->VolumeName[0] = 0;

   if (dev->is_open() && (!dev->is_tape() || !dev->has_cap(CAP_ALWAYSOPEN))) {
      dev->close();
   }

   /* If we have not closed the device, then at least rewind the tape */
   if (dev->is_open()) {
      dev->offline_or_rewind();
   }
   Dmsg0(190, "release_volume\n");
}

/*
 * If we are reading, we come here at the end of the tape
 *  and see if there are more volumes to be mounted.
 */
bool mount_next_read_volume(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   Dmsg2(90, "NumReadVolumes=%d CurReadVolume=%d\n", jcr->NumReadVolumes, jcr->CurReadVolume);
   /*
    * End Of Tape -- mount next Volume (if another specified)
    */
   if (jcr->NumReadVolumes > 1 && jcr->CurReadVolume < jcr->NumReadVolumes) {
      dev->close();
      if (!acquire_device_for_read(dcr)) {
         Jmsg2(jcr, M_FATAL, 0, _("Cannot open Dev=%s, Vol=%s\n"), dev->print_name(),
               dcr->VolumeName);
         return false;
      }
      return true;                    /* next volume mounted */
   }
   Dmsg0(90, "End of Device reached.\n");
   return false;
}
