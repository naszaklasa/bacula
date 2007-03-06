/*
 *  Create a file, and reset the modes
 *
 *    Kern Sibbald, November MM
 *
 *   Version $Id: create_file.c,v 1.46.2.1 2006/03/04 11:10:18 kerns Exp $
 *
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

#include "bacula.h"
#include "find.h"

#ifndef S_IRWXUGO
#define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#ifndef IS_CTG
#define IS_CTG(x) 0
#define O_CTG 0
#endif

static int separate_path_and_file(JCR *jcr, char *fname, char *ofile);
static int path_already_seen(JCR *jcr, char *path, int pnl);


/*
 * Create the file, or the directory
 *
 *  fname is the original filename
 *  ofile is the output filename (may be in a different directory)
 *
 * Returns:  CF_SKIP     if file should be skipped
 *           CF_ERROR    on error
 *           CF_EXTRACT  file created and data to restore
 *           CF_CREATED  file created no data to restore
 *
 *   Note, we create the file here, except for special files,
 *     we do not set the attributes because we want to first
 *     write the file, then when the writing is done, set the
 *     attributes.
 *   So, we return with the file descriptor open for normal
 *     files.
 *
 */
int create_file(JCR *jcr, ATTR *attr, BFILE *bfd, int replace)
{
   int new_mode, parent_mode, mode;
   uid_t uid;
   gid_t gid;
   int pnl;
   bool exists = false;
   struct stat mstatp;

   if (is_win32_stream(attr->data_stream)) {
      set_win32_backup(bfd);
   } else {
      set_portable_backup(bfd);
   }

   new_mode = attr->statp.st_mode;
   Dmsg2(300, "newmode=%x file=%s\n", new_mode, attr->ofname);
   parent_mode = S_IWUSR | S_IXUSR | new_mode;
   gid = attr->statp.st_gid;
   uid = attr->statp.st_uid;

   Dmsg2(400, "Replace=%c %d\n", (char)replace, replace);
   if (lstat(attr->ofname, &mstatp) == 0) {
      exists = true;
      switch (replace) {
      case REPLACE_IFNEWER:
         if (attr->statp.st_mtime <= mstatp.st_mtime) {
            Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Not newer: %s\n"), attr->ofname);
            return CF_SKIP;
         }
         break;

      case REPLACE_IFOLDER:
         if (attr->statp.st_mtime >= mstatp.st_mtime) {
            Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Not older: %s\n"), attr->ofname);
            return CF_SKIP;
         }
         break;

      case REPLACE_NEVER:
         Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Already exists: %s\n"), attr->ofname);
         return CF_SKIP;

      case REPLACE_ALWAYS:
         break;
      }
   }
   switch (attr->type) {
   case FT_LNKSAVED:                  /* Hard linked, file already saved */
   case FT_LNK:
   case FT_RAW:
   case FT_FIFO:
   case FT_SPEC:
   case FT_REGE:                      /* empty file */
   case FT_REG:                       /* regular file */
      /* 
       * Note, we do not delete FT_RAW because these are device files
       *  or FIFOs that should already exist. If we blow it away,
       *  we may blow away a FIFO that is being used to read the
       *  restore data, or we may blow away a partition definition.
       */
      if (exists && attr->type != FT_RAW && attr->type != FT_FIFO) {
         /* Get rid of old copy */
         if (unlink(attr->ofname) == -1) {
            berrno be;
            Jmsg(jcr, M_ERROR, 0, _("File %s already exists and could not be replaced. ERR=%s.\n"),
               attr->ofname, be.strerror());
            /* Continue despite error */
         }
      }
      /*
       * Here we do some preliminary work for all the above
       *   types to create the path to the file if it does
       *   not already exist.  Below, we will split to
       *   do the file type specific work
       */
      pnl = separate_path_and_file(jcr, attr->fname, attr->ofname);
      if (pnl < 0) {
         return CF_ERROR;
      }

      /*
       * If path length is <= 0 we are making a file in the root
       *  directory. Assume that the directory already exists.
       */
      if (pnl > 0) {
         char savechr;
         savechr = attr->ofname[pnl];
         attr->ofname[pnl] = 0;                 /* terminate path */

         if (!path_already_seen(jcr, attr->ofname, pnl)) {
            Dmsg1(100, "Make path %s\n", attr->ofname);
            /*
             * If we need to make the directory, ensure that it is with
             * execute bit set (i.e. parent_mode), and preserve what already
             * exists. Normally, this should do nothing.
             */
            if (make_path(jcr, attr->ofname, parent_mode, parent_mode, uid, gid, 1, NULL) != 0) {
               Dmsg1(10, "Could not make path. %s\n", attr->ofname);
               attr->ofname[pnl] = savechr;     /* restore full name */
               return CF_ERROR;
            }
         }
         attr->ofname[pnl] = savechr;           /* restore full name */
      }

      /* Now we do the specific work for each file type */
      switch(attr->type) {
      case FT_REGE:
      case FT_REG:
         Dmsg1(100, "Create file %s\n", attr->ofname);
         mode =  O_WRONLY | O_CREAT | O_TRUNC | O_BINARY; /*  O_NOFOLLOW; */
         if (IS_CTG(attr->statp.st_mode)) {
            mode |= O_CTG;               /* set contiguous bit if needed */
         }
         Dmsg1(50, "Create file: %s\n", attr->ofname);
         if (is_bopen(bfd)) {
            Jmsg1(jcr, M_ERROR, 0, _("bpkt already open fid=%d\n"), bfd->fid);
            bclose(bfd);
         }
         /*
          * If the open fails, we attempt to cd into the directory
          *  and create the file with a relative path rather than
          *  the full absolute path.  This is for Win32 where
          *  path names may be too long to create.
          */
         if ((bopen(bfd, attr->ofname, mode, S_IRUSR | S_IWUSR)) < 0) {
            berrno be;
            int stat;
            Dmsg2(000, "bopen failed errno=%d: ERR=%s\n", bfd->berrno,  
               be.strerror(bfd->berrno));
            if (strlen(attr->ofname) > 250) {   /* Microsoft limitation */
               char savechr;
               char *p, *e;
               struct saved_cwd cwd;
               savechr = attr->ofname[pnl];
               attr->ofname[pnl] = 0;                 /* terminate path */
               Dmsg1(000, "Do chdir %s\n", attr->ofname);
               if (save_cwd(&cwd) != 0) {
                  Jmsg0(jcr, M_ERROR, 0, _("Could not save_dirn"));
                  attr->ofname[pnl] = savechr;
                  return CF_ERROR;
               }
               p = attr->ofname;
               while ((e = strchr(p, '/'))) {
                  *e = 0;
                  if (chdir(p) < 0) {
                     berrno be;
                     Jmsg2(jcr, M_ERROR, 0, _("Could not chdir to %s: ERR=%s\n"),
                           attr->ofname, be.strerror());
                     restore_cwd(&cwd, NULL, NULL);
                     free_cwd(&cwd);
                     attr->ofname[pnl] = savechr;
                     *e = '/';
                     return CF_ERROR;
                  }
                  *e = '/';
                  p = e + 1;
               }
               if (chdir(p) < 0) {
                  berrno be;
                  Jmsg2(jcr, M_ERROR, 0, _("Could not chdir to %s: ERR=%s\n"),
                        attr->ofname, be.strerror());
                  restore_cwd(&cwd, NULL, NULL);
                  free_cwd(&cwd);
                  attr->ofname[pnl] = savechr;
                  return CF_ERROR;
               }
               attr->ofname[pnl] = savechr;
               Dmsg1(000, "Do open %s\n", &attr->ofname[pnl+1]);
               if ((bopen(bfd, &attr->ofname[pnl+1], mode, S_IRUSR | S_IWUSR)) < 0) {
                  stat = CF_ERROR;
               } else {
                  stat = CF_EXTRACT;
               }
               restore_cwd(&cwd, NULL, NULL);
               free_cwd(&cwd);
               if (stat == CF_EXTRACT) {
                  return CF_EXTRACT;
               }
            }
            Jmsg2(jcr, M_ERROR, 0, _("Could not create %s: ERR=%s\n"),
                  attr->ofname, be.strerror(bfd->berrno));
            return CF_ERROR;
         }
         return CF_EXTRACT;
#ifndef HAVE_WIN32  // none of these exists on MS Windows
      case FT_RAW:                    /* Bacula raw device e.g. /dev/sda1 */
      case FT_FIFO:                   /* Bacula fifo to save data */
      case FT_SPEC:
         if (S_ISFIFO(attr->statp.st_mode)) {
            Dmsg1(200, "Restore fifo: %s\n", attr->ofname);
            if (mkfifo(attr->ofname, attr->statp.st_mode) != 0 && errno != EEXIST) {
               berrno be;
               Jmsg2(jcr, M_ERROR, 0, _("Cannot make fifo %s: ERR=%s\n"),
                     attr->ofname, be.strerror());
               return CF_ERROR;
            }
         } else {
            Dmsg1(200, "Restore node: %s\n", attr->ofname);
            if (mknod(attr->ofname, attr->statp.st_mode, attr->statp.st_rdev) != 0 && errno != EEXIST) {
               berrno be;
               Jmsg2(jcr, M_ERROR, 0, _("Cannot make node %s: ERR=%s\n"),
                     attr->ofname, be.strerror());
               return CF_ERROR;
            }
         }
         if (attr->type == FT_RAW || attr->type == FT_FIFO) {
            btimer_t *tid;
            Dmsg1(200, "FT_RAW|FT_FIFO %s\n", attr->ofname);
            mode =  O_WRONLY | O_BINARY;
            /* Timeout open() in 60 seconds */
            if (attr->type == FT_FIFO) {
               tid = start_thread_timer(pthread_self(), 60);
            } else {
               tid = NULL;
            }
            if (is_bopen(bfd)) {
               Jmsg1(jcr, M_ERROR, 0, _("bpkt already open fid=%d\n"), bfd->fid);
            }
            if ((bopen(bfd, attr->ofname, mode, 0)) < 0) {
               berrno be;
               be.set_errno(bfd->berrno);
               Jmsg2(jcr, M_ERROR, 0, _("Could not open %s: ERR=%s\n"),
                     attr->ofname, be.strerror());
               stop_thread_timer(tid);
               return CF_ERROR;
            }
            stop_thread_timer(tid);
            return CF_EXTRACT;
         }
         Dmsg1(200, "FT_SPEC %s\n", attr->ofname);
         return CF_CREATED;

      case FT_LNK:
         Dmsg2(130, "FT_LNK should restore: %s -> %s\n", attr->ofname, attr->olname);
         if (symlink(attr->olname, attr->ofname) != 0 && errno != EEXIST) {
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Could not symlink %s -> %s: ERR=%s\n"),
                  attr->ofname, attr->olname, be.strerror());
            return CF_ERROR;
         }
         return CF_CREATED;

      case FT_LNKSAVED:                  /* Hard linked, file already saved */
         Dmsg2(130, "Hard link %s => %s\n", attr->ofname, attr->olname);
         if (link(attr->olname, attr->ofname) != 0) {
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Could not hard link %s -> %s: ERR=%s\n"),
                  attr->ofname, attr->olname, be.strerror());
            return CF_ERROR;
         }
         return CF_CREATED;
#endif
      } /* End inner switch */

   case FT_DIRBEGIN:
   case FT_DIREND:
      Dmsg2(200, "Make dir mode=%o dir=%s\n", new_mode, attr->ofname);
      if (make_path(jcr, attr->ofname, new_mode, parent_mode, uid, gid, 0, NULL) != 0) {
         return CF_ERROR;
      }
      /*
       * If we are using the Win32 Backup API, we open the
       *   directory so that the security info will be read
       *   and saved.
       */
      if (!is_portable_backup(bfd)) {
         if (is_bopen(bfd)) {
            Jmsg1(jcr, M_ERROR, 0, _("bpkt already open fid=%d\n"), bfd->fid);
         }
         if ((bopen(bfd, attr->ofname, O_WRONLY|O_BINARY, 0)) < 0) {
            berrno be;
            be.set_errno(bfd->berrno);
#ifdef HAVE_WIN32
            /* Check for trying to create a drive, if so, skip */
            if (attr->ofname[1] == ':' && attr->ofname[2] == '/' && attr->ofname[3] == 0) {
               return CF_SKIP;
            }
#endif
            Jmsg2(jcr, M_ERROR, 0, _("Could not open %s: ERR=%s\n"),
                  attr->ofname, be.strerror());
            return CF_ERROR;
         }
         return CF_EXTRACT;
      } else {
         return CF_CREATED;
      }

   /* The following should not occur */
   case FT_NOACCESS:
   case FT_NOFOLLOW:
   case FT_NOSTAT:
   case FT_DIRNOCHG:
   case FT_NOCHG:
   case FT_ISARCH:
   case FT_NORECURSE:
   case FT_NOFSCHG:
   case FT_NOOPEN:
      Jmsg2(jcr, M_ERROR, 0, _("Original file %s not saved: type=%d\n"), attr->fname, attr->type);
      break;
   default:
      Jmsg2(jcr, M_ERROR, 0, _("Unknown file type %d; not restored: %s\n"), attr->type, attr->fname);
      break;
   }
   return CF_ERROR;
}

