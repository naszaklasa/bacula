/* makepath.c -- Ensure that a directory path exists.

   Copyright (C) 1990, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and Jim Meyering.  */
/*
 *   Modified by Kern Sibbald for use in Bacula, December 2000
 *
 *   Version $Id: makepath.c 6867 2008-05-01 16:33:28Z kerns $
 */

#include "bacula.h"
#include "jcr.h"
#include "save-cwd.h"


#if !defined(S_ISDIR) && defined(S_IFDIR)
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_IRWXUGO
# define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif


#ifndef S_ISUID
# define S_ISUID 04000
#endif
#ifndef S_ISGID
# define S_ISGID 02000
#endif
#ifndef S_ISVTX
# define S_ISVTX 01000
#endif
#ifndef S_IRUSR
# define S_IRUSR 0200
#endif
#ifndef S_IWUSR
# define S_IWUSR 0200
#endif
#ifndef S_IXUSR
# define S_IXUSR 0100
#endif

#ifndef S_IRWXU
# define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif

#define WX_USR (S_IWUSR | S_IXUSR)

#define quote(path) path

extern void strip_trailing_slashes();

static int
cleanup(struct saved_cwd *cwd)
{
   if (cwd->do_chdir) {
      int _fail = restore_cwd(cwd, NULL, NULL);
      free_cwd(cwd);
      if (_fail) {
         return 1;
      }
   }
   return 0;
}


/* Attempt to create directory DIR (aka DIRPATH) with the specified MODE.
   If CREATED_DIR_P is non-NULL, set *CREATED_DIR_P to non-zero if this
   function creates DIR and to zero otherwise.  Give a diagnostic and
   return non-zero if DIR cannot be created or cannot be determined to
   exist already.  Use DIRPATH in any diagnostic, not DIR.
   Note that if DIR already exists, this function will return zero
   (indicating success) and will set *CREATED_DIR_P to zero.  */

static int
make_dir(JCR *jcr, const char *dir, const char *dirpath, mode_t mode, int *created_dir_p)
{
  int fail = 0;
  int created_dir;
  int save_errno;

  Dmsg2(300, "make_dir mode=%o dir=%s\n", mode, dir);
  created_dir = (mkdir(dir, mode) == 0);
  save_errno = errno;

  if (!created_dir) {
      struct stat stats;

      /* The mkdir and stat calls below may appear to be reversed.
         They are not.  It is important to call mkdir first and then to
         call stat (to distinguish the three cases) only if mkdir fails.
         The alternative to this approach is to `stat' each directory,
         then to call mkdir if it doesn't exist.  But if some other process
         were to create the directory between the stat & mkdir, the mkdir
         would fail with EEXIST.  */

      if (stat(dir, &stats)) {
          berrno be;
          be.set_errno(save_errno);
          Jmsg(jcr, M_ERROR, 0, _("Cannot create directory %s: ERR=%s\n"),
                  dirpath, be.bstrerror());
          fail = 1;
      } else if (!S_ISDIR(stats.st_mode)) {
          Jmsg(jcr, M_ERROR, 0, _("%s exists but is not a directory\n"), quote(dirpath));
          fail = 1;
      } else {
          /* DIR (aka DIRPATH) already exists and is a directory. */
      }
  }

  if (created_dir_p) {
     *created_dir_p = created_dir;
  }

  return fail;
}

/* return non-zero if path is absolute or zero if relative. */
int
isAbsolute(const char *path)
{
#if defined(HAVE_WIN32)
    return path[1] == ':' || IsPathSeparator(*path);     /* drivespec:/blah is absolute */
#else
    return IsPathSeparator(*path);
#endif
}

/* Ensure that the directory ARGPATH exists.
   Remove any trailing slashes from ARGPATH before calling this function.

   Create any leading directories that don't already exist, with
   permissions PARENT_MODE.

   If the last element of ARGPATH does not exist, create it as
   a new directory with permissions MODE.

   If OWNER and GROUP are non-negative, use them to set the UID and GID of
   any created directories.

   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory,
   with the name of the directory that was just made as an argument.

   If PRESERVE_EXISTING is non-zero and ARGPATH is an existing directory,
   then do not attempt to set its permissions and ownership.

   Return 0 if ARGPATH exists as a directory with the proper
   ownership and permissions when done, otherwise 1.  */

