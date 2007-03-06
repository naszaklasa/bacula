/*
 *
 *   Bacula Director -- authorize.c -- handles authorization of
 *     Storage and File daemons.
 *
 *     Kern Sibbald, May MMI
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *   Version $Id: authenticate.c,v 1.34.2.1 2005/11/04 09:16:49 kerns Exp $
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

#include "bacula.h"
#include "dird.h"

extern DIRRES *director;
extern char my_name[];

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

   auth_success = cram_md5_get_auth(sd, store->password, &tls_remote_need);
   if (auth_success) {
      auth_success = cram_md5_auth(sd, store->password, tls_local_need);
      if (!auth_success) {
         Dmsg1(50, "cram_auth failed for %s\n", sd->who);
      }
   } else {
      Dmsg1(50, "cram_get_auth failed for %s\n", sd->who);
   }

   if (!auth_success) {
      stop_bsock_timer(tid);
      Dmsg0(50, _("Director and Storage daemon passwords or names not the same.\n"));
      Jmsg0(jcr, M_FATAL, 0,
            _("Director unable to authenticate with Storage daemon. Possible causes:\n"
            "Passwords or names not the same or\n"
            "Maximum Concurrent Jobs exceeded on the SD or\n"
            "SD networking messed up (restart daemon).\n"
            "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"));
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

#ifdef HAVE_TLS
   /* Is TLS Enabled? */
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_client(store->tls_ctx, sd)) {
         stop_bsock_timer(tid);
         Jmsg(jcr, M_FATAL, 0, _("TLS negotiation failed.\n"));
         return 0;
      }
   }
#endif

   Dmsg1(116, ">stored: %s", sd->msg);
   if (bnet_recv(sd) <= 0) {
      stop_bsock_timer(tid);
      Jmsg1(jcr, M_FATAL, 0, _("bdird<stored: bad response to Hello command: ERR=%s\n"),
         bnet_strerror(sd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", sd->msg);
   stop_bsock_timer(tid);
   if (strncmp(sd->msg, OKhello, sizeof(OKhello)) != 0) {
      Dmsg0(50, _("Storage daemon rejected Hello command\n"));
      Jmsg0(jcr, M_FATAL, 0, _("Storage daemon rejected Hello command\n"));
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
      Jmsg(jcr, M_FATAL, 0, _("Error sending Hello to File daemon. ERR=%s\n"), bnet_strerror(fd));
      return 0;
   }

#ifdef HAVE_TLS
   /* TLS Requirement */
   if (client->tls_enable) {
     if (client->tls_require) {
        tls_local_need = BNET_TLS_REQUIRED;
     } else {
        tls_local_need = BNET_TLS_OK;
     }
   }
#endif

   auth_success = cram_md5_get_auth(fd, client->password, &tls_remote_need);
   if (auth_success) {
      auth_success = cram_md5_auth(fd, client->password, tls_local_need);
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
            _("Unable to authenticate with File daemon. Possible causes:\n"
            "Passwords or names not the same or\n"
            "Maximum Concurrent Jobs exceeded on the FD or\n"
            "FD networking messed up (restart daemon).\n"
            "Please see http://www.bacula.org/rel-manual/faq.html#AuthorizationErrors for help.\n"));
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

#ifdef HAVE_TLS
   /* Is TLS Enabled? */
   if (tls_local_need >= BNET_TLS_OK && tls_remote_need >= BNET_TLS_OK) {
      /* Engage TLS! Full Speed Ahead! */
      if (!bnet_tls_client(client->tls_ctx, fd)) {
         stop_bsock_timer(tid);
         Jmsg(jcr, M_FATAL, 0, _("TLS negotiation failed.\n"));
         return 0;
      }
   }
#endif

   Dmsg1(116, ">filed: %s", fd->msg);
   if (bnet_recv(fd) <= 0) {
      stop_bsock_timer(tid);
      Dmsg1(50, _("Bad response from File daemon to Hello command: ERR=%s\n"),
         bnet_strerror(fd));
      Jmsg(jcr, M_FATAL, 0, _("Bad response from File daemon to Hello command: ERR=%s\n"),
         bnet_strerror(fd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", fd->msg);
   stop_bsock_timer(tid);
   if (strncmp(fd->msg, FDOKhello, sizeof(FDOKhello)) != 0) {
      Dmsg0(50, _("File daemon rejected Hello command\n"));
      Jmsg(jcr, M_FATAL, 0, _("File daemon rejected Hello command\n"));
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
   CONRES *cons = NULL;
   BSOCK *ua = uac->UA_sock;
   bool auth_success = false;
#ifdef HAVE_TLS
   TLS_CONTEXT *tls_ctx = NULL;
   alist *verify_list = NULL;
#endif /* HAVE_TLS */
 

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

      auth_success = cram_md5_auth(ua, director->password, tls_local_need) &&
           cram_md5_get_auth(ua, director->password, &tls_remote_need);
   } else {
      unbash_spaces(name);
      cons = (CONRES *)GetResWithName(R_CONSOLE, name);
      if (cons) {
#ifdef HAVE_TLS
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
#endif /* HAVE_TLS */

         auth_success = cram_md5_auth(ua, cons->password, tls_local_need) &&
              cram_md5_get_auth(ua, cons->password, &tls_remote_need);

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

#ifdef HAVE_TLS
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
#endif /* HAVE_TLS */


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
