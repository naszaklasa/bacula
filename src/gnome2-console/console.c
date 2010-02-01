/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula GNOME Console interface to the Director
 *
 *     Kern Sibbald, March MMII
 *
 *     Version $Id$
 */

#include "bacula.h"
#include "console.h"

#include "interface.h"
#include "support.h"

/* Imported functions */
int authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons);
void select_restore_setup();
extern bool parse_gcons_config(CONFIG *config, const char *configfile, int exit_code);


/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) { return 1; }

/* Exported variables */
GtkWidget *console;   /* application window */
GtkWidget *text1;            /* text window */
GtkWidget *entry1;           /* entry box */
GtkWidget *status1;          /* status bar */
GtkWidget *combo1;           /* director combo */
GtkWidget *scroll1;          /* main scroll bar */
GtkWidget *run_dialog;       /* run dialog */
GtkWidget *dir_dialog;       /* director selection dialog */
GtkWidget *restore_dialog;   /* restore dialog */
GtkWidget *restore_file_selection;
GtkWidget *dir_select;
GtkWidget *about1;           /* about box */
GtkWidget *label_dialog;
PangoFontDescription *font_desc = NULL;
PangoFontDescription *text_font_desc = NULL;
pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cmd_wait;
char cmd[1000];
int reply;
BSOCK *UA_sock = NULL;
GList *job_list, *client_list, *fileset_list;
GList *messages_list, *pool_list, *storage_list;
GList *type_list, *level_list;

/* Forward referenced functions */
void terminate_console(int sig);

extern "C" {
    static gint message_handler(gpointer data);
    static int initial_connect_to_director(gpointer data);
}

static void set_scroll_bar_to_end(void);

/* Static variables */
static char *configfile = NULL;
static DIRRES *dir;
static int ndir;
static bool director_reader_running = false;
static bool at_prompt = false;
static bool ready = false;
static bool quit = false;
static guint initial;
static int numdir = 0;
static CONFIG *config;

#define CONFIG_FILE "./bgnome-console.conf"   /* default configuration file */

static void usage()
{
   fprintf(stderr, _(
PROG_COPYRIGHT
"\nVersion: %s (%s) %s %s %s\n\n"
"Usage: bgnome-console [-s] [-c config_file] [-d debug_level] [config_file]\n"
"       -c <file>   set configuration file to file\n"
"       -dnn        set debug level to nn\n"
"       -s          no signals\n"
"       -t          test - read configuration and exit\n"
"       -?          print this message.\n"
"\n"), 2002, VERSION, BDATE, HOST_OS, DISTNAME, DISTVER);

   exit(1);
}

/*
 * Call-back for reading a passphrase for an encrypted PEM file
 * This function uses getpass(), which uses a static buffer and is NOT thread-safe.
 */
static int tls_pem_callback(char *buf, int size, const void *userdata)
{
#ifdef HAVE_TLS
   const char *prompt = (const char *) userdata;
   char *passwd;

   passwd = getpass(prompt);
   bstrncpy(buf, passwd, size);
   return (strlen(buf));
#else
   buf[0] = 0;
   return 0;
#endif
}


/*
 * Make a quick check to see that we have all the
 * resources needed.
 */
static int check_resources()
{
   bool ok = true;
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
            ok = false;
            continue;
         }
      }

      if ((!director->tls_ca_certfile && !director->tls_ca_certdir) && director->tls_enable) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Director \"%s\" in %s."
                             " At least one CA certificate store is required.\n"),
                             director->hdr.name, configfile);
         ok = false;
      }
   }
   
   if (numdir == 0) {
      Emsg1(M_FATAL, 0, _("No Director resource defined in %s\n"
                          "Without that I don't how to speak to the Director :-(\n"), configfile);
      ok = false;
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
            ok = false;
            continue;
         }
      }

      if ((!cons->tls_ca_certfile && !cons->tls_ca_certdir) && cons->tls_enable) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Console \"%s\" in %s.\n"),
                             cons->hdr.name, configfile);
         ok = false;
      }
   }

   UnlockRes();

   return ok;
}


/*********************************************************************
 *
 *         Main Bacula GNOME Console -- User Interface Program
 *
 */
