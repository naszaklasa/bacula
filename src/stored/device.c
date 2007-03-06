/*
 *
 *  Higher Level Device routines.
 *  Knows about Bacula tape labels and such
 *
 *  NOTE! In general, subroutines that have the word
 *        "device" in the name do locking.  Subroutines
 *        that have the word "dev" in the name do not
 *        do locking.  Thus if xxx_device() calls
 *        yyy_dev(), all is OK, but if xxx_device()
 *        calls yyy_device(), everything will hang.
 *        Obviously, no zzz_dev() is allowed to call
 *        a www_device() or everything falls apart.
 *
 * Concerning the routines lock_device() and block_device()
 *  see the end of this module for details.  In general,
 *  blocking a device leaves it in a state where all threads
 *  other than the current thread block when they attempt to
 *  lock the device. They remain suspended (blocked) until the device
 *  is unblocked. So, a device is blocked during an operation
 *  that takes a long time (initialization, mounting a new
 *  volume, ...) locking a device is done for an operation
 *  that takes a short time such as writing data to the
 *  device.
 *
 *
 *   Kern Sibbald, MM, MMI
 *
 *   Version $Id: device.c,v 1.85.2.2 2005/12/22 21:35:25 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Forward referenced functions */

extern char my_name[];
extern int debug_level;

/*
 * This is the dreaded moment. We either have an end of
 * medium condition or worse, and error condition.
 * Attempt to "recover" by obtaining a new Volume.
 *
 * Here are a few things to know:
 *  dcr->VolCatInfo contains the info on the "current" tape for this job.
 *  dev->VolCatInfo contains the info on the tape in the drive.
 *    The tape in the drive could have changed several times since
 *    the last time the job used it (jcr->VolCatInfo).
 *  dcr->VolumeName is the name of the current/desired tape in the drive.
 *
 * We enter with device locked, and
 *     exit with device locked.
 *
 * Note, we are called only from one place in block.c
 *
 *  Returns: true  on success
 *           false on failure
 */
bool fixup_device_block_write_error(DCR *dcr)
{
   char PrevVolName[MAX_NAME_LENGTH];
   DEV_BLOCK *label_blk;
   DEV_BLOCK *block = dcr->block;
   char b1[30], b2[30];
   time_t wait_time;
   char dt[MAX_TIME_LENGTH];
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;

   wait_time = time(NULL);

   Dmsg0(100, "Enter fixup_device_block_write_error\n");

   block_device(dev, BST_DOING_ACQUIRE);
   /* Unlock, but leave BLOCKED */
   unlock_device(dev);

   bstrncpy(PrevVolName, dev->VolCatInfo.VolCatName, sizeof(PrevVolName));
   bstrncpy(dev->VolHdr.PrevVolumeName, PrevVolName, sizeof(dev->VolHdr.PrevVolumeName));

   label_blk = new_block(dev);
   dcr->block = label_blk;

   /* Inform User about end of medium */
   Jmsg(jcr, M_INFO, 0, _("End of medium on Volume \"%s\" Bytes=%s Blocks=%s at %s.\n"),
        PrevVolName, edit_uint64_with_commas(dev->VolCatInfo.VolCatBytes, b1),
        edit_uint64_with_commas(dev->VolCatInfo.VolCatBlocks, b2),
        bstrftime(dt, sizeof(dt), time(NULL)));

   if (!mount_next_write_volume(dcr, 1)) {
      free_block(label_blk);
      dcr->block = block;
      P(dev->mutex);
      unblock_device(dev);
      return false;                /* device locked */
   }
   P(dev->mutex);                  /* lock again */

   Jmsg(jcr, M_INFO, 0, _("New volume \"%s\" mounted on device %s at %s.\n"),
      dcr->VolumeName, dev->print_name(), bstrftime(dt, sizeof(dt), time(NULL)));

   /*
    * If this is a new tape, the label_blk will contain the
    *  label, so write it now. If this is a previously
    *  used tape, mount_next_write_volume() will return an
    *  empty label_blk, and nothing will be written.
    */
   Dmsg0(190, "write label block to dev\n");
   if (!write_block_to_dev(dcr)) {
      berrno be;
      Pmsg1(0, _("write_block_to_device Volume label failed. ERR=%s"),
        be.strerror(dev->dev_errno));
      free_block(label_blk);
      dcr->block = block;
      unblock_device(dev);
      return false;                /* device locked */
   }
   free_block(label_blk);
   dcr->block = block;

   /*
    * Walk through all attached jcrs indicating the volume has changed
    */
   Dmsg1(100, "Walk attached dcrs. Volume=%s\n", dev->VolCatInfo.VolCatName);
   DCR *mdcr;
   foreach_dlist(mdcr, dev->attached_dcrs) {
      JCR *mjcr = mdcr->jcr;
      if (mjcr->JobId == 0) {
         continue;                 /* ignore console */
      }
      mdcr->NewVol = true;
      if (jcr != mjcr) {
         bstrncpy(mdcr->VolumeName, dcr->VolumeName, sizeof(mdcr->VolumeName));
      }
   }

   /* Clear NewVol now because dir_get_volume_info() already done */
   jcr->dcr->NewVol = false;
   set_new_volume_parameters(dcr);

   jcr->run_time += time(NULL) - wait_time; /* correct run time for mount wait */

   /* Write overflow block to device */
   Dmsg0(190, "Write overflow block to dev\n");
   if (!write_block_to_dev(dcr)) {
      berrno be;
      Pmsg1(0, _("write_block_to_device overflow block failed. ERR=%s"),
        be.strerror(dev->dev_errno));
      unblock_device(dev);
      return false;                /* device locked */
   }

   unblock_device(dev);
   return true;                             /* device locked */
}

