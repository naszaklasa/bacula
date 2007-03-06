/*
 *
 *   dev.c  -- low level operations on device (storage device)
 *
 *              Kern Sibbald, MM 
 *
 *     NOTE!!!! None of these routines are reentrant. You must
 *        use lock_device() and unlock_device() at a higher level,
 *        or use the xxx_device() equivalents.  By moving the
 *        thread synchronization to a higher level, we permit
 *        the higher level routines to "seize" the device and 
 *        to carry out operations without worrying about who
 *        set what lock (i.e. race conditions).
 *
 *     Note, this is the device dependent code, and my have
 *           to be modified for each system, but is meant to
 *           be as "generic" as possible.
 * 
 *     The purpose of this code is to develop a SIMPLE Storage
 *     daemon. More complicated coding (double buffering, writer
 *     thread, ...) is left for a later version.
 *
 *     Unfortunately, I have had to add more and more complication
 *     to this code. This was not foreseen as noted above, and as
 *     a consequence has lead to something more contorted than is
 *     really necessary -- KES.  Note, this contortion has been
 *     corrected to a large extent by a rewrite (Apr MMI).
 *
 *   Version $Id: dev.c,v 1.95.2.1.2.1 2005/04/12 21:31:21 kerns Exp $
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

/*
 * Handling I/O errors and end of tape conditions are a bit tricky.
 * This is how it is currently done when writting.
 * On either an I/O error or end of tape,
 * we will stop writing on the physical device (no I/O recovery is
 * attempted at least in this daemon). The state flag will be sent
 * to include ST_EOT, which is ephimeral, and ST_WEOT, which is
 * persistent. Lots of routines clear ST_EOT, but ST_WEOT is
 * cleared only when the problem goes away.  Now when ST_WEOT
 * is set all calls to write_block_to_device() call the fix_up
 * routine. In addition, all threads are blocked
 * from writing on the tape by calling lock_dev(), and thread other
 * than the first thread to hit the EOT will block on a condition
 * variable. The first thread to hit the EOT will continue to
 * be able to read and write the tape (he sort of tunnels through
 * the locking mechanism -- see lock_dev() for details).
 *
 * Now presumably somewhere higher in the chain of command 
 * (device.c), someone will notice the EOT condition and 
 * get a new tape up, get the tape label read, and mark 
 * the label for rewriting. Then this higher level routine 
 * will write the unwritten buffer to the new volume.
 * Finally, he will release
 * any blocked threads by doing a broadcast on the condition
 * variable.  At that point, we should be totally back in 
 * business with no lost data.
 */


#include "bacula.h"
#include "stored.h"

/* Forward referenced functions */
void set_os_device_parameters(DEVICE *dev);

/* 
 * Allocate and initialize the DEVICE structure
 * Note, if dev is non-NULL, it is already allocated,
 * thus we neither allocate it nor free it. This allows
 * the caller to put the packet in shared memory.
 *
 *  Note, for a tape, the device->device_name is the device name
 *     (e.g. /dev/nst0), and for a file, the device name
 *     is the directory in which the file will be placed.
 *
 */
DEVICE *
init_dev(DEVICE *dev, DEVRES *device)
{
   struct stat statp;
   bool tape, fifo;
   int errstat;
   DCR *dcr = NULL;

   /* Check that device is available */
   if (stat(device->device_name, &statp) < 0) {
      berrno be;
      if (dev) {
         dev->dev_errno = errno;
      } 
      Emsg2(M_FATAL, 0, "Unable to stat device %s : %s\n", device->device_name, 
            be.strerror());
      return NULL;
   }
   tape = false;
   fifo = false;
   if (S_ISDIR(statp.st_mode)) {
      tape = false;
   } else if (S_ISCHR(statp.st_mode)) {
      tape = true;
   } else if (S_ISFIFO(statp.st_mode)) {
      fifo = true;
   } else {
      if (dev) {
         dev->dev_errno = ENODEV;
      }
      Emsg2(M_FATAL, 0, _("%s is an unknown device type. Must be tape or directory. st_mode=%x\n"),
         device->device_name, statp.st_mode);
      return NULL;
   }
   if (!dev) {
      dev = (DEVICE *)get_memory(sizeof(DEVICE));
      memset(dev, 0, sizeof(DEVICE));
      dev->state = ST_MALLOC;
   } else {
      memset(dev, 0, sizeof(DEVICE));
   }

   /* Copy user supplied device parameters from Resource */
   dev->dev_name = get_memory(strlen(device->device_name)+1);
   strcpy(dev->dev_name, device->device_name);
   dev->capabilities = device->cap_bits;
   dev->min_block_size = device->min_block_size;
   dev->max_block_size = device->max_block_size;
   dev->max_volume_size = device->max_volume_size;
   dev->max_file_size = device->max_file_size;
   dev->volume_capacity = device->volume_capacity;
   dev->max_rewind_wait = device->max_rewind_wait;
   dev->max_open_wait = device->max_open_wait;
   dev->max_open_vols = device->max_open_vols;
   dev->vol_poll_interval = device->vol_poll_interval;
   dev->max_spool_size = device->max_spool_size;
   dev->drive_index = device->drive_index;
   /* Sanity check */
   if (dev->vol_poll_interval && dev->vol_poll_interval < 60) {
      dev->vol_poll_interval = 60;
   }
   dev->device = device;

   if (tape) {
      dev->state |= ST_TAPE;
   } else if (fifo) {
      dev->state |= ST_FIFO;
      dev->capabilities |= CAP_STREAM; /* set stream device */
   } else {
      dev->state |= ST_FILE;
   }

   if (dev->max_block_size > 1000000) {
      Emsg3(M_ERROR, 0, _("Block size %u on device %s is too large, using default %u\n"), 
         dev->max_block_size, dev->dev_name, DEFAULT_BLOCK_SIZE);
      dev->max_block_size = 0;
   }
   if (dev->max_block_size % TAPE_BSIZE != 0) {
      Emsg2(M_WARNING, 0, _("Max block size %u not multiple of device %s block size.\n"),
         dev->max_block_size, dev->dev_name);
   }   
         
   dev->errmsg = get_pool_memory(PM_EMSG);
   *dev->errmsg = 0;

   if ((errstat = pthread_mutex_init(&dev->mutex, NULL)) != 0) {
      berrno be;
      dev->dev_errno = errstat;
      Mmsg1(dev->errmsg, _("Unable to init mutex: ERR=%s\n"), be.strerror(errstat));
      Emsg0(M_FATAL, 0, dev->errmsg);
   }
   if ((errstat = pthread_cond_init(&dev->wait, NULL)) != 0) {
      berrno be;
      dev->dev_errno = errstat;
      Mmsg1(dev->errmsg, _("Unable to init cond variable: ERR=%s\n"), be.strerror(errstat));
      Emsg0(M_FATAL, 0, dev->errmsg);
   }
   if ((errstat = pthread_cond_init(&dev->wait_next_vol, NULL)) != 0) {
      berrno be;
      dev->dev_errno = errstat;
      Mmsg1(dev->errmsg, _("Unable to init cond variable: ERR=%s\n"), be.strerror(errstat));
      Emsg0(M_FATAL, 0, dev->errmsg);
   }
   if ((errstat = pthread_mutex_init(&dev->spool_mutex, NULL)) != 0) {
      berrno be;
      dev->dev_errno = errstat;
      Mmsg1(dev->errmsg, _("Unable to init mutex: ERR=%s\n"), be.strerror(errstat));
      Emsg0(M_FATAL, 0, dev->errmsg);
   }
   if ((errstat = rwl_init(&dev->lock)) != 0) {
      berrno be;
      dev->dev_errno = errstat;
      Mmsg1(dev->errmsg, _("Unable to init mutex: ERR=%s\n"), be.strerror(errstat));
      Emsg0(M_FATAL, 0, dev->errmsg);
   }

   dev->fd = -1;
   dev->attached_dcrs = New(dlist(dcr, &dcr->dev_link));
   Dmsg2(29, "init_dev: tape=%d dev_name=%s\n", dev_is_tape(dev), dev->dev_name);

   return dev;
}

