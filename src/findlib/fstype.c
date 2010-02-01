/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2007 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *  Implement routines to determine file system types.
 *
 *   Written by Preben 'Peppe' Guldberg, December MMIV
 *
 *   Version $Id$
 */


#ifndef TEST_PROGRAM

#include "bacula.h"
#include "find.h"

#else /* Set up for testing a stand alone program */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SUPPORTEDOSES \
   "HAVE_DARWIN_OS\n" \
   "HAVE_FREEBSD_OS\n" \
   "HAVE_HPUX_OS\n" \
   "HAVE_IRIX_OS\n" \
   "HAVE_LINUX_OS\n" \
   "HAVE_NETBSD_OS\n" \
   "HAVE_OPENBSD_OS\n" \
   "HAVE_SUN_OS\n" \
   "HAVE_WIN32\n"
#define false              0
#define true               1
#define bstrncpy           strncpy
#define Dmsg0(n,s)         fprintf(stderr, s)
#define Dmsg1(n,s,a1)      fprintf(stderr, s, a1)
#define Dmsg2(n,s,a1,a2)   fprintf(stderr, s, a1, a2)
#endif

/*
 * These functions should be implemented for each OS
 *
 *       bool fstype(const char *fname, char *fs, int fslen);
 */
#if defined(HAVE_DARWIN_OS) \
   || defined(HAVE_FREEBSD_OS ) \
   || defined(HAVE_OPENBSD_OS)

#include <sys/param.h>
#include <sys/mount.h>

bool fstype(const char *fname, char *fs, int fslen)
{
   struct statfs st;
   if (statfs(fname, &st) == 0) {
      bstrncpy(fs, st.f_fstypename, fslen);
      return true;
   }
   Dmsg1(50, "statfs() failed for \"%s\"\n", fname);
   return false;
}
#elif defined(HAVE_NETBSD_OS)
#include <sys/param.h>
#include <sys/mount.h>
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#else
#define statvfs statfs
#endif

bool fstype(const char *fname, char *fs, int fslen)
{
   struct statvfs st;
   if (statvfs(fname, &st) == 0) {
      bstrncpy(fs, st.f_fstypename, fslen);
      return true;
   }
   Dmsg1(50, "statfs() failed for \"%s\"\n", fname);
   return false;
}
#elif defined(HAVE_HPUX_OS) \
   || defined(HAVE_IRIX_OS)

#include <sys/types.h>
#include <sys/statvfs.h>

bool fstype(const char *fname, char *fs, int fslen)
{
   struct statvfs st;
   if (statvfs(fname, &st) == 0) {
      bstrncpy(fs, st.f_basetype, fslen);
      return true;
   }
   Dmsg1(50, "statfs() failed for \"%s\"\n", fname);
   return false;
}

#elif defined(HAVE_LINUX_OS)

#include <sys/vfs.h>

