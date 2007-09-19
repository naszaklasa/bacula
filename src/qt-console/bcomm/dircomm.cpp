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
 *   Version $Id: dircomm.cpp 5366 2007-08-17 10:18:43Z kerns $
 *
 *  Bacula Communications class that is at a higher level than BSOCK
 *
 *   Kern Sibbald, May MMVII
 *
 */ 

#include "bat.h"
#include "console.h"
#include "restore.h"
#include "select.h"
#include "run/run.h"
#include "dircomm.h"

static int tls_pem_callback(char *buf, int size, const void *userdata);


DirComm::DirComm(Console *console)
{
   m_console = console;
   m_sock = NULL;
   m_at_prompt = false;
   m_at_main_prompt = false;
   m_timer = NULL;
}

DirComm::~DirComm()
{
   if (m_timer) {
      stopTimer();
   }
}

void DirComm::startTimer()
{
   m_timer = new QTimer(m_console);
   QWidget::connect(m_timer, SIGNAL(timeout()), this, SLOT(poll_messages()));
   m_timer->start(mainWin->m_checkMessagesInterval*1000);
}

void DirComm::stopTimer()
{
   if (m_timer) {
      QWidget::disconnect(m_timer, SIGNAL(timeout()), this, SLOT(poll_messages()));
      m_timer->stop();
      delete m_timer;
      m_timer = NULL;
   }
}
      
void DirComm::poll_messages()
{
   m_messages_pending = true;
   if ((m_at_main_prompt) && (mainWin->m_checkMessages)){
      write(".messages");
      displayToPrompt();
   }
}

/* Terminate any open socket */
void DirComm::terminate()
{
   if (m_sock) {
      stopTimer();
      m_sock->close();
      m_sock = NULL;
   }
}

/*
 * Connect to Director. 
 */
void DirComm::connect_dir(DIRRES *dir, CONRES *cons)
{
   JCR jcr;
   utime_t heart_beat;
   char buf[1024];

   m_dir = dir;
   if (!m_dir) {          
      mainWin->set_status("No Director found.");
      return;
   }
   if (m_sock) {
      mainWin->set_status("Already connected.");
      return;
   }

   memset(&jcr, 0, sizeof(jcr));

   mainWin->set_statusf(_("Connecting to Director %s:%d"), m_dir->address, m_dir->DIRport);
   m_console->display_textf(_("Connecting to Director %s:%d\n\n"), m_dir->address, m_dir->DIRport);

   /* Give GUI a chance */
   app->processEvents();
   
   /* Initialize Console TLS context once */
   if (cons && !cons->tls_ctx && (cons->tls_enable || cons->tls_require)) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), "Passphrase for Console \"%s\" TLS private key: ", 
                cons->name());

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer   
       */
      cons->tls_ctx = new_tls_context(cons->tls_ca_certfile,
         cons->tls_ca_certdir, cons->tls_certfile,
         cons->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!cons->tls_ctx) {
         m_console->display_textf(_("Failed to initialize TLS context for Console \"%s\".\n"),
            m_dir->name());
         return;
      }
   }

   /* Initialize Director TLS context once */
   if (!m_dir->tls_ctx && (m_dir->tls_enable || m_dir->tls_require)) {
      /* Generate passphrase prompt */
      bsnprintf(buf, sizeof(buf), "Passphrase for Director \"%s\" TLS private key: ", 
                m_dir->name());

      /* Initialize TLS context:
       * Args: CA certfile, CA certdir, Certfile, Keyfile,
       * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
      m_dir->tls_ctx = new_tls_context(m_dir->tls_ca_certfile,
                          m_dir->tls_ca_certdir, m_dir->tls_certfile,
                          m_dir->tls_keyfile, tls_pem_callback, &buf, NULL, true);

      if (!m_dir->tls_ctx) {
         m_console->display_textf(_("Failed to initialize TLS context for Director \"%s\".\n"),
            m_dir->name());
         mainWin->set_status("Connection failed");
         return;
      }
   }

   if (m_dir->heartbeat_interval) {
      heart_beat = m_dir->heartbeat_interval;
   } else if (cons) {
      heart_beat = cons->heartbeat_interval;
   } else {
      heart_beat = 0;
   }        

   m_sock = bnet_connect(NULL, 5, 15, heart_beat,
                          _("Director daemon"), m_dir->address,
                          NULL, m_dir->DIRport, 0);
   if (m_sock == NULL) {
      mainWin->set_status("Connection failed");
      return;
   } else {
      /* Update page selector to green to indicate that Console is connected */
      mainWin->actionConnect->setIcon(QIcon(":images/connected.png"));
      QBrush greenBrush(Qt::green);
      QTreeWidgetItem *item = mainWin->getFromHash(this);
      item->setForeground(0, greenBrush);
   }

   jcr.dir_bsock = m_sock;

   if (!authenticate_director(&jcr, m_dir, cons, buf, sizeof(buf))) {
      m_console->display_text(buf);
      return;
   }
   if (buf[0]) {
      m_console->display_text(buf);
   }

   /* Give GUI a chance */
   app->processEvents();

   mainWin->set_status(_("Initializing ..."));

   /* Set up input notifier */
   m_notifier = new QSocketNotifier(m_sock->m_fd, QSocketNotifier::Read, 0);
   QObject::connect(m_notifier, SIGNAL(activated(int)), this, SLOT(read_dir(int)));

   mainWin->set_status(_("Connected"));
   startTimer();                      /* start message timer */
   return;
}

