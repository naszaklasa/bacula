/*
 *
 *   dvd.c  -- Routines specific to DVD devices (and
 *             possibly other removable hard media). 
 *
 *    Nicolas Boichat, MMV
 *
 *   Version $Id: dvd.c,v 1.22.2.2 2005/10/28 07:40:42 kerns Exp $
 */
/*
   Copyright (C) 2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "stored.h"

/* Forward referenced functions */
static void edit_device_codes_dev(DEVICE *dev, POOL_MEM &omsg, const char *imsg);
static bool do_mount_dev(DEVICE* dev, int mount, int dotimeout);
static void add_file_and_part_name(DEVICE *dev, POOL_MEM &archive_name);

/* 
 * Write the current volume/part filename to archive_name.
 */
void make_mounted_dvd_filename(DEVICE *dev, POOL_MEM &archive_name) 
{
   pm_strcpy(archive_name, dev->device->mount_point);
   add_file_and_part_name(dev, archive_name);
   dev->set_part_spooled(false);
}

void make_spooled_dvd_filename(DEVICE *dev, POOL_MEM &archive_name)
{
   /* Use the working directory if spool directory is not defined */
   if (dev->device->spool_directory) {
      pm_strcpy(archive_name, dev->device->spool_directory);
   } else {
      pm_strcpy(archive_name, working_directory);
   }
   add_file_and_part_name(dev, archive_name);
   dev->set_part_spooled(true);
}      

static void add_file_and_part_name(DEVICE *dev, POOL_MEM &archive_name)
{
   char partnumber[20];
   if (archive_name.c_str()[strlen(archive_name.c_str())-1] != '/') {
      pm_strcat(archive_name, "/");
   }

   pm_strcat(archive_name, dev->VolCatInfo.VolCatName);
   /* if part > 1, append .# to the filename (where # is the part number) */
   if (dev->part > 0) {
      pm_strcat(archive_name, ".");
      bsnprintf(partnumber, sizeof(partnumber), "%d", dev->part);
      pm_strcat(archive_name, partnumber);
   }
   Dmsg1(100, "Exit make_dvd_filename: arch=%s\n", archive_name.c_str());
}  

/* Mount the device.
 * If timeout, wait until the mount command returns 0.
 * If !timeout, try to mount the device only once.
 */
bool mount_dev(DEVICE* dev, int timeout) 
{
   Dmsg0(90, "Enter mount_dev\n");
   if (dev->is_mounted()) {
      return true;
   } else if (dev->requires_mount()) {
      return do_mount_dev(dev, 1, timeout);
   }       
   return true;
}

/* Unmount the device
 * If timeout, wait until the unmount command returns 0.
 * If !timeout, try to unmount the device only once.
 */
bool unmount_dev(DEVICE *dev, int timeout) 
{
   Dmsg0(90, "Enter unmount_dev\n");
   if (dev->is_mounted()) {
      return do_mount_dev(dev, 0, timeout);
   }
   return true;
}

