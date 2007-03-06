/*
 *
 *   Bacula GNOME Console interface to the Director
 *
 *     Kern Sibbald, March MMII
 *
 *     Version $Id: console.c,v 1.19.8.1 2005/02/25 09:47:07 kerns Exp $
 */

/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "bacula.h"
#include "console.h"

#include "interface.h"
#include "support.h"

/* Imported functions */
int authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons);
void select_restore_setup();

/* Exported variables */
GtkWidget *console;   /* application window */
GtkWidget *text1;	     /* text window */
GtkWidget *entry1;	     /* entry box */
GtkWidget *status1;	     /* status bar */
GtkWidget *combo1;	     /* director combo */
GtkWidget *scroll1;	     /* main scroll bar */
GtkWidget *run_dialog;	     /* run dialog */
GtkWidget *dir_dialog;	     /* director selection dialog */
GtkWidget *restore_dialog;   /* restore dialog */
GtkWidget *restore_file_selection;
GtkWidget *dir_select;
GtkWidget *about1;	     /* about box */
GtkWidget *label_dialog;
GdkFont   *text_font = NULL;
PangoFontDescription *font_desc;
pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	cmd_wait;
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

#define CONFIG_FILE "./gnome-console.conf"   /* default configuration file */

static void usage()
{
   fprintf(stderr, _(
"Copyright (C) 2002-2004 Kern Sibbald and John Walker\n"
"\nVersion: " VERSION " (" BDATE ") %s %s %s\n\n"
"Usage: gnome-console [-s] [-c config_file] [-d debug_level] [config_file]\n"
"       -c <file>   set configuration file to file\n"
"       -dnn        set debug level to nn\n"
"       -s          no signals\n"
"       -t          test - read configuration and exit\n"
"       -?          print this message.\n"
"\n"), HOST_OS, DISTNAME, DISTVER);

   exit(1);
}


/*********************************************************************
 *
 *	   Main Bacula GNOME Console -- User Interface Program
 *
 */
int main(int argc, char *argv[])
{
   int ch, stat;
   int no_signals = TRUE;
   int test_config = FALSE;
   int gargc = 1;
   const char *gargv[2] = {"gnome-console", NULL};
   CONFONTRES *con_font;

   init_stack_dump();
   my_name_is(argc, argv, "gnome-console");
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

   parse_config(configfile);

   LockRes();
   ndir = 0;
   foreach_res(dir, R_DIRECTOR) {
      ndir++;
   }
   UnlockRes();
   if (ndir == 0) {
      Emsg1(M_ERROR_TERM, 0, _("No director resource defined in %s\n"
"Without that I don't how to speak to the Director :-(\n"), configfile);
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
 * Thanks to Phil Stracchino for providing the font configuration code.
 * original default:
   text_font = gdk_font_load("-misc-fixed-medium-r-normal-*-*-130-*-*-c-*-koi8-r");
 * this works for me:
   text_font = gdk_font_load("-Bigelow & Holmes-lucida console-medium-r-semi condensed-*-12-0-100-100-m-0-iso8859-1");
 * and, new automagic:font specification!
 */

   LockRes();
   foreach_res(con_font, R_CONSOLE_FONT) {
       if (!con_font->fontface) {
	  Dmsg1(400, "No fontface for %s\n", con_font->hdr.name);
	  continue;
       }
       text_font = gdk_font_load(con_font->fontface);
       if (text_font == NULL) {
	   Dmsg2(400, "Load of requested ConsoleFont \"%s\" (%s) failed!\n",
		  con_font->hdr.name, con_font->fontface);
       } else {
	   Dmsg2(400, "ConsoleFont \"%s\" (%s) loaded.\n",
		  con_font->hdr.name, con_font->fontface);
	   break;
       }
   }
   UnlockRes();

   if (text_font == NULL) {
       Dmsg1(400, "Attempting to load fallback font %s\n",
	      "-misc-fixed-medium-r-normal-*-*-130-*-*-c-*-iso8859-1");
       text_font = gdk_font_load("-misc-fixed-medium-r-normal-*-*-130-*-*-c-*-iso8859-1");
   }
   font_desc = pango_font_description_from_string("LucidaTypewriter 9");
   gtk_widget_modify_font(console, font_desc);
   gtk_widget_modify_font(text1, font_desc);
   gtk_widget_modify_font(entry1, font_desc);
   gtk_widget_modify_font(status1, font_desc);
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

static GList *get_and_fill_combo(GtkWidget *dialog, const char *combo_name, const char *cm)
{
   GtkWidget *combo;
   GList *options;

   combo = lookup_widget(dialog, combo_name);
   options = get_list(cmd);
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
	    g_free(ecmd);	      /* release director name string */
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
   UA_sock = bnet_connect(NULL, 5, 15, "Director daemon", dir->address,
			  NULL, dir->DIRport, 0);
   if (UA_sock == NULL) {
      return 0;
   }

   jcr.dir_bsock = UA_sock;
   LockRes();
   /* If cons==NULL, default console will be used */
   CONRES *cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)NULL);
   UnlockRes();
   if (!authenticate_director(&jcr, dir, cons)) {
      set_text(UA_sock->msg, UA_sock->msglen);
      return 0;
   }

   set_status(" Initializing ...");

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

   set_status(" Connected");
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

   if (!UA_sock || UA_sock->fd != fd) {
      return;
   }
   stat = bnet_recv(UA_sock);
   if (stat >= 0) {
      if (at_prompt) {
	 set_text("\n", 1);
	 at_prompt = false;
      }
      set_text(UA_sock->msg, UA_sock->msglen);
      return;
   }
   if (is_bnet_stop(UA_sock)) { 	/* error or term request */
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
   tag = gdk_input_add(UA_sock->fd, GDK_INPUT_READ, read_director, NULL);
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
   static int already_here = FALSE;

   if (already_here)		      /* avoid recursive temination problems */
      exit(1);
   already_here = TRUE;
   disconnect_from_director((gpointer)NULL);
   gtk_main_quit();
   exit(0);
}


/* Buffer approx 2000 lines -- assume 60 chars/line */
#define MAX_TEXT_CHARS	 (2000 * 60)
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
   gtk_label_set_text(GTK_LABEL(status1), " Ready");
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
