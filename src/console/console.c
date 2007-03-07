/*
 *
 *   Bacula Console interface to the Director
 *
 *     Kern Sibbald, September MM
 *
 *     Version $Id: console.c 4116 2007-02-06 14:37:57Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

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
#include "console_conf.h"
#include "jcr.h"


#ifdef HAVE_CONIO
#include "conio.h"
//#define CONIO_FIX 1
#else
#define con_init(x)
#define con_term()
#define con_set_zed_keys();
#define trapctlc()
#define clrbrk()
#define usrbrk() 0
#endif

#if defined(HAVE_WIN32)
#define isatty(fd) (fd==0)
#endif

/* Exported variables */

extern int rl_catch_signals;

/* Imported functions */
int authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons);

/* Forward referenced functions */
static void terminate_console(int sig);
static int check_resources();
int get_cmd(FILE *input, const char *prompt, BSOCK *sock, int sec);
static int do_outputcmd(FILE *input, BSOCK *UA_sock);
void senditf(const char *fmt, ...);
void sendit(const char *buf);

extern "C" void got_sigstop(int sig);
extern "C" void got_sigcontinue(int sig);
extern "C" void got_sigtout(int sig);
extern "C" void got_sigtin(int sig);


/* Static variables */
static char *configfile = NULL;
static BSOCK *UA_sock = NULL;
static DIRRES *dir = NULL;
static CONRES *cons = NULL;
static FILE *output = stdout;
static bool teeout = false;               /* output to output and stdout */
static bool stop = false;
static bool no_conio = false;
static int argc;
static int numdir;
static int numcon;
static POOLMEM *args;
static char *argk[MAX_CMD_ARGS];
static char *argv[MAX_CMD_ARGS];


/* Command prototypes */
static int versioncmd(FILE *input, BSOCK *UA_sock);
static int inputcmd(FILE *input, BSOCK *UA_sock);
static int outputcmd(FILE *input, BSOCK *UA_sock);
static int teecmd(FILE *input, BSOCK *UA_sock);
static int quitcmd(FILE *input, BSOCK *UA_sock);
static int echocmd(FILE *input, BSOCK *UA_sock);
static int timecmd(FILE *input, BSOCK *UA_sock);
static int sleepcmd(FILE *input, BSOCK *UA_sock);


#define CONFIG_FILE "bconsole.conf"   /* default configuration file */

static void usage()
{
   fprintf(stderr, _(
PROG_COPYRIGHT
"\nVersion: " VERSION " (" BDATE ") %s %s %s\n\n"
"Usage: bconsole [-s] [-c config_file] [-d debug_level]\n"
"       -c <file>   set configuration file to file\n"
"       -dnn        set debug level to nn\n"
"       -n          no conio\n"
"       -s          no signals\n"
"       -t          test - read configuration and exit\n"
"       -?          print this message.\n"
"\n"), 2000, HOST_OS, DISTNAME, DISTVER);
}


extern "C"
void got_sigstop(int sig)
{
   stop = true;
}

extern "C"
void got_sigcontinue(int sig)
{
   stop = false;
}

extern "C"
void got_sigtout(int sig)
{
// printf("Got tout\n");
}

extern "C"
void got_sigtin(int sig)
{
// printf("Got tin\n");
}


static int zed_keyscmd(FILE *input, BSOCK *UA_sock)
{
   con_set_zed_keys();
   return 1;
}

/*
 * These are the @command
 */
struct cmdstruct { const char *key; int (*func)(FILE *input, BSOCK *UA_sock); const char *help; };
static struct cmdstruct commands[] = {
 { N_("input"),      inputcmd,     _("input from file")},
 { N_("output"),     outputcmd,    _("output to file")},
 { N_("quit"),       quitcmd,      _("quit")},
 { N_("tee"),        teecmd,       _("output to file and terminal")},
 { N_("sleep"),      sleepcmd,     _("sleep specified time")},
 { N_("time"),       timecmd,      _("print current time")},
 { N_("version"),    versioncmd,   _("print Console's version")},
 { N_("echo"),       echocmd,      _("echo command string")},
 { N_("exit"),       quitcmd,      _("exit = quit")},
 { N_("zed_keys"),   zed_keyscmd,  _("zed_keys = use zed keys instead of bash keys")},
             };
