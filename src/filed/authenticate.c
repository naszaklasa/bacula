/*
 * Authenticate Director who is attempting to connect.
 *
 *   Kern Sibbald, October 2000
 *
 *   Version $Id: authenticate.c,v 1.18 2004/11/21 17:53:01 kerns Exp $
 * 
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
  
#include "bacula.h"
#include "filed.h"

static char OK_hello[]  = "2000 OK Hello\n";
static char Dir_sorry[] = "2999 No go\n";


/********************************************************************* 
 *
 */
static int authenticate(int rcode, BSOCK *bs, JCR* jcr)
{
   POOLMEM *dirname;
   DIRRES *director;
   int ssl_need = BNET_SSL_NONE;
   bool auth, get_auth = false;

   if (rcode != R_DIRECTOR) {
      Dmsg1(50, _("I only authenticate directors, not %d\n"), rcode);
      Emsg1(M_FATAL, 0, _("I only authenticate directors, not %d\n"), rcode);
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

   if (sscanf(bs->msg, "Hello Director %s calling\n", dirname) != 1) {
      free_pool_memory(dirname);
      bs->msg[100] = 0;
      Dmsg2(50, _("Bad Hello command from Director at %s: %s\n"), 
	    bs->who, bs->msg);
      Emsg2(M_FATAL, 0, _("Bad Hello command from Director at %s: %s\n"), 
	    bs->who, bs->msg);
      return 0;
   }
   unbash_spaces(dirname);
   LockRes();
   foreach_res(director, R_DIRECTOR) {
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
   btimer_t *tid = start_bsock_timer(bs, AUTH_TIMEOUT);
   auth = cram_md5_auth(bs, director->password, ssl_need);  
   if (auth) {
      get_auth = cram_md5_get_auth(bs, director->password, ssl_need);  
      if (!get_auth) {
         Dmsg1(50, "cram_get_auth failed for %s\n", bs->who);
      }
   } else {
      Dmsg1(50, "cram_auth failed for %s\n", bs->who);
   }
   if (!auth || !get_auth) {
      Emsg1(M_FATAL, 0, _("Incorrect password given by Director at %s.\n"  
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"), 
	    bs->who);
      director = NULL;
   }
   stop_bsock_timer(tid);
   free_pool_memory(dirname);
   jcr->director = director;
   return (director != NULL);
}

/*
 * Inititiate the communications with the Director.
 * He has made a connection to our server.
 * 
 * Basic tasks done here:
 *   We read Director's initial message and authorize him.
 *
 */
int authenticate_director(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;

   if (!authenticate(R_DIRECTOR, dir, jcr)) {
      bnet_fsend(dir, "%s", Dir_sorry);
      Emsg0(M_FATAL, 0, _("Unable to authenticate Director\n"));
      bmicrosleep(5, 0);
      return 0;
   }
   return bnet_fsend(dir, "%s", OK_hello);
}

/*
 * First prove our identity to the Storage daemon, then
 * make him prove his identity.
 */
int authenticate_storagedaemon(JCR *jcr)
{
   BSOCK *sd = jcr->store_bsock;
   int ssl_need = BNET_SSL_NONE;
   bool get_auth, auth = false;

   btimer_t *tid = start_bsock_timer(sd, AUTH_TIMEOUT);
   get_auth = cram_md5_get_auth(sd, jcr->sd_auth_key, ssl_need);  
   if (!get_auth) {
      Dmsg1(50, "cram_get_auth failed for %s\n", sd->who);
   } else {
      auth = cram_md5_auth(sd, jcr->sd_auth_key, ssl_need);
      if (!auth) {
         Dmsg1(50, "cram_auth failed for %s\n", sd->who);
      }
   }
   stop_bsock_timer(tid);
   memset(jcr->sd_auth_key, 0, strlen(jcr->sd_auth_key));
   if (!get_auth || !auth) {
      Jmsg(jcr, M_FATAL, 0, _("Authorization key rejected by Storage daemon.\n"   
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"));
   }
   return get_auth && auth;
}
