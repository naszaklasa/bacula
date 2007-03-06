/*
 * Bacula Shared Memory structure
 *
 *  Kern Sibbald, May MM
 *
 *     NB: these items are deprecated.	Shared memory was
 *	   used in a first version of the Storage daemon
 *	   when it forked. Since then it has been converted
 *	   to use threads.  However, there are still some
 *	   vestiges of the shared memory code that remain and
 *	   can be removed.
 *
 *   Version $Id: bshm.h,v 1.2 2002/05/19 07:38:06 kerns Exp $
 */
/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

typedef struct s_bshm {
   int size;			      /* length desired */
   int perms;			      /* permissions desired */

   int shmid;			      /* id set by shm_create and shm_open */
   key_t shmkey;		      /* key set by shm_create */
   void *shmbuf;		      /* set by shm_open */
} BSHM;