#define comsize (sizeof(commands)/sizeof(struct cmdstruct))

static int do_a_command(FILE *input, BSOCK *UA_sock)
{
   unsigned int i;
   int stat;
   int found;
   int len;
   char *cmd;

   found = 0;
   stat = 1;

   Dmsg1(120, "Command: %s\n", UA_sock->msg);
   if (argc == 0) {
      return 1;
   }

   cmd = argk[0]+1;
   if (*cmd == '#') {                 /* comment */
      return 1;
   }
   len = strlen(cmd);
   for (i=0; i<comsize; i++) {     /* search for command */
      if (strncasecmp(cmd,  _(commands[i].key), len) == 0) {
         stat = (*commands[i].func)(input, UA_sock);   /* go execute command */
         found = 1;
         break;
      }
   }
   if (!found) {
      pm_strcat(&UA_sock->msg, _(": is an invalid command\n"));
      UA_sock->msglen = strlen(UA_sock->msg);
      sendit(UA_sock->msg);
   }
   return stat;
}


static void read_and_process_input(FILE *input, BSOCK *UA_sock)
{
   const char *prompt = "*";
   bool at_prompt = false;
   int tty_input = isatty(fileno(input));
   int stat;

   for ( ;; ) {
      if (at_prompt) {                /* don't prompt multiple times */
         prompt = "";
      } else {
         prompt = "*";
         at_prompt = true;
      }
      if (tty_input) {
         stat = get_cmd(input, prompt, UA_sock, 30);
         if (usrbrk() == 1) {
            clrbrk();
         }
         if (usrbrk()) {
            break;
         }
      } else {
         /* Reading input from a file */
         int len = sizeof_pool_memory(UA_sock->msg) - 1;
         if (usrbrk()) {
            break;
         }
         if (fgets(UA_sock->msg, len, input) == NULL) {
            stat = -1;
         } else {
            sendit(UA_sock->msg);  /* echo to terminal */
            strip_trailing_junk(UA_sock->msg);
            UA_sock->msglen = strlen(UA_sock->msg);
            stat = 1;
         }
      }
      if (stat < 0) {
         break;                       /* error or interrupt */
      } else if (stat == 0) {         /* timeout */
         if (strcmp(prompt, "*") == 0) {
            bnet_fsend(UA_sock, ".messages");
         } else {
            continue;
         }
      } else {
         at_prompt = FALSE;
         /* @ => internal command for us */
         if (UA_sock->msg[0] == '@') {
            parse_args(UA_sock->msg, &args, &argc, argk, argv, MAX_CMD_ARGS);
            if (!do_a_command(input, UA_sock)) {
               break;
            }
            continue;
         }
         if (!bnet_send(UA_sock)) {   /* send command */
            break;                    /* error */
         }
      }
      if (strcmp(UA_sock->msg, ".quit") == 0 || strcmp(UA_sock->msg, ".exit") == 0) {
         break;
      }
      while ((stat = bnet_recv(UA_sock)) >= 0) {
         if (at_prompt) {
            if (!stop) {
               sendit("\n");
            }
            at_prompt = false;
         }
         /* Suppress output if running in background or user hit ctl-c */
         if (!stop && !usrbrk()) {
            sendit(UA_sock->msg);
         }
      }
      if (usrbrk() > 1) {
         break;
      } else {
         clrbrk();
      }
      if (!stop) {
         fflush(stdout);
      }
      if (is_bnet_stop(UA_sock)) {
         break;                       /* error or term */
      } else if (stat == BNET_SIGNAL) {
         if (UA_sock->msglen == BNET_PROMPT) {
            at_prompt = true;
         }
         Dmsg1(100, "Got poll %s\n", bnet_sig_to_ascii(UA_sock));
      }
   }
}

