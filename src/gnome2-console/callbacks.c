/*
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "bacula.h"
#include "console.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define KEY_Enter 65293
#define KEY_Up    65362
#define KEY_Down  65364
#define KEY_Left  65361
#define KEY_Right 65363

void terminate_console(int sig);

extern "C" gint compare_func(const void *data1, const void *data2);

gboolean
on_console_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
   gtk_main_quit();
   terminate_console(0);
   return TRUE;
}

void
on_connect_activate(GtkMenuItem *menuitem, gpointer user_data)
{
   if (connect_to_director(user_data)) {
      start_director_reader(user_data);
   }
}


void
on_disconnect_activate(GtkMenuItem *menuitem, gpointer user_data)
{
   if (disconnect_from_director(user_data)) {
      stop_director_reader(user_data);
   }
}


void
on_exit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
   gtk_main_quit();
}


void
on_cut1_activate(GtkMenuItem *menuitem, gpointer user_data)
{

}


void
on_copy1_activate(GtkMenuItem *menuitem, gpointer user_data)
{

}


void
on_paste1_activate(GtkMenuItem *menuitem, gpointer user_data)
{

}


void
on_clear1_activate(GtkMenuItem *menuitem, gpointer user_data)
{

}

void
on_preferences1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
}


void
on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
   gtk_widget_show(about1);
   gtk_main();
}


void
on_connect_button_clicked(GtkButton *button, gpointer user_data)
{
   if (connect_to_director(user_data)) {
      start_director_reader(user_data);
   }
}

/* Define max length of history and how many to delete when it gets there.
 * If you make HIST_DEL > HIST_MAX, you shoot yourself in the foot.
 */
#define HIST_MAX 2500
#define HIST_DEL  500

static GList *hist = NULL;
static GList *hc = NULL;
static int hist_len = 0;

static void add_to_history(gchar *ecmd)
{
   int len = strlen(ecmd);

   if (len > 0) {
      hist = g_list_append(hist, bstrdup(ecmd));
      hist_len++;
   }
   if (hist_len >= HIST_MAX) {
      int i;
      GList *hp;
      for (i=0; i<HIST_DEL; i++) {
         hp = g_list_next(hist);
         free(hp->data);
         hist = g_list_remove(hist, hp->data);
      }
      hist_len -= HIST_DEL;
   }
   hc = NULL;
}

/*
 * Note, apparently there is a bug in GNOME where some of the
 *  key press events are lost -- at least it loses every other
 *  up arrow!
 */
gboolean
on_entry1_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
   if (event->keyval == KEY_Enter) {
      char *ecmd = (char *)gtk_entry_get_text((GtkEntry *)entry1);
      set_text(ecmd, strlen(ecmd));
      set_text("\n", 1);
      add_to_history(ecmd);
      write_director(ecmd);
      gtk_entry_set_text((GtkEntry *)entry1, "");
   } else if (event->keyval == KEY_Up) {
      if (!hc) {
         if (!hist) {
            return FALSE;
         }
         hc = g_list_last(hist);
      } else {
         hc = g_list_previous(hc);
      }
      if (!hc) {
         if (!hist) {
            return FALSE;
         }
         hc = g_list_first(hist);
      }
      gtk_entry_set_text((GtkEntry *)entry1, (gchar *)hc->data);
   } else if (event->keyval == KEY_Down) {
      if (!hc) {
         if (!hist) {
            return FALSE;
         }
         hc = g_list_first(hist);
      } else {
         hc = g_list_next(hc);
      }
      if (!hc) {
         if (!hist) {
            return FALSE;
         }
         hc = g_list_last(hist);
      }
      gtk_entry_set_text((GtkEntry *)entry1, (gchar *)hc->data);
   }
// gtk_entry_set_position((GtkEntry *)entry1, -1);
// gtk_window_set_focus((GtkWindow *)app1, entry1);
   return FALSE;
}


void
on_app1_show(GtkWidget *widget, gpointer user_data)
{
}



void
on_msgs_button_clicked(GtkButton *button, gpointer user_data)
{
   write_director("messages");
}

void
on_msgs_activate(GtkMenuItem *menuitem, gpointer user_data)
{
   write_director("messages");
}

void
on_about_button_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(about1);
   gtk_main_quit();
   set_status_ready();
}

void
on_select_director_OK_clicked(GtkButton *button, gpointer user_data)
{
   reply = OK;
   gtk_widget_hide(dir_dialog);
   gtk_main_quit();
   set_status_ready();
}


void
on_select_director_cancel_clicked(GtkButton *button, gpointer user_data)
{
   reply = CANCEL;
   gtk_widget_hide(dir_dialog);
   gtk_main_quit();
   set_status_ready();
}

/*
 * Compare list string items
 */
extern "C"
gint compare_func(const void *data1, const void *data2)
{
   return strcmp((const char *)data1, (const char *)data2);
}

