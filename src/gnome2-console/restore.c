/*
 * Handle console GUI selection of files
 *  Kern Sibbald, March, 2004
 *
*/
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/


#include "bacula.h"
#include "console.h"
#include "interface.h"
#include "support.h"
#include "restore.h"

extern BSOCK *UA_sock;
void write_director(const gchar *msg);
void start_director_reader(gpointer data);
void stop_director_reader(gpointer data);


/* Forward referenced subroutines */
void FillDirectory(const char *path, Window *window);
Window *new_window();
static void click_column_cb(GtkCList *item, gint column, Window *restore);
static void select_row_cb(GtkCList *item, gint row, gint column,
             GdkEventButton *event, Window *restore);
void row_data_destroy_cb(gpointer data);
void split_path_and_filename(const char *fname, POOLMEM **path, int *pnl,
        POOLMEM **file, int *fnl);

#ifdef needed
static GdkPixmap *check_pixmap = NULL;
static GdkPixmap *check_trans = NULL;
static GdkPixmap *blank_pixmap = NULL;
static GdkPixmap *blank_trans = NULL;
#endif

static GtkWidget *restore_dir;        /* current directory edit box */
static GtkWidget *scrolled;           /* select files scrolled window */
static Window *restore;

const int NUM_COLUMNS = 7;
const int CHECK_COLUMN = 0;
const int FILE_COLUMN = 1;
const int MODES_COLUMN = 2;
const int DATE_COLUMN = 6;


/*
 * Read Director output and discard it until next prompt
 */
static void discard_to_prompt()
{
   while (bnet_recv(UA_sock) > 0) {
      set_text(UA_sock->msg, UA_sock->msglen);
   }
}

/*
 * Move up one directory
 */
void
on_restore_up_button_clicked(GtkButton *button, gpointer user_data)
{
   split_path_and_filename(restore->fname, &restore->path, &restore->pnl,
                              &restore->file, &restore->fnl);
   FillDirectory(restore->path, restore);
}

static void mark_row(int row, bool mark)
{
   char *file;
   int len;
   char new_mark[10];

   gtk_clist_get_text(restore->list, row, FILE_COLUMN, &file);
   if (mark) {
      bstrncpy(new_mark, "x", sizeof(new_mark));
      len = Mmsg(&restore->buf, "mark %s", file);
   } else {
      bstrncpy(new_mark, " ", sizeof(new_mark));
      len = Mmsg(&restore->buf, "unmark %s", file);
   }
   gtk_clist_set_text(restore->list, row, CHECK_COLUMN, new_mark);
    /* strip trailing slash from directory name */
   while (len > 1 && restore->buf[len-1] == '/') {
      restore->buf[len-1] = 0;
   }
   write_director(restore->buf);
   discard_to_prompt();
}

void
on_restore_add_button_clicked(GtkButton *button, gpointer user_data)
{
   int num_selected = g_list_length(restore->list->selection);
   int row;

   for (int i=0; i < num_selected; i++) {
      row = (int)(long int)g_list_nth_data(restore->list->selection, i);
      mark_row(row, true);
   }
}


void
on_restore_remove_button_clicked(GtkButton *button, gpointer user_data)
{
   int num_selected = g_list_length(restore->list->selection);
   int row;

   for (int i=0; i < num_selected; i++) {
      row = (int)(long int)g_list_nth_data(restore->list->selection, i);
      mark_row(row, false);
   }
}

/*
 * Called once at the beginning of the program for setup
 */
