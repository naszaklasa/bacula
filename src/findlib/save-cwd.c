/* save-cwd.c -- Save and restore current working directory.

   Copyright (C) 1995, 1997, 1998 Free Software Foundation, Inc.

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

/* Written by Jim Meyering <meyering@na-net.ornl.gov>.  */

#include "bacula.h"
#include "save-cwd.h"



/* Record the location of the current working directory in CWD so that
   the program may change to other directories and later use restore_cwd
   to return to the recorded location.  This function may allocate
   space using malloc (via xgetcwd) or leave a file descriptor open;
   use free_cwd to perform the necessary free or close.  Upon failure,
   no memory is allocated, any locally opened file descriptors are
   closed;  return non-zero -- in that case, free_cwd need not be
   called, but doing so is ok.  Otherwise, return zero.  */

int
save_cwd(struct saved_cwd *cwd)
{
  static int have_working_fchdir = 1;

  cwd->desc = -1;
  cwd->name = NULL;

  if (have_working_fchdir) {
#if HAVE_FCHDIR
      cwd->desc = open(".", O_RDONLY);
      if (cwd->desc < 0) {
         berrno be;
         Emsg1(M_ERROR, 0, _("Cannot open current directory: %s\n"), be.bstrerror());
         return 1;
      }

# if __sun__ || sun
      /* On SunOS 4, fchdir returns EINVAL if accounting is enabled,
         so we have to fall back to chdir.  */
      if (fchdir(cwd->desc)) {
          if (errno == EINVAL) {
              close(cwd->desc);
              cwd->desc = -1;
              have_working_fchdir = 0;
          } else {
              berrno be;
              Emsg1(M_ERROR, 0, _("Current directory: %s\n"), be.bstrerror());
              close(cwd->desc);
              cwd->desc = -1;
              return 1;
          }
      }
# endif /* __sun__ || sun */
#else
# define fchdir(x) (abort (), 0)
      have_working_fchdir = 0;
#endif
    }

  if (!have_working_fchdir) {
#ifdef HAVE_WIN32
      POOLMEM *buf = get_memory(MAX_PATH);
#else
      POOLMEM *buf = get_memory(5000);
#endif
      cwd->name = (POOLMEM *)getcwd(buf, sizeof_pool_memory(buf));
      if (cwd->name == NULL) {
         berrno be;
         Emsg1(M_ERROR, 0, _("Cannot get current directory: %s\n"), be.bstrerror());
         free_pool_memory(buf);
         return 1;
      }
  }
  return 0;
}

/* Change to recorded location, CWD, in directory hierarchy.
   If "saved working directory", NULL))
   */

int
restore_cwd(const struct saved_cwd *cwd, const char *dest, const char *from)
{
  int fail = 0;
  if (cwd->desc >= 0) {
      if (fchdir(cwd->desc)) {
         berrno be;
         if (from) {
            if (dest) {
               Emsg3(M_ERROR, 0, _("Cannot return to %s from %s: %s\n"),
                  dest, from, be.bstrerror());
            }
            else {
               Emsg2(M_ERROR, 0, _("Cannot return to saved working directory from %s: %s\n"),
                  from, be.bstrerror());
            }
         }
         else {
            if (dest) {
               Emsg2(M_ERROR, 0, _("Cannot return to %s: %s\n"),
                  dest, be.bstrerror());
            }
            else {
               Emsg1(M_ERROR, 0, _("Cannot return to saved working directory: %s\n"),
                  be.bstrerror());
            }
         }
         fail = 1;
      }
  } else if (chdir(cwd->name) < 0) {
      berrno be;
      Emsg2(M_ERROR, 0, "%s: %s\n", cwd->name, be.bstrerror());
      fail = 1;
  }
  return fail;
}

void
free_cwd(struct saved_cwd *cwd)
{
  if (cwd->desc >= 0) {
     close(cwd->desc);
  }
  if (cwd->name) {
     free_pool_memory(cwd->name);
  }
}