bool fstype(const char *fname, char *fs, int fslen)
{
   struct statfs st;
   if (statfs(fname, &st) == 0) {
      /*
       * Values nicked from statfs(2), testing and
       *
       *    $ grep -r SUPER_MAGIC /usr/include/linux
       *
       * Entries are sorted on ("fsname")
       */
      switch (st.f_type) {

      /* Known good values */
      case 0xef53:         bstrncpy(fs, "ext2", fslen); return true;          /* EXT2_SUPER_MAGIC */
   /* case 0xef53:         ext2 and ext3 are the same */                      /* EXT3_SUPER_MAGIC */
      case 0x3153464a:     bstrncpy(fs, "jfs", fslen); return true;           /* JFS_SUPER_MAGIC */
      case 0x5346544e:     bstrncpy(fs, "ntfs", fslen); return true;          /* NTFS_SB_MAGIC */
      case 0x9fa0:         bstrncpy(fs, "proc", fslen); return true;          /* PROC_SUPER_MAGIC */
      case 0x52654973:     bstrncpy(fs, "reiserfs", fslen); return true;      /* REISERFS_SUPER_MAGIC */
      case 0x58465342:     bstrncpy(fs, "xfs", fslen); return true;           /* XFS_SB_MAGIC */
      case 0x9fa2:         bstrncpy(fs, "usbdevfs", fslen); return true;      /* USBDEVICE_SUPER_MAGIC */
      case 0x62656572:     bstrncpy(fs, "sysfs", fslen); return true;         /* SYSFS_MAGIC */
      case 0x517B:         bstrncpy(fs, "smbfs", fslen); return true;         /* SMB_SUPER_MAGIC */
      case 0x9660:         bstrncpy(fs, "iso9660", fslen); return true;       /* ISOFS_SUPER_MAGIC */

#if 0       /* These need confirmation */
      case 0xadf5:         bstrncpy(fs, "adfs", fslen); return true;          /* ADFS_SUPER_MAGIC */
      case 0xadff:         bstrncpy(fs, "affs", fslen); return true;          /* AFFS_SUPER_MAGIC */
      case 0x6B414653:     bstrncpy(fs, "afs", fslen); return true;           /* AFS_FS_MAGIC */
      case 0x0187:         bstrncpy(fs, "autofs", fslen); return true;        /* AUTOFS_SUPER_MAGIC */
      case 0x62646576:     bstrncpy(fs, "bdev", fslen); return true;          /* ??? */
      case 0x42465331:     bstrncpy(fs, "befs", fslen); return true;          /* BEFS_SUPER_MAGIC */
      case 0x1BADFACE:     bstrncpy(fs, "bfs", fslen); return true;           /* BFS_MAGIC */
      case 0x42494e4d:     bstrncpy(fs, "binfmt_misc", fslen); return true;   /* ??? */
      case (('C'<<8)|'N'): bstrncpy(fs, "capifs", fslen); return true;        /* CAPIFS_SUPER_MAGIC */
      case 0xFF534D42:     bstrncpy(fs, "cifs", fslen); return true;          /* CIFS_MAGIC_NUMBER */
      case 0x73757245:     bstrncpy(fs, "coda", fslen); return true;          /* CODA_SUPER_MAGIC */
      case 0x012ff7b7:     bstrncpy(fs, "coherent", fslen); return true;      /* COH_SUPER_MAGIC */
      case 0x28cd3d45:     bstrncpy(fs, "cramfs", fslen); return true;        /* CRAMFS_MAGIC */
      case 0x1373:         bstrncpy(fs, "devfs", fslen); return true;         /* DEVFS_SUPER_MAGIC */
      case 0x1cd1:         bstrncpy(fs, "devpts", fslen); return true;        /* ??? */
      case 0x414A53:       bstrncpy(fs, "efs", fslen); return true;           /* EFS_SUPER_MAGIC */
      case 0x03111965:     bstrncpy(fs, "eventpollfs", fslen); return true;   /* EVENTPOLLFS_MAGIC */
      case 0x137d:         bstrncpy(fs, "ext", fslen); return true;           /* EXT_SUPER_MAGIC */
      case 0xef51:         bstrncpy(fs, "ext2", fslen); return true;          /* EXT2_OLD_SUPER_MAGIC */
      case 0xBAD1DEA:      bstrncpy(fs, "futexfs", fslen); return true;       /* ??? */
      case 0xaee71ee7:     bstrncpy(fs, "gadgetfs", fslen); return true;      /* GADGETFS_MAGIC */
      case 0x00c0ffee:     bstrncpy(fs, "hostfs", fslen); return true;        /* HOSTFS_SUPER_MAGIC */
      case 0xf995e849:     bstrncpy(fs, "hpfs", fslen); return true;          /* HPFS_SUPER_MAGIC */
      case 0xb00000ee:     bstrncpy(fs, "hppfs", fslen); return true;         /* HPPFS_SUPER_MAGIC */
      case 0x958458f6:     bstrncpy(fs, "hugetlbfs", fslen); return true;     /* HUGETLBFS_MAGIC */
      case 0x12061983:     bstrncpy(fs, "hwgfs", fslen); return true;         /* HWGFS_MAGIC */
      case 0x66726f67:     bstrncpy(fs, "ibmasmfs", fslen); return true;      /* IBMASMFS_MAGIC */
      case 0x9660:         bstrncpy(fs, "isofs", fslen); return true;         /* ISOFS_SUPER_MAGIC */
      case 0x07c0:         bstrncpy(fs, "jffs", fslen); return true;          /* JFFS_MAGIC_SB_BITMASK */
      case 0x72b6:         bstrncpy(fs, "jffs2", fslen); return true;         /* JFFS2_SUPER_MAGIC */
      case 0x2468:         bstrncpy(fs, "minix", fslen); return true;         /* MINIX2_SUPER_MAGIC */
      case 0x2478:         bstrncpy(fs, "minix", fslen); return true;         /* MINIX2_SUPER_MAGIC2 */
      case 0x137f:         bstrncpy(fs, "minix", fslen); return true;         /* MINIX_SUPER_MAGIC */
      case 0x138f:         bstrncpy(fs, "minix", fslen); return true;         /* MINIX_SUPER_MAGIC2 */
      case 0x19800202:     bstrncpy(fs, "mqueue", fslen); return true;        /* MQUEUE_MAGIC */
      case 0x4d44:         bstrncpy(fs, "msdos", fslen); return true;         /* MSDOS_SUPER_MAGIC */
      case 0x564c:         bstrncpy(fs, "ncpfs", fslen); return true;         /* NCP_SUPER_MAGIC */
      case 0x6969:         bstrncpy(fs, "nfs", fslen); return true;           /* NFS_SUPER_MAGIC */
      case 0x9fa1:         bstrncpy(fs, "openpromfs", fslen); return true;    /* OPENPROM_SUPER_MAGIC */
      case 0x6f70726f:     bstrncpy(fs, "oprofilefs", fslen); return true;    /* OPROFILEFS_MAGIC */
      case 0xa0b4d889:     bstrncpy(fs, "pfmfs", fslen); return true;         /* PFMFS_MAGIC */
      case 0x50495045:     bstrncpy(fs, "pipfs", fslen); return true;         /* PIPEFS_MAGIC */
      case 0x002f:         bstrncpy(fs, "qnx4", fslen); return true;          /* QNX4_SUPER_MAGIC */
      case 0x858458f6:     bstrncpy(fs, "ramfs", fslen); return true;         /* RAMFS_MAGIC */
      case 0x7275:         bstrncpy(fs, "romfs", fslen); return true;         /* ROMFS_MAGIC */
      case 0x858458f6:     bstrncpy(fs, "rootfs", fslen); return true;        /* RAMFS_MAGIC */
      case 0x67596969:     bstrncpy(fs, "rpc_pipefs", fslen); return true;    /* RPCAUTH_GSSMAGIC */
      case 0x534F434B:     bstrncpy(fs, "sockfs", fslen); return true;        /* SOCKFS_MAGIC */
      case 0x012ff7b6:     bstrncpy(fs, "sysv2", fslen); return true;         /* SYSV2_SUPER_MAGIC */
      case 0x012ff7b5:     bstrncpy(fs, "sysv4", fslen); return true;         /* SYSV4_SUPER_MAGIC */
      case 0x858458f6:     bstrncpy(fs, "tmpfs", fslen); return true;         /* RAMFS_MAGIC */
      case 0x01021994:     bstrncpy(fs, "tmpfs", fslen); return true;         /* TMPFS_MAGIC */
      case 0x15013346:     bstrncpy(fs, "udf", fslen); return true;           /* UDF_SUPER_MAGIC */
      case 0x00011954:     bstrncpy(fs, "ufs", fslen); return true;           /* UFS_MAGIC */
      case 0xa501FCF5:     bstrncpy(fs, "vxfs", fslen); return true;          /* VXFS_SUPER_MAGIC */
      case 0x012ff7b4:     bstrncpy(fs, "xenix", fslen); return true;         /* XENIX_SUPER_MAGIC */
      case 0x012fd16d:     bstrncpy(fs, "xiafs", fslen); return true;         /* _XIAFS_SUPER_MAGIC */
#endif

      default:
         Dmsg2(10, "Unknown file system type \"0x%x\" for \"%s\".\n", st.f_type,
               fname);
         return false;
      }
   }
   Dmsg1(50, "statfs() failed for \"%s\"\n", fname);
   return false;
}