bool DirComm::dir_cmd(QString &cmd, QStringList &results)
{
   return dir_cmd(cmd.toUtf8().data(), results);
}

/*
 * Send a command to the Director, and return the
 *  results in a QStringList.  
 */
bool DirComm::dir_cmd(const char *cmd, QStringList &results)
{
   int stat;

   notify(false);
   write(cmd);
   while ((stat = read()) > 0) {
      if (mainWin->m_displayAll) m_console->display_text(msg());
      strip_trailing_junk(msg());
      results << msg();
   }
   notify(true);
   discardToPrompt();
   return true;              /* ***FIXME*** return any command error */
}

bool DirComm::sql_cmd(QString &query, QStringList &results)
{
   return sql_cmd(query.toUtf8().data(), results);
}

/*
 * Send an sql query to the Director, and return the
 *  results in a QStringList.  
 */
bool DirComm::sql_cmd(const char *query, QStringList &results)
{
   int stat;
   POOL_MEM cmd(PM_MESSAGE);

   if (!is_connectedGui()) {
      return false;
   }

   notify(false);
   
   pm_strcpy(cmd, ".sql query=\"");
   pm_strcat(cmd, query);
   pm_strcat(cmd, "\"");
   write(cmd.c_str());
   while ((stat = read()) > 0) {
      if (mainWin->m_displayAll) m_console->display_text(msg());
      strip_trailing_junk(msg());
      results << msg();
   }
   notify(true);
   discardToPrompt();
   return true;              /* ***FIXME*** return any command error */
}

/* 
 * This should be moved into a bSocket class 
 */
char *DirComm::msg()
{
   if (m_sock) {
      return m_sock->msg;
   }
   return NULL;
}

/* Send a command to the Director */
void DirComm::write_dir(const char *msg)
{
   if (m_sock) {
      mainWin->set_status(_("Processing command ..."));
      QApplication::setOverrideCursor(Qt::WaitCursor);
      write(msg);
   } else {
      mainWin->set_status(" Director not connected. Click on connect button.");
      mainWin->actionConnect->setIcon(QIcon(":images/disconnected.png"));
      QBrush redBrush(Qt::red);
      QTreeWidgetItem *item = mainWin->getFromHash(this);
      item->setForeground(0, redBrush);
      m_at_prompt = false;
      m_at_main_prompt = false;
   }
}