static GList *find_combo_list(char *name)
{
   if (strcmp(name, "job") == 0) {
      return job_list;
   }
   if (strcmp(name, "pool") == 0) {
      return pool_list;
   }
   if (strcmp(name, "client") == 0) {
      return client_list;
   }
   if (strcmp(name, "storage") == 0) {
      return storage_list;
   }
   if (strcmp(name, "fileset") == 0) {
      return fileset_list;
   }
   if (strcmp(name, "messages") == 0) {
      return messages_list;
   }
   if (strcmp(name, "type") == 0) {
      return type_list;
   }
   if (strcmp(name, "level") == 0) {
      return level_list;
   }
   return NULL;
}

/*
 * Set correct default values in the Run dialog box
 */
static void set_run_defaults()
{
   GtkWidget *combo, *entry;
   char *job, *def;
   GList *item, *list;
   char cmd[1000];
   int pos;

   stop_director_reader(NULL);

   combo = lookup_widget(run_dialog, "combo_job");
   job = (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
   bsnprintf(cmd, sizeof(cmd), ".defaults job=\"%s\"", job);
   write_director(cmd);
   while (bnet_recv(UA_sock) > 0) {
      def = strchr(UA_sock->msg, '=');
      if (!def) {
         continue;
      }
      *def++ = 0;
      if (strcmp(UA_sock->msg, "job") == 0 ||
          strcmp(UA_sock->msg, "when") == 0) {
         continue;
      }
      /* Where is an entry box */
      if (strcmp(UA_sock->msg, "where") == 0) {
         entry = lookup_widget(run_dialog, "entry_where");
         gtk_entry_set_text(GTK_ENTRY(entry), def);
         continue;
      }

      /* Now handle combo boxes */
      list = find_combo_list(UA_sock->msg);
      if (!list) {
         continue;
      }
      item = g_list_find_custom(list, def, compare_func);
      bsnprintf(cmd, sizeof(cmd), "combo_%s", UA_sock->msg);
      combo = lookup_widget(run_dialog, cmd);
      if (!combo) {
         continue;
      }
      pos = g_list_position(list, item);
      gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list), pos);
   }
   start_director_reader(NULL);
}

void
on_entry_job_changed(GtkEditable *editable, gpointer user_data)
{
   set_run_defaults();
}


void
on_run_button_clicked(GtkButton *button, gpointer user_data)
{
   char dt[50];
   GtkWidget *entry;

   bstrutime(dt, sizeof(dt), time(NULL));
   entry = lookup_widget(run_dialog, "entry_when");
   gtk_entry_set_text(GTK_ENTRY(entry), dt);
   set_run_defaults();
   gtk_widget_show(run_dialog);
   gtk_main();
}


static char *get_combo_text(GtkWidget *dialog, const char *combo_name)
{
   GtkWidget *combo;
   combo = lookup_widget(dialog, combo_name);
   if (!combo) {
      return NULL;
   }
   return (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
}

static char *get_entry_text(GtkWidget *dialog, const char *entry_name)
{
   GtkWidget *entry;
   entry = lookup_widget(dialog, entry_name);
   if (!entry) {
      return NULL;
   }
   return (char *)gtk_entry_get_text(GTK_ENTRY(entry));
}

static char *get_spin_text(GtkWidget *dialog, const char *spin_name)
{
   GtkSpinButton *spin;
   spin = (GtkSpinButton *)lookup_widget(dialog, spin_name);
   if (!spin) {
      return NULL;
   }
   return (char *)gtk_entry_get_text(GTK_ENTRY(&spin->entry));
}




void
on_run_ok_clicked(GtkButton *button, gpointer user_data)
{
   char *job, *fileset, *level, *client, *pool, *when, *where, *storage, *priority;
   
   gtk_widget_hide(run_dialog);
   gtk_main_quit();

   job     = get_combo_text(run_dialog, "combo_job");
   fileset = get_combo_text(run_dialog, "combo_fileset");
   client  = get_combo_text(run_dialog, "combo_client");
   pool    = get_combo_text(run_dialog, "combo_pool");
   storage = get_combo_text(run_dialog, "combo_storage");
   level   = get_combo_text(run_dialog, "combo_level");
   priority = get_spin_text(run_dialog, "spinbutton1");
   when    = get_entry_text(run_dialog, "entry_when");
   where   = get_entry_text(run_dialog, "entry_where");

   if (!job || !fileset || !client || !pool || !storage ||
       !level || !priority || !when || !where) {
      set_status_ready();
      return;
   }

   bsnprintf(cmd, sizeof(cmd),
             "run job=\"%s\" fileset=\"%s\" level=%s client=\"%s\" pool=\"%s\" "
             "when=\"%s\" where=\"%s\" storage=\"%s\" priority=\"%s\"\n",
             job, fileset, level, client, pool, when, where, storage, priority);
   write_director(cmd);
   set_text(cmd, strlen(cmd));
   write_director("yes");
   return;
}


void
on_run_cancel_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(run_dialog);
   gtk_main_quit();
   set_status_ready();
}

