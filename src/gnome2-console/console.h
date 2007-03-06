/*
 * Bacula GNOME User Agent specific configuration and defines
 *
 *     Kern Sibbald, March 2002
 *
 *     Version $Id: console.h,v 1.6 2004/06/15 17:22:19 kerns Exp $
 */

#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#include "console_conf.h"
#include "jcr.h"
/* Super kludge for GNOME 2.0 */
#undef _
#undef N_
#undef textdomain

#include <gnome.h>

extern GtkWidget *app1;       /* application window */
extern GtkWidget *text1;      /* text window */
extern GtkWidget *entry1;     /* entry box */
extern GtkWidget *combo1;     /* Directory combo */
extern GtkWidget *dir_dialog; 
extern GtkWidget *dir_select;
extern GtkWidget *run_dialog;       /* run dialog */
extern GtkWidget *label_dialog;     
extern GtkWidget *restore_dialog;   /* restore dialog */
extern GtkWidget *restore_file_selection;   /* restore files dialog */
extern GtkWidget *about1;
extern GList *job_list, *client_list, *fileset_list;
extern GList *messages_list, *pool_list, *storage_list;
extern GList *type_list, *level_list;


extern pthread_mutex_t cmd_mutex;
extern pthread_cond_t  cmd_wait;
extern char cmd[1000];
extern int cmd_ready;
extern int reply;
extern BSOCK *UA_sock;



#define OK     1
#define CANCEL 0

void set_textf(const char *fmt, ...);
void set_text(const char *buf, int len);
void set_status(const char *buf);
void set_status_ready();
void set_statusf(const char *fmt, ...);
int connect_to_director(gpointer data);
int disconnect_from_director(gpointer data);
void start_director_reader(gpointer data);
void stop_director_reader(gpointer data);
void write_director(const gchar *msg);
void set_restore_dialog_defaults();
void select_restore_files();

extern "C" void read_director(gpointer data, gint fd, GdkInputCondition condition);

#endif