int main(int argc, char *argv[])
{
   int ch, stat;
   int no_signals = TRUE;
   int test_config = FALSE;
   int gargc = 1;
   const char *gargv[2] = {"bgnome-console", NULL};
   CONFONTRES *con_font;

#ifdef ENABLE_NLS
   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");
#endif

   init_stack_dump();
   my_name_is(argc, argv, "bgnome-console");
   init_msg(NULL, NULL);
   working_directory  = "/tmp";

   struct sigaction sigignore;
   sigignore.sa_flags = 0;
   sigignore.sa_handler = SIG_IGN;
   sigfillset(&sigignore.sa_mask);
   sigaction(SIGPIPE, &sigignore, NULL);

   if ((stat=pthread_cond_init(&cmd_wait, NULL)) != 0) {
      Emsg1(M_ABORT, 0, _("Pthread cond init error = %s\n"),
         strerror(stat));
   }

   gnome_init("bacula", VERSION, gargc, (char **)&gargv);

   while ((ch = getopt(argc, argv, "bc:d:r:st?")) != -1) {
      switch (ch) {
      case 'c':                    /* configuration file */
         if (configfile != NULL)
            free(configfile);
         configfile = bstrdup(optarg);
         break;

      case 'd':
         debug_level = atoi(optarg);
         if (debug_level <= 0)
            debug_level = 1;
         break;

      case 's':                    /* turn off signals */
         no_signals = TRUE;
         break;

      case 't':
         test_config = TRUE;
         break;

      case '?':
      default:
         usage();
      }
   }
   argc -= optind;
   argv += optind;


   if (!no_signals) {
      init_signals(terminate_console);
   }

   if (argc) {
      usage();
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   config = new_config_parser();
   parse_gcons_config(config, configfile, M_ERROR_TERM);

   if (init_crypto() != 0) {
      Emsg0(M_ERROR_TERM, 0, _("Cryptography library initialization failed.\n"));
   }

   if (!check_resources()) {
      Emsg1(M_ERROR_TERM, 0, _("Please correct configuration file: %s\n"), configfile);
   }

   console = create_console();
   gtk_window_set_default_size(GTK_WINDOW(console), 800, 700);
   run_dialog = create_RunDialog();
   label_dialog = create_label_dialog();
   restore_dialog = create_RestoreDialog();
   about1 = create_about1();

   text1 = lookup_widget(console, "text1");
   entry1 = lookup_widget(console, "entry1");
   status1 = lookup_widget(console, "status1");
   scroll1 = lookup_widget(console, "scroll1");

   select_restore_setup();

   gtk_widget_show(console);

/*
 * Gtk2/pango have different font names. Gnome2 comes with "Monospace 10"
 */

   LockRes();
   foreach_res(con_font, R_CONSOLE_FONT) {
       if (!con_font->fontface) {
          Dmsg1(400, "No fontface for %s\n", con_font->hdr.name);
          continue;
       }
       Dmsg1(100, "Now loading: %s\n",con_font->fontface);
       text_font_desc = pango_font_description_from_string(con_font->fontface);
       if (text_font_desc == NULL) {
           Dmsg2(400, "Load of requested ConsoleFont \"%s\" (%s) failed!\n",
                  con_font->hdr.name, con_font->fontface);
       } else {
           Dmsg2(400, "ConsoleFont \"%s\" (%s) loaded.\n",
                  con_font->hdr.name, con_font->fontface);
           break;
       }
   }
   UnlockRes();
    
   font_desc = pango_font_description_from_string("LucidaTypewriter 9");
   if (!text_font_desc) {
      text_font_desc = pango_font_description_from_string("Monospace 10");
   }
   if (!text_font_desc) {
      text_font_desc = pango_font_description_from_string("monospace");
   }

   gtk_widget_modify_font(console, font_desc);
   gtk_widget_modify_font(entry1, font_desc);
   gtk_widget_modify_font(status1, font_desc);
   if (text_font_desc) {
      gtk_widget_modify_font(text1, text_font_desc);
      pango_font_description_free(text_font_desc);
   } else {
      gtk_widget_modify_font(text1, font_desc);
   }
   pango_font_description_free(font_desc);

   if (test_config) {
      terminate_console(0);
      exit(0);
   }

   initial = gtk_timeout_add(100, initial_connect_to_director, (gpointer)NULL);

   gtk_main();
   quit = true;
   disconnect_from_director((gpointer)NULL);
   return 0;
}

/*
 * Every 5 seconds, ask the Director for our
 *  messages.
 */
static gint message_handler(gpointer data)
{
   if (ready && UA_sock) {
      bnet_fsend(UA_sock, ".messages");
   }
   return TRUE;
}

int disconnect_from_director(gpointer data)
{
   if (!quit) {
      set_status(_(" Not Connected"));
   }
   if (UA_sock) {
      bnet_sig(UA_sock, BNET_TERMINATE); /* send EOF */
      bnet_close(UA_sock);
      UA_sock = NULL;
   }
   return 1;
}

/*
 * Called just after the main loop is started to allow
 *  us to connect to the Director.
 */
static int initial_connect_to_director(gpointer data)
{
   gtk_timeout_remove(initial);
   if (connect_to_director(data)) {
      start_director_reader(data);
   }
   gtk_timeout_add(5000, message_handler, (gpointer)NULL);
   return TRUE;
}

static GList *get_list(char *cmd)
{
   GList *options;
   char *msg;

   options = NULL;
   write_director(cmd);
   while (bnet_recv(UA_sock) > 0) {
      strip_trailing_junk(UA_sock->msg);
      msg = (char *)malloc(strlen(UA_sock->msg) + 1);
      strcpy(msg, UA_sock->msg);
      options = g_list_append(options, msg);
   }
   return options;

}

static GList *get_and_fill_combo(GtkWidget *dialog, const char *combo_name, const char *dircmd)
{
   GtkWidget *combo;
   GList *options;

   combo = lookup_widget(dialog, combo_name);
   options = get_list((char *)dircmd);
   if (combo && options) {
      gtk_combo_set_popdown_strings(GTK_COMBO(combo), options);
   }
   return options;
}

static void fill_combo(GtkWidget *dialog, const char *combo_name, GList *options)
{
   GtkWidget *combo;

   combo = lookup_widget(dialog, combo_name);
   if (combo && options) {
      gtk_combo_set_popdown_strings(GTK_COMBO(combo), options);
   }
   return;
}


/*
 * Connect to Director. If there are more than one, put up
 * a modal dialog so that the user chooses one.
 */
int connect_to_director(gpointer data)
{
   GList *dirs = NULL;
   GtkWidget *combo;
   JCR jcr;


   if (UA_sock) {
      return 0;
   }

   if (ndir > 1) {
      LockRes();
      foreach_res(dir, R_DIRECTOR) {
         dirs = g_list_append(dirs, dir->hdr.name);
      }
      UnlockRes();
      dir_dialog = create_SelectDirectorDialog();
      combo = lookup_widget(dir_dialog, "combo1");
      dir_select = lookup_widget(dir_dialog, "dirselect");
      if (dirs) {
         gtk_combo_set_popdown_strings(GTK_COMBO(combo), dirs);
      }
      gtk_widget_show(dir_dialog);
      gtk_main();

      if (reply == OK) {
         gchar *ecmd = gtk_editable_get_chars((GtkEditable *)dir_select, 0, -1);
         dir = (DIRRES *)GetResWithName(R_DIRECTOR, ecmd);
         if (ecmd) {
            g_free(ecmd);             /* release director name string */
         }
      }
      if (dirs) {
         g_free(dirs);
      }
      gtk_widget_destroy(dir_dialog);
      dir_dialog = NULL;
   } else {
      /* Just take the first Director */
      LockRes();
      dir = (DIRRES *)GetNextRes(R_DIRECTOR, NULL);
      UnlockRes();
   }

   if (!dir) {
      return 0;
   }

   memset(&jcr, 0, sizeof(jcr));

   set_statusf(_(" Connecting to Director %s:%d"), dir->address,dir->DIRport);
   set_textf(_("Connecting to Director %s:%d\n\n"), dir->address,dir->DIRport);

   while (gtk_events_pending()) {     /* fully paint screen */
      gtk_main_iteration();
   }

   LockRes();
   /* If cons==NULL, default console will be used */
   CONRES *cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)NULL);
   UnlockRes();

   char buf[1024];
   /* Initialize Console TLS context */
   if (cons && (cons->tls_enable || cons->tls_require)) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), _("Passphrase for Console \"%s\" TLS private key: "), cons->hdr.name);

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
      cons->tls_ctx = new_tls_context(cons->tls_ca_certfile,
         cons->tls_ca_certdir, cons->tls_certfile,
         cons->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!cons->tls_ctx) {
         bsnprintf(buf, sizeof(buf), _("Failed to initialize TLS context for Console \"%s\".\n"),
            dir->hdr.name);
         set_text(buf, strlen(buf));
         terminate_console(0);
         return 1;
      }

   }

   /* Initialize Director TLS context */
   if (dir->tls_enable || dir->tls_require) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), _("Passphrase for Director \"%s\" TLS private key: "), dir->hdr.name);

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
      dir->tls_ctx = new_tls_context(dir->tls_ca_certfile,
         dir->tls_ca_certdir, dir->tls_certfile,
         dir->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!dir->tls_ctx) {
         bsnprintf(buf, sizeof(buf), _("Failed to initialize TLS context for Director \"%s\".\n"),
            dir->hdr.name);
         set_text(buf, strlen(buf));
         terminate_console(0);
         return 1;
      }
   }


   UA_sock = bnet_connect(NULL, 5, 15, 0, _("Director daemon"), dir->address,
                          NULL, dir->DIRport, 0);
   if (UA_sock == NULL) {
      return 0;
   }

   jcr.dir_bsock = UA_sock;
   if (!authenticate_director(&jcr, dir, cons)) {
      set_text(UA_sock->msg, UA_sock->msglen);
      return 0;
   }

   set_status(_(" Initializing ..."));

   bnet_fsend(UA_sock, "autodisplay on");

   /* Read and display all initial messages */
   while (bnet_recv(UA_sock) > 0) {
      set_text(UA_sock->msg, UA_sock->msglen);
   }

   /* Paint changes */
   while (gtk_events_pending()) {
      gtk_main_iteration();
   }

   /* Fill the run_dialog combo boxes */
   job_list      = get_and_fill_combo(run_dialog, "combo_job", ".jobs");
   client_list   = get_and_fill_combo(run_dialog, "combo_client", ".clients");
   fileset_list  = get_and_fill_combo(run_dialog, "combo_fileset", ".filesets");
   messages_list = get_and_fill_combo(run_dialog, "combo_messages", ".msgs");
   pool_list     = get_and_fill_combo(run_dialog, "combo_pool", ".pools");
   storage_list  = get_and_fill_combo(run_dialog, "combo_storage", ".storage");
   type_list     = get_and_fill_combo(run_dialog, "combo_type", ".types");
   level_list    = get_and_fill_combo(run_dialog, "combo_level", ".levels");

   /* Fill the label dialog combo boxes */
   fill_combo(label_dialog, "label_combo_storage", storage_list);
   fill_combo(label_dialog, "label_combo_pool", pool_list);


   /* Fill the restore_dialog combo boxes */
   fill_combo(restore_dialog, "combo_restore_job", job_list);
   fill_combo(restore_dialog, "combo_restore_client", client_list);
   fill_combo(restore_dialog, "combo_restore_fileset", fileset_list);
   fill_combo(restore_dialog, "combo_restore_pool", pool_list);
   fill_combo(restore_dialog, "combo_restore_storage", storage_list);

   set_status(_(" Connected"));
   return 1;
}

