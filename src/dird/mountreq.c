/*
 *
 *   Bacula Director -- mountreq.c -- handles the message channel
 *    Mount request from the Storage daemon.
 *
 *     Kern Sibbald, March MMI
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *	Handle Mount services.
 *
 *   Version $Id: mountreq.c,v 1.3 2004/04/19 14:27:01 kerns Exp $
 */
/*
   Copyright (C) 2001-2004 Kern Sibbald and John Walker

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
#include "dird.h"

/*
 * Handle mount request
 *  For now, we put the bsock in the UA's queue
 */

/* Requests from the Storage daemon */


/* Responses  sent to Storage daemon */
#ifdef xxx
static char OK_mount[]  = "1000 OK MountVolume\n";
#endif

static BQUEUE mountq = {&mountq, &mountq};
static int num_reqs = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct mnt_req_s {
   BQUEUE bq;
   BSOCK *bs;
   JCR *jcr;
} MNT_REQ;


void mount_request(JCR *jcr, BSOCK *bs, char *buf)
{
   MNT_REQ *mr;

   mr = (MNT_REQ *) malloc(sizeof(MNT_REQ));
   memset(mr, 0, sizeof(MNT_REQ));
   mr->jcr = jcr;
   mr->bs = bs;
   P(mutex);
   num_reqs++;
   qinsert(&mountq, &mr->bq);
   V(mutex);
   return;
}
