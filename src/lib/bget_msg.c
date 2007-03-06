/*
 *  Subroutines to receive network data and handle
 *   network signals for the FD and the SD.
 *
 *   Kern Sibbald, May MMI previously in src/stored/fdmsg.c
 *
 *   Version $Id: bget_msg.c,v 1.3 2004/09/27 07:01:35 kerns Exp $
 *			      
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

#include "bacula.h"                   /* pull in global headers */

extern char OK_msg[];
extern char TERM_msg[];

#define msglvl 500

/*	 
 * This routine does a bnet_recv(), then if a signal was
 *   sent, it handles it.  The return codes are the same as
 *   bne_recv() except the BNET_SIGNAL messages that can
 *   be handled are done so without returning.
 *
 * Returns number of bytes read (may return zero)
 * Returns -1 on signal (BNET_SIGNAL) 
 * Returns -2 on hard end of file (BNET_HARDEOF)
 * Returns -3 on error	(BNET_ERROR)
 */
int bget_msg(BSOCK *sock)
{
   int n;
   for ( ;; ) {
      n = bnet_recv(sock);
      if (n >= 0) {		     /* normal return */
	 return n;
      }  
      if (is_bnet_stop(sock)) {      /* error return */
	 return n;
      }

      /* BNET_SIGNAL (-1) return from bnet_recv() => network signal */
      switch (sock->msglen) {
      case BNET_EOD:		   /* end of data */
         Dmsg0(msglvl, "Got BNET_EOD\n");
	 return n;
      case BNET_EOD_POLL:
         Dmsg0(msglvl, "Got BNET_EOD_POLL\n");
	 if (sock->terminated) {
	    bnet_fsend(sock, TERM_msg);
	 } else {
	    bnet_fsend(sock, OK_msg); /* send response */
	 }
	 return n;		   /* end of data */
      case BNET_TERMINATE:
         Dmsg0(msglvl, "Got BNET_TERMINATE\n");
	 sock->terminated = 1;
	 return n;
      case BNET_POLL:
         Dmsg0(msglvl, "Got BNET_POLL\n");
	 if (sock->terminated) {
	    bnet_fsend(sock, TERM_msg);
	 } else {
	    bnet_fsend(sock, OK_msg); /* send response */
	 }
	 break;
      case BNET_HEARTBEAT:
      case BNET_HB_RESPONSE:
	 break;
      case BNET_STATUS:
	 /* *****FIXME***** Implement BNET_STATUS */
         Dmsg0(msglvl, "Got BNET_STATUS\n");
         bnet_fsend(sock, "Status OK\n");
	 bnet_sig(sock, BNET_EOD);
	 break;
      default:
         Emsg1(M_ERROR, 0, "bget_msg: unknown signal %d\n", sock->msglen);
	 break;
      }
   }
}