void select_restore_setup()
{
   const gchar *title[NUM_COLUMNS] = {_("Mark"), _("File"), _("Mode"), _("User"), _("Group"), _("Size"), _("Date")};

   restore_file_selection = create_restore_file_selection();
   if (!restore_file_selection) {
      Dmsg0(000, "Restore_files not setup.\n");
   }
   restore_dir = lookup_widget(restore_file_selection, "restore_dir");
   scrolled = lookup_widget(restore_file_selection, "scrolled");
   if (!scrolled) {
      Dmsg0(000, "Scrolled not setup.\n");
   }

   restore = new_window();

#ifdef needed
   check_pixmap = gdk_pixmap_colormap_create_from_xpm(NULL,
                  gdk_colormap_get_system(), &check_trans, NULL,
                  "check.xpm");
   blank_pixmap = gdk_pixmap_colormap_create_from_xpm(NULL,
                  gdk_colormap_get_system(), &blank_trans, NULL,
                  "blank.xpm");
#endif

   /* XXX: Stupid gtk_clist_set_selection_mode() has incorrect declaration of the title argument */
   /* XXX: Workaround by typecast... peter@ifm.liu.se */

   restore->list = (GtkCList *)gtk_clist_new_with_titles(NUM_COLUMNS, (gchar **) title);
   gtk_clist_set_selection_mode(restore->list, GTK_SELECTION_EXTENDED);
   gtk_clist_set_sort_column(restore->list, FILE_COLUMN);
   gtk_clist_set_auto_sort(restore->list, true);
   gtk_signal_connect(GTK_OBJECT(restore->list), "click_column",
                      G_CALLBACK(click_column_cb), restore);
   gtk_signal_connect(GTK_OBJECT(restore->list), "select_row",
                      G_CALLBACK(select_row_cb), restore);

   gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(restore->list));
   restore->buf   = get_pool_memory(PM_FNAME);
   restore->fname = get_pool_memory(PM_FNAME);
   restore->path  = get_pool_memory(PM_NAME);
   restore->file  = get_pool_memory(PM_NAME);
}

/*
 * Select files dialog called
 */
void select_restore_files()
{
   discard_to_prompt();
   set_restore_dialog_defaults();
   gtk_widget_show(GTK_WIDGET(restore->list));
   FillDirectory("/", restore);
   gtk_main();
}


/*
 * Fill the CList box with files at path
 */
void FillDirectory(const char *path, Window *restore)
{
   char pathbuf[MAXSTRING];
   char modes[20], user[20], group[20], size[20], date[30];
   char file[1000];
   char marked[10];
   gchar *text[NUM_COLUMNS] = {marked, file, modes, user, group, size, date};
   GtkCList *list = restore->list;
   int row = 0;

   stop_director_reader(NULL);
   pm_strcpy(&restore->fname, path);
   gtk_entry_set_text(GTK_ENTRY(restore_dir), restore->fname);
   gtk_clist_freeze(list);
   gtk_clist_clear(list);

   bsnprintf(pathbuf, sizeof(pathbuf), "cd %s", path);
   Dmsg1(100, "%s\n", pathbuf);
   write_director(pathbuf);
   discard_to_prompt();

   write_director("dir");
   while (bnet_recv(UA_sock) > 0) {
      char *p = UA_sock->msg;
      char *l;
      strip_trailing_junk(UA_sock->msg);
      if (*p == '$') {
         break;
      }
      Dmsg1(200, "Got: %s\n", p);
      if (!*p) {
         continue;
      }
      l = p;
      skip_nonspaces(&p);             /* permissions */
      *p++ = 0;
      bstrncpy(modes, l, sizeof(modes));
      skip_spaces(&p);
      skip_nonspaces(&p);             /* link count */
      *p++ = 0;
      skip_spaces(&p);
      l = p;
      skip_nonspaces(&p);             /* user */
      *p++ = 0;
      skip_spaces(&p);
      bstrncpy(user, l, sizeof(user));
      l = p;
      skip_nonspaces(&p);             /* group */
      *p++ = 0;
      bstrncpy(group, l, sizeof(group));
      skip_spaces(&p);
      l = p;
      skip_nonspaces(&p);             /* size */
      *p++ = 0;
      bstrncpy(size, l, sizeof(size));
      skip_spaces(&p);
      l = p;
      skip_nonspaces(&p);             /* date/time */
      skip_spaces(&p);
      skip_nonspaces(&p);
      *p++ = 0;
      bstrncpy(date, l, sizeof(date));
      skip_spaces(&p);
      if (*p == '*') {
         bstrncpy(marked, "x", sizeof(marked));
         p++;
      } else {
         bstrncpy(marked, " ", sizeof(marked));
      }
      split_path_and_filename(p, &restore->path, &restore->pnl,
                              &restore->file, &restore->fnl);

//    Dmsg1(000, "restore->fname=%s\n", restore->fname);
      bstrncpy(file, restore->file, sizeof(file));
//    printf("modes=%s user=%s group=%s size=%s date=%s file=%s\n",
//       modes, user, group, size, date, file);

      gtk_clist_append(list, text);

      row++;
   }

   /* Fix up length of file column */
   gtk_clist_set_column_width(list, FILE_COLUMN, gtk_clist_optimal_column_width(list, FILE_COLUMN));
   gtk_clist_set_column_width(list, MODES_COLUMN, gtk_clist_optimal_column_width(list, MODES_COLUMN));
   gtk_clist_thaw(list);
   start_director_reader(NULL);
}

