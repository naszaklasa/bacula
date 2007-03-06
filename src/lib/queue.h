/*
 *   Version $Id: queue.h,v 1.2 2002/05/19 07:38:06 kerns Exp $
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



/*  General purpose queue  */

struct b_queue {
	struct b_queue *qnext,	   /* Next item in queue */
		     *qprev;	   /* Previous item in queue */
};

typedef struct b_queue BQUEUE;

/*  Queue functions  */

void	qinsert(BQUEUE *qhead, BQUEUE *object);
BQUEUE *qnext(BQUEUE *qhead, BQUEUE *qitem);
BQUEUE *qdchain(BQUEUE *qitem);
BQUEUE *qremove(BQUEUE *qhead);