/*
 * Call-back for reading a passphrase for an encrypted PEM file
 * This function uses getpass(), 
 *  which uses a static buffer and is NOT thread-safe.
 */
static int tls_pem_callback(char *buf, int size, const void *userdata)
{
#ifdef HAVE_TLS
   const char *prompt = (const char *)userdata;
# if defined(HAVE_WIN32)
   sendit(prompt);
   if (win32_cgets(buf, size) == NULL) {
      buf[0] = 0;
      return 0;
   } else {
      return strlen(buf);
   }
# else
   char *passwd;

   passwd = getpass(prompt);
   bstrncpy(buf, passwd, size);
   return strlen(buf);
# endif
#else
   buf[0] = 0;
   return 0;
#endif
}


/*********************************************************************
 *
 *         Main Bacula Console -- User Interface Program
 *
 */
int main(int argc, char *argv[])
{
   int ch, i, item;
   bool no_signals = false;
   bool test_config = false;
   JCR jcr;

   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   init_stack_dump();
   my_name_is(argc, argv, "bconsole");
   init_msg(NULL, NULL);
   working_directory = "/tmp";
   args = get_pool_memory(PM_FNAME);

   while ((ch = getopt(argc, argv, "bc:d:nst?")) != -1) {
      switch (ch) {
      case 'c':                    /* configuration file */
         if (configfile != NULL) {
            free(configfile);
         }
         configfile = bstrdup(optarg);
         break;

      case 'd':
         debug_level = atoi(optarg);
         if (debug_level <= 0) {
            debug_level = 1;
         }
         break;

      case 'n':                    /* no conio */
         no_conio = true;
         break;

      case 's':                    /* turn off signals */
         no_signals = true;
         break;

      case 't':
         test_config = true;
         break;

      case '?':
      default:
         usage();
         exit(1);
      }
   }
   argc -= optind;
   argv += optind;

   if (!no_signals) {
      init_signals(terminate_console);
   }


#if !defined(HAVE_WIN32)
   /* Override Bacula default signals */
   signal(SIGQUIT, SIG_IGN);
   signal(SIGTSTP, got_sigstop);
   signal(SIGCONT, got_sigcontinue);
   signal(SIGTTIN, got_sigtin);
   signal(SIGTTOU, got_sigtout);
   trapctlc();
#endif

   OSDependentInit();

   if (argc) {
      usage();
      exit(1);
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   parse_config(configfile);

   if (init_crypto() != 0) {
      Emsg0(M_ERROR_TERM, 0, _("Cryptography library initialization failed.\n"));
   }

   if (!check_resources()) {
      Emsg1(M_ERROR_TERM, 0, _("Please correct configuration file: %s\n"), configfile);
   }

   if (!no_conio) {
      con_init(stdin);
   }

   if (test_config) {
      terminate_console(0);
      exit(0);
   }

   memset(&jcr, 0, sizeof(jcr));

   (void)WSA_Init();                        /* Initialize Windows sockets */

   LockRes();
   numdir = 0;
   foreach_res(dir, R_DIRECTOR) {
      numdir++;
   }
   numcon = 0;
   foreach_res(cons, R_CONSOLE) {
      numcon++;
   }
   UnlockRes();

   if (numdir > 1) {
      struct sockaddr client_addr;
      memset(&client_addr, 0, sizeof(client_addr));
      UA_sock = init_bsock(NULL, 0, "", "", 0, &client_addr);
try_again:
      sendit(_("Available Directors:\n"));
      LockRes();
      numdir = 0;
      foreach_res(dir, R_DIRECTOR) {
         senditf( _("%d  %s at %s:%d\n"), 1+numdir++, dir->hdr.name, dir->address,
            dir->DIRport);
      }
      UnlockRes();
      if (get_cmd(stdin, _("Select Director: "), UA_sock, 600) < 0) {
         (void)WSACleanup();               /* Cleanup Windows sockets */
         return 1;
      }
      item = atoi(UA_sock->msg);
      if (item < 0 || item > numdir) {
         senditf(_("You must enter a number between 1 and %d\n"), numdir);
         goto try_again;
      }
      term_bsock(UA_sock);
      LockRes();
      for (i=0; i<item; i++) {
         dir = (DIRRES *)GetNextRes(R_DIRECTOR, (RES *)dir);
      }
      /* Look for a console linked to this director */
      for (i=0; i<numcon; i++) {
         cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)cons);
         if (cons->director && strcmp(cons->director, dir->hdr.name) == 0) {
            break;
         }
         cons = NULL;
      }
      /* Look for the first non-linked console */
      if (cons == NULL) {
         for (i=0; i<numcon; i++) {
            cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)cons);
            if (cons->director == NULL)
               break;
            cons = NULL;
        }
      }
      UnlockRes();
   }
   /* If no director, take first one */
   if (!dir) {
      LockRes();
      dir = (DIRRES *)GetNextRes(R_DIRECTOR, NULL);
      UnlockRes();
   }
   /* If no console, take first one */
   if (!cons) {
      LockRes();
      cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)NULL);
      UnlockRes();
   }

   senditf(_("Connecting to Director %s:%d\n"), dir->address,dir->DIRport);

   char buf[1024];
   /* Initialize Console TLS context */
   if (cons && (cons->tls_enable || cons->tls_require)) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), "Passphrase for Console \"%s\" TLS private key: ", cons->hdr.name);

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
      cons->tls_ctx = new_tls_context(cons->tls_ca_certfile,
         cons->tls_ca_certdir, cons->tls_certfile,
         cons->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!cons->tls_ctx) {
         senditf(_("Failed to initialize TLS context for Console \"%s\".\n"),
            dir->hdr.name);
         terminate_console(0);
         return 1;
      }
   }

   /* Initialize Director TLS context */
   if (dir->tls_enable || dir->tls_require) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), "Passphrase for Director \"%s\" TLS private key: ", dir->hdr.name);

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
      dir->tls_ctx = new_tls_context(dir->tls_ca_certfile,
         dir->tls_ca_certdir, dir->tls_certfile,
         dir->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!dir->tls_ctx) {
         senditf(_("Failed to initialize TLS context for Director \"%s\".\n"),
            dir->hdr.name);
         terminate_console(0);
         return 1;
      }
   }

   UA_sock = bnet_connect(NULL, 5, 15, "Director daemon", dir->address,
                          NULL, dir->DIRport, 0);
   if (UA_sock == NULL) {
      terminate_console(0);
      return 1;
   }
   jcr.dir_bsock = UA_sock;

   /* If cons==NULL, default console will be used */
   if (!authenticate_director(&jcr, dir, cons)) {
      terminate_console(0);
      return 1;
   }

   Dmsg0(40, "Opened connection with Director daemon\n");

   sendit(_("Enter a period to cancel a command.\n"));

   /* Run commands in ~/.bconsolerc if any */
   char *env = getenv("HOME");
   if (env) {
      FILE *fd;
      pm_strcpy(&UA_sock->msg, env);
      pm_strcat(&UA_sock->msg, "/.bconsolerc");
      fd = fopen(UA_sock->msg, "rb");
      if (fd) {
         read_and_process_input(fd, UA_sock);
         fclose(fd);
      }
   }

   read_and_process_input(stdin, UA_sock);

   if (UA_sock) {
      bnet_sig(UA_sock, BNET_TERMINATE); /* send EOF */
      bnet_close(UA_sock);
   }

   terminate_console(0);
   return 0;
}


