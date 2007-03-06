/*
 *  daemon.c by Kern Sibbald
 *
 *   Version $Id: daemon.c,v 1.14.4.1 2005/02/14 10:02:25 kerns Exp $
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
   if ( (cpid = fork() ) < 0)
      Emsg1(M_ABORT, 0, "Cannot fork to become daemon: %s\n", strerror(errno));
   else if (cpid > 0)
      exit(0);		    /* parent exits */
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