/* Open the device with the operating system and
 * initialize buffer pointers.
 *
 * Note, for a tape, the VolName is the name we give to the
 *    volume (not really used here), but for a file, the
 *    VolName represents the name of the file to be created/opened.
 *    In the case of a file, the full name is the device name
 *    (archive_name) with the VolName concatenated.
 */
int
open_dev(DEVICE *dev, char *VolName, int mode)
{
   POOLMEM *archive_name;

   if (dev->state & ST_OPENED) {
      /*
       *  *****FIXME***** how to handle two threads wanting
       *  different volumes mounted???? E.g. one is waiting
       *  for the next volume to be mounted, and a new job
       *  starts and snatches up the device.
       */
      if (VolName && strcmp(dev->VolCatInfo.VolCatName, VolName) != 0) {
         return -1;
      }
      dev->use_count++;
      Mmsg2(&dev->errmsg, _("WARNING!!!! device %s opened %d times!!!\n"), 
            dev->dev_name, dev->use_count);
      Emsg1(M_WARNING, 0, "%s", dev->errmsg);
      return dev->fd;
   }
   if (VolName) {
      bstrncpy(dev->VolCatInfo.VolCatName, VolName, sizeof(dev->VolCatInfo.VolCatName));
   }

   Dmsg3(29, "open_dev: tape=%d dev_name=%s vol=%s\n", dev_is_tape(dev), 
         dev->dev_name, dev->VolCatInfo.VolCatName);
   dev->state &= ~(ST_LABEL|ST_APPEND|ST_READ|ST_EOT|ST_WEOT|ST_EOF);
   dev->file_size = 0;
   if (dev->state & (ST_TAPE|ST_FIFO)) {
      int timeout;
      Dmsg0(29, "open_dev: device is tape\n");
      if (mode == OPEN_READ_WRITE) {
         dev->mode = O_RDWR | O_BINARY;
      } else if (mode == OPEN_READ_ONLY) {
         dev->mode = O_RDONLY | O_BINARY;
      } else if (mode == OPEN_WRITE_ONLY) {
         dev->mode = O_WRONLY | O_BINARY;
      } else {
         Emsg0(M_ABORT, 0, _("Illegal mode given to open_dev.\n")); 
      }
      timeout = dev->max_open_wait;
      errno = 0;
      if (dev->state & ST_FIFO && timeout) {
         /* Set open timer */
         dev->tid = start_thread_timer(pthread_self(), timeout);
      }
      /* If busy retry each second for max_open_wait seconds */
      while ((dev->fd = open(dev->dev_name, dev->mode, MODE_RW)) < 0) {
         berrno be;
         if (errno == EINTR || errno == EAGAIN) {
            continue;
         }
         if (errno == EBUSY && timeout-- > 0) {
            Dmsg2(100, "Device %s busy. ERR=%s\n", dev->dev_name, be.strerror());
            bmicrosleep(1, 0);
            continue;
         }
         dev->dev_errno = errno;
         Mmsg2(&dev->errmsg, _("stored: unable to open device %s: ERR=%s\n"), 
               dev->dev_name, be.strerror());
         /* Stop any open timer we set */
         if (dev->tid) {
            stop_thread_timer(dev->tid);
            dev->tid = 0;
         }
         Emsg0(M_FATAL, 0, dev->errmsg);
         break;
      }
      if (dev->fd >= 0) {
         dev->dev_errno = 0;
         dev->state |= ST_OPENED;
         dev->use_count = 1;
         update_pos_dev(dev);             /* update position */
         set_os_device_parameters(dev);      /* do system dependent stuff */
      }
      /* Stop any open() timer we started */
      if (dev->tid) {
         stop_thread_timer(dev->tid);
         dev->tid = 0;
      }
      Dmsg1(29, "open_dev: tape %d opened\n", dev->fd);
   } else {
      /*
       * Handle opening of File Archive (not a tape)
       */
      if (VolName == NULL || *VolName == 0) {
         Mmsg(dev->errmsg, _("Could not open file device %s. No Volume name given.\n"),
            dev->dev_name);
         return -1;
      }
      archive_name = get_pool_memory(PM_FNAME);
      pm_strcpy(archive_name, dev->dev_name);
      if (archive_name[strlen(archive_name)] != '/') {
         pm_strcat(archive_name, "/");
      }
      pm_strcat(archive_name, VolName);
      Dmsg1(29, "open_dev: device is disk %s\n", archive_name);
      if (mode == OPEN_READ_WRITE) {
         dev->mode = O_CREAT | O_RDWR | O_BINARY;
      } else if (mode == OPEN_READ_ONLY) {
         dev->mode = O_RDONLY | O_BINARY;
      } else if (mode == OPEN_WRITE_ONLY) {
         dev->mode = O_WRONLY | O_BINARY;
      } else {
         Emsg0(M_ABORT, 0, _("Illegal mode given to open_dev.\n")); 
      }
      /* If creating file, give 0640 permissions */
      if ((dev->fd = open(archive_name, dev->mode, 0640)) < 0) {
         berrno be;
         dev->dev_errno = errno;
         Mmsg2(&dev->errmsg, _("Could not open: %s, ERR=%s\n"), archive_name, be.strerror());
         Emsg0(M_FATAL, 0, dev->errmsg);
      } else {
         dev->dev_errno = 0;
         dev->state |= ST_OPENED;
         dev->use_count = 1;
         update_pos_dev(dev);                /* update position */
      }
      Dmsg1(29, "open_dev: disk fd=%d opened\n", dev->fd);
      free_pool_memory(archive_name);
   }
   return dev->fd;
}

