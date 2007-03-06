/*
 * Bacula job queue routines.
 *
 *  Kern Sibbald, July MMIII
 *
 *  This code adapted from Bacula work queue code, which was
 *    adapted from "Programming with POSIX Threads", by
 *    David R. Butenhof
 *
 *   Version $Id: jobq.h,v 1.4 2004/12/21 16:18:33 kerns Exp $
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

#ifndef __JOBQ_H
#define __JOBQ_H 1

/*
 * Structure to keep track of job queue request
 */
struct jobq_item_t {
   dlink link;
   JCR *jcr;
};

/*
 * Structure describing a work queue
 */
struct jobq_t {
   pthread_mutex_t   mutex;	      /* queue access control */
   pthread_cond_t    work;	      /* wait for work */
   pthread_attr_t    attr;	      /* create detached threads */
   dlist	    *waiting_jobs;    /* list of jobs waiting */
   dlist	    *running_jobs;    /* jobs running */
   dlist	    *ready_jobs;      /* jobs ready to run */
   int		     valid;	      /* queue initialized */
   bool 	     quit;	      /* jobq should quit */
   int		     max_workers;     /* max threads */
   int		     num_workers;     /* current threads */
   int		     idle_workers;    /* idle threads */
   void 	    *(*engine)(void *arg); /* user engine */
};

#define JOBQ_VALID  0xdec1993

extern int jobq_init(
	      jobq_t *wq,
	      int     threads,		  /* maximum threads */
	      void   *(*engine)(void *)   /* engine routine */
		    );
extern int jobq_destroy(jobq_t *wq);
extern int jobq_add(jobq_t *wq, JCR *jcr);
extern int jobq_remove(jobq_t *wq, JCR *jcr);

#endif /* __JOBQ_H */
