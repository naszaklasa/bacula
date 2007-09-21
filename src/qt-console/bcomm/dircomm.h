#ifndef _DIRCOMM_H_
#define _DIRCOMM_H_
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
 *   Version $Id: dircomm.h 5366 2007-08-17 10:18:43Z kerns $
 * 
 *   This is an attempt to move the higher level read/write type
 *     functionality to the Director out of the Console class into
 *     a Bacula Communications class.  This class builds on the 
 *     BSOCK class.
 * 
 *
 *   Kern Sibbald, May 2007
 */

#include "bat.h"
#include "pages.h"

class DIRRES;
class BSOCK;
class CONRES;

class DirComm : public Pages
{
   Q_OBJECT

public:
   DirComm(Console *console);
   ~DirComm();
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
   char *msg();
   void notify(bool enable);
   void terminate();
   void beginNewCommand();
   void displayToPrompt();
   void discardToPrompt();
   void setDirectorTreeItem(QTreeWidgetItem *);
   void setDirRes(DIRRES *dir);
   void getDirResName(QString &);
   void startTimer();
   void stopTimer();

public slots:
   void connect_dir(DIRRES *dir, CONRES *cons);
   void read_dir(int fd);
   int read(void);
   int write(const char *msg);
   int write(QString msg);
   void poll_messages(void);

public:

private:
   DIRRES *m_dir;
   BSOCK *m_sock;   
   bool m_at_prompt;
   bool m_at_main_prompt;
   QSocketNotifier *m_notifier;
   Console *m_console;
   bool m_api_set;
   bool m_messages_pending;
   QTimer *m_timer;
};

#endif /* _DIRCOMM_H_ */