#ifdef debug_tracing
#undef rewind_dev
bool _rewind_dev(char *file, int line, DEVICE *dev)
{
   Dmsg2(100, "rewind_dev called from %s:%d\n", file, line);
   return rewind_dev(dev);
}
#endif


/*
 * Rewind the device.
 *  Returns: true  on success
 *           false on failure
 */
bool rewind_dev(DEVICE *dev)
{
   struct mtop mt_com;
   unsigned int i;

   Dmsg1(29, "rewind_dev %s\n", dev->dev_name);
   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg1(&dev->errmsg, _("Bad call to rewind_dev. Device %s not open\n"),
            dev->dev_name);
      Emsg0(M_ABORT, 0, dev->errmsg);
      return false;
   }
   dev->state &= ~(ST_EOT|ST_EOF|ST_WEOT);  /* remove EOF/EOT flags */
   dev->block_num = dev->file = 0;
   dev->file_size = 0;
   dev->file_addr = 0;
   if (dev->state & ST_TAPE) {
      mt_com.mt_op = MTREW;
      mt_com.mt_count = 1;
      /* If we get an I/O error on rewind, it is probably because
       * the drive is actually busy. We loop for (about 5 minutes)
       * retrying every 5 seconds.
       */
      for (i=dev->max_rewind_wait; ; i -= 5) {
         if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
            berrno be;
            clrerror_dev(dev, MTREW);
            if (i == dev->max_rewind_wait) {
               Dmsg1(200, "Rewind error, %s. retrying ...\n", be.strerror());
            }
            if (dev->dev_errno == EIO && i > 0) {
               Dmsg0(200, "Sleeping 5 seconds.\n");
               bmicrosleep(5, 0);
               continue;
            }
            Mmsg2(&dev->errmsg, _("Rewind error on %s. ERR=%s.\n"),
               dev->dev_name, be.strerror());
            return false;
         }
         break;
      }
   } else if (dev->state & ST_FILE) {
      if (lseek(dev->fd, (off_t)0, SEEK_SET) < 0) {
         berrno be;
         dev->dev_errno = errno;
         Mmsg2(&dev->errmsg, _("lseek error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         return false;
      }
   }
   return true;
}

/* 
 * Position device to end of medium (end of data)
 *  Returns: 1 on succes
 *           0 on error
 */
int 
eod_dev(DEVICE *dev)
{
   struct mtop mt_com;
   struct mtget mt_stat;
   int stat = 0;
   off_t pos;

   Dmsg0(29, "eod_dev\n");
   if (dev->state & ST_EOT) {
      return 1;
   }
   dev->state &= ~(ST_EOF);  /* remove EOF flags */
   dev->block_num = dev->file = 0;
   dev->file_size = 0;
   dev->file_addr = 0;
   if (dev->state & (ST_FIFO | ST_PROG)) {
      return 1;
   }
   if (!(dev->state & ST_TAPE)) {
      pos = lseek(dev->fd, (off_t)0, SEEK_END);
//    Dmsg1(100, "====== Seek to %lld\n", pos);
      if (pos >= 0) {
         update_pos_dev(dev);
         dev->state |= ST_EOT;
         return 1;
      }
      dev->dev_errno = errno;
      berrno be;
      Mmsg2(&dev->errmsg, _("lseek error on %s. ERR=%s.\n"),
             dev->dev_name, be.strerror());
      return 0;
   }
#ifdef MTEOM

   if (dev_cap(dev, CAP_FASTFSF) && !dev_cap(dev, CAP_EOM)) {
      struct mtget mt_stat;
      Dmsg0(100,"Using FAST FSF for EOM\n");
      if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) == 0 && mt_stat.mt_fileno <= 0) {
        if (!rewind_dev(dev)) {
          return 0;
        }
      }
      mt_com.mt_op = MTFSF;
      /*
       * ***FIXME*** fix code to handle case that INT16_MAX is
       *   not large enough.
       */
      mt_com.mt_count = INT16_MAX;    /* use big positive number */
      if (mt_com.mt_count < 0) {
         mt_com.mt_count = INT16_MAX; /* brain damaged system */
      }
   }

   if (dev_cap(dev, CAP_EOM)) {
      Dmsg0(100,"Using EOM for EOM\n");
      mt_com.mt_op = MTEOM;
      mt_com.mt_count = 1;
   }
   if (dev_cap(dev, CAP_FASTFSF) || dev_cap(dev, CAP_EOM)) {
      if ((stat=ioctl(dev->fd, MTIOCTOP, (char *)&mt_com)) < 0) {
         berrno be;
         clrerror_dev(dev, mt_com.mt_op);
         Dmsg1(50, "ioctl error: %s\n", be.strerror());
         update_pos_dev(dev);
         Mmsg2(&dev->errmsg, _("ioctl MTEOM error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         return 0;
      }

      if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) < 0) {
         berrno be;
         clrerror_dev(dev, -1);
         Mmsg2(&dev->errmsg, _("ioctl MTIOCGET error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         return 0;
      }
      Dmsg2(100, "EOD file=%d block=%d\n", mt_stat.mt_fileno, mt_stat.mt_blkno);
      dev->file = mt_stat.mt_fileno;

   /*
    * Rewind then use FSF until EOT reached
    */
   } else {
#else
   {
#endif
      if (!rewind_dev(dev)) {
         return 0;
      }
      /* 
       * Move file by file to the end of the tape
       */
      int file_num;
      for (file_num=dev->file; !(dev->state & ST_EOT); file_num++) {
         Dmsg0(200, "eod_dev: doing fsf 1\n");
         if (!fsf_dev(dev, 1)) {
            Dmsg0(200, "fsf_dev error.\n");
            return 0;
         }
         /*
          * Avoid infinite loop. ***FIXME*** possibly add code
          *   to set EOD or to turn off CAP_FASTFSF if on.
          */
         if (file_num == (int)dev->file) {
            struct mtget mt_stat;
            Dmsg1(100, "fsf_dev did not advance from file %d\n", file_num);
#ifndef HAVE_OPENBSD_OS
            if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) == 0 && 
                      mt_stat.mt_fileno >= 0) {
               Dmsg2(100, "Adjust file from %d to %d\n", dev->file , mt_stat.mt_fileno);
               dev->file = mt_stat.mt_fileno;
            }
#endif
            stat = 0;
            break;                    /* we are not progressing, bail out */
         }
      }
   }
   /*
    * Some drivers leave us after second EOF when doing
    * MTEOM, so we must backup so that appending overwrites
    * the second EOF.
    */
   if (dev_cap(dev, CAP_BSFATEOM)) {
      struct mtget mt_stat;
      /* Backup over EOF */
      stat = bsf_dev(dev, 1);
      /* If BSF worked and fileno is known (not -1), set file */
      if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) == 0 && mt_stat.mt_fileno >= 0) {
         Dmsg2(100, "BSFATEOF adjust file from %d to %d\n", dev->file , mt_stat.mt_fileno);
         dev->file = mt_stat.mt_fileno;
      } else {
         dev->file++;                 /* wing it -- not correct on all OSes */
      }
   } else {
      update_pos_dev(dev);                   /* update position */
      stat = 1;
   }
   Dmsg1(200, "EOD dev->file=%d\n", dev->file);
   return stat;
}