Window *new_window()
{
   Window *window = (Window *)malloc(sizeof(Window));
   memset(window, 0, sizeof(window));
   return window;
}


/*
 * User clicked a column title
 */
static void click_column_cb(GtkCList *item, gint column, Window *restore)
{
}

/*
 * User selected a row
 */
static void select_row_cb(GtkCList *item, gint row, gint column,
             GdkEventButton *event, Window *restore)
{
   char *file;
   char *marked = NULL;
   /* Column non-negative => double click */
   if (column >= 0) {
      gtk_clist_unselect_row(item, row, column);
      /* Double click on column 0 means to mark or unmark */
      if (column == 0) {
         gtk_clist_get_text(restore->list, row, CHECK_COLUMN, &marked);
         Dmsg1(200, "Marked=%s\n", marked);
         if (!marked || strcmp(marked, "x") != 0) {
            mark_row(row, true);
         } else {
            mark_row(row, false);
         }
      } else {
      /* Double clicking on directory means to move to it */
         int len;
         gtk_clist_get_text(item, row, FILE_COLUMN, &file);
         len = strlen(file);
         if (len > 0 && file[len-1] == '/') {
            /* Change to new directory */
            pm_strcpy(restore->path, restore->fname);
            if (*file == '*') {
               Mmsg(restore->fname, "%s%s", restore->path, file+1);
            } else {
               Mmsg(restore->fname, "%s%s", restore->path, file);
            }
            FillDirectory(restore->fname, restore);
         }
      }
   }
}

/*
 * CList row is being destroyed get rid of our data
 */
void row_data_destroy_cb(gpointer data)
{
   if (data) {
      free(data);
   }
}

#ifdef xxx
   GdkPixmap *pixmap, *trans;
         utf8_mark = g_locale_to_utf8(new_mark, -1, NULL, NULL, NULL);
         gtk_clist_get_pixmap(restore->list, row, CHECK_COLUMN, &pixmap, &trans);
         if (pixmap == blank_pixmap) {
            bstrncpy(new_mark, "x", sizeof(new_mark));
//          gtk_clist_set_pixmap(item, row, CHECK_COLUMN, check_pixmap, check_trans);
#endif
#ifdef xxx
static void window_delete_cb(GtkWidget *item, GdkEvent *event, Window *restore)
{
   gtk_widget_destroy(restore->window);
   gtk_main_quit();
   free_pool_memory(restore->buf);
   free_pool_memory(restore->fname);
   free_pool_memory(restore->path);
   free_pool_memory(restore->file);
   free(restore);
}
#endif
#ifdef xxx
      if (marked) {
         gtk_clist_set_pixmap(list, row, CHECK_COLUMN, check_pixmap, check_trans);
      } else {
         gtk_clist_set_pixmap(list, row, CHECK_COLUMN, blank_pixmap, blank_trans);
      }
#endif