void write_director(const gchar *msg)
{
   if (UA_sock) {
      at_prompt = false;
      set_status(_(" Processing command ..."));
      UA_sock->msglen = strlen(msg);
      pm_strcpy(&UA_sock->msg, msg);
      bnet_send(UA_sock);
   }
   if (strcmp(msg, ".quit") == 0 || strcmp(msg, ".exit") == 0) {
      disconnect_from_director((gpointer)NULL);
      gtk_main_quit();
   }
}

extern "C"
void read_director(gpointer data, gint fd, GdkInputCondition condition)
{
   int stat;

   if (!UA_sock || UA_sock->m_fd != fd) {
      return;
   }
   stat = UA_sock->recv();
   if (stat >= 0) {
      if (at_prompt) {
         set_text("\n", 1);
         at_prompt = false;
      }
      set_text(UA_sock->msg, UA_sock->msglen);
      return;
   }
   if (is_bnet_stop(UA_sock)) {         /* error or term request */
      gtk_main_quit();
      return;
   }
   /* Must be a signal -- either do something or ignore it */
   if (UA_sock->msglen == BNET_PROMPT) {
      at_prompt = true;
      set_status(_(" At prompt waiting for input ..."));
   }
   if (UA_sock->msglen == BNET_EOD) {
      set_status_ready();
   }
   return;
}