int DirComm::write(const QString msg)
{
   return write(msg.toUtf8().data());
}

int DirComm::write(const char *msg)
{
   if (!m_sock) {
      return -1;
   }
   m_sock->msglen = pm_strcpy(m_sock->msg, msg);
   m_at_prompt = false;
   m_at_main_prompt = false;
   if (mainWin->m_commDebug) Pmsg1(000, "send: %s\n", msg);
   return m_sock->send();

}

/*
 * Get to main command prompt -- i.e. abort any suDirCommand
 */
void DirComm::beginNewCommand()
{
   for (int i=0; i < 3; i++) {
      write(".\n");
      while (read() > 0) {
         if (mainWin->m_displayAll) m_console->display_text(msg());
      }
      if (m_at_main_prompt) {
         break;
      }
   }
   m_console->display_text("\n");
}

void DirComm::displayToPrompt()
{ 
   int stat = 0;
   if (mainWin->m_commDebug) Pmsg0(000, "DisplaytoPrompt\n");
   while (!m_at_prompt) {
      if ((stat=read()) > 0) {
         m_console->display_text(msg());
      }
   }
   if (mainWin->m_commDebug) Pmsg1(000, "endDisplaytoPrompt=%d\n", stat);
}

void DirComm::discardToPrompt()
{ 
   int stat = 0;
   if (mainWin->m_commDebug) Pmsg0(000, "discardToPrompt\n");
   while (!m_at_prompt) {
      if ((stat=read()) > 0) {
         if (mainWin->m_displayAll) m_console->display_text(msg());
      }
   }
   if (mainWin->m_commDebug) Pmsg1(000, "endDisplayToPrompt=%d\n", stat);
}


/* 
 * Blocking read from director
 */
