/*
 *  Bacula File Daemon
 *
 *    Kern Sibbald, March MM
 *
 *   Version $Id: filed.c,v 1.51.2.1 2006/01/15 10:11:59 kerns Exp $
 *
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
#include "filed.h"

/* Imported Functions */
extern void *handle_client_request(void *dir_sock);

/* Imported Variables */
extern time_t watchdog_sleep_time;

/* Forward referenced functions */
void terminate_filed(int sig);
static int check_resources();

/* Exported variables */
CLIENT *me;                           /* my resource */
char OK_msg[]   = "2000 OK\n";
char TERM_msg[] = "2999 Terminate\n";
bool no_signals = false;

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
const int win32_client = 1;
#else
const int win32_client = 0;
#endif


#define CONFIG_FILE "./bacula-fd.conf" /* default config file */

char *configfile = NULL;
static bool foreground = false;
static bool inetd_request = false;
static workq_t dir_workq;             /* queue of work from Director */
static pthread_t server_tid;


static void usage()
{
   Pmsg2(-1, _(
"Copyright (C) 2000-2005 Kern Sibbald\n"
"\nVersion: %s (%s)\n\n"
"Usage: bacula-fd [-f -s] [-c config_file] [-d debug_level]\n"
"        -c <file>   use <file> as configuration file\n"
"        -dnn        set debug level to nn\n"
"        -f          run in foreground (for debugging)\n"
"        -g          groupid\n"
"        -i          inetd request\n"
"        -s          no signals (for debugging)\n"
"        -t          test configuration file and exit\n"
"        -u          userid\n"
"        -v          verbose user messages\n"
"        -?          print this message.\n"
"\n"), VERSION, BDATE);
   exit(1);
}


/*********************************************************************
 *
 *  Main Bacula Unix Client Program
 *
 */
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
#define main BaculaMain
#endif

int main (int argc, char *argv[])
{
   int ch;
   bool test_config = false;
   char *uid = NULL;
   char *gid = NULL;

   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   init_stack_dump();
   my_name_is(argc, argv, "bacula-fd");
   init_msg(NULL, NULL);
   daemon_start_time = time(NULL);

   while ((ch = getopt(argc, argv, "c:d:fg:istu:v?")) != -1) {
      switch (ch) {
      case 'c':                    /* configuration file */
         if (configfile != NULL) {
            free(configfile);
         }
         configfile = bstrdup(optarg);
         break;

      case 'd':                    /* debug level */
         debug_level = atoi(optarg);
         if (debug_level <= 0) {
            debug_level = 1;
         }
         break;

      case 'f':                    /* run in foreground */
         foreground = true;
         break;

      case 'g':                    /* set group */
         gid = optarg;
         break;

      case 'i':
         inetd_request = true;
         break;
      case 's':
         no_signals = true;
         break;

      case 't':
         test_config = true;
         break;

      case 'u':                    /* set userid */
         uid = optarg;
         break;

      case 'v':                    /* verbose */
         verbose++;
         break;

      case '?':
      default:
         usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (argc) {
      if (configfile != NULL)
         free(configfile);
      configfile = bstrdup(*argv);
      argc--;
      argv++;
   }
   if (argc) {
      usage();
   }

   server_tid = pthread_self();
   if (!no_signals) {
      init_signals(terminate_filed);
   } else {
      /* This reduces the number of signals facilitating debugging */
      watchdog_sleep_time = 120;      /* long timeout for debugging */
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   parse_config(configfile);

   if (init_tls() != 0) {
      Emsg0(M_ERROR, 0, _("TLS library initialization failed.\n"));
      terminate_filed(1);
   }

   if (!check_resources()) {
      Emsg1(M_ERROR, 0, _("Please correct configuration file: %s\n"), configfile);
      terminate_filed(1);
   }

   set_working_directory(me->working_directory);

   if (test_config) {
      terminate_filed(0);
   }

   if (!foreground &&!inetd_request) {
      daemon_start();
      init_stack_dump();              /* set new pid */
   }

   /* Maximum 1 daemon at a time */
   create_pid_file(me->pid_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));
   read_state_file(me->working_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));

   drop(uid, gid);

#ifdef BOMB
   me += 1000000;
#endif

   init_python_interpreter(me->hdr.name, me->scripts_directory, "FDStartUp");

   set_thread_concurrency(10);

   if (!no_signals) {
      start_watchdog();               /* start watchdog thread */
      init_jcr_subsystem();           /* start JCR watchdogs etc. */
   }
   server_tid = pthread_self();

   if (inetd_request) {
      /* Socket is on fd 0 */
      struct sockaddr client_addr;
      int port = -1;
      socklen_t client_addr_len = sizeof(client_addr);
      if (getsockname(0, &client_addr, &client_addr_len) == 0) {
                /* MA BUG 6 remove ifdefs */
                port = sockaddr_get_port_net_order(&client_addr);
      }
      BSOCK *bs = init_bsock(NULL, 0, "client", "unknown client", port, &client_addr);
      handle_client_request((void *)bs);
   } else {
      /* Become server, and handle requests */
      IPADDR *p;
      foreach_dlist(p, me->FDaddrs) {
         Dmsg1(10, "filed: listening on port %d\n", p->get_port_host_order());
      }
      bnet_thread_server(me->FDaddrs, me->MaxConcurrentJobs, &dir_workq, handle_client_request);
   }

   terminate_filed(0);
   exit(0);                           /* should never get here */
}

void terminate_filed(int sig)
{
   bnet_stop_thread_server(server_tid);
   generate_daemon_event(NULL, "Exit");
   write_state_file(me->working_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));
   delete_pid_file(me->pid_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));

   if (configfile != NULL) {
      free(configfile);
   }
   if (debug_level > 0) {
      print_memory_pool_stats();
   }
   free_config_resources();
   term_msg();
   stop_watchdog();
   cleanup_tls();
   close_memory_pool();               /* release free memory in pool */
   sm_dump(false);                    /* dump orphaned buffers */
   exit(sig);
}