static gint tag;

void start_director_reader(gpointer data)
{

   if (director_reader_running || !UA_sock) {
      return;
   }
   tag = gdk_input_add(UA_sock->m_fd, GDK_INPUT_READ, read_director, NULL);
   director_reader_running = true;
}

void stop_director_reader(gpointer data)
{
   if (!director_reader_running) {
      return;
   }
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   gdk_input_remove(tag);
   director_reader_running = false;
}



/* Cleanup and then exit */
void terminate_console(int sig)
{
   static bool already_here = false;

   if (already_here)                  /* avoid recursive temination problems */
      exit(1);
   already_here = true;
   config->free_resources();
   free(config);
   config = NULL;
   cleanup_crypto();
   disconnect_from_director((gpointer)NULL);
   gtk_main_quit();
   exit(0);
}


/* Buffer approx 2000 lines -- assume 60 chars/line */
#define MAX_TEXT_CHARS   (2000 * 60)
static int text_chars = 0;

static void truncate_text_chars()
{
   GtkTextBuffer *textbuf;
   GtkTextIter iter, iter2;
   guint len;
   int del_chars = MAX_TEXT_CHARS / 4;

   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
   len = gtk_text_buffer_get_char_count(textbuf);
   gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
   gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, del_chars);
   gtk_text_buffer_delete (textbuf, &iter, &iter2);
   text_chars -= del_chars;
   len = gtk_text_buffer_get_char_count(textbuf);
   gtk_text_iter_set_offset(&iter, len);
}