int DirComm::read()
{
   int stat = 0;
   while (m_sock) {
      for (;;) {
         stat = bnet_wait_data_intr(m_sock, 1);
         if (stat > 0) {
            break;
         } 
         app->processEvents();
         if (m_api_set && m_messages_pending) {
            write_dir(".messages");
            m_messages_pending = false;
         }
      }
      m_sock->msg[0] = 0;
      stat = m_sock->recv();
      if (stat >= 0) {
         if (mainWin->m_commDebug) Pmsg1(000, "got: %s\n", m_sock->msg);
         if (m_at_prompt) {
            m_console->display_text("\n");
            m_at_prompt = false;
            m_at_main_prompt = false;
         }
      }
      switch (m_sock->msglen) {
      case BNET_MSGS_PENDING :
         if (m_notifier->isEnabled()) {
            if (mainWin->m_commDebug) Pmsg0(000, "MSGS PENDING\n");
            write_dir(".messages");
            displayToPrompt();
            m_messages_pending = false;
         }
         m_messages_pending = true;
         continue;
      case BNET_CMD_OK:
         if (mainWin->m_commDebug) Pmsg0(000, "CMD OK\n");
         m_at_prompt = false;
         m_at_main_prompt = false;
         mainWin->set_status(_("Command completed ..."));
         continue;
      case BNET_CMD_BEGIN:
         if (mainWin->m_commDebug) Pmsg0(000, "CMD BEGIN\n");
         m_at_prompt = false;
         m_at_main_prompt = false;
         mainWin->set_status(_("Processing command ..."));
         continue;
      case BNET_MAIN_PROMPT:
         if (mainWin->m_commDebug) Pmsg0(000, "MAIN PROMPT\n");
         m_at_prompt = true;
         m_at_main_prompt = true;
         mainWin->set_status(_("At main prompt waiting for input ..."));
         QApplication::restoreOverrideCursor();
         break;
      case BNET_PROMPT:
         if (mainWin->m_commDebug) Pmsg0(000, "PROMPT\n");
         m_at_prompt = true;
         m_at_main_prompt = false;
         mainWin->set_status(_("At prompt waiting for input ..."));
         QApplication::restoreOverrideCursor();
         break;
      case BNET_CMD_FAILED:
         if (mainWin->m_commDebug) Pmsg0(000, "CMD FAILED\n");
         mainWin->set_status(_("Command failed."));
         QApplication::restoreOverrideCursor();
         break;
      /* We should not get this one */
      case BNET_EOD:
         if (mainWin->m_commDebug) Pmsg0(000, "EOD\n");
         mainWin->set_status_ready();
         QApplication::restoreOverrideCursor();
         if (!m_api_set) {
            break;
         }
         continue;
      case BNET_START_SELECT:
         if (mainWin->m_commDebug) Pmsg0(000, "START SELECT\n");
         new selectDialog(m_console);
         break;
      case BNET_YESNO:
         if (mainWin->m_commDebug) Pmsg0(000, "YESNO\n");
         new yesnoPopUp(m_console);
         break;
      case BNET_RUN_CMD:
         if (mainWin->m_commDebug) Pmsg0(000, "RUN CMD\n");
         new runCmdPage();
         break;
      case BNET_ERROR_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "ERROR MSG\n");
         m_sock->recv();              /* get the message */
         m_console->display_text(msg());
         QMessageBox::critical(this, "Error", msg(), QMessageBox::Ok);
         break;
      case BNET_WARNING_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "WARNING MSG\n");
         m_sock->recv();              /* get the message */
         m_console->display_text(msg());
         QMessageBox::critical(this, "Warning", msg(), QMessageBox::Ok);
         break;
      case BNET_INFO_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "INFO MSG\n");
         m_sock->recv();              /* get the message */
         m_console->display_text(msg());
         mainWin->set_status(msg());
         break;
      }
      if (is_bnet_stop(m_sock)) {         /* error or term request */
         if (mainWin->m_commDebug) Pmsg0(000, "BNET STOP\n");
         stopTimer();
         m_sock->close();
         m_sock = NULL;
         mainWin->actionConnect->setIcon(QIcon(":images/disconnected.png"));
         QBrush redBrush(Qt::red);
         QTreeWidgetItem *item = mainWin->getFromHash(this);
         item->setForeground(0, redBrush);
         m_notifier->setEnabled(false);
         delete m_notifier;
         m_notifier = NULL;
         mainWin->set_status(_("Director disconnected."));
         QApplication::restoreOverrideCursor();
         stat = BNET_HARDEOF;
      }
      break;
   } 
   return stat;
}

/* Called by signal when the Director has output for us */
void DirComm::read_dir(int /* fd */)
{
   if (mainWin->m_commDebug) Pmsg0(000, "read_dir\n");
   while (read() >= 0) {
      m_console->display_text(msg());
   }
}

/*
 * When the notifier is enabled, read_dir() will automatically be
 * called by the Qt event loop when ever there is any output 
 * from the Directory, and read_dir() will then display it on
 * the console.
 *
 * When we are in a bat dialog, we want to control *all* output
 * from the Directory, so we set notify to off.
 *    m_console->notifiy(false);
 */
void DirComm::notify(bool enable) 
{ 
   m_notifier->setEnabled(enable);   
}

bool DirComm::is_connectedGui()
{
   if (is_connected()) {
      return true;
   } else {
      QString message("Director ");
      message += " is curerntly disconnected\n  Please reconnect!!";
      QMessageBox::warning(this, tr("Bat"),
         tr(message.toUtf8().data()), QMessageBox::Ok );
      return false;
   }
}

/*
 * Call-back for reading a passphrase for an encrypted PEM file
 * This function uses getpass(), 
 *  which uses a static buffer and is NOT thread-safe.
 */
static int tls_pem_callback(char *buf, int size, const void *userdata)
{
   (void)size;
   (void)userdata;
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
