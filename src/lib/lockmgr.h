/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2008-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation, which is 
   listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#ifndef _LOCKMGR_H
#define _LOCKMGR_H 1

#include "bacula.h"

/*
 * P and V op that don't use the lock manager (for memory allocation or on
 * win32)
 */
void lmgr_p(pthread_mutex_t *m);
void lmgr_v(pthread_mutex_t *m);

#ifdef _USE_LOCKMGR

/* 
 * We decide that a thread won't lock more than LMGR_MAX_LOCK at the same time
 */
#define LMGR_MAX_LOCK 32

/* Not yet working */
int lmgr_cond_wait(pthread_cond_t *cond,
                   pthread_mutex_t *mutex);

/* Replacement of pthread_mutex_lock() */
int lmgr_mutex_lock(pthread_mutex_t *m, 
                    const char *file="*unknown*", int line=0);

/* Replacement of pthread_mutex_unlock() */
int lmgr_mutex_unlock(pthread_mutex_t *m, 
                      const char *file="*unknown*", int line=0);

/* 
 * Use them when you want use your lock yourself (ie rwlock)
 */
void lmgr_pre_lock(void *m);    /* Call before requesting the lock */
void lmgr_post_lock();          /* Call after getting it */
void lmgr_do_lock(void *m);     /* Same as pre+post lock */
void lmgr_do_unlock(void *m);   /* Call just before releasing the lock */

/*
 * Each thread have to call this function to put a lmgr_thread_t object
 * in the stack and be able to call mutex_lock/unlock
 */
void lmgr_init_thread();

/*
 * Call this function at the end of the thread
 */
void lmgr_cleanup_thread();

/*
 * Call this at the end of the program, it will release the 
 * global lock manager
 */
void lmgr_cleanup_main();

/*
 * Dump each lmgr_thread_t object to stdout
 */
void lmgr_dump();

/*
 * Search a deadlock
 */
bool lmgr_detect_deadlock();

/*
 * Search a deadlock after a fatal signal
 * no lock are granted, so the program must be
 * stopped.
 */
bool lmgr_detect_deadlock_unlocked();

/*
 * This function will run your thread with lmgr_init_thread() and
 * lmgr_cleanup_thread().
 */
int lmgr_thread_create(pthread_t *thread,
                       const pthread_attr_t *attr,
                       void *(*start_routine)(void*), void *arg);

/* 
 * Define _LOCKMGR_COMPLIANT to use real pthread functions
 */

#ifdef _LOCKMGR_COMPLIANT
# define P(x) lmgr_p(&(x))
# define V(x) lmgr_v(&(x))
#else
# define P(x) lmgr_mutex_lock(&(x), __FILE__, __LINE__)
# define V(x) lmgr_mutex_unlock(&(x), __FILE__, __LINE__)
# define pthread_mutex_lock(x)       lmgr_mutex_lock(x, __FILE__, __LINE__)
# define pthread_mutex_unlock(x)     lmgr_mutex_unlock(x, __FILE__, __LINE__)
# define pthread_cond_wait(x,y)      lmgr_cond_wait(x,y)
# define pthread_create(a, b, c, d)  lmgr_thread_create(a,b,c,d)
#endif

#else   /* _USE_LOCKMGR */

# define lmgr_detect_deadloc()
# define lmgr_dump()
# define lmgr_init_thread()
# define lmgr_cleanup_thread()
# define lmgr_pre_lock(m)
# define lmgr_post_lock()
# define lmgr_do_lock(m)
# define lmgr_do_unlock(m)
# define lmgr_cleanup_main()
# define P(x) lmgr_p(&(x))
# define V(x) lmgr_v(&(x))

#endif  /* _USE_LOCKMGR */

#endif  /* _LOCKMGR_H */
