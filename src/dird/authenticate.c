/*
 *
 *   Bacula Director -- authorize.c -- handles authorization of
 *     Storage and File daemons.
 *
 *     Kern Sibbald, May MMI
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *   Version $Id: authenticate.c,v 1.42 2006/11/21 13:20:08 kerns Exp $
 *
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
#include "dird.h"

extern DIRRES *director;

/* Commands sent to Storage daemon and File daemon and received
 *  from the User Agent */
static char hello[]    = "Hello Director %s calling\n";

/* Response from Storage daemon */
static char OKhello[]   = "3000 OK Hello\n";
static char FDOKhello[] = "2000 OK Hello\n";

/* Sent to User Agent */
static char Dir_sorry[]  = "1999 You are not authorized.\n";

/* Forward referenced functions */

/*
 * Authenticate Storage daemon connection
 */
bool authenticate_storage_daemon(JCR *jcr, STORE *store)
{
   BSOCK *sd = jcr->store_bsock;
   char dirname[MAX_NAME_LENGTH];
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   int compatible = true;
   bool auth_success = false;

   /*
    * Send my name to the Storage daemon then do authentication
    */
   bstrncpy(dirname, director->hdr.name, sizeof(dirname));
   bash_spaces(dirname);
   /* Timeout Hello after 1 min */
   btimer_t *tid = start_bsock_timer(sd, AUTH_TIMEOUT);
   if (!bnet_fsend(sd, hello, dirname)) {
      stop_bsock_timer(tid);
      Dmsg1(50, _("Error sending Hello to Storage daemon. ERR=%s\n"), bnet_strerror(sd));
      Jmsg(jcr, M_FATAL, 0, _("Error sending Hello to Storage daemon. ERR=%s\n"), bnet_strerror(sd));
      return 0;
   }

   /* TLS Requirement */
   if (store->tls_enable) {
     if (store->tls_require) {
        tls_local_need = BNET_TLS_REQUIRED;
     } else {
        tls_local_need = BNET_TLS_OK;
     }
   }

   auth_success = cram_md5_respond(sd, store->password, &tls_remote_need, &compatible);
   if (auth_success) {
      auth_success = cram_md5_challenge(sd, store->password, tls_local_need, compatible);
      if (!auth_success) {
         Dmsg1(50, "cram_challenge failed for %s\n", sd->who);
      }
   } else {
      Dmsg1(50, "cram_respond failed for %s\n", sd->who);
   }

   if (!auth_success) {
      stop_bsock_timer(tid);
      Dmsg0(50, _("Director and Storage daemon passwords or names not the same.\n"));
      Jmsg2(jcr, M_FATAL, 0,
            _("Director unable to authenticate with Storage daemon on \"%s:%d\". Possible causes:\n"
            "Passwords or names not the same or\n"
            "Maximum Concurrent Jobs exceeded on the SD or\n"
            "SD networking messed up (restart daemon).\n"
            "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"),
            sd->host, sd->port);
      return 0;
   }

   /* Verify that the remote host is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: Remote server did not advertise required TLS support.\n"));
      return 0;
   }

   /* Verify that we are willing to meet the remote host's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: Remote server requires TLS.\n"));
      return 0;
   }

   /* Is TLS Enabled? */
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_client(store->tls_ctx, sd)) {
         stop_bsock_timer(tid);
         Jmsg(jcr, M_FATAL, 0, _("TLS negotiation failed with SD on \"%s:%d\"\n"),
            sd->host, sd->port);
         return 0;
      }
   }

   Dmsg1(116, ">stored: %s", sd->msg);
   if (bnet_recv(sd) <= 0) {
      stop_bsock_timer(tid);
      Jmsg3(jcr, M_FATAL, 0, _("bdird<stored: \"%s:%s\" bad response to Hello command: ERR=%s\n"),
         sd->who, sd->host, bnet_strerror(sd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", sd->msg);
   stop_bsock_timer(tid);
   if (strncmp(sd->msg, OKhello, sizeof(OKhello)) != 0) {
      Dmsg0(50, _("Storage daemon rejected Hello command\n"));
      Jmsg2(jcr, M_FATAL, 0, _("Storage daemon on \"%s:%d\" rejected Hello command\n"),
         sd->host, sd->port);
      return 0;
   }
   return 1;
}

/*
 * Authenticate File daemon connection
 */
int authenticate_file_daemon(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   CLIENT *client = jcr->client;
   char dirname[MAX_NAME_LENGTH];
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   int compatible = true;
   bool auth_success = false;

   /*
    * Send my name to the File daemon then do authentication
    */
   bstrncpy(dirname, director->hdr.name, sizeof(dirname));
   bash_spaces(dirname);
   /* Timeout Hello after 10 mins */
   btimer_t *tid = start_bsock_timer(fd, AUTH_TIMEOUT);
   if (!bnet_fsend(fd, hello, dirname)) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Error sending Hello to File daemon on \"%s:%d\". ERR=%s\n"), 
           fd->host, fd->port, bnet_strerror(fd));
      return 0;
   }
   Dmsg1(50, "Sent: %s", fd->msg);

   /* TLS Requirement */
   if (client->tls_enable) {
     if (client->tls_require) {
        tls_local_need = BNET_TLS_REQUIRED;
     } else {
        tls_local_need = BNET_TLS_OK;
     }
   }

   auth_success = cram_md5_respond(fd, client->password, &tls_remote_need, &compatible);
   if (auth_success) {
      auth_success = cram_md5_challenge(fd, client->password, tls_local_need, compatible);
      if (!auth_success) {
         Dmsg1(50, "cram_auth failed for %s\n", fd->who);
      }
   } else {
      Dmsg1(50, "cram_get_auth failed for %s\n", fd->who);
   }
   if (!auth_success) {
      stop_bsock_timer(tid);
      Dmsg0(50, _("Director and File daemon passwords or names not the same.\n"));
      Jmsg(jcr, M_FATAL, 0,
            _("Unable to authenticate with File daemon on \"%s:%d\". Possible causes:\n"
            "Passwords or names not the same or\n"
            "Maximum Concurrent Jobs exceeded on the FD or\n"
            "FD networking messed up (restart daemon).\n"
            "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"),
            fd->host, fd->port);
      return 0;
   }

   /* Verify that the remote host is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: FD \"%s:%s\" did not advertise required TLS support.\n"),
           fd->who, fd->host);
      return 0;
   }

   /* Verify that we are willing to meet the remote host's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Authorization problem: FD on \"%s:%d\" requires TLS.\n"),
           fd->host, fd->port);
      return 0;
   }

   /* Is TLS Enabled? */
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_client(client->tls_ctx, fd)) {
         stop_bsock_timer(tid);
         Jmsg(jcr, M_FATAL, 0, _("TLS negotiation failed with FD on \"%s:%d\".\n"),
              fd->host, fd->port);
         return 0;
      }
   }

   Dmsg1(116, ">filed: %s", fd->msg);
   if (bnet_recv(fd) <= 0) {
      stop_bsock_timer(tid);
      Dmsg1(50, _("Bad response from File daemon to Hello command: ERR=%s\n"),
         bnet_strerror(fd));
      Jmsg(jcr, M_FATAL, 0, _("Bad response from File daemon on \"%s:%d\" to Hello command: ERR=%s\n"),
         fd->host, fd->port, bnet_strerror(fd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", fd->msg);
   stop_bsock_timer(tid);
   if (strncmp(fd->msg, FDOKhello, sizeof(FDOKhello)) != 0) {
      Dmsg0(50, _("File daemon rejected Hello command\n"));
      Jmsg(jcr, M_FATAL, 0, _("File daemon on \"%s:%d\" rejected Hello command\n"),
           fd->host, fd->port);
      return 0;
   }
   return 1;
}

/*********************************************************************
 *
 */
int authenticate_user_agent(UAContext *uac)
{
   char name[MAX_NAME_LENGTH];
   int tls_local_need = BNET_TLS_NONE;
   int tls_remote_need = BNET_TLS_NONE;
   int compatible = true;
   CONRES *cons = NULL;
   BSOCK *ua = uac->UA_sock;
   bool auth_success = false;
   TLS_CONTEXT *tls_ctx = NULL;
   alist *verify_list = NULL;
 

//  Emsg4(M_INFO, 0, _("UA Hello from %s:%s:%d is invalid. Len=%d\n"), ua->who,
//          ua->host, ua->port, ua->msglen);
   if (ua->msglen < 16 || ua->msglen >= MAX_NAME_LENGTH + 15) {
      Emsg4(M_ERROR, 0, _("UA Hello from %s:%s:%d is invalid. Len=%d\n"), ua->who,
            ua->host, ua->port, ua->msglen);
      return 0;
   }

   if (sscanf(ua->msg, "Hello %127s calling\n", name) != 1) {
      ua->msg[100] = 0;               /* terminate string */
      Emsg4(M_ERROR, 0, _("UA Hello from %s:%s:%d is invalid. Got: %s\n"), ua->who,
            ua->host, ua->port, ua->msg);
      return 0;
   }

   name[sizeof(name)-1] = 0;             /* terminate name */
   if (strcmp(name, "*UserAgent*") == 0) {  /* default console */
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

      auth_success = cram_md5_challenge(ua, director->password, tls_local_need,
                                        compatible) &&
                     cram_md5_respond(ua, director->password, &tls_remote_need, &compatible);
   } else {
      unbash_spaces(name);
      cons = (CONRES *)GetResWithName(R_CONSOLE, name);
      if (cons) {
         /* TLS Requirement */
         if (cons->tls_enable) {
            if (cons->tls_require) {
               tls_local_need = BNET_TLS_REQUIRED;
            } else {
               tls_local_need = BNET_TLS_OK;
            }
         }

         if (cons->tls_verify_peer) {
            verify_list = cons->tls_allowed_cns;
         }

         auth_success = cram_md5_challenge(ua, cons->password, tls_local_need,
                                           compatible) &&
                     cram_md5_respond(ua, cons->password, &tls_remote_need, &compatible);

         if (auth_success) {
            uac->cons = cons;         /* save console resource pointer */
         }
      } else {
         auth_success = false;
         goto auth_done;
      }
   }

   /* Verify that the remote peer is willing to meet our TLS requirements */
   if (tls_remote_need < tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Emsg0(M_FATAL, 0, _("Authorization problem:"
            " Remote client did not advertise required TLS support.\n"));
      auth_success = false;
      goto auth_done;
   }

   /* Verify that we are willing to meet the peer's requirements */
   if (tls_remote_need > tls_local_need && tls_local_need != BNET_TLS_OK && tls_remote_need != BNET_TLS_OK) {
      Emsg0(M_FATAL, 0, _("Authorization problem:"
            " Remote client requires TLS.\n"));
      auth_success = false;
      goto auth_done;
   }

   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      if (cons) {
         tls_ctx = cons->tls_ctx;
      } else {
         tls_ctx = director->tls_ctx;
      }

      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_server(tls_ctx, ua, verify_list)) {
         Emsg0(M_ERROR, 0, _("TLS negotiation failed.\n"));
         auth_success = false;
         goto auth_done;
      }
   }


/* Authorization Completed */
auth_done:
   if (!auth_success) {
      bnet_fsend(ua, "%s", _(Dir_sorry));
      Emsg4(M_ERROR, 0, _("Unable to authenticate console \"%s\" at %s:%s:%d.\n"),
            name, ua->who, ua->host, ua->port);
      sleep(5);
      return 0;
   }
   bnet_fsend(ua, _("1000 OK: %s Version: %s (%s)\n"), my_name, VERSION, BDATE);
   return 1;
}
