/*
 *   Bacula shared memory routines
 *
 * To avoid problems with several return arguments, we
 * pass a packet.
 *
 *  BSHM definition is in bshm.h
 *
 *  By Kern Sibbald, May MM
 *
 *   Version $Id: bshm.c,v 1.7 2005/08/10 16:35:19 nboichat Exp $
 *
 *  Note, this routine was originally written so that the DEVICE structures
 *  could be shared between the child processes.  Now that the Storage
 *  daemon is threaded, these routines are no longer needed. Rather than
 *  rewrite all the code, I simply #ifdef it on NEED_SHARED_MEMORY, and
 *  when not defined, I simply malloc() a buffer, which is, of course,
 *  available to all the threads.
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

#ifdef implemented

#include "bacula.h"

#if !defined(HAVE_CYGWIN) && !defined(HAVE_WIN32)

#ifdef NEED_SHARED_MEMORY
#define SHM_KEY 0x0BACB01	     /* key for shared memory */
static key_t shmkey = SHM_KEY;
#define MAX_TRIES 1000

#else /* threaded model */
/* Multiple thread protection */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/* Create shared memory segment defined by BSHM */
void shm_create(BSHM *shm)
{
#ifdef NEED_SHARED_MEMORY
   int shmid, i;
   int not_found = TRUE;

   Dmsg1(110, "shm_create size=%d\n", shm->size);
   for (i=0; i<MAX_TRIES; i++) {
      if ((shmid = shmget(shmkey, shm->size, shm->perms | IPC_CREAT)) < 0) {
	 Emsg1(M_WARN, 0, _("shmget failure key = %x\n"), shmkey);
	 shmkey++;
	 continue;
      }
      not_found = FALSE;
      break;
   }
   if (not_found)
      Emsg2(M_ABORT, 0, _("Could not get %d bytes of shared memory: %s\n"), shm->size, strerror(errno));
   shm->shmkey = shmkey;
   shm->shmid = shmid;
   Dmsg2(110, "shm_create return key=%x id=%d\n", shmkey, shmid);
   shmkey++;			      /* leave set for next time */
#else
   shm->shmbuf = NULL;
   shm->shmkey = 0;		      /* reference count */
#endif
}

/* Attach to shared memory segement defined in BSHM */
void *shm_open(BSHM *shm)
{
#ifdef NEED_SHARED_MEMORY
   int shmid;
   char *shmbuf;

   Dmsg2(110, "shm_open key=%x size=%d\n", shm->shmkey, shm->size);
   if ((shmid = shmget(shm->shmkey, shm->size, 0)) < 0)
      Emsg2(M_ABORT, 0, "Could not get %d bytes of shared memory: %s\n", shm->size, strerror(errno));
   Dmsg1(110, "shm_open shmat with id=%d\n", shmid);
   shmbuf = shmat(shmid, NULL, 0);
   Dmsg1(110, "shm_open buf=%x\n", shmbuf);
   if (shmbuf == (char *) -1)
      Emsg1(M_ABORT, 0, _("Could not attach shared memory: %s\n"), strerror(errno));
   shm->shmbuf = shmbuf;
   shm->shmid = shmid;
   return shmbuf;
#else
   P(mutex);
   if (!shm->shmbuf) {
      shm->shmbuf = bmalloc(shm->size);
   }
   shm->shmkey++;		      /* reference count */
   V(mutex);
   return shm->shmbuf;
#endif
}

/* Detach from shared memory segement */
void shm_close(BSHM *shm)
{
#ifdef NEED_SHARED_MEMORY
   if (shm->size) {
      if (shmdt(shm->shmbuf) < 0) {
	 Emsg1(M_ERROR, 0, _("Error detaching shared memory: %s\n"), strerror(errno));
      }
   }
#else
   P(mutex);
   shm->shmkey--;		      /* reference count */
   V(mutex);
#endif
}

/* Destroy the shared memory segment */
void shm_destroy(BSHM *shm)
{
#ifdef NEED_SHARED_MEMORY
   if (shm->size) {
      if (shmctl(shm->shmid, IPC_RMID, NULL) < 0) {
	 Emsg1(M_ERROR, 0, _("Could not destroy shared memory: %s\n"), strerror(errno));
      }
   }
#else
   /* We really should check that the ref count is zero */
   P(mutex);
   if (shm->shmbuf) {
      free(shm->shmbuf);
      shm->shmbuf = NULL;
   }
   V(mutex);
#endif
}

#endif /* ! HAVE_CYGWIN */

#endif
