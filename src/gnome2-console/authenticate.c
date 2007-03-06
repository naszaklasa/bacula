/*
 *
 *   Bacula UA authentication. Provides authentication with
 *     the Director.
 *
 *     Kern Sibbald, June MMI
 *
 *     Version $Id: authenticate.c,v 1.7 2004/08/05 11:51:53 kerns Exp $
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *
 */
/*
   Copyright (C) 2000, 2001 Kern Sibbald and John Walker

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
#include "console.h"


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
      printf(_("%s: Director authorization problem.\n"), my_name);
      set_text(_("Director authorization problem.\n"), -1);
      set_text(_(
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"), 
	-1);
      return 0;
   }

   Dmsg1(6, ">dird: %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      stop_bsock_timer(tid);
      set_textf(_("Bad response to Hello command: ERR=%s\n"),
	 bnet_strerror(dir));
      printf(_("%s: Bad response to Hello command: ERR=%s\n"),
	 my_name, bnet_strerror(dir));
      set_text(_("The Director is probably not running.\n"), -1);
      return 0;
   }
  stop_bsock_timer(tid);
   Dmsg1(10, "<dird: %s", dir->msg);
   if (strncmp(dir->msg, OKhello, sizeof(OKhello)-1) != 0) {
      set_text(_("Director rejected Hello command\n"), -1);
      return 0;
   } else {
      set_text(dir->msg, dir->msglen);
   }
   return 1;
}