/* (Un)mount the device */
static bool do_mount_dev(DEVICE* dev, int mount, int dotimeout) 
{
   POOL_MEM ocmd(PM_FNAME);
   POOLMEM *results;
   char *icmd;
   int status, timeout;
   
   sm_check(__FILE__, __LINE__, false);
   if (mount) {
      if (dev->is_mounted()) {
         Dmsg0(200, "======= DVD mount=1\n");
         return true;
      }
      icmd = dev->device->mount_command;
   } else {
      if (!dev->is_mounted()) {
         Dmsg0(200, "======= DVD mount=0\n");
         return true;
      }
      icmd = dev->device->unmount_command;
   }
   
   edit_device_codes_dev(dev, ocmd, icmd);
   
   Dmsg2(200, "do_mount_dev: cmd=%s mounted=%d\n", ocmd.c_str(), !!dev->is_mounted());

   if (dotimeout) {
      /* Try at most 1 time to (un)mount the device. This should perhaps be configurable. */
      timeout = 1;
   } else {
      timeout = 0;
   }
   results = get_memory(2000);
   results[0] = 0;
   /* If busy retry each second */
   while ((status = run_program_full_output(ocmd.c_str(), 
                       dev->max_open_wait/2, results)) != 0) {
      /* Doesn't work with internationalisation (This is not a problem) */
      if (fnmatch("*is already mounted on", results, 0) == 0) {
         break;
      }
      if (timeout-- > 0) {
         /* Sometimes the device cannot be mounted because it is already mounted.
          * Try to unmount it, then remount it */
         if (mount) {
            Dmsg1(400, "Trying to unmount the device %s...\n", dev->print_name());
            do_mount_dev(dev, 0, 0);
         }
         bmicrosleep(1, 0);
         continue;
      }
      Dmsg2(40, "Device %s cannot be mounted. ERR=%s\n", dev->print_name(), results);
      Mmsg(dev->errmsg, _("Device %s cannot be mounted. ERR=%s\n"), 
           dev->print_name(), results);
      /*
       * Now, just to be sure it is not mounted, try to read the
       *  filesystem.
       */
      DIR* dp;
      struct dirent *entry, *result;
      int name_max;
      int count = 0;
      
      name_max = pathconf(".", _PC_NAME_MAX);
      if (name_max < 1024) {
         name_max = 1024;
      }
         
      if (!(dp = opendir(dev->device->mount_point))) {
         berrno be;
         dev->dev_errno = errno;
         Dmsg3(29, "open_mounted_dev: failed to open dir %s (dev=%s), ERR=%s\n", 
               dev->device->mount_point, dev->print_name(), be.strerror());
         goto get_out;
      }
      
      entry = (struct dirent *)malloc(sizeof(struct dirent) + name_max + 1000);
      while (1) {
         if ((readdir_r(dp, entry, &result) != 0) || (result == NULL)) {
            dev->dev_errno = EIO;
            Dmsg2(129, "open_mounted_dev: failed to find suitable file in dir %s (dev=%s)\n", 
                  dev->device->mount_point, dev->print_name());
            break;
         }
         if ((strcmp(result->d_name, ".")) && (strcmp(result->d_name, "..")) && (strcmp(result->d_name, ".keep"))) {
            count++; /* result->d_name != ., .. or .keep (Gentoo-specific) */
         }
         else {
            Dmsg2(129, "open_mounted_dev: ignoring %s in %s\n", 
                  result->d_name, dev->device->mount_point);
         }
      }
      free(entry);
      closedir(dp);
      
      Dmsg1(29, "open_mounted_dev: got %d files in the mount point (not counting ., .. and .keep)\n", count);
      
      if (count > 0) {
         mount = 1;                      /* If we got more than ., .. and .keep */
         break;                          /*   there must be something mounted */
      }
get_out:
      dev->set_mounted(false);
      sm_check(__FILE__, __LINE__, false);
      free_pool_memory(results);
      Dmsg0(200, "============ DVD mount=0\n");
      return false;
   }
   
   dev->set_mounted(mount);              /* set/clear mounted flag */
   free_pool_memory(results);
   /* Do not check free space when unmounting */
   if (mount) {
      update_free_space_dev(dev);
   }
   Dmsg1(200, "============ DVD mount=%d\n", mount);
   return true;
}

