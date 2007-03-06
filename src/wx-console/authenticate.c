/*
 *
 *   Bacula UA authentication. Provides authentication with
 *     the Director.
 *
 *     Kern Sibbald, June MMI
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *
 */
/*
   Copyright (C) 2001-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "bacula.h"
#include "console_conf.h"
#include "jcr.h"

#include "csprint.h"

void senditf(char *fmt, ...);
void sendit(char *buf);

/* Commands sent to Director */
static char hello[]    = "Hello %s calling\n";

/* Response from Director */
static char OKhello[]   = "1000 OK:";

/* Forward referenced functions */

/*
 * Authenticate Director
 */
int authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons)
{
   BSOCK *dir = jcr->dir_bsock;
   int ssl_need = BNET_SSL_NONE;
   char bashed_name[MAX_NAME_LENGTH];
   char *password;

   /*
    * Send my name to the Director then do authentication
    */
   if (cons) {
      bstrncpy(bashed_name, cons->hdr.name, sizeof(bashed_name));
      bash_spaces(bashed_name);
      password = cons->password;
   } else {
      bstrncpy(bashed_name, "*UserAgent*", sizeof(bashed_name));
      password = director->password;
   }
   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(dir, 60 * 5);
   bnet_fsend(dir, hello, bashed_name);

   if (!cram_md5_get_auth(dir, password, ssl_need) ||
       !cram_md5_auth(dir, password, ssl_need)) {
      stop_bsock_timer(tid);
      csprint("Director authorization problem.\nMost likely the passwords do not agree.\n", CS_DATA);
      csprint(
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n", CS_DATA);
      return 0;
   }

   Dmsg1(6, ">dird: %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      stop_bsock_timer(tid);
      csprint("Bad response to Hello command: ERR=", CS_DATA);
      csprint(bnet_strerror(dir), CS_DATA);
      csprint("\n", CS_DATA);
      return 0;
   }
   Dmsg1(10, "<dird: %s", dir->msg);
   stop_bsock_timer(tid);
   if (strncmp(dir->msg, OKhello, sizeof(OKhello)-1) != 0) {
      csprint("Director rejected Hello command\n", CS_DATA);
      return 0;
   } else {
      csprint(dir->msg, CS_DATA);
   }
   return 1;
}

