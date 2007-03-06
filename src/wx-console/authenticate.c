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
   Copyright (C) 2001-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

/* _("...") macro returns a wxChar*, so if we need a char*, we need to convert it with:
 * wxString(_("...")).mb_str(*wxConvCurrent) */

#include <wx/intl.h>

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
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   char bashed_name[MAX_NAME_LENGTH];
   char *password;
   TLS_CONTEXT *tls_ctx = NULL;

   /*
    * Send my name to the Director then do authentication
    */
   if (cons) {
      bstrncpy(bashed_name, cons->hdr.name, sizeof(bashed_name));
      bash_spaces(bashed_name);
      password = cons->password;
      /* TLS Requirement */
      if (cons->tls_enable) {
	 if (cons->tls_require) {
	    tls_local_need = BNET_TLS_REQUIRED;
	 } else {
	    tls_local_need = BNET_TLS_OK;
	 }
      }

      tls_ctx = cons->tls_ctx;
   } else {
      bstrncpy(bashed_name, "*UserAgent*", sizeof(bashed_name));
      password = director->password;
      /* TLS Requirement */
      if (director->tls_enable) {
	 if (director->tls_require) {
	    tls_local_need = BNET_TLS_REQUIRED;
	 } else {
	    tls_local_need = BNET_TLS_OK;
	 }
      }

      tls_ctx = director->tls_ctx;
   }


   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(dir, 60 * 5);
   bnet_fsend(dir, hello, bashed_name);

   if (!cram_md5_get_auth(dir, password, &tls_remote_need) ||
       !cram_md5_auth(dir, password, tls_local_need)) {
      goto bail_out;
   }	   

   /* Verify that the remote host is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      csprint(_("Authorization problem: Remote server did not advertise required TLS support.\n"));
      goto bail_out;
   }

   /* Verify that we are willing to meet the remote host's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      csprint(_("Authorization problem: Remote server requires TLS.\n"));
      goto bail_out;
   }

   /* Is TLS Enabled? */
   if (have_tls) {
      if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
	 /* Engage TLS! Full Speed Ahead! */
	 if (!bnet_tls_client(tls_ctx, dir)) {
            csprint(_("TLS negotiation failed\n"));
	    goto bail_out;
	 }
      }
   }

   Dmsg1(6, ">dird: %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      csprint(_("Bad response to Hello command: ERR="), CS_DATA);
      csprint(bnet_strerror(dir), CS_DATA);
      csprint("\n", CS_DATA);
      goto bail_out;
   }
   Dmsg1(10, "<dird: %s", dir->msg);
   if (strncmp(dir->msg, OKhello, sizeof(OKhello)-1) != 0) {
      csprint(_("Director rejected Hello command\n"), CS_DATA);
      goto bail_out;
   } else {
      csprint(dir->msg, CS_DATA);
   }
   stop_bsock_timer(tid);
   return 1;

bail_out:
   stop_bsock_timer(tid);
   csprint( _("Director authorization problem.\nMost likely the passwords do not agree.\nIf you are using TLS, there may have been a certificate validation error during the TLS handshake.\nPlease see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"));
   return 0;

}