/* Update the free space on the device */
void update_free_space_dev(DEVICE* dev) 
{
   POOL_MEM ocmd(PM_FNAME);
   POOLMEM* results;
   char* icmd;
   int timeout;
   long long int free;
   char ed1[50];
   
   /* The device must be mounted in order to dvd-freespace to work */
   mount_dev(dev, 1);
   
   sm_check(__FILE__, __LINE__, false);
   icmd = dev->device->free_space_command;
   
   if (!icmd) {
      dev->free_space = 0;
      dev->free_space_errno = 0;
      dev->clear_freespace_ok();   /* No valid freespace */
      dev->clear_media();
      Dmsg2(29, "update_free_space_dev: free_space=%s, free_space_errno=%d (!icmd)\n", 
            edit_uint64(dev->free_space, ed1), dev->free_space_errno);
      return;
   }
   
   edit_device_codes_dev(dev, ocmd, icmd);
   
   Dmsg1(29, "update_free_space_dev: cmd=%s\n", ocmd.c_str());

   results = get_pool_memory(PM_MESSAGE);
   
   /* Try at most 3 times to get the free space on the device. This should perhaps be configurable. */
   timeout = 3;
   
   while (1) {
      if (run_program_full_output(ocmd.c_str(), dev->max_open_wait/2, results) == 0) {
         Dmsg1(100, "Free space program run : %s\n", results);
         free = str_to_int64(results);
         if (free >= 0) {
            dev->free_space = free;
            dev->free_space_errno = 0;
            dev->set_freespace_ok();     /* have valid freespace */
            dev->set_media();
            Mmsg0(dev->errmsg, "");
            break;
         }
      }
      dev->free_space = 0;
      dev->free_space_errno = EPIPE;
      dev->clear_freespace_ok();         /* no valid freespace */
      Mmsg1(dev->errmsg, _("Cannot run free space command (%s)\n"), results);
      
      if (--timeout > 0) {
         Dmsg4(40, "Cannot get free space on device %s. free_space=%s, "
            "free_space_errno=%d ERR=%s\n", dev->print_name(), 
               edit_uint64(dev->free_space, ed1), dev->free_space_errno, 
               dev->errmsg);
         bmicrosleep(1, 0);
         continue;
      }

      dev->dev_errno = dev->free_space_errno;
      Dmsg4(40, "Cannot get free space on device %s. free_space=%s, "
         "free_space_errno=%d ERR=%s\n",
            dev->print_name(), edit_uint64(dev->free_space, ed1),
            dev->free_space_errno, dev->errmsg);
      break;
   }
   
   free_pool_memory(results);
   Dmsg4(29, "update_free_space_dev: free_space=%s freespace_ok=%d free_space_errno=%d have_media=%d\n", 
      edit_uint64(dev->free_space, ed1), !!dev->is_freespace_ok(), dev->free_space_errno, !!dev->have_media());
   sm_check(__FILE__, __LINE__, false);
   return;
}

/*
 * Write a part (Vol, Vol.1, ...) from the spool to the DVD   
 * This routine does not update the part number, so normally, you
 *  should call open_next_part()
 * It is also called from truncate_dvd_dev to "blank" the medium, as
 *  well as from block.c when the DVD is full to write the last part.
 */