/*
 * We have a new Volume mounted, so reset the Volume parameters
 *  concerning this job.  The global changes were made earlier
 *  in the dev structure.
 */
void set_new_volume_parameters(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   if (dcr->NewVol && !dir_get_volume_info(dcr, GET_VOL_INFO_FOR_WRITE)) {
      Jmsg1(jcr, M_ERROR, 0, "%s", jcr->errmsg);
   }
   /* Set new start/end positions */
   if (dev->is_tape()) {
      dcr->StartBlock = dev->block_num;
      dcr->StartFile = dev->file;
   } else {
      dcr->StartBlock = (uint32_t)dev->file_addr;
      dcr->StartFile  = (uint32_t)(dev->file_addr >> 32);
   }
   /* Reset indicies */
   dcr->VolFirstIndex = 0;
   dcr->VolLastIndex = 0;
   jcr->NumVolumes++;
   dcr->NewVol = false;
   dcr->WroteVol = false;
}

/*
 * We are now in a new Volume file, so reset the Volume parameters
 *  concerning this job.  The global changes were made earlier
 *  in the dev structure.
 */
void set_new_file_parameters(DCR *dcr)
{
   DEVICE *dev = dcr->dev;

   /* Set new start/end positions */
   if (dev->is_tape()) {
      dcr->StartBlock = dev->block_num;
      dcr->StartFile = dev->file;
   } else {
      dcr->StartBlock = (uint32_t)dev->file_addr;
      dcr->StartFile  = (uint32_t)(dev->file_addr >> 32);
   }
   /* Reset indicies */
   dcr->VolFirstIndex = 0;
   dcr->VolLastIndex = 0;
   dcr->NewFile = false;
   dcr->WroteVol = false;
}



/*
 *   First Open of the device. Expect dev to already be initialized.
 *
 *   This routine is used only when the Storage daemon starts
 *   and always_open is set, and in the stand-alone utility
 *   routines such as bextract.
 *
 *   Note, opening of a normal file is deferred to later so
 *    that we can get the filename; the device_name for
 *    a file is the directory only.
 *
 *   Returns: false on failure
 *            true  on success
 */
bool first_open_device(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   bool ok = true;

   Dmsg0(120, "start open_output_device()\n");
   if (!dev) {
      return false;
   }

   lock_device(dev);

   /* Defer opening files */
   if (!dev->is_tape()) {
      Dmsg0(129, "Device is file, deferring open.\n");
      goto bail_out;
   }

    int mode;
    if (dev_cap(dev, CAP_STREAM)) {
       mode = OPEN_WRITE_ONLY;
    } else {
       mode = OPEN_READ_ONLY;
    }
   Dmsg0(129, "Opening device.\n");
   if (dev->open(dcr, mode) < 0) {
      Emsg1(M_FATAL, 0, _("dev open failed: %s\n"), dev->errmsg);
      ok = false;
      goto bail_out;
   }
   Dmsg1(129, "open dev %s OK\n", dev->print_name());

bail_out:
   unlock_device(dev);
   return ok;
}

/*
 * Make sure device is open, if not do so
 */
bool open_device(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   /* Open device */
   int mode;
   if (dev_cap(dev, CAP_STREAM)) {
      mode = OPEN_WRITE_ONLY;
   } else {
      mode = OPEN_READ_WRITE;
   }
   if (dev->open(dcr, mode) < 0) {
      /* If polling, ignore the error */
      /* If DVD, also ignore the error, very often you cannot open the device
       * (when there is no DVD, or when the one inserted is a wrong one) */
      if ((!dev->poll) && (!dev->is_dvd())) {
         Jmsg2(dcr->jcr, M_FATAL, 0, _("Unable to open device %s: ERR=%s\n"),
            dev->print_name(), strerror_dev(dev));
         Pmsg2(000, _("Unable to open archive %s: ERR=%s\n"), 
            dev->print_name(), strerror_dev(dev));
      }
      return false;
   }
   return true;
}

