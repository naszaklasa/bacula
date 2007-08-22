#ifndef _CONSOLE_H_
#define _CONSOLE_H_
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2007 Free Software Foundation Europe e.V.

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
/*
 *   Version $Id: console.h 5090 2007-06-25 19:15:34Z bartleyd2 $
 *
 *   Kern Sibbald, January 2007
 */

#include <QtGui>
#include "pages.h"
#include "ui_console.h"

#ifndef MAX_NAME_LENGTH
#define MAX_NAME_LENGTH 128
#endif

/*
 * Structure for obtaining the defaults for a job
 */
struct job_defaults {
   QString job_name;
   QString pool_name;
   QString messages_name;
   QString client_name;
   QString store_name;
   QString where;
   QString level;
   QString type;
   QString fileset_name;
   QString catalog_name;
   bool enabled;
};

class DIRRES;
class BSOCK;
class JCR;
class CONRES;

class Console : public Pages, public Ui::ConsoleForm
{
   Q_OBJECT 

public:
   Console(QStackedWidget *parent);
   ~Console();
   void display_text(const char *buf);
   void display_text(const QString buf);
   void display_textf(const char *fmt, ...);
   void display_html(const QString buf);
   void update_cursor(void);
   void write_dir(const char *buf);
   bool dir_cmd(const char *cmd, QStringList &results);
   bool dir_cmd(QString &cmd, QStringList &results);
   bool sql_cmd(const char *cmd, QStringList &results);
   bool sql_cmd(QString &cmd, QStringList &results);
   bool authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons, 
          char *buf, int buflen);
   bool is_connected() { return m_sock != NULL; };
   bool is_connectedGui();
   bool preventInUseConnect();
   const QFont get_font();
   void writeSettings();
   void readSettings();
   char *msg();
   void notify(bool enable);
   QStringList get_list(char *cmd);
   bool get_job_defaults(struct job_defaults &);
   void terminate();
   void beginNewCommand();
   void displayToPrompt();
   void discardToPrompt();
   void setDirectorTreeItem(QTreeWidgetItem *);
   void setDirRes(DIRRES *dir);
   QTreeWidgetItem *directorTreeItem() { return m_directorTreeItem; };
   void getDirResName(QString &);
   void startTimer();
   void stopTimer();
   void getVolumeList(QStringList &);
   void getStatusList(QStringList &);

   QStringList job_list;
   QStringList client_list;
   QStringList fileset_list;
   QStringList messages_list;
   QStringList pool_list;
   QStringList storage_list;
   QStringList type_list;
   QStringList level_list;


public slots:
   void connect_dir();                     
   void read_dir(int fd);
   int read(void);
   int write(const char *msg);
   int write(QString msg);
   void status_dir(void);
   void messages(void);
   void set_font(void);
   void poll_messages(void);
   void consoleHelp();
   void consoleReload();

public:
   DIRRES *m_dir;                  /* so various pages can reference it */

private:
   QTextEdit *m_textEdit;
   BSOCK *m_sock;   
   bool m_at_prompt;
   bool m_at_main_prompt;
   QSocketNotifier *m_notifier;
   QTextCursor *m_cursor;
   QTreeWidgetItem *m_directorTreeItem;
   bool m_api_set;
   bool m_messages_pending;
   QTimer *m_timer;
};

#endif /* _CONSOLE_H_ */