bool dvd_write_part(DCR *dcr) 
{
   DEVICE *dev = dcr->dev;
   POOL_MEM archive_name(PM_FNAME);
   
   /* Don't write empty part files.
    * This is only useful when growisofs does not support write beyond
    * the 4GB boundary.
    * Example :
    *   - 3.9 GB on the volume, dvd-freespace reports 0.4 GB free
    *   - Write 0.2 GB on the volume, Bacula thinks it could still
    *     append data, it creates a new empty part.
    *   - dvd-freespace reports 0 GB free, as the 4GB boundary has
    *     been crossed
    *   - Bacula thinks he must finish to write to the device, so it
    *     tries to write the last part (0-byte), but dvd-writepart fails...
    *
    * There is one exception: when recycling a volume, we write a blank part
    * file, so, then, we need to accept to write it.
    */
   if ((dev->part_size == 0) && (dev->part > 0)) {
      Dmsg2(29, "dvd_write_part: device is %s, won't write blank part %d\n", dev->print_name(), dev->part);
      /* Delete spool file */
      make_spooled_dvd_filename(dev, archive_name);
      unlink(archive_name.c_str());
      Dmsg1(29, "unlink(%s)\n", archive_name.c_str());
      sm_check(__FILE__, __LINE__, false);
      return true;
   }
   
   POOL_MEM ocmd(PM_FNAME);
   POOL_MEM results(PM_MESSAGE);
   char* icmd;
   int status;
   int timeout;
   char ed1[50];
   
   sm_check(__FILE__, __LINE__, false);
   Dmsg3(29, "dvd_write_part: device is %s, part is %d, is_mounted=%d\n", dev->print_name(), dev->part, dev->is_mounted());
   icmd = dev->device->write_part_command;
   
   edit_device_codes_dev(dev, ocmd, icmd);
      
   /*
    * original line follows
    * timeout = dev->max_open_wait + (dev->max_part_size/(1350*1024/2));
    * I modified this for a longer timeout; pre-formatting, blanking and
    * writing can take quite a while
    */

   /* Explanation of the timeout value, when writing the first part,
    *  by Arno Lehmann :
    * 9 GB, write speed 1x: 6990 seconds (almost 2 hours...)
    * Overhead: 900 seconds (starting, initializing, finalizing,probably 
    *   reloading 15 minutes)
    * Sum: 15780.
    * A reasonable last-exit timeout would be 16000 seconds. Quite long - 
    * almost 4.5 hours, but hopefully, that timeout will only ever be needed 
    * in case of a serious emergency.
    */

   if (dev->part == 0)
      timeout = 16000;
   else
      timeout = dev->max_open_wait + (dev->part_size/(1350*1024/4));

   Dmsg2(29, "dvd_write_part: cmd=%s timeout=%d\n", ocmd.c_str(), timeout);
      
   status = run_program_full_output(ocmd.c_str(), timeout, results.c_str());
   if (status != 0) {
      Mmsg1(dev->errmsg, _("Error while writing current part to the DVD: %s"), 
            results.c_str());
      Dmsg1(000, "%s", dev->errmsg);
      dev->dev_errno = EIO;
      mark_volume_in_error(dcr);
      sm_check(__FILE__, __LINE__, false);
      return false;
   } else {
      dev->num_parts++;            /* there is now one more part on DVD */
   }

   /* Delete spool file */
   make_spooled_dvd_filename(dev, archive_name);
   unlink(archive_name.c_str());
   Dmsg1(29, "unlink(%s)\n", archive_name.c_str());
   sm_check(__FILE__, __LINE__, false);
   
   /* growisofs umounted the device, so remount it (it will update the free space) */
   dev->clear_mounted();
   mount_dev(dev, 1);
   Jmsg(dcr->jcr, M_INFO, 0, _("Remaining free space %s on %s\n"), 
      edit_uint64_with_commas(dev->free_space, ed1), dev->print_name());
   sm_check(__FILE__, __LINE__, false);
   return true;
}

/* Open the next part file.
 *  - Close the fd
 *  - Increment part number 
 *  - Reopen the device
 */
