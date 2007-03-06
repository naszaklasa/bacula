/*
 * Bacula Semaphore code. This code permits setting up
 *  a semaphore that lets through a specified number
 *  of callers simultaneously. Once the number of callers
 *  exceed the limit, they block.
 *
 *  Kern Sibbald, March MMIII
 *
 *   Derived from rwlock.h which was in turn derived from code in
 *     "Programming with POSIX Threads" By David R. Butenhof
 *
 *   Version $Id: semlock.h,v 1.2 2004/12/21 16:18:40 kerns Exp $
 */
/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

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

#ifndef __SEMLOCK_H
#define __SEMLOCK_H 1

typedef struct s_semlock_tag {
   pthread_mutex_t   mutex;           /* main lock */
   pthread_cond_t    wait;            /* wait for available slot */
   int               valid;           /* set when valid */
   int               waiting;         /* number of callers waiting */
   int               max_active;      /* maximum active callers */
   int               active;          /* number of active callers */
} semlock_t;

#define SEMLOCK_VALID  0xfacade

#define SEM_INIIALIZER \
   {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, \
    PTHREAD_COND_INITIALIZER, SEMLOCK_VALID, 0, 0, 0, 0}

/*
 * semaphore lock prototypes
 */
extern int sem_init(semlock_t *sem, int max_active);
extern int sem_destroy(semlock_t *sem);
extern int sem_lock(semlock_t *sem);
extern int sem_trylock(semlock_t *sem);
extern int sem_unlock(semlock_t *sem);

#endif /* __SEMLOCK_H */