void set_textf(const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[1000];
   int len;
   va_start(arg_ptr, fmt);
   len = bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   set_text(buf, len);
}

void set_text(const char *buf, int len)
{
   GtkTextBuffer *textbuf;
   GtkTextIter iter;
   guint buf_len;

   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
   buf_len = gtk_text_buffer_get_char_count(textbuf);
   gtk_text_buffer_get_iter_at_offset(textbuf, &iter, buf_len - 1);
   gtk_text_buffer_insert(textbuf, &iter, buf, -1);
   text_chars += len;
   if (text_chars > MAX_TEXT_CHARS) {
      truncate_text_chars();
   }
   set_scroll_bar_to_end();
}

void set_statusf(const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[1000];
   int len;
   va_start(arg_ptr, fmt);
   len = bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   gtk_label_set_text(GTK_LABEL(status1), buf);
// set_scroll_bar_to_end();
   ready = false;
}

void set_status_ready()
{
   gtk_label_set_text(GTK_LABEL(status1), _(" Ready"));
   ready = true;
// set_scroll_bar_to_end();
}

void set_status(const char *buf)
{
   gtk_label_set_text(GTK_LABEL(status1), buf);
// set_scroll_bar_to_end();
   ready = false;
}

static void set_scroll_bar_to_end(void)
{
   GtkTextBuffer* textbuf = NULL;
   GtkTextIter iter;
   guint buf_len;

   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
   buf_len = gtk_text_buffer_get_char_count(textbuf);
   gtk_text_buffer_get_iter_at_offset(textbuf, &iter, buf_len - 1);
   gtk_text_iter_set_offset(&iter, buf_len);
   gtk_text_buffer_place_cursor(textbuf, &iter);
   gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text1),
              gtk_text_buffer_get_mark(textbuf, "insert"),
              0, TRUE, 0.0, 1.0);
}