int dvd_open_next_part(DCR *dcr)
{
   DEVICE *dev = dcr->dev;

   Dmsg6(29, "Enter: ==== open_next_part part=%d npart=%d dev=%s vol=%s mode=%d file_addr=%d\n", 
      dev->part, dev->num_parts, dev->print_name(),
         dev->VolCatInfo.VolCatName, dev->openmode, dev->file_addr);
   if (!dev->is_dvd()) {
      Dmsg1(000, "Device %s is not dvd!!!!\n", dev->print_name()); 
      return -1;
   }
   
   /* When appending, do not open a new part if the current is empty */
   if (dev->can_append() && (dev->part >= dev->num_parts) && 
       (dev->part_size == 0)) {
      Dmsg0(29, "open_next_part exited immediately (dev->part_size == 0).\n");
      return dev->fd;
   }
   
   if (dev->fd >= 0) {
      close(dev->fd);
   }
   
   dev->fd = -1;
   dev->clear_opened();
   
   /*
    * If we have a part open for write, then write it to
    *  DVD before opening the next part.
    */
   if (dev->is_dvd() && (dev->part >= dev->num_parts) && dev->can_append()) {
      if (!dvd_write_part(dcr)) {
         Dmsg0(29, "Error in dvd_write part.\n");
         return -1;
      }
   }
     
   if (dev->part > dev->num_parts) {
      Dmsg2(000, "In open_next_part: part=%d nump=%d\n", dev->part, dev->num_parts);
      ASSERT(dev->part <= dev->num_parts);
   }
   dev->part_start += dev->part_size;
   dev->part++;
   
   Dmsg2(29, "part=%d num_parts=%d\n", dev->part, dev->num_parts);
   /* I think this dev->can_append() should not be there */
   if ((dev->num_parts < dev->part) && dev->can_append()) {
      POOL_MEM archive_name(PM_FNAME);
      struct stat buf;
      /* 
       * First check what is on DVD.  If our part is there, we
       *   are in trouble, so bail out.
       * NB: This is however not a problem if we are writing the first part.
       * It simply means that we are overriding an existing volume...
       */
      if (dev->num_parts > 0) {
         make_mounted_dvd_filename(dev, archive_name);   /* makes dvd name */
         if (stat(archive_name.c_str(), &buf) == 0) {
            /* bad news bail out */
            Mmsg1(&dev->errmsg, _("Next Volume part already exists on DVD. Cannot continue: %s\n"),
               archive_name.c_str());
            return -1;
         }
      }

      Dmsg2(100, "num_parts=%d part=%d\n", dev->num_parts, dev->part);
      dev->VolCatInfo.VolCatParts = dev->part;
      make_spooled_dvd_filename(dev, archive_name);   /* makes spool name */
      
      /* Check if the next part exists in spool directory . */
      if ((stat(archive_name.c_str(), &buf) == 0) || (errno != ENOENT)) {
         Dmsg1(29, "open_next_part %s is in the way, moving it away...\n", archive_name.c_str());
         /* Then try to unlink it */
         if (unlink(archive_name.c_str()) < 0) {
            berrno be;
            dev->dev_errno = errno;
            Mmsg2(dev->errmsg, _("open_next_part can't unlink existing part %s, ERR=%s\n"), 
                   archive_name.c_str(), be.strerror());
            return -1;
         }
      }
   }
   /* KES.  It seems to me that this if should not be
    *  needed. If num_parts represents what is on the DVD
    *  we should only need to change it when writing a part
    *  to the DVD.
    * NB. As dvd_write_part increments dev->num_parts, I also
    *  think it is not needed.
    */
   if (dev->num_parts < dev->part) {
      Dmsg2(100, "Set npart=%d to part=%d\n", dev->num_parts, dev->part);
      dev->num_parts = dev->part;
      dev->VolCatInfo.VolCatParts = dev->part;
   }
   Dmsg2(50, "Call dev->open(vol=%s, mode=%d\n", dev->VolCatInfo.VolCatName, 
         dev->openmode);
   /* Open next part */
   
   int append = dev->can_append();
   if (dev->open(dcr, dev->openmode) < 0) {
      return -1;
   } 
   dev->set_labeled();          /* all next parts are "labeled" */
   if (append && (dev->part == dev->num_parts)) { /* If needed, set the append flag back */
      dev->set_append();
   }
   
   return dev->fd;
}

/* Open the first part file.
 *  - Close the fd
 *  - Reopen the device
 */
int dvd_open_first_part(DCR *dcr, int mode)
{
   DEVICE *dev = dcr->dev;

   Dmsg5(29, "Enter: ==== open_first_part dev=%s Vol=%s mode=%d num_parts=%d append=%d\n", dev->print_name(), 
         dev->VolCatInfo.VolCatName, dev->openmode, dev->num_parts, dev->can_append());

   if (dev->fd >= 0) {
      close(dev->fd);
   }
   dev->fd = -1;
   dev->clear_opened();
   
   dev->part_start = 0;
   dev->part = 0;
   
   Dmsg2(50, "Call dev->open(vol=%s, mode=%d)\n", dcr->VolCatInfo.VolCatName, 
         mode);
   int append = dev->can_append();
   if (dev->open(dcr, mode) < 0) {
      Dmsg0(50, "open dev() failed\n");
      return -1;
   }
   if (append && (dev->part == dev->num_parts)) { /* If needed, set the append flag back */
      dev->set_append();
   }
   Dmsg2(50, "Leave open_first_part state=%s append=%d\n", dev->is_open()?"open":"not open", dev->can_append());
   
   return dev->fd;
}


