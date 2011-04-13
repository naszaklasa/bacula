/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2009-2010 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

/**
 * This code implements a cache with the current mounted filesystems for which
 * its uses the mostly in kernel mount information and export the different OS
 * specific interfaces using a generic interface. We use a hashed cache which is
 * accessed using a hash on the device id and we keep the previous cache hit as
 * most of the time we get called quite a lot with most of the time the same
 * device so keeping the previous cache hit we have a very optimized code path.
 *
 * This interface is implemented for the following OS-es:
 *
 * - Linux
 * - HPUX
 * - DARWIN (OSX)
 * - IRIX
 * - AIX
 * - OSF1 (True64)
 * - Solaris
 *
 * Currently we only use this code for Linux and OSF1 based fstype determination.
 * For the other OS-es we can use the fstype present in stat structure on those OS-es.
 *
 * This code replaces the big switch we used before based on SUPER_MAGIC present in
 * the statfs(2) structure but which need extra code for each new filesystem added to
 * the OS and for Linux that tends to be often as it has quite some different filesystems.
 * This new implementation should eliminate this as we use the Linux /proc/mounts in kernel
 * data which automatically adds any new filesystem when added to the kernel.
 */

/*
 *  Marco van Wieringen, August 2009
 */

#include "bacula.h"
#include "mntent_cache.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(HAVE_GETMNTENT)
#if defined(HAVE_LINUX_OS) || defined(HAVE_HPUX_OS)
#include <mntent.h>
#elif defined(HAVE_SUN_OS)
#include <sys/mnttab.h>
#endif /* HAVE_GETMNTENT */
#elif defined(HAVE_GETMNTINFO)
#if defined(HAVE_DARWIN_OS)
#include <sys/param.h>
#include <sys/ucred.h> 
#include <sys/mount.h> 
#else
#include <sys/types.h>
#include <sys/statvfs.h>
#endif
#elif defined(HAVE_AIX_OS)
#include <fshelp.h>
#include <sys/vfs.h>
#elif defined(HAVE_OSF1_OS)
#include <sys/mount.h>
#endif

static char cache_initialized = 0;

/**
 * Protected data by mutex lock.
 */
static pthread_mutex_t mntent_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static mntent_cache_entry_t *mntent_cache_entry_hashtable[NR_MNTENT_CACHE_ENTRIES];
static mntent_cache_entry_t *previous_cache_hit = NULL;

/**
 * Simple hash function.
 */
static uint32_t mntent_hash_function(uint32_t dev)
{
   return (dev % NR_MNTENT_CACHE_ENTRIES);
}

/**
 * Add a new entry to the cache.
 * This function should be called with a write lock on the mntent_cache.
 */
static void add_mntent_mapping(uint32_t dev, const char *special, const char *mountpoint,
                               const char *fstype, const char *mntopts)
{
   uint32_t hash;
   mntent_cache_entry_t *mce;

   /**
    * Select the correct hash bucket.
    */
   hash = mntent_hash_function(dev);

   /**
    * See if this is the first being put into the hash bucket.
    */
   if (mntent_cache_entry_hashtable[hash] == (mntent_cache_entry_t *)NULL) {
      mce = (mntent_cache_entry_t *)malloc(sizeof(mntent_cache_entry_t));
      memset((caddr_t)mce, 0, sizeof(mntent_cache_entry_t));
      mntent_cache_entry_hashtable[hash] = mce;
   } else {
      /**
       * Walk the linked list in the hash bucket.
       */
      for (mce = mntent_cache_entry_hashtable[hash]; mce->next != NULL; mce = mce->next) ;
      mce->next = (mntent_cache_entry_t *)malloc(sizeof(mntent_cache_entry_t));
      mce = mce->next;
      memset((caddr_t)mce, 0, sizeof(mntent_cache_entry_t));
   }

   mce->dev = dev;
   mce->special = bstrdup(special);
   mce->mountpoint = bstrdup(mountpoint);
   mce->fstype = bstrdup(fstype);
   if (mntopts) {
      mce->mntopts = bstrdup(mntopts);
   }
}

