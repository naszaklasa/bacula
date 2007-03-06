/*
 *  Challenge Response Authentication Method using MD5 (CRAM-MD5)
 *
 * cram-md5 is based on RFC2104.
 * 
 * Written for Bacula by Kern E. Sibbald, May MMI.
 *
 *   Version $Id: cram-md5.c,v 1.17 2004/11/20 07:49:43 kerns Exp $
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"

/* Authorize other end */
int cram_md5_auth(BSOCK *bs, char *password, int ssl_need)
{
   struct timeval t1;
   struct timeval t2;
   struct timezone tz;
   int i, ok;
   char chal[MAXSTRING];
   char host[MAXSTRING];
   uint8_t hmac[20];

   gettimeofday(&t1, &tz);
   for (i=0; i<4; i++) {
      gettimeofday(&t2, &tz);
   }
   srandom((t1.tv_sec&0xffff) * (t2.tv_usec&0xff));
   if (!gethostname(host, sizeof(host))) {
      bstrncpy(host, my_name, sizeof(host));
   }
   bsnprintf(chal, sizeof(chal), "<%u.%u@%s>", (uint32_t)random(), (uint32_t)time(NULL), host);
   Dmsg2(50, "send: auth cram-md5 %s ssl=%d\n", chal, ssl_need);
   if (!bnet_fsend(bs, "auth cram-md5 %s ssl=%d\n", chal, ssl_need)) {
      Dmsg1(50, "Bnet send challenge error.\n", bnet_strerror(bs));
      return 0;
   }

   if (!bnet_ssl_client(bs, password, ssl_need)) {
      return 0;
   }
   if (bnet_wait_data(bs, 180) <= 0 || bnet_recv(bs) <= 0) {
      Dmsg1(50, "Bnet receive challenge response error.\n", bnet_strerror(bs));
      bmicrosleep(5, 0);
      return 0;
   }
   hmac_md5((uint8_t *)chal, strlen(chal), (uint8_t *)password, strlen(password), hmac);
   bin_to_base64(host, (char *)hmac, 16);
   ok = strcmp(bs->msg, host) == 0;
   if (ok) {
      Dmsg1(50, "Authenticate OK %s\n", host);
   } else {
      Dmsg2(50, "Authenticate NOT OK: wanted %s, got %s\n", host, bs->msg);
   }
   if (ok) {
      bnet_fsend(bs, "1000 OK auth\n");
   } else {
      Dmsg1(50, "Auth failed PW: %s\n", password);
      bnet_fsend(bs, "1999 Authorization failed.\n");
      bmicrosleep(5, 0);
   }
   return ok;
}

/* Get authorization from other end */
int cram_md5_get_auth(BSOCK *bs, char *password, int ssl_need)
{
   char chal[MAXSTRING];
   uint8_t hmac[20];
   int ssl_has; 		      /* This is what the other end has */

   if (bnet_recv(bs) <= 0) {
      bmicrosleep(5, 0);
      return 0;
   }
   if (bs->msglen >= MAXSTRING) {
      Dmsg1(50, "Msg too long wanted auth cram... Got: %s", bs->msg);
      bmicrosleep(5, 0);
      return 0;
   }
   Dmsg1(100, "cram-get: %s", bs->msg);
   if (sscanf(bs->msg, "auth cram-md5 %s ssl=%d\n", chal, &ssl_has) != 2) {
      ssl_has = BNET_SSL_NONE;
      if (sscanf(bs->msg, "auth cram-md5 %s\n", chal) != 1) {
         Dmsg1(50, "Cannot scan challenge: %s", bs->msg);
         bnet_fsend(bs, "1999 Authorization failed.\n");
	 bmicrosleep(5, 0);
	 return 0;
      }
   }
   if (!bnet_ssl_server(bs, password, ssl_need, ssl_has)) {
      return 0;
   }

   hmac_md5((uint8_t *)chal, strlen(chal), (uint8_t *)password, strlen(password), hmac);
   bs->msglen = bin_to_base64(bs->msg, (char *)hmac, 16) + 1;
   if (!bnet_send(bs)) {
      Dmsg1(50, "Send challenge failed. ERR=%s\n", bnet_strerror(bs));
      return 0;
   }
   Dmsg1(99, "sending resp to challenge: %s\n", bs->msg);
   if (bnet_wait_data(bs, 180) <= 0 || bnet_recv(bs) <= 0) {
      Dmsg1(50, "Receive chanllenge response failed. ERR=%s\n", bnet_strerror(bs));
      bmicrosleep(5, 0);
      return 0;
   }
   if (strcmp(bs->msg, "1000 OK auth\n") == 0) {
      return 1;
   }
   Dmsg1(50, "Bad auth response: %s\n", bs->msg);
   bmicrosleep(5, 0);
   return 0;
}