/* Protected version of lseek, which opens the right part if necessary */
off_t lseek_dev(DEVICE *dev, off_t offset, int whence)
{
   DCR *dcr;
   off_t pos;
   char ed1[50], ed2[50];
   
   Dmsg3(100, "Enter lseek_dev fd=%d part=%d nparts=%d\n", dev->fd,
      dev->part, dev->num_parts);
   if (!dev->is_dvd()) { 
      Dmsg0(100, "Using sys lseek\n");
      return lseek(dev->fd, offset, whence);
   }
      
   dcr = (DCR *)dev->attached_dcrs->first();  /* any dcr will do */
   switch(whence) {
   case SEEK_SET:
      Dmsg2(100, "lseek_dev SEEK_SET to %s (part_start=%s)\n",
         edit_uint64(offset, ed1), edit_uint64(dev->part_start, ed2));
      if ((uint64_t)offset >= dev->part_start) {
         if (((uint64_t)offset == dev->part_start) || ((uint64_t)offset < (dev->part_start+dev->part_size))) {
            /* We are staying in the current part, just seek */
            if ((pos = lseek(dev->fd, offset-dev->part_start, SEEK_SET)) < 0) {
               return pos;
            } else {
               return pos + dev->part_start;
            }
         } else {
            /* Load next part, and start again */
            if (dvd_open_next_part(dcr) < 0) {
               Dmsg0(100, "lseek_dev failed while trying to open the next part\n");
               return -1;
            }
            return lseek_dev(dev, offset, SEEK_SET);
         }
      } else {
         /*
          * pos < dev->part_start :
          * We need to access a previous part, 
          * so just load the first one, and seek again
          * until the right one is loaded
          */
         if (dvd_open_first_part(dcr, dev->openmode) < 0) {
            Dmsg0(100, "lseek_dev failed while trying to open the first part\n");
            return -1;
         }
         return lseek_dev(dev, offset, SEEK_SET);
      }
      break;
   case SEEK_CUR:
      Dmsg1(100, "lseek_dev SEEK_CUR to %s\n", edit_uint64(offset, ed1));
      if ((pos = lseek(dev->fd, (off_t)0, SEEK_CUR)) < 0) {
         return pos;   
      }
      pos += dev->part_start;
      if (offset == 0) {
         Dmsg1(100, "lseek_dev SEEK_CUR returns %s\n", edit_uint64(pos, ed1));
         return pos;
      } else { /* Not used in Bacula, but should work */
         return lseek_dev(dev, pos, SEEK_SET);
      }
      break;
   case SEEK_END:
      Dmsg1(100, "lseek_dev SEEK_END to %s\n", edit_uint64(offset, ed1));
      /*
       * Bacula does not use offsets for SEEK_END
       *  Also, Bacula uses seek_end only when it wants to
       *  append to the volume, so for a dvd that means
       *  that the volume must be spooled since the DVD
       *  itself is read-only (as currently implemented).
       */
      if (offset > 0) { /* Not used by bacula */
         Dmsg1(100, "lseek_dev SEEK_END called with an invalid offset %s\n", 
            edit_uint64(offset, ed1));
         errno = EINVAL;
         return -1;
      }
      /* If we are already on a spooled part and have the
       *  right part number, simply seek
       */
      if (dev->is_part_spooled() && dev->part == dev->num_parts) {
         if ((pos = lseek(dev->fd, (off_t)0, SEEK_END)) < 0) {
            return pos;   
         } else {
            Dmsg1(100, "lseek_dev SEEK_END returns %s\n", 
                  edit_uint64(pos + dev->part_start, ed1));
            return pos + dev->part_start;
         }
      } else {
         /*
          * Load the first part, then load the next until we reach the last one.
          * This is the only way to be sure we compute the right file address.
          *
          * Save previous openmode, and open all but last part read-only 
          * (useful for DVDs) 
          */
         int modesave = dev->openmode;
         /* Works because num_parts > 0. */
         if (dvd_open_first_part(dcr, OPEN_READ_ONLY) < 0) {
            Dmsg0(100, "lseek_dev failed while trying to open the first part\n");
            return -1;
         }
         if (dev->num_parts > 0) {
            while (dev->part < (dev->num_parts-1)) {
               if (dvd_open_next_part(dcr) < 0) {
                  Dmsg0(100, "lseek_dev failed while trying to open the next part\n");
                  return -1;
               }
            }
            dev->openmode = modesave;
            if (dvd_open_next_part(dcr) < 0) {
               Dmsg0(100, "lseek_dev failed while trying to open the next part\n");
               return -1;
            }
         }
         return lseek_dev(dev, 0, SEEK_END);
      }
      break;
   default:
      errno = EINVAL;
      return -1;
   }
}