/* Cleanup and then exit */
static void terminate_console(int sig)
{

   static bool already_here = false;

   if (already_here) {                /* avoid recursive temination problems */
      exit(1);
   }
   already_here = true;
   cleanup_crypto();
   free_pool_memory(args);
   if (!no_conio) {
      con_term();
   }
   (void)WSACleanup();               /* Cleanup Windows sockets */
   if (sig != 0) {
      exit(1);
   }
   return;
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

   numdir = 0;
   foreach_res(director, R_DIRECTOR) {

      numdir++;
      /* tls_require implies tls_enable */
      if (director->tls_require) {
         if (have_tls) {
            director->tls_enable = true;
         } else {
            Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
            OK = false;
            continue;
         }
      }

      if ((!director->tls_ca_certfile && !director->tls_ca_certdir) && director->tls_enable) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Director \"%s\" in %s."
                             " At least one CA certificate store is required.\n"),
                             director->hdr.name, configfile);
         OK = false;
      }
   }
   
   if (numdir == 0) {
      Emsg1(M_FATAL, 0, _("No Director resource defined in %s\n"
                          "Without that I don't how to speak to the Director :-(\n"), configfile);
      OK = false;
   }

   CONRES *cons;
   /* Loop over Consoles */
   foreach_res(cons, R_CONSOLE) {
      /* tls_require implies tls_enable */
      if (cons->tls_require) {
         if (have_tls) {
            cons->tls_enable = true;
         } else {
            Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
            OK = false;
            continue;
         }
      }

      if ((!cons->tls_ca_certfile && !cons->tls_ca_certdir) && cons->tls_enable) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Console \"%s\" in %s.\n"),
                             cons->hdr.name, configfile);
         OK = false;
      }
   }

   UnlockRes();

   return OK;
}


