/*
 *
 *   Bacula Director -- User Agent Server
 *
 *     Kern Sibbald, September MM
 *
 *    Version $Id: ua_server.c,v 1.40.2.1 2005/12/20 23:15:01 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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

/* Imported variables */
extern int r_first;
extern int r_last;
extern struct s_res resources[];
extern int console_msg_pending;
extern char my_name[];


/* Forward referenced functions */
extern "C" void *connect_thread(void *arg);
static void *handle_UA_client_request(void *arg);


/* Global variables */
static int started = FALSE;
static workq_t ua_workq;

struct s_addr_port {
   char *addr;
   char *port;
};

/* Called here by Director daemon to start UA (user agent)
 * command thread. This routine creates the thread and then
 * returns.
 */
void start_UA_server(dlist *addrs)
{
   pthread_t thid;
   int status;
   static dlist *myaddrs = addrs;

   if ((status=pthread_create(&thid, NULL, connect_thread, (void *)myaddrs)) != 0) {
      berrno be;
      Emsg1(M_ABORT, 0, _("Cannot create UA thread: %s\n"), be.strerror(status));
   }
   started = TRUE;
   return;
}

extern "C"
void *connect_thread(void *arg)
{
   pthread_detach(pthread_self());

   /* Permit 10 console connections */
   bnet_thread_server((dlist*)arg, 10, &ua_workq, handle_UA_client_request);
   return NULL;
}

/*
 * Create a Job Control Record for a control "job",
 *   filling in all the appropriate fields.
 */
JCR *new_control_jcr(const char *base_name, int job_type)
{
   JCR *jcr;
   jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   /*
    * The job and defaults are not really used, but
    *  we set them up to ensure that everything is correctly
    *  initialized.
    */
   LockRes();
   jcr->job = (JOB *)GetNextRes(R_JOB, NULL);
   set_jcr_defaults(jcr, jcr->job);
   UnlockRes();
   jcr->sd_auth_key = bstrdup("dummy"); /* dummy Storage daemon key */
   create_unique_job_name(jcr, base_name);
   jcr->sched_time = jcr->start_time;
   jcr->JobType = job_type;
   jcr->JobLevel = L_NONE;
   jcr->JobStatus = JS_Running;
   jcr->JobId = 0;
   return jcr;
}

/*
 * Handle Director User Agent commands
 *
 */
static void *handle_UA_client_request(void *arg)
{
   int stat;
   UAContext *ua;
   JCR *jcr;

   pthread_detach(pthread_self());

   jcr = new_control_jcr("*Console*", JT_CONSOLE);

   ua = new_ua_context(jcr);
   ua->UA_sock = (BSOCK *)arg;

   bnet_recv(ua->UA_sock);          /* Get first message */
   if (!authenticate_user_agent(ua)) {
      goto getout;
   }

   while (!ua->quit) {
      stat = bnet_recv(ua->UA_sock);
      if (stat >= 0) {
         pm_strcpy(ua->cmd, ua->UA_sock->msg);
         parse_ua_args(ua);
         if (ua->argc > 0 && ua->argk[0][0] == '.') {
            do_a_dot_command(ua, ua->cmd);
         } else {
            do_a_command(ua, ua->cmd);
         }
         if (!ua->quit) {
            if (ua->auto_display_messages) {
               pm_strcpy(ua->cmd, "messages");
               qmessagescmd(ua, ua->cmd);
               ua->user_notified_msg_pending = FALSE;
            } else if (!ua->gui && !ua->user_notified_msg_pending && console_msg_pending) {
               bsendmsg(ua, _("You have messages.\n"));
               ua->user_notified_msg_pending = TRUE;
            }
            bnet_sig(ua->UA_sock, BNET_EOD); /* send end of command */
         }
      } else if (is_bnet_stop(ua->UA_sock)) {
         ua->quit = true;
      } else { /* signal */
         bnet_sig(ua->UA_sock, BNET_POLL);
      }
   }

getout:
   close_db(ua);
   free_ua_context(ua);
   free_jcr(jcr);

   return NULL;
}

/*
 * Create a UAContext for a Job that is running so that
 *   it can the User Agent routines and
 *   to ensure that the Job gets the proper output.
 *   This is a sort of mini-kludge, and should be
 *   unified at some point.
 */
UAContext *new_ua_context(JCR *jcr)
{
   UAContext *ua;

   ua = (UAContext *)malloc(sizeof(UAContext));
   memset(ua, 0, sizeof(UAContext));
   ua->jcr = jcr;
   ua->db = jcr->db;
   ua->cmd = get_pool_memory(PM_FNAME);
   ua->args = get_pool_memory(PM_FNAME);
   ua->verbose = 1;
   ua->automount = TRUE;
   return ua;
}

void free_ua_context(UAContext *ua)
{
   if (ua->cmd) {
      free_pool_memory(ua->cmd);
   }
   if (ua->args) {
      free_pool_memory(ua->args);
   }
   if (ua->prompt) {
      free(ua->prompt);
   }

   if (ua->UA_sock) {
      bnet_close(ua->UA_sock);
      ua->UA_sock = NULL;
   }
   free(ua);
}


/*
 * Called from main Bacula thread
 */
void term_ua_server()
{
   if (!started) {
      return;
   }
}