/*
* Make a quick check to see that we have all the
* resources needed.
*/
static int check_resources()
{
   bool OK = true;
   DIRRES *director;

   LockRes();

   me = (CLIENT *)GetNextRes(R_CLIENT, NULL);
   if (!me) {
      Emsg1(M_FATAL, 0, _("No File daemon resource defined in %s\n"
            "Without that I don't know who I am :-(\n"), configfile);
      OK = false;
   } else {
      if (GetNextRes(R_CLIENT, (RES *) me) != NULL) {
         Emsg1(M_FATAL, 0, _("Only one Client resource permitted in %s\n"),
              configfile);
         OK = false;
      }
      my_name_is(0, NULL, me->hdr.name);
      if (!me->messages) {
         me->messages = (MSGS *)GetNextRes(R_MSGS, NULL);
         if (!me->messages) {
             Emsg1(M_FATAL, 0, _("No Messages resource defined in %s\n"), configfile);
             OK = false;
         }
      }
      /* tls_require implies tls_enable */
      if (me->tls_require) {
#ifndef HAVE_TLS
         Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
         OK = false;
#else
         me->tls_enable = true;
#endif
      }

      if ((!me->tls_ca_certfile && !me->tls_ca_certdir) && me->tls_enable) {
         Emsg1(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
            " or \"TLS CA Certificate Dir\" are defined for File daemon in %s.\n"),
                            configfile);
        OK = false;
      }

      /* If everything is well, attempt to initialize our per-resource TLS context */
      if (OK && (me->tls_enable || me->tls_require)) {
         /* Initialize TLS context:
          * Args: CA certfile, CA certdir, Certfile, Keyfile,
          * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
         me->tls_ctx = new_tls_context(me->tls_ca_certfile,
            me->tls_ca_certdir, me->tls_certfile, me->tls_keyfile,
            NULL, NULL, NULL, true);

         if (!me->tls_ctx) { 
            Emsg2(M_FATAL, 0, _("Failed to initialize TLS context for File daemon \"%s\" in %s.\n"),
                                me->hdr.name, configfile);
            OK = false;
         }
      }
   }


   /* Verify that a director record exists */
   LockRes();
   director = (DIRRES *)GetNextRes(R_DIRECTOR, NULL);
   UnlockRes();
   if (!director) {
      Emsg1(M_FATAL, 0, _("No Director resource defined in %s\n"),
            configfile);
      OK = false;
   }

   foreach_res(director, R_DIRECTOR) { 
      /* tls_require implies tls_enable */
      if (director->tls_require) {
#ifndef HAVE_TLS
         Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
         OK = false;
         continue;
#else
         director->tls_enable = true;
#endif
      }

      if (!director->tls_certfile && director->tls_enable) {
         Emsg2(M_FATAL, 0, _("\"TLS Certificate\" file not defined for Director \"%s\" in %s.\n"),
               director->hdr.name, configfile);
         OK = false;
      }

      if (!director->tls_keyfile && director->tls_enable) {
         Emsg2(M_FATAL, 0, _("\"TLS Key\" file not defined for Director \"%s\" in %s.\n"),
               director->hdr.name, configfile);
         OK = false;
      }

      if ((!director->tls_ca_certfile && !director->tls_ca_certdir) && director->tls_enable && director->tls_verify_peer) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Director \"%s\" in %s."
                             " At least one CA certificate store is required"
                             " when using \"TLS Verify Peer\".\n"),
                             director->hdr.name, configfile);
         OK = false;
      }

      /* If everything is well, attempt to initialize our per-resource TLS context */
      if (OK && (director->tls_enable || director->tls_require)) {
         /* Initialize TLS context:
          * Args: CA certfile, CA certdir, Certfile, Keyfile,
          * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
         director->tls_ctx = new_tls_context(director->tls_ca_certfile,
            director->tls_ca_certdir, director->tls_certfile,
            director->tls_keyfile, NULL, NULL, director->tls_dhfile,
            director->tls_verify_peer);

         if (!director->tls_ctx) { 
            Emsg2(M_FATAL, 0, _("Failed to initialize TLS context for Director \"%s\" in %s.\n"),
                                director->hdr.name, configfile);
            OK = false;
         }
      }
   }

   UnlockRes();

   if (OK) {
      close_msg(NULL);                /* close temp message handler */
      init_msg(NULL, me->messages);   /* open user specified message handler */
   }

   return OK;
}