#elif defined(HAVE_SUN_OS)

#include <sys/types.h>
#include <sys/stat.h>

bool fstype(const char *fname, char *fs, int fslen)
{
   struct stat st;
   if (lstat(fname, &st) == 0) {
      bstrncpy(fs, st.st_fstype, fslen);
      return true;
   }
   Dmsg1(50, "lstat() failed for \"%s\"\n", fname);
   return false;
}

#elif defined (__digital__) && defined (__unix__)  /* Tru64 */
/* Tru64 */
#include <sys/stat.h>
#include <sys/mount.h>

bool fstype(const char *fname, char *fs, int fslen)
{
   struct statfs st;
   if (statfs((char *)fname, &st) == 0) {
      switch (st.f_type) {
      /* Known good values */
      case 0xa:         bstrncpy(fs, "advfs", fslen); return true;        /* Tru64 AdvFS */
      case 0xe:         bstrncpy(fs, "nfs", fslen); return true;          /* Tru64 NFS   */
      default:
         Dmsg2(10, "Unknown file system type \"0x%x\" for \"%s\".\n", st.f_type,
               fname);
         return false;
      }
   }
   Dmsg1(50, "statfs() failed for \"%s\"\n", fname);
   return false;
}
/* Tru64 */

#elif defined (HAVE_WIN32)
/* Windows */

