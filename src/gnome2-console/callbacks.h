
/*   Version $Id$  */

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


gboolean
on_app1_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_new_file1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_properties1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_clicked                      (GnomeDialog     *gnomedialog,
                                        gint             arg1,
                                        gpointer         user_data);

gboolean
on_entry1_key_press_event              (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_app1_key_press_event                (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_select_director_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_connect_button1_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_app1_realize                        (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_combo_entry1_key_press_event        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_entry1_key_press_event              (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_entry1_key_press_event              (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_app1_show                           (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_connect_button1_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_run_button1_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_msgs_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_msgs_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_run_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_disconnect_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_connect_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_connect_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_clicked                       (GnomeDialog     *gnomedialog,
                                        gint             arg1,
                                        gpointer         user_data);

gboolean
on_app2_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_app2_show                           (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_app1_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_app1_show                           (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_entry1_key_press_event              (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_about_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_select_director_OK_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_select_director_cancel_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_run_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_run_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_entry1_key_release_event            (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_restore_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_type_combo_selection_received       (GtkWidget       *widget,
                                        GtkSelectionData *data,
                                        guint            time,
                                        gpointer         user_data);

void
on_view_fileset_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_entry_job_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_restore_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_file_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_label_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_label_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_label_cancel_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cut2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_properties1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_restore_files_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_restore_up_button_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_add_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_remove_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_select_ok_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_select_cancel_clicked       (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_console_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_select_files_button_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_job_entry_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_files_button_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_restore_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_dir_button_clicked                  (GtkButton   *toolbutton,
                                        gpointer         user_data);
