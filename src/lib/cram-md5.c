/*
 *  Challenge Response Authentication Method using MD5 (CRAM-MD5)
 *
 * cram-md5 is based on RFC2104.
 *
 * Written for Bacula by Kern E. Sibbald, May MMI.
 *
 *   Version $Id: cram-md5.c,v 1.25 2006/11/21 16:13:57 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#include "bacula.h"

/* Authorize other end
 * Codes that tls_local_need and tls_remote_need can take:
 *   BNET_TLS_NONE     I cannot do tls
 *   BNET_TLS_OK       I can do tls, but it is not required on my end
 *   BNET_TLS_REQUIRED  tls is required on my end
 *
 *   Returns: false if authentication failed
 *            true if OK
 */
bool cram_md5_challenge(BSOCK *bs, char *password, int tls_local_need, int compatible)
{
   struct timeval t1;
   struct timeval t2;
   struct timezone tz;
   int i;
   bool ok;
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
   /* Send challenge -- no hashing yet */
   bsnprintf(chal, sizeof(chal), "<%u.%u@%s>", (uint32_t)random(), (uint32_t)time(NULL), host);
   if (compatible) {
      Dmsg2(50, "send: auth cram-md5 %s ssl=%d\n", chal, tls_local_need);
      if (!bnet_fsend(bs, "auth cram-md5 %s ssl=%d\n", chal, tls_local_need)) {
         Dmsg1(50, "Bnet send challenge error.\n", bnet_strerror(bs));
         return false;
      }
   } else {
      /* Old non-compatible system */
      Dmsg2(50, "send: auth cram-md5 %s ssl=%d\n", chal, tls_local_need);
      if (!bnet_fsend(bs, "auth cram-md5 %s ssl=%d\n", chal, tls_local_need)) {
         Dmsg1(50, "Bnet send challenge error.\n", bnet_strerror(bs));
         return false;
      }
   }

   /* Read hashed response to challenge */
   if (bnet_wait_data(bs, 180) <= 0 || bnet_recv(bs) <= 0) {
      Dmsg1(50, "Bnet receive challenge response error.\n", bnet_strerror(bs));
      bmicrosleep(5, 0);
      return false;
   }

   /* Attempt to duplicate hash with our password */
   hmac_md5((uint8_t *)chal, strlen(chal), (uint8_t *)password, strlen(password), hmac);
   bin_to_base64(host, sizeof(host), (char *)hmac, 16, compatible);
   ok = strcmp(bs->msg, host) == 0;
   if (ok) {
      Dmsg1(50, "Authenticate OK %s\n", host);
   } else {
      bin_to_base64(host, sizeof(host), (char *)hmac, 16, false);     
      ok = strcmp(bs->msg, host) == 0;
      if (!ok) {
         Dmsg2(50, "Authenticate NOT OK: wanted %s, got %s\n", host, bs->msg);
      }
   }
   if (ok) {
      bnet_fsend(bs, "1000 OK auth\n");
   } else {
      Dmsg1(50, "Auth failed PW: %s\n", password);
      bnet_fsend(bs, _("1999 Authorization failed.\n"));
      bmicrosleep(5, 0);
   }
   return ok;
}

/* Respond to challenge from other end */
bool cram_md5_respond(BSOCK *bs, char *password, int *tls_remote_need, int *compatible)
{
   char chal[MAXSTRING];
   uint8_t hmac[20];

   *compatible = false;
   if (bnet_recv(bs) <= 0) {
      bmicrosleep(5, 0);
      return false;
   }
   if (bs->msglen >= MAXSTRING) {
      Dmsg1(50, "Msg too long wanted auth cram... Got: %s", bs->msg);
      bmicrosleep(5, 0);
      return false;
   }
   Dmsg1(100, "cram-get: %s", bs->msg);
   if (sscanf(bs->msg, "auth cram-md5c %s ssl=%d", chal, tls_remote_need) == 2) {
      *compatible = true;
   } else if (sscanf(bs->msg, "auth cram-md5 %s ssl=%d", chal, tls_remote_need) != 2) {
      if (sscanf(bs->msg, "auth cram-md5 %s\n", chal) != 1) {
         Dmsg1(50, "Cannot scan challenge: %s", bs->msg);
         bnet_fsend(bs, _("1999 Authorization failed.\n"));
         bmicrosleep(5, 0);
         return false;
      }
   }

   hmac_md5((uint8_t *)chal, strlen(chal), (uint8_t *)password, strlen(password), hmac);
   bs->msglen = bin_to_base64(bs->msg, 50, (char *)hmac, 16, *compatible) + 1;
// Dmsg3(100, "get_auth: chal=%s pw=%s hmac=%s\n", chal, password, bs->msg);
   if (!bnet_send(bs)) {
      Dmsg1(50, "Send challenge failed. ERR=%s\n", bnet_strerror(bs));
      return false;
   }
   Dmsg1(99, "sending resp to challenge: %s\n", bs->msg);
   if (bnet_wait_data(bs, 180) <= 0 || bnet_recv(bs) <= 0) {
      Dmsg1(50, "Receive chanllenge response failed. ERR=%s\n", bnet_strerror(bs));
      bmicrosleep(5, 0);
      return false;
   }
   if (strcmp(bs->msg, "1000 OK auth\n") == 0) {
      return true;
   }
   Dmsg1(50, "Bad auth response: %s\n", bs->msg);
   bmicrosleep(5, 0);
   return false;
}