#ifdef HAVE_READLINE
#define READLINE_LIBRARY 1
#undef free
#include "readline.h"
#include "history.h"


int
get_cmd(FILE *input, const char *prompt, BSOCK *sock, int sec)
{
   char *line;

   rl_catch_signals = 0;              /* do it ourselves */
   line = readline((char *)prompt);   /* cast needed for old readlines */

   if (!line) {
      exit(1);
   }
   strip_trailing_junk(line);
   sock->msglen = pm_strcpy(&sock->msg, line);
   if (sock->msglen) {
      add_history(sock->msg);
   }
   free(line);
   return 1;
}

#else /* no readline, do it ourselves */

#if !defined(HAVE_WIN32)
static bool bisatty(int fd)
{
   if (no_conio) {
      return false;
   }
   return isatty(fd);
}
#endif

/*
 *   Returns: 1 if data available
 *            0 if timeout
 *           -1 if error
 */
static int
wait_for_data(int fd, int sec)
{
#if defined(HAVE_WIN32)
   return 1;
#else
   fd_set fdset;
   struct timeval tv;

   tv.tv_sec = sec;
   tv.tv_usec = 0;
   for ( ;; ) {
      FD_ZERO(&fdset);
      FD_SET((unsigned)fd, &fdset);
      switch(select(fd + 1, &fdset, NULL, NULL, &tv)) {
      case 0:                         /* timeout */
         return 0;
      case -1:
         if (errno == EINTR || errno == EAGAIN) {
            continue;
         }
         return -1;                  /* error return */
      default:
         return 1;
      }
   }
#endif
}

/*
 * Get next input command from terminal.
 *
 *   Returns: 1 if got input
 *            0 if timeout
 *           -1 if EOF or error
 */
int
get_cmd(FILE *input, const char *prompt, BSOCK *sock, int sec)
{
   int len;
   if (!stop) {
      if (output == stdout || teeout) {
         sendit(prompt);
      }
   }
again:
   switch (wait_for_data(fileno(input), sec)) {
   case 0:
      return 0;                    /* timeout */
   case -1:
      return -1;                   /* error */
   default:
      len = sizeof_pool_memory(sock->msg) - 1;
      if (stop) {
         sleep(1);
         goto again;
      }
#ifdef HAVE_CONIO
      if (bisatty(fileno(input))) {
         input_line(sock->msg, len);
         break;
      }
#endif
#ifdef HAVE_WIN32 /* use special console for input on win32 */
      if (input == stdin) {
         if (win32_cgets(sock->msg, len) == NULL) {
            return -1;
         }
      }
      else
#endif
      if (fgets(sock->msg, len, input) == NULL) {
         return -1;

      }
      break;
   }
   if (usrbrk()) {
      clrbrk();
   }
   strip_trailing_junk(sock->msg);
   sock->msglen = strlen(sock->msg);
   return 1;
}

