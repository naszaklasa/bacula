/*
 * Process and thread timer routines, built on top of watchdogs.
 * 
 *    Nic Bellamy <nic@bellamy.co.nz>, October 2003.
 *
*/
/*
   Copyright (C) 2003-2004 Kern Sibbald and John Walker

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

typedef struct s_btimer_t {
   watchdog_t *wd;		      /* Parent watchdog */
   int type;
   bool killed;
   pid_t pid;			      /* process id if TYPE_CHILD */
   pthread_t tid;		      /* thread id if TYPE_PTHREAD */
   BSOCK *bsock;		      /* Pointer to BSOCK */
} btimer_t;

/* EOF */
