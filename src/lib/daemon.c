/*
 *  daemon.c by Kern Sibbald
 *
 *   Version $Id: daemon.c,v 1.18 2005/09/20 12:36:00 kerns Exp $
 *
 *   this code is inspired by the Prentice Hall book
 *   "Unix Network Programming" by W. Richard Stevens
 *   and later updated from his book "Advanced Programming
 *   in the UNIX Environment"
 *
 * Initialize a daemon process completely detaching us from
 * any terminal processes.
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
extern int debug_level;

void
daemon_start()
{
#if !defined(HAVE_CYGWIN) && !defined(HAVE_WIN32)
   int i;
   pid_t cpid;
   mode_t oldmask;
#ifdef DEVELOPER
   int low_fd = 2;
#else
   int low_fd = -1;
#endif
   /*
    *  Become a daemon.
    */

   Dmsg0(900, "Enter daemon_start\n");
   if ( (cpid = fork() ) < 0) {
      berrno be;
      Emsg1(M_ABORT, 0, _("Cannot fork to become daemon: %s\n"), be.strerror());
   } else if (cpid > 0) {
      exit(0);              /* parent exits */
   }
   /* Child continues */

   setsid();

   /* In the PRODUCTION system, we close ALL
    * file descriptors except stdin, stdout, and stderr.
    */
   if (debug_level > 0) {
      low_fd = 2;                     /* don't close debug output */
   }
   for (i=sysconf(_SC_OPEN_MAX)-1; i > low_fd; i--) {
      close(i);
   }

   /* Move to root directory. For debug we stay
    * in current directory so dumps go there.
    */
#ifndef DEBUG
   chdir("/");
#endif

   /*
    * Avoid creating files 666 but don't override any
    * more restrictive mask set by the user.
    */
   oldmask = umask(026);
   oldmask |= 026;
   umask(oldmask);


   /*
    * Make sure we have fd's 0, 1, 2 open
    *  If we don't do this one of our sockets may open
    *  there and if we then use stdout, it could
    *  send total garbage to our socket.
    *
    */
   int fd;
   fd = open("/dev/null", O_RDONLY, 0644);
   if (fd > 2) {
      close(fd);
   } else {
      for(i=1; fd + i <= 2; i++) {
         dup2(fd, fd+i);
      }
   }

#endif /* HAVE_CYGWIN */
   Dmsg0(900, "Exit daemon_start\n");
}