/*
 *  Returns: > 0 index into path where last path char is.
 *           0  no path
 *           -1 filename is zero length
 */
static int separate_path_and_file(JCR *jcr, char *fname, char *ofile)
{
   char *f, *p;
   int fnl, pnl;

   /* Separate pathname and filename */
   for (p=f=ofile; *p; p++) {
      if (*p == '/') {
         f = p;                    /* possible filename */
      }
   }
   if (*f == '/') {
      f++;
   }

   fnl = p - f;
   if (fnl == 0) {
      /* The filename length must not be zero here because we
       *  are dealing with a file (i.e. FT_REGE or FT_REG).
       */
      Jmsg1(jcr, M_ERROR, 0, _("Zero length filename: %s\n"), fname);
      return -1;
   }
   pnl = f - ofile - 1;
   return pnl;
}

/*
 * Primitive caching of path to prevent recreating a pathname
 *   each time as long as we remain in the same directory.
 */
static int path_already_seen(JCR *jcr, char *path, int pnl)
{
   if (!jcr->cached_path) {
      jcr->cached_path = get_pool_memory(PM_FNAME);
   }
   if (jcr->cached_pnl == pnl && strcmp(path, jcr->cached_path) == 0) {
      return 1;
   }
   pm_strcpy(&jcr->cached_path, path);
   jcr->cached_pnl = pnl;
   return 0;
}