gboolean
on_entry1_key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  return FALSE;
}

void
on_restore_button_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_show(restore_dialog);
   gtk_main();
}

void
on_view_fileset_clicked(GtkButton *button, gpointer user_data)
{
}


void
on_restore_cancel_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(restore_dialog);
   gtk_main_quit();
   set_status_ready();
}


void
on_label_button_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_show(label_dialog);
   gtk_main();
}

void
on_label_ok_clicked(GtkButton *button, gpointer user_data)
{
   char *pool, *volume, *storage, *slot;

   gtk_widget_hide(label_dialog);
   gtk_main_quit();

   pool    = get_combo_text(label_dialog, "label_combo_pool");
   storage = get_combo_text(label_dialog, "label_combo_storage");

   volume  = get_entry_text(label_dialog, "label_entry_volume");

   slot    = get_spin_text(label_dialog, "label_slot");

   if (!pool || !storage || !volume || !(*volume)) {
      set_status_ready();
      return;
   }

   bsnprintf(cmd, sizeof(cmd),
             "label volume=\"%s\" pool=\"%s\" storage=\"%s\" slot=%s\n", 
             volume, pool, storage, slot);
   write_director(cmd);
   set_text(cmd, strlen(cmd));
}


void
on_label_cancel_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(label_dialog);
   gtk_main_quit();
   set_status_ready();
}


void
on_select_files_button_clicked(GtkButton *button, gpointer user_data)
{
   char *job, *fileset, *client, *pool, *before, *storage;

   gtk_widget_hide(restore_dialog);

   job     = get_combo_text(restore_dialog, "combo_restore_job");
   fileset = get_combo_text(restore_dialog, "combo_restore_fileset");
   client  = get_combo_text(restore_dialog, "combo_restore_client");
   pool    = get_combo_text(restore_dialog, "combo_restore_pool");
   storage = get_combo_text(restore_dialog, "combo_restore_storage");

   before  = get_entry_text(restore_dialog, "restore_before_entry");

   if (!job || !fileset || !client || !pool || !storage || !before) {
      set_status_ready();
      return;
   }

   bsnprintf(cmd, sizeof(cmd),
             "restore select current fileset=\"%s\" client=\"%s\" pool=\"%s\" "
             "storage=\"%s\"\n", fileset, client, pool, storage);
   write_director(cmd);
   set_text(cmd, strlen(cmd));
   gtk_widget_show(restore_file_selection);
   select_restore_files();            /* put up select files dialog */
}

void
on_restore_select_ok_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(restore_file_selection);
   write_director("done");
   gtk_main_quit();
   set_status_ready();
}


void
on_restore_select_cancel_clicked(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(restore_file_selection);
   write_director("quit");
   gtk_main_quit();
   set_status_ready();
}

gboolean
on_restore_files_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
   gtk_widget_hide(restore_file_selection);
   gtk_main_quit();
   set_status_ready();
   return FALSE;
}


void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_cut2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_copy2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_paste2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_clear2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_properties1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_preferences2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

/*
 * Set correct default values in the Restore dialog box
 */
void set_restore_dialog_defaults()
{
   GtkWidget *combo;
   char *job, *def;
   GList *item, *list;
   char cmd[1000];
   int pos;

   stop_director_reader(NULL);

   combo = lookup_widget(restore_dialog, "combo_restore_job");
   job = (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
   bsnprintf(cmd, sizeof(cmd), ".defaults job=\"%s\"", job);
   write_director(cmd);
   while (bnet_recv(UA_sock) > 0) {
      def = strchr(UA_sock->msg, '=');
      if (!def) {
         continue;
      }
      *def++ = 0;
      if (strcmp(UA_sock->msg, "job") == 0 ||
          strcmp(UA_sock->msg, "when") == 0 ||
          strcmp(UA_sock->msg, "where") == 0 ||
          strcmp(UA_sock->msg, "messages") == 0 ||
          strcmp(UA_sock->msg, "level") == 0 ||
          strcmp(UA_sock->msg, "type") == 0) {
         continue;
      }

      /* Now handle combo boxes */
      list = find_combo_list(UA_sock->msg);
      if (!list) {
         continue;
      }
      item = g_list_find_custom(list, def, compare_func);
      bsnprintf(cmd, sizeof(cmd), "combo_restore_%s", UA_sock->msg);
      combo = lookup_widget(restore_dialog, cmd);
      if (!combo) {
         continue;
      }
      pos = g_list_position(list, item);
      gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list), pos);
   }
   start_director_reader(NULL);
}


void
on_restore_job_entry_changed(GtkEditable *editable, gpointer user_data)
{
   /* Set defaults that correspond to new job selection */
   set_restore_dialog_defaults();
}

void
on_dir_button_clicked(GtkButton *toolbutton, gpointer user_data)
{
   write_director("status dir");
}