bool dvd_close_job(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   bool ok = true;

   /* If the device is a dvd and WritePartAfterJob
    * is set to yes, open the next part, so, in case of a device
    * that requires mount, it will be written to the device.
    */
   if (dev->is_dvd() && jcr->write_part_after_job && (dev->part_size > 0)) {
      Dmsg1(100, "Writing last part=%d write_partafter_job is set.\n",
         dev->part);
      if (dev->part < dev->num_parts) {
         Jmsg3(jcr, M_FATAL, 0, _("Error while writing, current part number is less than the total number of parts (%d/%d, device=%s)\n"),
               dev->part, dev->num_parts, dev->print_name());
         dev->dev_errno = EIO;
         ok = false;
      }
      
      /* This should be !dvd_write_part(dcr)
         NB: No! If you call dvd_write_part, the part number is not updated.
         You must open the next part, it will automatically write the part and
         update the part number. */
      if (ok && (dvd_open_next_part(dcr) < 0)) {
         Jmsg2(jcr, M_FATAL, 0, _("Unable to write part %s: ERR=%s\n"),
               dev->print_name(), strerror_dev(dev));
         dev->dev_errno = EIO;
         ok = false;
      }
   }
   Dmsg1(200, "Set VolCatParts=%d\n", dev->num_parts);
   dev->VolCatInfo.VolCatParts = dev->num_parts;
   return ok;
}

bool truncate_dvd_dev(DCR *dcr) {
   DEVICE* dev = dcr->dev;

   /* Set num_parts to zero (on disk) */
   dev->num_parts = 0;
   dcr->VolCatInfo.VolCatParts = 0;
   dev->VolCatInfo.VolCatParts = 0;
   
   Dmsg0(100, "truncate_dvd_dev: Opening first part (1)...\n");
   
   dev->truncating = true;
   if (dvd_open_first_part(dcr, OPEN_READ_WRITE) < 0) {
      Dmsg0(100, "truncate_dvd_dev: Error while opening first part (1).\n");
      dev->truncating = false;
      return false;
   }
   dev->truncating = false;

   Dmsg0(100, "truncate_dvd_dev: Truncating...\n");

   /* If necessary, truncate it. */
   if (ftruncate(dev->fd, 0) != 0) {
      berrno be;
      Mmsg2(dev->errmsg, _("Unable to truncate device %s. ERR=%s\n"), 
         dev->print_name(), be.strerror());
      return false;
   }
   
   close(dev->fd);
   dev->fd = -1;
   dev->clear_opened();
   
   Dmsg0(100, "truncate_dvd_dev: Opening first part (2)...\n");
   
   if (!dvd_write_part(dcr)) {
      Dmsg0(100, "truncate_dvd_dev: Error while writing to DVD.\n");
      return false;
   }
   
   /* Set num_parts to zero (on disk) */
   dev->num_parts = 0;
   dcr->VolCatInfo.VolCatParts = 0;
   dev->VolCatInfo.VolCatParts = 0;
   
   if (dvd_open_first_part(dcr, OPEN_READ_WRITE) < 0) {
      Dmsg0(100, "truncate_dvd_dev: Error while opening first part (2).\n");
      return false;
   }

   return true;
}

/* Checks if we can write on a non-blank DVD: meaning that it just have been
 * truncated (there is only one zero-sized file on the DVD, with the right
 * volume name). */