/**
 * OS specific function to load the different mntents into the cache.
 * This function should be called with a write lock on the mntent_cache.
 */
static void refresh_mount_cache(void)
{
#if defined(HAVE_GETMNTENT)
   FILE *fp;
   struct stat st;
#if defined(HAVE_LINUX_OS) || defined(HAVE_HPUX_OS) || defined(HAVE_IRIX_OS)
   struct mntent *mnt;

#if defined(HAVE_LINUX_OS)
   if ((fp = setmntent("/proc/mounts", "r")) == (FILE *)NULL) {
      if ((fp = setmntent(_PATH_MOUNTED, "r")) == (FILE *)NULL) {
         return;
      }
   }
#elif defined(HAVE_HPUX_OS)
   if ((fp = fopen(MNT_MNTTAB, "r")) == (FILE *)NULL) {
      return;
   }
#elif defined(HAVE_IRIX_OS)
   if ((fp = setmntent(MOUNTED, "r")) == (FILE *)NULL) {
      return;
   }
#endif

   while ((mnt = getmntent(fp)) != (struct mntent *)NULL) {
      if (stat(mnt->mnt_dir, &st) < 0) {
         continue;
      }

      add_mntent_mapping(st.st_dev, mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts);
   }

   endmntent(fp);
#elif defined(HAVE_SUN_OS)
   struct mnttab mnt;

   if ((fp = fopen(MNTTAB, "r")) == (FILE *)NULL)
      return;

   while (getmntent(fp, &mnt) == 0) {
      if (stat(mnt.mnt_mountp, &st) < 0) {
         continue;
      }

      add_mntent_mapping(st.st_dev, mnt.mnt_special, mnt.mnt_mountp, mnt.mnt_fstype, mnt.mnt_mntopts);
   }

   fclose(fp);
#endif /* HAVE_SUN_OS */
#elif defined(HAVE_GETMNTINFO)
   int cnt;
   struct stat st;
#if defined(HAVE_DARWIN_OS)
   struct statfs *mntinfo;
#else
   struct statvfs *mntinfo;
#endif

   if ((cnt = getmntinfo(&mntinfo, MNT_NOWAIT)) > 0) {
      while (cnt > 0) {
         if (stat(mntinfo->f_mntonname, &st) == 0) {
            add_mntent_mapping(st.st_dev,
                               mntinfo->f_mntfromname,
                               mntinfo->f_mntonname,
                               mntinfo->f_fstypename,
                               NULL);
         }
         mntinfo++;
         cnt--;
      }
   }
#elif defined(HAVE_AIX_OS)
   int bufsize;
   char *entries, *current;
   struct vmount *vmp;
   struct stat st;
   struct vfs_ent *ve;
   int n_entries, cnt;

   if (mntctl(MCTL_QUERY, sizeof(bufsize), (struct vmount *)&bufsize) != 0) {
      return;
   }

   entries = malloc(bufsize);
   if ((n_entries = mntctl(MCTL_QUERY, bufsize, (struct vmount *) entries)) < 0) {
      free(entries);
      return;
   }

   cnt = 0;
   current = entries;
   while (cnt < n_entries) {
      vmp = (struct vmount *)current;

      if (stat(current + vmp->vmt_data[VMT_STUB].vmt_off, &st) < 0) {
         continue;
      }

      ve = getvfsbytype(vmp->vmt_gfstype);
      if (ve && ve->vfsent_name) {
         add_mntent_mapping(st.st_dev,
                            current + vmp->vmt_data[VMT_OBJECT].vmt_off,
                            current + vmp->vmt_data[VMT_STUB].vmt_off,
                            ve->vfsent_name,
                            current + vmp->vmt_data[VMT_ARGS].vmt_off);
      }
      current = current + vmp->vmt_length;
      cnt++;
   }
   free(entries);
#elif defined(HAVE_OSF1_OS)
   struct statfs *entries, *current;
   struct stat st;
   int n_entries, cnt;
   int size;

   if ((n_entries = getfsstat((struct statfs *)0, 0L, MNT_NOWAIT)) < 0) {
      return;
   }

   size = (n_entries + 1) * sizeof(struct statfs);
   entries = malloc(size);

   if ((n_entries = getfsstat(entries, size, MNT_NOWAIT)) < 0) {
      free(entries);
      return;
   }

   cnt = 0;
   current = entries;
   while (cnt < n_entries) {
      if (stat(current->f_mntonname, &st) < 0) {
         continue;
      }
      add_mntent_mapping(st.st_dev,
                         current->f_mntfromname,
                         current->f_mntonname,
                         current->f_fstypename,
                         NULL);
      current++;
      cnt++;
   }
   free(stats);
#endif
}