#endif /* end non-readline code */

static int versioncmd(FILE *input, BSOCK *UA_sock)
{
   senditf("Version: " VERSION " (" BDATE ") %s %s %s\n",
      HOST_OS, DISTNAME, DISTVER);
   return 1;
}

static int inputcmd(FILE *input, BSOCK *UA_sock)
{
   FILE *fd;

   if (argc > 2) {
      sendit(_("Too many arguments on input command.\n"));
      return 1;
   }
   if (argc == 1) {
      sendit(_("First argument to input command must be a filename.\n"));
      return 1;
   }
   fd = fopen(argk[1], "rb");
   if (!fd) {
      senditf(_("Cannot open file %s for input. ERR=%s\n"),
         argk[1], strerror(errno));
      return 1;
   }
   read_and_process_input(fd, UA_sock);
   fclose(fd);
   return 1;
}

/* Send output to both termina and specified file */
static int teecmd(FILE *input, BSOCK *UA_sock)
{
   teeout = true;
   return do_outputcmd(input, UA_sock);
}

/* Send output to specified "file" */
static int outputcmd(FILE *input, BSOCK *UA_sock)
{
   teeout = false;
   return do_outputcmd(input, UA_sock);
}


static int do_outputcmd(FILE *input, BSOCK *UA_sock)
{
   FILE *fd;
   const char *mode = "a+b";

   if (argc > 3) {
      sendit(_("Too many arguments on output/tee command.\n"));
      return 1;
   }
   if (argc == 1) {
      if (output != stdout) {
         fclose(output);
         output = stdout;
         teeout = false;
      }
      return 1;
   }
   if (argc == 3) {
      mode = argk[2];
   }
   fd = fopen(argk[1], mode);
   if (!fd) {
      senditf(_("Cannot open file %s for output. ERR=%s\n"),
         argk[1], strerror(errno));
      return 1;
   }
   output = fd;
   return 1;
}

static int echocmd(FILE *intut, BSOCK *UA_sock)
{
   for (int i=1; i < argc; i++) {
      senditf("%s", argk[i]);
      sendit(" ");
   }
   sendit("\n");
   return 1;
}

static int quitcmd(FILE *input, BSOCK *UA_sock)
{
   return 0;
}

static int sleepcmd(FILE *input, BSOCK *UA_sock)
{
   if (argc > 1) {
      sleep(atoi(argk[1]));
   }
   return 1;
}


static int timecmd(FILE *input, BSOCK *UA_sock)
{
   char sdt[50];
   time_t ttime = time(NULL);
   struct tm tm;
   (void)localtime_r(&ttime, &tm);
   strftime(sdt, sizeof(sdt), "%d-%b-%Y %H:%M:%S", &tm);
   sendit("\n");
   return 1;
}

/*
 * Send a line to the output file and or the terminal
 */
void senditf(const char *fmt,...)
{
   char buf[3000];
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   bvsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);
   sendit(buf);
}

void sendit(const char *buf)
{
#ifdef CONIO_FIX
   char obuf[3000];
   if (output == stdout || teeout) {
      const char *p, *q;
      /*
       * Here, we convert every \n into \r\n because the
       *  terminal is in raw mode when we are using
       *  conio.
       */
      for (p=q=buf; (p=strchr(q, '\n')); ) {
         int len = p - q;
         if (len > 0) {
            memcpy(obuf, q, len);
         }
         memcpy(obuf+len, "\r\n", 3);
         q = ++p;                    /* point after \n */
         fputs(obuf, output);
      }
      if (*q) {
         fputs(q, output);
      }
      fflush(output);
   }
   if (output != stdout) {
      fputs(buf, output);
   }
#else

   fputs(buf, output);
   fflush(output);
   if (teeout) {
      fputs(buf, stdout);
      fflush(stdout);
   }
#endif
}
