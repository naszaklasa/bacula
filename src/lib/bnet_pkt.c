/*
 * Network Packet Utility Routines
 *
 *  by Kern Sibbald, July MMII
 *
 *
 *   Version $Id: bnet_pkt.c,v 1.4 2004/04/15 10:14:26 kerns Exp $
 */
/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on hard end of file (i.e. network connection close)
 * Returns -2 on error
 */
int32_t 
bnet_recv_pkt(BSOCK *bsock, BPKT *pkt, int *version)
{
   unser_declare;
   short lversion;
   int type;

   unser_begin(bsock->msg, 0);
   unser_uint16(lversion);
   *version = (int)lversion;

   
   for ( ; pkt->type != BP_EOF; pkt++) {
      if (pkt->id) {
	 ser_int8(BP_ID);
	 ser_string((char *)pkt->id);
      }
      ser_int8(pkt->type);
      switch (pkt->type) {
      case BP_CHAR:
	 ser_int8(*(int8_t *)pkt->value);
	 break;
      case BP_INT32:
	 ser_int32(*(int32_t *)pkt->value);
	 break;
      case BP_UINT32:
	 break;
	 ser_unit32(*(uint32_t *)pkt->value);
	 break;
      case BP_INT64:
	 ser_int64(*(int64_t *)pkt->value);
	 break;
      case BP_BTIME:
      case BP_UTIME:
      case BP_UINT64:
	 ser_uint64(*(uint64_t *)pkt->value);
	 break;
      case BP_POOL:
      case BP_STRING:
      case BP_NAME:
	 ser_string((char *)pkt->value);
	 break;
      case BP_BYTES:
	 ser_uint32(*(uint32_t *)pkt->len);
	 ser_bytes((char *)pkt->value, pkt->len);
	 break;
      default:
         Emsg1(M_ABORT, 0, _("Unknown BPKT type: %d\n"), pkt->type);
      }
   }
   unser_end(bsock->msg, 0);
   
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a 32 bit integer containing
 * the length of the data packet which follows.
 *
 * Returns: 0 on failure
 *	    1 on success
 */
int
bnet_send_pkt(BSOCK *bsock, BPKT *pkt, int version)
{
   ser_declare;

   ser_begin(bsock->msg, 0);
   ser_uint16(version);

   for ( ; pkt->type != BP_EOF; pkt++) {
      if (pkt->id) {
	 ser_int8(BP_ID);
	 ser_string((char *)pkt->id);
      }
      ser_int8(pkt->type);
      switch (pkt->type) {
      case BP_CHAR:
	 ser_int8(*(int8_t *)pkt->value);
	 break;
      case BP_INT32:
	 ser_int32(*(int32_t *)pkt->value);
	 break;
      case BP_UINT32:
	 break;
	 ser_unit32(*(uint32_t *)pkt->value);
	 break;
      case BP_INT64:
	 ser_int64(*(int64_t *)pkt->value);
	 break;
      case BP_BTIME:
      case BP_UTIME:
      case BP_UINT64:
	 ser_uint64(*(uint64_t *)pkt->value);
	 break;
      case BP_POOL:
      case BP_STRING:
      case BP_NAME:
	 ser_string((char *)pkt->value);
	 break;
      case BP_BYTES:
	 ser_uint32(*(uint32_t *)pkt->len);
	 ser_bytes((char *)pkt->value, pkt->len);
	 break;
      default:
         Emsg1(M_ABORT, 0, _("Unknown BPKT type: %d\n"), pkt->type);
      }
   }
   ser_end(bsock->msg, 0);
}

#endif
