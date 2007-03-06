/*
 * Authenticate caller
 *
 *   Kern Sibbald, October 2000
 *
 *   Version $Id: authenticate.c,v 1.18.2.1 2005/02/15 11:51:04 kerns Exp $
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

#include "bacula.h"
#include "stored.h"

static char Dir_sorry[] = "3999 No go\n";
static char OK_hello[]  = "3000 OK Hello\n";


/*********************************************************************
 *
 *
 */
static int authenticate(int rcode, BSOCK *bs, JCR* jcr)
{
   POOLMEM *dirname;
   DIRRES *director = NULL;
   int ssl_need = BNET_SSL_NONE;
   bool auth, get_auth = false;

   if (rcode != R_DIRECTOR) {
      Dmsg1(50, _("I only authenticate Directors, not %d\n"), rcode);
      Emsg1(M_FATAL, 0, _("I only authenticate Directors, not %d\n"), rcode);
      return 0;
   }
   if (bs->msglen < 25 || bs->msglen > 200) {
      Dmsg2(50, _("Bad Hello command from Director at %s. Len=%d.\n"),
	    bs->who, bs->msglen);
      Emsg2(M_FATAL, 0, _("Bad Hello command from Director at %s. Len=%d.\n"),
	    bs->who, bs->msglen);
      return 0;
   }
   dirname = get_pool_memory(PM_MESSAGE);
   dirname = check_pool_memory_size(dirname, bs->msglen);

   if (sscanf(bs->msg, "Hello Director %127s calling\n", dirname) != 1) {
      bs->msg[100] = 0;
      Dmsg2(50, _("Bad Hello command from Director at %s: %s\n"),
	    bs->who, bs->msg);
      Emsg2(M_FATAL, 0, _("Bad Hello command from Director at %s: %s\n"),
	    bs->who, bs->msg);
      return 0;
   }
   director = NULL;
   unbash_spaces(dirname);
   LockRes();
   foreach_res(director, rcode) {
      if (strcmp(director->hdr.name, dirname) == 0)
	 break;
   }
   UnlockRes();
   if (!director) {
      Dmsg2(50, _("Connection from unknown Director %s at %s rejected.\n"),
	    dirname, bs->who);
      Emsg2(M_FATAL, 0, _("Connection from unknown Director %s at %s rejected.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"),
	    dirname, bs->who);
      free_pool_memory(dirname);
      return 0;
   }

   /* Timeout Hello after 10 mins */
   btimer_t *tid = start_bsock_timer(bs, AUTH_TIMEOUT);
   auth = cram_md5_auth(bs, director->password, ssl_need);
   if (auth) {
      get_auth = cram_md5_get_auth(bs, director->password, ssl_need);
      if (!get_auth) {
	 Dmsg1(50, "cram_get_auth failed with %s\n", bs->who);
      }
   } else {
      Dmsg1(50, "cram_auth failed with %s\n", bs->who);
   }
   if (!auth || !get_auth) {
      stop_bsock_timer(tid);
      Emsg0(M_FATAL, 0, _("Incorrect password given by Director.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"));
      free_pool_memory(dirname);
      return 0;
   }
   stop_bsock_timer(tid);
   free_pool_memory(dirname);
   jcr->director = director;
   return 1;
}

/*
 * Inititiate the message channel with the Director.
 * He has made a connection to our server.
 *
 * Basic tasks done here:
 *   Assume the Hello message is already in the input
 *     buffer.	We then authenticate him.
 *   Get device, media, and pool information from Director
 *
 *   This is the channel across which we will send error
 *     messages and job status information.
 */
int authenticate_director(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;

   if (!authenticate(R_DIRECTOR, dir, jcr)) {
      bnet_fsend(dir, "%s", Dir_sorry);
      Dmsg1(50, _("Unable to authenticate Director at %s.\n"), dir->who);
      Emsg1(M_ERROR, 0, _("Unable to authenticate Director at %s.\n"), dir->who);
      bmicrosleep(5, 0);
      return 0;
   }
   return bnet_fsend(dir, "%s", OK_hello);
}

int authenticate_filed(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   int ssl_need = BNET_SSL_NONE;
   bool auth, get_auth = false;

   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(fd, AUTH_TIMEOUT);
   auth = cram_md5_auth(fd, jcr->sd_auth_key, ssl_need);
   if (auth) {
       get_auth = cram_md5_get_auth(fd, jcr->sd_auth_key, ssl_need);
       if (!get_auth) {
	  Dmsg1(50, "cram-get-auth failed with %s\n", fd->who);
       }
   } else {
      Dmsg1(50, "cram-auth failed with %s\n", fd->who);
   }
   if (auth && get_auth) {
      jcr->authenticated = true;
   }
   stop_bsock_timer(tid);
   if (!jcr->authenticated) {
      Jmsg(jcr, M_FATAL, 0, _("Incorrect authorization key from File daemon at %s rejected.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"),
	   fd->who);
   }
   return jcr->authenticated;
}
