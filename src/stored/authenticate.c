/*
 * Authenticate caller
 *
 *   Kern Sibbald, October 2000
 *
 *   Version $Id: authenticate.c,v 1.24 2005/08/10 16:35:37 nboichat Exp $
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
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   bool auth_success = false;
#ifdef HAVE_TLS
   alist *verify_list = NULL;
#endif

   if (rcode != R_DIRECTOR) {
      Dmsg1(50, "I only authenticate Directors, not %d\n", rcode);
      Emsg1(M_FATAL, 0, _("I only authenticate Directors, not %d\n"), rcode);
      return 0;
   }
   if (bs->msglen < 25 || bs->msglen > 200) {
      Dmsg2(50, "Bad Hello command from Director at %s. Len=%d.\n",
            bs->who, bs->msglen);
      Emsg2(M_FATAL, 0, _("Bad Hello command from Director at %s. Len=%d.\n"),
            bs->who, bs->msglen);
      return 0;
   }
   dirname = get_pool_memory(PM_MESSAGE);
   dirname = check_pool_memory_size(dirname, bs->msglen);

   if (sscanf(bs->msg, "Hello Director %127s calling\n", dirname) != 1) {
      bs->msg[100] = 0;
      Dmsg2(50, "Bad Hello command from Director at %s: %s\n",
            bs->who, bs->msg);
      Emsg2(M_FATAL, 0, _("Bad Hello command from Director at %s: %s\n"),
            bs->who, bs->msg);
      return 0;
   }
   director = NULL;
   unbash_spaces(dirname);
// LockRes();
   foreach_res(director, rcode) {
      if (strcmp(director->hdr.name, dirname) == 0)
         break;
   }
// UnlockRes();
   if (!director) {
      Dmsg2(50, "Connection from unknown Director %s at %s rejected.\n",
            dirname, bs->who);
      Emsg2(M_FATAL, 0, _("Connection from unknown Director %s at %s rejected.\n"
       "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"),
            dirname, bs->who);
      free_pool_memory(dirname);
      return 0;
   }

#ifdef HAVE_TLS
   /* TLS Requirement */
   if (director->tls_enable) {
      if (director->tls_require) {
         tls_local_need = BNET_TLS_REQUIRED;
      } else {
         tls_local_need = BNET_TLS_OK;
      }
   }

   if (director->tls_verify_peer) {
      verify_list = director->tls_allowed_cns;
   }
#endif /* HAVE_TLS */

   /* Timeout Hello after 10 mins */
   btimer_t *tid = start_bsock_timer(bs, AUTH_TIMEOUT);
   auth_success = cram_md5_auth(bs, director->password, tls_local_need);
   if (auth_success) {
      auth_success = cram_md5_get_auth(bs, director->password, &tls_remote_need);
      if (!auth_success) {
         Dmsg1(50, "cram_get_auth failed with %s\n", bs->who);
      }
   } else {
      Dmsg1(50, "cram_auth failed with %s\n", bs->who);
   }

   if (!auth_success) {
      Emsg0(M_FATAL, 0, _("Incorrect password given by Director.\n"
       "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"));
      auth_success = false;
      goto auth_fatal;
   }

   /* Verify that the remote host is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Emsg0(M_FATAL, 0, _("Authorization problem: Remote server did not" 
           " advertise required TLS support.\n"));
      auth_success = false;
      goto auth_fatal;
   }

   /* Verify that we are willing to meet the remote host's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Emsg0(M_FATAL, 0, _("Authorization problem: Remote server requires TLS.\n"));
      auth_success = false;
      goto auth_fatal;
   }

#ifdef HAVE_TLS
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_server(director->tls_ctx, bs, verify_list)) {
         Emsg0(M_FATAL, 0, _("TLS negotiation failed.\n"));
         auth_success = false;
         goto auth_fatal;
      }
   }
#endif /* HAVE_TLS */

auth_fatal:
   stop_bsock_timer(tid);
   free_pool_memory(dirname);
   jcr->director = director;
   return auth_success;
}

/*
 * Inititiate the message channel with the Director.
 * He has made a connection to our server.
 *
 * Basic tasks done here:
 *   Assume the Hello message is already in the input
 *     buffer.  We then authenticate him.
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
      Dmsg1(50, "Unable to authenticate Director at %s.\n", dir->who);
      Emsg1(M_ERROR, 0, _("Unable to authenticate Director at %s.\n"), dir->who);
      bmicrosleep(5, 0);
      return 0;
   }
   return bnet_fsend(dir, "%s", OK_hello);
}

int authenticate_filed(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   bool auth_success = false;
#ifdef HAVE_TLS
   alist *verify_list = NULL;
#endif

#ifdef HAVE_TLS
   /* TLS Requirement */
   if (me->tls_enable) {
      if (me->tls_require) {
         tls_local_need = BNET_TLS_REQUIRED;
      } else {
         tls_local_need = BNET_TLS_OK;
      }
   }

   if (me->tls_verify_peer) {
      verify_list = me->tls_allowed_cns;
   }
#endif /* HAVE_TLS */

   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(fd, AUTH_TIMEOUT);
   auth_success = cram_md5_auth(fd, jcr->sd_auth_key, tls_local_need);
   if (auth_success) {
       auth_success = cram_md5_get_auth(fd, jcr->sd_auth_key, &tls_remote_need);
       if (!auth_success) {
          Dmsg1(50, "cram-get-auth failed with %s\n", fd->who);
       }
   } else {
      Dmsg1(50, "cram-auth failed with %s\n", fd->who);
   }

   if (!auth_success) {
      Jmsg(jcr, M_FATAL, 0, _("Incorrect authorization key from File daemon at %s rejected.\n"
       "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"),
           fd->who);
      auth_success = false;
      goto auth_fatal;
   }

   /* Verify that the remote host is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: Remote server did not" 
           " advertise required TLS support.\n"));
      auth_success = false;
      goto auth_fatal;
   }

   /* Verify that we are willing to meet the remote host's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: Remote server requires TLS.\n"));
      auth_success = false;
      goto auth_fatal;
   }

#ifdef HAVE_TLS
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_server(me->tls_ctx, fd, verify_list)) {
         Jmsg(jcr, M_FATAL, 0, _("TLS negotiation failed.\n"));
         auth_success = false;
         goto auth_fatal;
      }
   }
#endif /* HAVE_TLS */

auth_fatal:
   stop_bsock_timer(tid);
   if (!auth_success) {
      Jmsg(jcr, M_FATAL, 0, _("Incorrect authorization key from File daemon at %s rejected.\n"
       "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"),
           fd->who);
   }
   jcr->authenticated = auth_success;
   return auth_success;
}