/**
 * Clear the cache (either by flushing it or by initializing it.)
 * This function should be called with a write lock on the mntent_cache.
 */
static void clear_mount_cache()
{
   uint32_t hash;
   mntent_cache_entry_t *mce, *mce_next;

   if (cache_initialized == 0) {
      /**
       * Initialize the hash table.
       */
      memset((caddr_t)mntent_cache_entry_hashtable, 0, NR_MNTENT_CACHE_ENTRIES * sizeof(mntent_cache_entry_t *));
      cache_initialized = 1;
   } else {
      /**
       * Clear the previous_cache_hit.
       */
      previous_cache_hit = NULL;

      /**
       * Walk all hash buckets.
       */
      for (hash = 0; hash < NR_MNTENT_CACHE_ENTRIES; hash++) {
         /**
          * Walk the content of this hash bucket.
          */
         mce = mntent_cache_entry_hashtable[hash];
         mntent_cache_entry_hashtable[hash] = NULL;
         while (mce != NULL) {
            /**
             * Save the pointer to the next entry.
             */
            mce_next = mce->next;

            /**
             * Free the structure.
             */
            if (mce->mntopts)
               free(mce->mntopts);
            free(mce->fstype);
            free(mce->mountpoint);
            free(mce->special);
            free(mce);

            mce = mce_next;
         }
      }
   }
}

/**
 * Initialize the cache for use.
 */
static void initialize_mntent_cache(void)
{
   /**
    * Lock the cache while we update it.
    */
   P(mntent_cache_lock);

   /**
    * Make sure the cache is empty (either by flushing it or by initializing it.)
    */
   clear_mount_cache();

   /**
    * Refresh the cache.
    */
   refresh_mount_cache();

   /**
    * We are done updating the cache.
    */
   V(mntent_cache_lock);
}

void preload_mntent_cache(void)
{
   initialize_mntent_cache();
}

void flush_mntent_cache(void)
{
   /**
    * Lock the cache while we update it.
    */
   P(mntent_cache_lock);

   /**
    * Make sure the cache is empty (either by flushing it or by initializing it.)
    */
   clear_mount_cache();

   /**
    * We are done updating the cache.
    */
   V(mntent_cache_lock);
}

/**
 * Find a mapping in the cache.
 */
mntent_cache_entry_t *find_mntent_mapping(uint32_t dev)
{
   uint32_t hash;
   mntent_cache_entry_t *mce;

   /**
    * Initialize the cache if that was not done before.
    */
   if (cache_initialized == 0) {
      initialize_mntent_cache();
   }

   /**
    * Shortcut when we get a request for the same device again.
    */
   if (previous_cache_hit && previous_cache_hit->dev == dev) {
      return previous_cache_hit;
   }

   /**
    * Lock the cache while we walk it.
    */
   P(mntent_cache_lock);

   /**
    * Select the correct hash bucket.
    */
   hash = mntent_hash_function(dev);

   /**
    * Walk the hash bucket.
    */
   for (mce = mntent_cache_entry_hashtable[hash]; mce != NULL; mce = mce->next) {
      if (mce->dev == dev) {
         previous_cache_hit = mce;
         V(mntent_cache_lock);
         return mce;
      }
   }

   /**
    * We are done walking the cache.
    */
   V(mntent_cache_lock);
   return NULL;
}
