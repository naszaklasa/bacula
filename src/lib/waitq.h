/*
 * Bacula wait queue routines. Permits waiting for something
 *   to be done. I.e. for operator to mount new volume.
 *
 *  Kern Sibbald, March MMI
 *
 *  This code inspired from "Programming with POSIX Threads", by
 *    David R. Butenhof
 *
 *   Version $Id: waitq.h,v 1.2 2002/05/19 07:38:06 kerns Exp $
 *
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

#ifndef __WAITQ_H 
#define __WAITQ_H 1

/* 
 * Structure to keep track of wait queue request
 */
typedef struct waitq_ele_tag {
   struct waitq_ele_tag *next;
   int		     done_flag;       /* predicate for wait */
   pthread_cont_t    done;	      /* wait for completion */
   void 	    *msg;	      /* message to be passed */
} waitq_ele_t;

/* 
 * Structure describing a wait queue
 */
typedef struct workq_tag {
   pthread_mutex_t   mutex;	      /* queue access control */
   pthread_cond_t    wait_req;	      /* wait for OK */
   int		     num_msgs;	      /* number of waiters */
   waitq_ele_t	     *first;	      /* wait queue first item */
   waitq_ele_t	     *last;	      /* wait queue last item */
} workq_t;

extern int waitq_init(waitq_t *wq);
extern int waitq_destroy(waitq_t *wq);
extern int waitq_add(waitq_t *wq, void *msg);

#endif /* __WAITQ_H */