bool check_can_write_on_non_blank_dvd(DCR *dcr) {
   DEVICE* dev = dcr->dev;
   DIR* dp;
   struct dirent *entry, *result;
   int name_max;
   int count = 0;
   int matched = 0; /* We found an empty file with the right name. */
   struct stat filestat;
      
   name_max = pathconf(".", _PC_NAME_MAX);
   if (name_max < 1024) {
      name_max = 1024;
   }
   
   if (!(dp = opendir(dev->device->mount_point))) {
      berrno be;
      dev->dev_errno = errno;
      Dmsg3(29, "check_can_write_on_non_blank_dvd: failed to open dir %s (dev=%s), ERR=%s\n", 
            dev->device->mount_point, dev->print_name(), be.strerror());
      return false;
   }
   
   entry = (struct dirent *)malloc(sizeof(struct dirent) + name_max + 1000);
   while (1) {
      if ((readdir_r(dp, entry, &result) != 0) || (result == NULL)) {
         dev->dev_errno = EIO;
         Dmsg2(129, "check_can_write_on_non_blank_dvd: failed to find suitable file in dir %s (dev=%s)\n", 
               dev->device->mount_point, dev->print_name());
         break;
      }
      else {
         Dmsg2(99, "check_can_write_on_non_blank_dvd: found %s (versus %s)\n", 
               result->d_name, dev->VolCatInfo.VolCatName);
         if (strcmp(result->d_name, dev->VolCatInfo.VolCatName) == 0) {
            /* Found the file, checking it is empty */
            POOL_MEM filename(PM_FNAME);
            pm_strcpy(filename, dev->device->mount_point);
            if (filename.c_str()[strlen(filename.c_str())-1] != '/') {
               pm_strcat(filename, "/");
            }
            pm_strcat(filename, dev->VolCatInfo.VolCatName);
            if (stat(filename.c_str(), &filestat) < 0) {
               berrno be;
               dev->dev_errno = errno;
               Dmsg2(29, "check_can_write_on_non_blank_dvd: cannot stat file (file=%s), ERR=%s\n", 
                  filename.c_str(), be.strerror());
               return false;
            }
            Dmsg2(99, "check_can_write_on_non_blank_dvd: size of %s is %d\n", 
               filename.c_str(), filestat.st_size);
            matched = (filestat.st_size == 0);
         }
      }
      count++;
   }
   free(entry);
   closedir(dp);
   
   Dmsg2(29, "check_can_write_on_non_blank_dvd: got %d files in the mount point (matched=%d)\n", count, matched);
   
   if (count != 3) {
      /* There is more than 3 files (., .., and the volume file) */
      return false;
   }
   
   return matched;
}

/*
 * Edit codes into (Un)MountCommand, Write(First)PartCommand
 *  %% = %
 *  %a = archive device name
 *  %e = erase (set if cannot mount and first part)
 *  %n = part number
 *  %m = mount point
 *  %v = last part name
 *
 *  omsg = edited output message
 *  imsg = input string containing edit codes (%x)
 *
 */
static void edit_device_codes_dev(DEVICE* dev, POOL_MEM &omsg, const char *imsg)
{
   const char *p;
   const char *str;
   char add[20];
   
   POOL_MEM archive_name(PM_FNAME);

   omsg.c_str()[0] = 0;
   Dmsg1(800, "edit_device_codes: %s\n", imsg);
   for (p=imsg; *p; p++) {
      if (*p == '%') {
         switch (*++p) {
         case '%':
            str = "%";
            break;
         case 'a':
            str = dev->dev_name;
            break;
         case 'e':
            if (dev->num_parts == 0) {
               str = "1";
            } else {
               str = "0";
            }
            break;
         case 'n':
            bsnprintf(add, sizeof(add), "%d", dev->part);
            str = add;
            break;
         case 'm':
            str = dev->device->mount_point;
            break;
         case 'v':
            make_spooled_dvd_filename(dev, archive_name);
            str = archive_name.c_str();
            break;
         default:
            add[0] = '%';
            add[1] = *p;
            add[2] = 0;
            str = add;
            break;
         }
      } else {
         add[0] = *p;
         add[1] = 0;
         str = add;
      }
      Dmsg1(1900, "add_str %s\n", str);
      pm_strcat(omsg, (char *)str);
      Dmsg1(1800, "omsg=%s\n", omsg.c_str());
   }
}
