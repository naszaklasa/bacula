
/*   Version $Id: callbacks.h,v 1.11 2003/05/03 10:39:35 kerns Exp $  */

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