bool fstype(const char *fname, char *fs, int fslen)
{
   DWORD componentlength;
   DWORD fsflags;
   CHAR rootpath[4];
   UINT oldmode;
   BOOL result;

   /* Copy Drive Letter, colon, and backslash to rootpath */
   bstrncpy(rootpath, fname, sizeof(rootpath));

   /* We don't want any popups if there isn't any media in the drive */
   oldmode = SetErrorMode(SEM_FAILCRITICALERRORS);

   result = GetVolumeInformation(rootpath, NULL, 0, NULL, &componentlength, &fsflags, fs, fslen);

   SetErrorMode(oldmode);

   if (result) {
      /* Windows returns NTFS, FAT, etc.  Make it lowercase to be consistent with other OSes */
      lcase(fs);
   } else {
      Dmsg2(10, "GetVolumeInformation() failed for \"%s\", Error = %d.\n", rootpath, GetLastError());
   }

   return result != 0;
}
/* Windows */

#else    /* No recognised OS */

bool fstype(const char *fname, char *fs, int fslen)
{
   Dmsg0(10, "!!! fstype() not implemented for this OS. !!!\n");
#ifdef TEST_PROGRAM
   Dmsg1(10, "Please define one of the following when compiling:\n\n%s\n",
         SUPPORTEDOSES);
   exit(EXIT_FAILURE);
#endif

   return false;
}
#endif

#ifdef TEST_PROGRAM
int main(int argc, char **argv)
{
   char *p;
   char fs[1000];
   int status = 0;

   if (argc < 2) {
      p = (argc < 1) ? "fstype" : argv[0];
      printf("usage:\t%s path ...\n"
            "\t%s prints the file system type and pathname of the paths.\n",
            p, p);
      return EXIT_FAILURE;
   }
   while (*++argv) {
      if (!fstype(*argv, fs, sizeof(fs))) {
         status = EXIT_FAILURE;
      } else {
         printf("%s\t%s\n", fs, *argv);
      }
   }
   return status;
}
#endif