/*
 * Set the position of the device -- only for files
 *   For other devices, there is no generic way to do it.
 *  Returns: true  on succes
 *           false on error
 */
bool update_pos_dev(DEVICE *dev)
{
   off_t pos;
   bool ok = true;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(&dev->errmsg, _("Bad device call. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   /* Find out where we are */
   if (dev->state & ST_FILE) {
      dev->file = 0;
      dev->file_addr = 0;
      pos = lseek(dev->fd, (off_t)0, SEEK_CUR);
      if (pos < 0) {
         berrno be;
         dev->dev_errno = errno;
         Pmsg1(000, "Seek error: ERR=%s\n", be.strerror());
         Mmsg2(&dev->errmsg, _("lseek error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         ok = false;
      } else {
         dev->file_addr = pos;
      }
   }
   return ok;
}


/* 
 * Return the status of the device.  This was meant
 * to be a generic routine. Unfortunately, it doesn't
 * seem possible (at least I do not know how to do it
 * currently), which means that for the moment, this
 * routine has very little value.
 *
 *   Returns: status
 */
uint32_t status_dev(DEVICE *dev)
{
   struct mtget mt_stat;
   uint32_t stat = 0;

   if (dev->state & (ST_EOT | ST_WEOT)) {
      stat |= BMT_EOD;
      Dmsg0(-20, " EOD");
   }
   if (dev->state & ST_EOF) {
      stat |= BMT_EOF;
      Dmsg0(-20, " EOF");
   }
   if (dev->state & ST_TAPE) {
      stat |= BMT_TAPE;
      Dmsg0(-20," Bacula status:");
      Dmsg2(-20," file=%d block=%d\n", dev->file, dev->block_num);
      if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) < 0) {
         berrno be;
         dev->dev_errno = errno;
         Mmsg2(&dev->errmsg, _("ioctl MTIOCGET error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         return 0;
      }
      Dmsg0(-20, " Device status:");

#if defined(HAVE_LINUX_OS)
      if (GMT_EOF(mt_stat.mt_gstat)) {
         stat |= BMT_EOF;
         Dmsg0(-20, " EOF");
      }
      if (GMT_BOT(mt_stat.mt_gstat)) {
         stat |= BMT_BOT;
         Dmsg0(-20, " BOT");
      }
      if (GMT_EOT(mt_stat.mt_gstat)) {
         stat |= BMT_EOT;
         Dmsg0(-20, " EOT");
      }
      if (GMT_SM(mt_stat.mt_gstat)) {
         stat |= BMT_SM;
         Dmsg0(-20, " SM");
      }
      if (GMT_EOD(mt_stat.mt_gstat)) {
         stat |= BMT_EOD;
         Dmsg0(-20, " EOD");
      }
      if (GMT_WR_PROT(mt_stat.mt_gstat)) {
         stat |= BMT_WR_PROT;
         Dmsg0(-20, " WR_PROT");
      }
      if (GMT_ONLINE(mt_stat.mt_gstat)) {
         stat |= BMT_ONLINE;
         Dmsg0(-20, " ONLINE");
      }
      if (GMT_DR_OPEN(mt_stat.mt_gstat)) {
         stat |= BMT_DR_OPEN;
         Dmsg0(-20, " DR_OPEN");       
      }
      if (GMT_IM_REP_EN(mt_stat.mt_gstat)) {
         stat |= BMT_IM_REP_EN;
         Dmsg0(-20, " IM_REP_EN");
      }
#endif /* !SunOS && !OSF */
      Dmsg2(-20, " file=%d block=%d\n", mt_stat.mt_fileno, mt_stat.mt_blkno);
   } else {
      stat |= BMT_ONLINE | BMT_BOT;
   }
   return stat;
}


/*
 * Load medium in device
 *  Returns: true  on success
 *           false on failure
 */
bool load_dev(DEVICE *dev)
{
#ifdef MTLOAD
   struct mtop mt_com;
#endif

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(&dev->errmsg, _("Bad call to load_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }
   if (!(dev->state & ST_TAPE)) {
      return true;
   }
#ifndef MTLOAD
   Dmsg0(200, "stored: MTLOAD command not available\n");
   berrno be;
   dev->dev_errno = ENOTTY;           /* function not available */
   Mmsg2(&dev->errmsg, _("ioctl MTLOAD error on %s. ERR=%s.\n"),
         dev->dev_name, be.strerror());
   return false;
#else

   dev->block_num = dev->file = 0;
   dev->file_size = 0;
   dev->file_addr = 0;
   mt_com.mt_op = MTLOAD;
   mt_com.mt_count = 1;
   if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
      berrno be;
      dev->dev_errno = errno;
      Mmsg2(&dev->errmsg, _("ioctl MTLOAD error on %s. ERR=%s.\n"),
         dev->dev_name, be.strerror());
      return false;
   }
   return true;
#endif
}

/*
 * Rewind device and put it offline
 *  Returns: true  on success
 *           false on failure
 */
bool offline_dev(DEVICE *dev)
{
   struct mtop mt_com;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(&dev->errmsg, _("Bad call to offline_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }
   if (!(dev->state & ST_TAPE)) {
      return true;
   }

   dev->state &= ~(ST_APPEND|ST_READ|ST_EOT|ST_EOF|ST_WEOT);  /* remove EOF/EOT flags */
   dev->block_num = dev->file = 0;
   dev->file_size = 0;
   dev->file_addr = 0;
#ifdef MTUNLOCK
   mt_com.mt_op = MTUNLOCK;
   mt_com.mt_count = 1;
   ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
#endif
   mt_com.mt_op = MTOFFL;
   mt_com.mt_count = 1;
   if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
      berrno be;
      dev->dev_errno = errno;
      Mmsg2(&dev->errmsg, _("ioctl MTOFFL error on %s. ERR=%s.\n"),
         dev->dev_name, be.strerror());
      return false;
   }
   Dmsg1(100, "Offlined device %s\n", dev->dev_name);
   return true;
}

bool offline_or_rewind_dev(DEVICE *dev)
{
   if (dev->fd < 0) {
      return false;
   }
   if (dev_cap(dev, CAP_OFFLINEUNMOUNT)) {
      return offline_dev(dev);
   } else {
   /*            
    * Note, this rewind probably should not be here (it wasn't
    *  in prior versions of Bacula), but on FreeBSD, this is
    *  needed in the case the tape was "frozen" due to an error
    *  such as backspacing after writing and EOF. If it is not
    *  done, all future references to the drive get and I/O error.
    */
      clrerror_dev(dev, MTREW);
      return rewind_dev(dev);
   }
}

/* 
 * Foward space a file  
 *   Returns: true  on success
 *            false on failure
 */
bool
fsf_dev(DEVICE *dev, int num)
{ 
   struct mtget mt_stat;
   struct mtop mt_com;
   int stat = 0;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(&dev->errmsg, _("Bad call to fsf_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   if (!(dev->state & ST_TAPE)) {
      return true;
   }
   if (dev->state & ST_EOT) {
      dev->dev_errno = 0;
      Mmsg1(dev->errmsg, _("Device %s at End of Tape.\n"), dev->dev_name);
      return false;
   }
   if (dev->state & ST_EOF) {
      Dmsg0(200, "ST_EOF set on entry to FSF\n");
   }
      
   Dmsg0(100, "fsf_dev\n");
   dev->block_num = 0;
   /*
    * If Fast forward space file is set, then we
    *  use MTFSF to forward space and MTIOCGET
    *  to get the file position. We assume that 
    *  the SCSI driver will ensure that we do not
    *  forward space past the end of the medium.
    */
   if (dev_cap(dev, CAP_FSF) && dev_cap(dev, CAP_FASTFSF)) {
      mt_com.mt_op = MTFSF;
      mt_com.mt_count = num;
      stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
      if (stat < 0 || ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) < 0) {
         berrno be;
         dev->state |= ST_EOT;
         Dmsg0(200, "Set ST_EOT\n");
         clrerror_dev(dev, MTFSF);
         Mmsg2(dev->errmsg, _("ioctl MTFSF error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         Dmsg1(200, "%s", dev->errmsg);
         return false;
      }
      Dmsg2(200, "fsf file=%d block=%d\n", mt_stat.mt_fileno, mt_stat.mt_blkno);
      dev->file = mt_stat.mt_fileno;
      dev->state |= ST_EOF;     /* just read EOF */
      dev->file_addr = 0;
      dev->file_size = 0;
      return true;

   /* 
    * Here if CAP_FSF is set, and virtually all drives
    *  these days support it, we read a record, then forward
    *  space one file. Using this procedure, which is slow,
    *  is the only way we can be sure that we don't read
    *  two consecutive EOF marks, which means End of Data.
    */
   } else if (dev_cap(dev, CAP_FSF)) {
      POOLMEM *rbuf;
      int rbuf_len;
      Dmsg0(200, "FSF has cap_fsf\n");
      if (dev->max_block_size == 0) {
         rbuf_len = DEFAULT_BLOCK_SIZE;
      } else {
         rbuf_len = dev->max_block_size;
      }
      rbuf = get_memory(rbuf_len);
      mt_com.mt_op = MTFSF;
      mt_com.mt_count = 1;
      while (num-- && !(dev->state & ST_EOT)) {
         Dmsg0(100, "Doing read before fsf\n");
         if ((stat = read(dev->fd, (char *)rbuf, rbuf_len)) < 0) {
            if (errno == ENOMEM) {     /* tape record exceeds buf len */
               stat = rbuf_len;        /* This is OK */
            } else {
               berrno be;
               dev->state |= ST_EOT;
               clrerror_dev(dev, -1);
               Dmsg2(100, "Set ST_EOT read errno=%d. ERR=%s\n", dev->dev_errno,
                  be.strerror());
               Mmsg2(dev->errmsg, _("read error on %s. ERR=%s.\n"),
                  dev->dev_name, be.strerror());
               Dmsg1(100, "%s", dev->errmsg);
               break;
            }
         }
         if (stat == 0) {                /* EOF */
            update_pos_dev(dev);
            Dmsg1(100, "End of File mark from read. File=%d\n", dev->file+1);
            /* Two reads of zero means end of tape */
            if (dev->state & ST_EOF) {
               dev->state |= ST_EOT;
               Dmsg0(100, "Set ST_EOT\n");
               break;
            } else {
               dev->state |= ST_EOF;
               dev->file++;
               dev->file_addr = 0;
               dev->file_size = 0;
               continue;
            }
         } else {                        /* Got data */
            dev->state &= ~(ST_EOF|ST_EOT);
         }

         Dmsg0(100, "Doing MTFSF\n");
         stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
         if (stat < 0) {                 /* error => EOT */
            berrno be;
            dev->state |= ST_EOT;
            Dmsg0(100, "Set ST_EOT\n");
            clrerror_dev(dev, MTFSF);
            Mmsg2(&dev->errmsg, _("ioctl MTFSF error on %s. ERR=%s.\n"),
               dev->dev_name, be.strerror());
            Dmsg0(100, "Got < 0 for MTFSF\n");
            Dmsg1(100, "%s", dev->errmsg);
         } else {
            dev->state |= ST_EOF;     /* just read EOF */
            dev->file++;
            dev->file_addr = 0;
            dev->file_size = 0;
         }   
      }
      free_memory(rbuf);
   
   /*
    * No FSF, so use FSR to simulate it
    */
   } else {
      Dmsg0(200, "Doing FSR for FSF\n");
      while (num-- && !(dev->state & ST_EOT)) {
         fsr_dev(dev, INT32_MAX);    /* returns -1 on EOF or EOT */
      }
      if (dev->state & ST_EOT) {
         dev->dev_errno = 0;
         Mmsg1(dev->errmsg, _("Device %s at End of Tape.\n"), dev->dev_name);
         stat = -1;
      } else {
         stat = 0;
      }
   }
   update_pos_dev(dev);
   Dmsg1(200, "Return %d from FSF\n", stat);
   if (dev->state & ST_EOF)
      Dmsg0(200, "ST_EOF set on exit FSF\n");
   if (dev->state & ST_EOT)
      Dmsg0(200, "ST_EOT set on exit FSF\n");
   Dmsg1(200, "Return from FSF file=%d\n", dev->file);
   return stat == 0;
}

/* 
 * Backward space a file  
 *  Returns: false on failure
 *           true  on success
 */
bool
bsf_dev(DEVICE *dev, int num)
{ 
   struct mtop mt_com;
   int stat;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(dev->errmsg, _("Bad call to bsf_dev. Archive device not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   if (!(dev_state(dev, ST_TAPE))) {
      Mmsg1(dev->errmsg, _("Device %s cannot BSF because it is not a tape.\n"),
         dev->dev_name);
      return false;
   }
   Dmsg0(29, "bsf_dev\n");
   dev->state &= ~(ST_EOT|ST_EOF);
   dev->file -= num;
   dev->file_addr = 0;
   dev->file_size = 0;
   mt_com.mt_op = MTBSF;
   mt_com.mt_count = num;
   stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   if (stat < 0) {
      berrno be;
      clrerror_dev(dev, MTBSF);
      Mmsg2(dev->errmsg, _("ioctl MTBSF error on %s. ERR=%s.\n"),
         dev->dev_name, be.strerror());
   }
   update_pos_dev(dev);
   return stat == 0;
}


/* 
 * Foward space a record
 *  Returns: false on failure
 *           true  on success
 */
bool
fsr_dev(DEVICE *dev, int num)
{ 
   struct mtop mt_com;
   int stat;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(dev->errmsg, _("Bad call to fsr_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   if (!(dev_state(dev, ST_TAPE))) {
      return false;
   }
   if (!dev_cap(dev, CAP_FSR)) {
      Mmsg1(dev->errmsg, _("ioctl MTFSR not permitted on %s.\n"), dev->dev_name);
      return false;
   }

   Dmsg1(29, "fsr_dev %d\n", num);
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = num;
   stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   if (stat == 0) {
      dev->state &= ~ST_EOF;
      dev->block_num += num;
   } else {
      berrno be;
      struct mtget mt_stat;
      clrerror_dev(dev, MTFSR);
      Dmsg1(100, "FSF fail: ERR=%s\n", be.strerror());
#ifndef HAVE_OPENBSD_OS
      if (ioctl(dev->fd, MTIOCGET, (char *)&mt_stat) == 0 && mt_stat.mt_fileno >= 0) {
         Dmsg4(100, "Adjust from %d:%d to %d:%d\n", dev->file, 
            dev->block_num, mt_stat.mt_fileno, mt_stat.mt_blkno);
         dev->file = mt_stat.mt_fileno;
         dev->block_num = mt_stat.mt_blkno;
      } else
#endif
      {
         if (dev->state & ST_EOF) {
            dev->state |= ST_EOT;
         } else {
            dev->state |= ST_EOF;           /* assume EOF */
            dev->file++;
            dev->block_num = 0;
            dev->file_addr = 0;
            dev->file_size = 0;
         }
      }
      Mmsg3(dev->errmsg, _("ioctl MTFSR %d error on %s. ERR=%s.\n"),
         num, dev->dev_name, be.strerror());
   }
   update_pos_dev(dev);
   return stat == 0;
}

/* 
 * Backward space a record
 *   Returns:  false on failure
 *             true  on success
 */
bool
bsr_dev(DEVICE *dev, int num)
{ 
   struct mtop mt_com;
   int stat;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(dev->errmsg, _("Bad call to bsr_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   if (!(dev->state & ST_TAPE)) {
      return false;
   }

   if (!dev_cap(dev, CAP_BSR)) {
      Mmsg1(dev->errmsg, _("ioctl MTBSR not permitted on %s.\n"), dev->dev_name);
      return false;
   }

   Dmsg0(29, "bsr_dev\n");
   dev->block_num -= num;
   dev->state &= ~(ST_EOF|ST_EOT|ST_EOF);
   mt_com.mt_op = MTBSR;
   mt_com.mt_count = num;
   stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   if (stat < 0) {
      berrno be;
      clrerror_dev(dev, MTBSR);
      Mmsg2(dev->errmsg, _("ioctl MTBSR error on %s. ERR=%s.\n"),
         dev->dev_name, be.strerror());
   }
   update_pos_dev(dev);
   return stat == 0;
}

/* 
 * Reposition the device to file, block
 * Returns: false on failure
 *          true  on success
 */
bool
reposition_dev(DEVICE *dev, uint32_t file, uint32_t block)
{ 
   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(dev->errmsg, _("Bad call to reposition_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return false;
   }

   if (!(dev_state(dev, ST_TAPE))) {
      off_t pos = (((off_t)file)<<32) + block;
      Dmsg1(100, "===== lseek to %d\n", (int)pos);
      if (lseek(dev->fd, pos, SEEK_SET) == (off_t)-1) {
         berrno be;
         dev->dev_errno = errno;
         Mmsg2(dev->errmsg, _("lseek error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
         return false;
      }
      dev->file = file;
      dev->block_num = block;
      dev->file_addr = pos;
      return true;
   }
   Dmsg4(100, "reposition_dev from %u:%u to %u:%u\n", 
      dev->file, dev->block_num, file, block);
   if (file < dev->file) {
      Dmsg0(100, "Rewind_dev\n");
      if (!rewind_dev(dev)) {
         return false;
      }
   }
   if (file > dev->file) {
      Dmsg1(100, "fsf %d\n", file-dev->file);
      if (!fsf_dev(dev, file-dev->file)) {
         Dmsg1(100, "fsf failed! ERR=%s\n", strerror_dev(dev));
         return false;
      }
      Dmsg2(100, "wanted_file=%d at_file=%d\n", file, dev->file);
   }
   if (block < dev->block_num) {
      Dmsg2(100, "wanted_blk=%d at_blk=%d\n", block, dev->block_num);
      Dmsg0(100, "bsf_dev 1\n");
      bsf_dev(dev, 1);
      Dmsg0(100, "fsf_dev 1\n");
      fsf_dev(dev, 1);
      Dmsg2(100, "wanted_blk=%d at_blk=%d\n", block, dev->block_num);
   }
   if (dev_cap(dev, CAP_POSITIONBLOCKS) && block > dev->block_num) {
      /* Ignore errors as Bacula can read to the correct block */
      Dmsg1(100, "fsr %d\n", block-dev->block_num);
      return fsr_dev(dev, block-dev->block_num);
   }
   return true;
}



/*
 * Write an end of file on the device
 *   Returns: 0 on success
 *            non-zero on failure
 */
int 
weof_dev(DEVICE *dev, int num)
{ 
   struct mtop mt_com;
   int stat;

   if (dev->fd < 0) {
      dev->dev_errno = EBADF;
      Mmsg0(dev->errmsg, _("Bad call to weof_dev. Archive drive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return -1;
   }
   dev->file_size = 0;

   if (!dev->is_tape()) {
      return 0;
   }
   if (!dev->can_append()) {
      Mmsg0(dev->errmsg, _("Attempt to WEOF on non-appendable Volume\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return -1;
   }

   dev->state &= ~(ST_EOT | ST_EOF);  /* remove EOF/EOT flags */
   Dmsg0(29, "weof_dev\n");
   mt_com.mt_op = MTWEOF;
   mt_com.mt_count = num;
   stat = ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   if (stat == 0) {
      dev->block_num = 0;
      dev->file += num;
      dev->file_addr = 0;
   } else {
      berrno be;
      clrerror_dev(dev, MTWEOF);
      if (stat == -1) {
         Mmsg2(dev->errmsg, _("ioctl MTWEOF error on %s. ERR=%s.\n"),
            dev->dev_name, be.strerror());
       }
   }
   return stat;
}

/*
 * Return string message with last error in English
 *  Be careful not to call this routine from within dev.c
 *  while editing an Mmsg() or you will end up in a recursive
 *  loop creating a Segmentation Violation.
 */
char *
strerror_dev(DEVICE *dev)
{
   return dev->errmsg;
}


/*
 * If implemented in system, clear the tape
 * error status.
 */
void
clrerror_dev(DEVICE *dev, int func)
{
   const char *msg = NULL;
   struct mtget mt_stat;

   dev->dev_errno = errno;         /* save errno */
   if (errno == EIO) {
      dev->VolCatInfo.VolCatErrors++;
   }

   if (!(dev->state & ST_TAPE)) {
      return;
   }
   if (errno == ENOTTY || errno == ENOSYS) { /* Function not implemented */
      switch (func) {
      case -1:
         Emsg0(M_ABORT, 0, "Got ENOTTY on read/write!\n");
         break;
      case MTWEOF:
         msg = "WTWEOF";
         dev->capabilities &= ~CAP_EOF; /* turn off feature */
         break;
#ifdef MTEOM
      case MTEOM:
         msg = "WTEOM";
         dev->capabilities &= ~CAP_EOM; /* turn off feature */
         break;
#endif 
      case MTFSF:
         msg = "MTFSF";
         dev->capabilities &= ~CAP_FSF; /* turn off feature */
         break;
      case MTBSF:
         msg = "MTBSF";
         dev->capabilities &= ~CAP_BSF; /* turn off feature */
         break;
      case MTFSR:
         msg = "MTFSR";
         dev->capabilities &= ~CAP_FSR; /* turn off feature */
         break;
      case MTBSR:
         msg = "MTBSR";
         dev->capabilities &= ~CAP_BSR; /* turn off feature */
         break;
      default:
         msg = "Unknown";
         break;
      }
      if (msg != NULL) {
         dev->dev_errno = ENOSYS;
         Mmsg1(&dev->errmsg, _("This device does not support %s.\n"), msg);
         Emsg0(M_ERROR, 0, dev->errmsg);
      }
   }
   /* On some systems such as NetBSD, this clears all errors */
   ioctl(dev->fd, MTIOCGET, (char *)&mt_stat);      

/* Found on Linux */
#ifdef MTIOCLRERR
{
   struct mtop mt_com;
   mt_com.mt_op = MTIOCLRERR;
   mt_com.mt_count = 1;
   /* Clear any error condition on the tape */
   ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   Dmsg0(200, "Did MTIOCLRERR\n");
}
#endif

/* Typically on FreeBSD */
#ifdef MTIOCERRSTAT
{
   /* Read and clear SCSI error status */
   union mterrstat mt_errstat;
   Dmsg2(200, "Doing MTIOCERRSTAT errno=%d ERR=%s\n", dev->dev_errno,
      strerror(dev->dev_errno));
   ioctl(dev->fd, MTIOCERRSTAT, (char *)&mt_errstat);
}
#endif

/* Clear Subsystem Exception OSF1 */
#ifdef MTCSE
{
   struct mtop mt_com;
   mt_com.mt_op = MTCSE;
   mt_com.mt_count = 1;
   /* Clear any error condition on the tape */
   ioctl(dev->fd, MTIOCTOP, (char *)&mt_com);
   Dmsg0(200, "Did MTCSE\n");
}
#endif
}

/*
 * Flush buffer contents
 *  No longer used.
 */
int flush_dev(DEVICE *dev)
{
   return 1;
}

static void do_close(DEVICE *dev)
{

   Dmsg1(29, "really close_dev %s\n", dev->dev_name);
   if (dev->fd >= 0) {
      close(dev->fd);
   }
   /* Clean up device packet so it can be reused */
   dev->fd = -1;
   dev->state &= ~(ST_OPENED|ST_LABEL|ST_READ|ST_APPEND|ST_EOT|ST_WEOT|ST_EOF);
   dev->file = dev->block_num = 0;
   dev->file_size = 0;
   dev->file_addr = 0;
   dev->EndFile = dev->EndBlock = 0;
   memset(&dev->VolCatInfo, 0, sizeof(dev->VolCatInfo));
   memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));
   if (dev->tid) {
      stop_thread_timer(dev->tid);
      dev->tid = 0;
   }
   dev->use_count = 0;
}

/* 
 * Close the device
 */
void
close_dev(DEVICE *dev)
{
   if (!dev) {
      Mmsg0(&dev->errmsg, _("Bad call to close_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return;
   }
   if (dev->fd >= 0 && dev->use_count == 1) {
      do_close(dev);
   } else if (dev->use_count > 0) {
      dev->use_count--;
   }
           
#ifdef FULL_DEBUG
   ASSERT(dev->use_count >= 0);
#endif
}

/*
 * Used when unmounting the device, ignore use_count
 */
void force_close_dev(DEVICE *dev)
{
   if (!dev) {
      Mmsg0(&dev->errmsg, _("Bad call to force_close_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return;
   }
   Dmsg1(29, "Force close_dev %s\n", dev->dev_name);
   do_close(dev);

#ifdef FULL_DEBUG
   ASSERT(dev->use_count >= 0);
#endif
}

bool truncate_dev(DEVICE *dev)
{
   if (dev->state & ST_TAPE) {
      return true;                    /* we don't really truncate tapes */
      /* maybe we should rewind and write and eof ???? */
   }
   if (ftruncate(dev->fd, 0) != 0) {
      berrno be;
      Mmsg1(&dev->errmsg, _("Unable to truncate device. ERR=%s\n"), be.strerror());
      return false;
   }
   return true;
}

bool
dev_is_tape(DEVICE *dev)
{  
   return (dev->state & ST_TAPE) ? true : false;
}


/*
 * return 1 if the device is read for write, and 0 otherwise
 *   This is meant for checking at the end of a job to see
 *   if we still have a tape (perhaps not if at end of tape
 *   and the job is canceled).
 */
bool
dev_can_write(DEVICE *dev)
{
   if ((dev->state & ST_OPENED) &&  (dev->state & ST_APPEND) &&
       (dev->state & ST_LABEL)  && !(dev->state & ST_WEOT)) {
      return true;
   } else {
      return false;
   }
}

char *
dev_name(DEVICE *dev)
{
   return dev->dev_name;
}

char *
dev_vol_name(DEVICE *dev)
{
   return dev->VolCatInfo.VolCatName;
}

uint32_t dev_block(DEVICE *dev)
{
   update_pos_dev(dev);
   return dev->block_num;
}

uint32_t dev_file(DEVICE *dev)
{
   update_pos_dev(dev);
   return dev->file;
}

/* 
 * Free memory allocated for the device
 */
void
term_dev(DEVICE *dev)
{
   if (!dev) {
      dev->dev_errno = EBADF;
      Mmsg0(&dev->errmsg, _("Bad call to term_dev. Archive not open\n"));
      Emsg0(M_FATAL, 0, dev->errmsg);
      return;
   }
   do_close(dev);
   Dmsg0(29, "term_dev\n");
   if (dev->dev_name) {
      free_memory(dev->dev_name);
      dev->dev_name = NULL;
   }
   if (dev->errmsg) {
      free_pool_memory(dev->errmsg);
      dev->errmsg = NULL;
   }
   pthread_mutex_destroy(&dev->mutex);
   pthread_cond_destroy(&dev->wait);
   pthread_cond_destroy(&dev->wait_next_vol);
   pthread_mutex_destroy(&dev->spool_mutex);
   rwl_destroy(&dev->lock);
   if (dev->attached_dcrs) {
      delete dev->attached_dcrs;
      dev->attached_dcrs = NULL;
   }
   if (dev->state & ST_MALLOC) {
      free_pool_memory((POOLMEM *)dev);
   }
}

/*
 * This routine initializes the device wait timers
 */
void init_dev_wait_timers(DEVICE *dev)
{
   /* ******FIXME******* put these on config variables */
   dev->min_wait = 60 * 60;
   dev->max_wait = 24 * 60 * 60;
   dev->max_num_wait = 9;              /* 5 waits =~ 1 day, then 1 day at a time */
   dev->wait_sec = dev->min_wait;
   dev->rem_wait_sec = dev->wait_sec;
   dev->num_wait = 0;
   dev->poll = false;
   dev->BadVolName[0] = 0;
}

/*
 * Returns: true if time doubled
 *          false if max time expired
 */
bool double_dev_wait_time(DEVICE *dev)
{
   dev->wait_sec *= 2;               /* double wait time */
   if (dev->wait_sec > dev->max_wait) {   /* but not longer than maxtime */
      dev->wait_sec = dev->max_wait;
   }
   dev->num_wait++;
   dev->rem_wait_sec = dev->wait_sec;
   if (dev->num_wait >= dev->max_num_wait) {
      return false;
   }
   return true;
}

void set_os_device_parameters(DEVICE *dev)
{
#ifdef HAVE_LINUX_OS
   struct mtop mt_com;
   if (dev->min_block_size == dev->max_block_size &&
       dev->min_block_size == 0) {    /* variable block mode */
      mt_com.mt_op = MTSETBLK;
      mt_com.mt_count = 0;
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTSETBLK);
      }
      mt_com.mt_op = MTSETDRVBUFFER;
      mt_com.mt_count = MT_ST_CLEARBOOLEANS;
      if (!dev_cap(dev, CAP_TWOEOF)) {
         mt_com.mt_count |= MT_ST_TWO_FM;
      }
      if (dev_cap(dev, CAP_EOM)) {
         mt_com.mt_count |= MT_ST_FAST_MTEOM;
      }
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTSETBLK);
      }
   }
   return;
#endif

#ifdef HAVE_NETBSD_OS
   struct mtop mt_com;
   if (dev->min_block_size == dev->max_block_size &&
       dev->min_block_size == 0) {    /* variable block mode */
      mt_com.mt_op = MTSETBSIZ;
      mt_com.mt_count = 0;
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTSETBSIZ);
      }
      /* Get notified at logical end of tape */
      mt_com.mt_op = MTEWARN;
      mt_com.mt_count = 1;
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTEWARN);
      }
   }
   return;
#endif

#if HAVE_FREEBSD_OS || HAVE_OPENBSD_OS
   struct mtop mt_com;
   if (dev->min_block_size == dev->max_block_size &&
       dev->min_block_size == 0) {    /* variable block mode */
      mt_com.mt_op = MTSETBSIZ;
      mt_com.mt_count = 0;
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTSETBSIZ);
      }
   }
   return;
#endif

#ifdef HAVE_SUN_OS
   struct mtop mt_com;
   if (dev->min_block_size == dev->max_block_size &&
       dev->min_block_size == 0) {    /* variable block mode */
      mt_com.mt_op = MTSRSZ;
      mt_com.mt_count = 0;
      if (ioctl(dev->fd, MTIOCTOP, (char *)&mt_com) < 0) {
         clrerror_dev(dev, MTSRSZ);
      }
   }
   return;
#endif
}