int
make_path(
           JCR *jcr,
           const char *argpath,
           int mode,
           int parent_mode,
           uid_t owner,
           gid_t group,
           int preserve_existing,
           char *verbose_fmt_string)
{
  struct stat stats;
  int retval = 0;

  if (stat(argpath, &stats)) {
      char *slash;
      int tmp_mode;              /* Initial perms for leading dirs.  */
      int re_protect;            /* Should leading dirs be unwritable? */
      struct ptr_list {
        char *dirname_end;
        struct ptr_list *next;
      };
      struct ptr_list *p, *leading_dirs = NULL;
      struct saved_cwd cwd;
      char *basename_dir;
      char *dirpath;

      /* Temporarily relax umask in case it's overly restrictive.  */
      mode_t oldmask = umask(0);

      /* Make a copy of ARGPATH that we can scribble NULs on.  */
      dirpath = (char *)alloca(strlen(argpath) + 1);
      strcpy (dirpath, argpath);
      strip_trailing_slashes(dirpath);

      /* If leading directories shouldn't be writable or executable,
         or should have set[ug]id or sticky bits set and we are setting
         their owners, we need to fix their permissions after making them.  */
      if (((parent_mode & WX_USR) != WX_USR)
          || ((owner != (uid_t)-1 || group != (gid_t)-1)
              && (parent_mode & (S_ISUID | S_ISGID | S_ISVTX)) != 0)) {
         tmp_mode = S_IRWXU;
         re_protect = 1;
      } else {
         tmp_mode = parent_mode;
         re_protect = 0;
      }

#if defined(HAVE_WIN32)
      /* chdir can fail if permissions are sufficiently restricted since I don't think
         backup/restore security rights affect ChangeWorkingDirectory */
      cwd.do_chdir = 0;

      /* Validate drive letter */
      if (dirpath[1] == ':') {
         char drive[4] = "X:\\";

         drive[0] = dirpath[0];

         UINT drive_type = GetDriveType(drive);

         if (drive_type == DRIVE_UNKNOWN || drive_type == DRIVE_NO_ROOT_DIR) {
            Jmsg(jcr, M_ERROR, 0, _("%c: is not a valid drive\n"), dirpath[0]);
            return 1;
         }

         if (dirpath[2] == '\0') {
            return 0;
         }

         slash = &dirpath[3];
      } else {
         slash = dirpath;
      }
#else
      /* If we can record the current working directory, we may be able
         to do the chdir optimization.  */
      cwd.do_chdir = !save_cwd(&cwd);

      slash = dirpath;
#endif

      /* If we've saved the cwd and DIRPATH is an absolute pathname,
         we must chdir to `/' in order to enable the chdir optimization.
         So if chdir ("/") fails, turn off the optimization.  */
      if (cwd.do_chdir && isAbsolute(dirpath) && (chdir("/") < 0)) {
         cwd.do_chdir = 0;
      }

      /* Skip over leading slashes.  */
      while (IsPathSeparator(*slash))
         slash++;

      for ( ; ; ) {
          int newly_created_dir;
          int fail;

          /* slash points to the leftmost unprocessed component of dirpath.  */
          basename_dir = slash;
          slash = first_path_separator(slash);
          if (slash == NULL) {
             break;
          }

          /* If we're *not* doing chdir before each mkdir, then we have to refer
             to the target using the full (multi-component) directory name.  */
          if (!cwd.do_chdir) {
             basename_dir = dirpath;
          }

          *slash = '\0';
          fail = make_dir(jcr, basename_dir, dirpath, tmp_mode, &newly_created_dir);
          if (fail) {
              umask(oldmask);
              cleanup(&cwd);
              return 1;
          }

          if (newly_created_dir) {
              Dmsg0(300, "newly_created_dir\n");

              if ((owner != (uid_t)-1 || group != (gid_t)-1)
                  && chown(basename_dir, owner, group)
#if defined(AFS) && defined (EPERM)
                  && errno != EPERM
#endif
                  ) {
                 /* Note, if we are restoring as NON-root, this may not be fatal */
                 berrno be;
                 Jmsg(jcr, M_WARNING, 0, _("Cannot change owner and/or group of %s: ERR=%s\n"),
                      quote(dirpath), be.bstrerror());
              }
              Dmsg0(300, "Chown done.\n");

              if (re_protect) {
                 struct ptr_list *pnew = (struct ptr_list *)
                    alloca(sizeof (struct ptr_list));
                 pnew->dirname_end = slash;
                 pnew->next = leading_dirs;
                 leading_dirs = pnew;
                 Dmsg0(300, "re_protect\n");
              }
          }

          /* If we were able to save the initial working directory,
             then we can use chdir to change into each directory before
             creating an entry in that directory.  This avoids making
             stat and mkdir process O(n^2) file name components.  */
          if (cwd.do_chdir && chdir(basename_dir) < 0) {
              berrno be;
              Jmsg(jcr, M_ERROR, 0, _("Cannot chdir to directory, %s: ERR=%s\n"),
                     quote(dirpath), be.bstrerror());
              umask(oldmask);
              cleanup(&cwd);
              return 1;
          }

          *slash++ = '/';

          /* Avoid unnecessary calls to `stat' when given
             pathnames containing multiple adjacent slashes.  */
         while (IsPathSeparator(*slash))
            slash++;
      } /* end while (1) */

      if (!cwd.do_chdir) {
         basename_dir = dirpath;
      }

      /* We're done making leading directories.
         Create the final component of the path.  */

      Dmsg1(300, "Create final component. mode=%o\n", mode);
      if (make_dir(jcr, basename_dir, dirpath, mode, NULL)) {
          umask(oldmask);
          cleanup(&cwd);
          return 1;
      }

      /* Done creating directories.  Restore original umask.  */
      umask(oldmask);

      if (owner != (uid_t)-1 || group != (gid_t)-1) {
          if (chown(basename_dir, owner, group)
#ifdef AFS
              && errno != EPERM
#endif
              )
            {
              berrno be;
              Jmsg(jcr, M_WARNING, 0, _("Cannot change owner and/or group of %s: ERR=%s\n"),
                     quote(dirpath), be.bstrerror());
            }
      }

      /* The above chown may have turned off some permission bits in MODE.
         Another reason we may have to use chmod here is that mkdir(2) is
         required to honor only the file permission bits.  In particular,
         it need not honor the `special' bits, so if MODE includes any
         special bits, set them here.  */
      if (mode & ~S_IRWXUGO) {
         Dmsg1(300, "Final chmod mode=%o\n", mode);
      }
      if ((mode & ~S_IRWXUGO) && chmod(basename_dir, mode)) {
          berrno be;
          Jmsg(jcr, M_WARNING, 0, _("Cannot change permissions of %s: ERR=%s\n"),
             quote(dirpath), be.bstrerror());
      }

     if (cleanup(&cwd)) {
        return 1;
     }

      /* If the mode for leading directories didn't include owner "wx"
         privileges, we have to reset their protections to the correct
         value.  */
      for (p = leading_dirs; p != NULL; p = p->next) {
          *(p->dirname_end) = '\0';
          Dmsg2(300, "Reset parent mode=%o dir=%s\n", parent_mode, dirpath);
          if (chmod(dirpath, parent_mode)) {
              berrno be;
              Jmsg(jcr, M_WARNING, 0, _("Cannot change permissions of %s: ERR=%s\n"),
                     quote(dirpath), be.bstrerror());
          }
      }
  } else {
      /* We get here if the entire path already exists.  */

      const char *dirpath = argpath;

      if (!S_ISDIR(stats.st_mode)) {
          Jmsg(jcr, M_ERROR, 0, _("%s exists but is not a directory\n"), quote(dirpath));
          return 1;
      }

      if (!preserve_existing) {
          Dmsg0(300, "Do not preserve existing.\n");
          /* chown must precede chmod because on some systems,
             chown clears the set[ug]id bits for non-superusers,
             resulting in incorrect permissions.
             On System V, users can give away files with chown and then not
             be able to chmod them.  So don't give files away.  */

          if ((owner != (uid_t)-1 || group != (gid_t)-1)
              && chown(dirpath, owner, group)
#ifdef AFS
              && errno != EPERM
#endif
              ) {
              berrno be;
              Jmsg(jcr, M_WARNING, 0, _("Cannot change owner and/or group of %s: ERR=%s\n"),
                     quote(dirpath), be.bstrerror());
            }
          if (chmod(dirpath, mode)) {
              berrno be;
              Jmsg(jcr, M_WARNING, 0, _("Cannot change permissions of %s: ERR=%s\n"),
                                 quote(dirpath), be.bstrerror());
          }
          Dmsg2(300, "pathexists chmod mode=%o dir=%s\n", mode, dirpath);
      }
  }
  return retval;
}
