/*
 * bacula.h -- main header file to include in all Bacula source
 *
 *   Version $Id: bacula.h,v 1.14 2004/10/19 13:35:09 kerns Exp $
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

#ifndef _BACULA_H
#define _BACULA_H 1

#ifdef __cplusplus
/* Workaround for SGI IRIX 6.5 */
#define _LANGUAGE_C_PLUS_PLUS 1
#endif

#ifdef HAVE_WIN32
#include "winconfig.h"
#include "winhost.h"
#else
#include "config.h"
#include "host.h"
#endif


#define _REENTRANT    1
#define _THREAD_SAFE  1
#define _POSIX_PTHREAD_SEMANTICS 1


/* System includes */
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <errno.h>
#include <fcntl.h>

#ifdef xxxxx
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif
#endif 

#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#ifndef _SPLINT_
#include <syslog.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_WIN32
#include <winsock2.h>
#else
#include <sys/stat.h>
#endif
#include <sys/time.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/* Local Bacula includes. Be sure to put all the system
 *  includes before these.  
 */
#include "version.h"
#include "bc_types.h"
#include "baconfig.h"
#include "lib/lib.h"

#ifndef HAVE_ZLIB_H
#undef HAVE_LIBZ                      /* no good without headers */
#endif

#endif