/*
 * Release any Volume attached to this device 
 *  then close the device.
 */
void close_device(DEVICE *dev)
{
   free_volume(dev);
   dev->close();
}

/*
 */
void force_close_device(DEVICE *dev)
{
   if (!dev || dev->fd < 0) {
      return;
   }
   Dmsg1(29, "Force close_dev %s\n", dev->print_name());
   free_volume(dev);
   dev->close();
}


void dev_lock(DEVICE *dev)
{
   int errstat;
   if ((errstat=rwl_writelock(&dev->lock))) {
      Emsg1(M_ABORT, 0, _("Device write lock failure. ERR=%s\n"), strerror(errstat));
   }
}

void dev_unlock(DEVICE *dev)
{
   int errstat;
   if ((errstat=rwl_writeunlock(&dev->lock))) {
      Emsg1(M_ABORT, 0, _("Device write unlock failure. ERR=%s\n"), strerror(errstat));
   }
}

/*
 * When dev_blocked is set, all threads EXCEPT thread with id no_wait_id
 * must wait. The no_wait_id thread is out obtaining a new volume
 * and preparing the label.
 */
void _lock_device(const char *file, int line, DEVICE *dev)
{
   int stat;
   Dmsg3(500, "lock %d from %s:%d\n", dev->dev_blocked, file, line);
   P(dev->mutex);
   if (dev->dev_blocked && !pthread_equal(dev->no_wait_id, pthread_self())) {
      dev->num_waiting++;             /* indicate that I am waiting */
      while (dev->dev_blocked) {
         if ((stat = pthread_cond_wait(&dev->wait, &dev->mutex)) != 0) {
            V(dev->mutex);
            Emsg1(M_ABORT, 0, _("pthread_cond_wait failure. ERR=%s\n"),
               strerror(stat));
         }
      }
      dev->num_waiting--;             /* no longer waiting */
   }
}

/*
 * Check if the device is blocked or not
 */
bool is_device_unmounted(DEVICE *dev)
{
   bool stat;
   int blocked = dev->dev_blocked;
   stat = (blocked == BST_UNMOUNTED) ||
          (blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);
   return stat;
}

void _unlock_device(const char *file, int line, DEVICE *dev)
{
   Dmsg2(500, "unlock from %s:%d\n", file, line);
   V(dev->mutex);
}

/*
 * Block all other threads from using the device
 *  Device must already be locked.  After this call,
 *  the device is blocked to any thread calling lock_device(),
 *  but the device is not locked (i.e. no P on device).  Also,
 *  the current thread can do slip through the lock_device()
 *  calls without blocking.
 */
void _block_device(const char *file, int line, DEVICE *dev, int state)
{
   Dmsg3(500, "block set %d from %s:%d\n", state, file, line);
   ASSERT(dev->get_blocked() == BST_NOT_BLOCKED);
   dev->set_blocked(state);           /* make other threads wait */
   dev->no_wait_id = pthread_self();  /* allow us to continue */
}



/*
 * Unblock the device, and wake up anyone who went to sleep.
 */
void _unblock_device(const char *file, int line, DEVICE *dev)
{
   Dmsg3(500, "unblock %s from %s:%d\n", dev->print_blocked(), file, line);
   ASSERT(dev->dev_blocked);
   dev->set_blocked(BST_NOT_BLOCKED);
   dev->no_wait_id = 0;
   if (dev->num_waiting > 0) {
      pthread_cond_broadcast(&dev->wait); /* wake them up */
   }
}

/*
 * Enter with device locked and blocked
 * Exit with device unlocked and blocked by us.
 */
void _steal_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold, int state)
{

   Dmsg3(400, "steal lock. old=%s from %s:%d\n", dev->print_blocked(),
      file, line);
   hold->dev_blocked = dev->get_blocked();
   hold->dev_prev_blocked = dev->dev_prev_blocked;
   hold->no_wait_id = dev->no_wait_id;
   dev->set_blocked(state);
   Dmsg1(400, "steal lock. new=%s\n", dev->print_blocked());
   dev->no_wait_id = pthread_self();
   V(dev->mutex);
}

/*
 * Enter with device blocked by us but not locked
 * Exit with device locked, and blocked by previous owner
 */
void _give_back_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold)
{
   Dmsg3(400, "return lock. old=%s from %s:%d\n",
      dev->print_blocked(), file, line);
   P(dev->mutex);
   dev->dev_blocked = hold->dev_blocked;
   dev->dev_prev_blocked = hold->dev_prev_blocked;
   dev->no_wait_id = hold->no_wait_id;
   Dmsg1(400, "return lock. new=%s\n", dev->print_blocked());
   if (dev->num_waiting > 0) {
      pthread_cond_broadcast(&dev->wait); /* wake them up */
   }
}
