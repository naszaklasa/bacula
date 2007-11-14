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
 *   Version $Id: console.cpp 5713 2007-10-03 11:36:47Z kerns $
 *
 *  Console Class
 *
 *   Kern Sibbald, January MMVII
 *
 */ 

#include "bat.h"
#include "console.h"
#include "restore.h"
#include "select.h"
#include "run/run.h"

static int tls_pem_callback(char *buf, int size, const void *userdata);


Console::Console(QStackedWidget *parent)
{
   QFont font;
   m_parent = parent;
   m_closeable = false;
   m_console = this;

   setupUi(this);
   m_sock = NULL;
   m_at_prompt = false;
   m_at_main_prompt = false;
   m_textEdit = textEdit;   /* our console screen */
   m_cursor = new QTextCursor(m_textEdit->document());
   mainWin->actionConnect->setIcon(QIcon(":images/disconnected.png"));

   m_timer = NULL;
   m_contextActions.append(actionStatusDir);
   m_contextActions.append(actionConsoleHelp);
   m_contextActions.append(actionRequestMessages);
   m_contextActions.append(actionConsoleReload);
   connect(actionStatusDir, SIGNAL(triggered()), this, SLOT(status_dir()));
   connect(actionConsoleHelp, SIGNAL(triggered()), this, SLOT(consoleHelp()));
   connect(actionConsoleReload, SIGNAL(triggered()), this, SLOT(consoleReload()));
   connect(actionRequestMessages, SIGNAL(triggered()), this, SLOT(messages()));
}

Console::~Console()
{
}

void Console::startTimer()
{
   m_timer = new QTimer(this);
   QWidget::connect(m_timer, SIGNAL(timeout()), this, SLOT(poll_messages()));
   m_timer->start(mainWin->m_checkMessagesInterval*1000);
}

void Console::stopTimer()
{
   if (m_timer) {
      QWidget::disconnect(m_timer, SIGNAL(timeout()), this, SLOT(poll_messages()));
      m_timer->stop();
      delete m_timer;
      m_timer = NULL;
   }
}
      
void Console::poll_messages()
{
   m_messages_pending = true;
   if ((m_at_main_prompt) && (mainWin->m_checkMessages)){
      write(".messages");
      displayToPrompt();
   }
}

/* Terminate any open socket */
void Console::terminate()
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
void Console::connect_dir()
{
   JCR *jcr = new JCR;
   utime_t heart_beat;
   char buf[1024];
   CONRES *cons;
      
   buf[0] = 0;

   m_textEdit = textEdit;   /* our console screen */

   if (!m_dir) {          
      mainWin->set_status("No Director found.");
      goto bail_out;
   }
   if (m_sock) {
      mainWin->set_status("Already connected.");
      goto bail_out;
   }

   memset(jcr, 0, sizeof(JCR));

   mainWin->set_statusf(_("Connecting to Director %s:%d"), m_dir->address, m_dir->DIRport);
   display_textf(_("Connecting to Director %s:%d\n\n"), m_dir->address, m_dir->DIRport);

   /* Give GUI a chance */
   app->processEvents();
   
   LockRes();
   /* If cons==NULL, default console will be used */
   cons = (CONRES *)GetNextRes(R_CONSOLE, NULL);
   UnlockRes();

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
         display_textf(_("Failed to initialize TLS context for Console \"%s\".\n"),
            m_dir->name());
         goto bail_out;
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
         display_textf(_("Failed to initialize TLS context for Director \"%s\".\n"),
            m_dir->name());
         mainWin->set_status("Connection failed");
         goto bail_out;
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
      goto bail_out;
   } else {
      /* Update page selector to green to indicate that Console is connected */
      mainWin->actionConnect->setIcon(QIcon(":images/connected.png"));
      QBrush greenBrush(Qt::green);
      QTreeWidgetItem *item = mainWin->getFromHash(this);
      item->setForeground(0, greenBrush);
   }

   jcr->dir_bsock = m_sock;

   if (!authenticate_director(jcr, m_dir, cons, buf, sizeof(buf))) {
      display_text(buf);
      goto bail_out;
   }

   if (buf[0]) {
      display_text(buf);
   }

   /* Give GUI a chance */
   app->processEvents();

   mainWin->set_status(_("Initializing ..."));

   /* Set up input notifier */
   m_notifier = new QSocketNotifier(m_sock->m_fd, QSocketNotifier::Read, 0);
   QObject::connect(m_notifier, SIGNAL(activated(int)), this, SLOT(read_dir(int)));

   write(".api 1");
   displayToPrompt();

   beginNewCommand();
   job_list.clear();
   client_list.clear();
   fileset_list.clear();
   fileset_list.clear();
   messages_list.clear();
   pool_list.clear();
   storage_list.clear();
   type_list.clear();
   level_list.clear();
   dir_cmd(".jobs", job_list);
   dir_cmd(".clients", client_list);
   dir_cmd(".filesets", fileset_list);  
   dir_cmd(".msgs", messages_list);
   dir_cmd(".pools", pool_list);
   dir_cmd(".storage", storage_list);
   dir_cmd(".types", type_list);
   dir_cmd(".levels", level_list);

   mainWin->set_status(_("Connected"));
   startTimer();                      /* start message timer */

bail_out:
   delete jcr;
   return;
}

bool Console::dir_cmd(QString &cmd, QStringList &results)
{
   return dir_cmd(cmd.toUtf8().data(), results);
}

/*
 * Send a command to the Director, and return the
 *  results in a QStringList.  
 */
bool Console::dir_cmd(const char *cmd, QStringList &results)
{
   int stat;

   notify(false);
   write(cmd);
   while ((stat = read()) > 0) {
      if (mainWin->m_displayAll) display_text(msg());
      strip_trailing_junk(msg());
      results << msg();
   }
   notify(true);
   discardToPrompt();
   return true;              /* ***FIXME*** return any command error */
}

bool Console::sql_cmd(QString &query, QStringList &results)
{
   return sql_cmd(query.toUtf8().data(), results);
}

/*
 * Send an sql query to the Director, and return the
 *  results in a QStringList.  
 */
bool Console::sql_cmd(const char *query, QStringList &results)
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
      if (mainWin->m_displayAll) display_text(msg());
      strip_trailing_junk(msg());
      results << msg();
   }
   notify(true);
   discardToPrompt();
   return true;              /* ***FIXME*** return any command error */
}


/*  
 * Send a job name to the director, and read all the resulting
 *  defaults. 
 */
bool Console::get_job_defaults(struct job_defaults &job_defs)
{
   QString scmd;
   int stat;
   char *def;

   notify(false);
   beginNewCommand();
   scmd = QString(".defaults job=\"%1\"").arg(job_defs.job_name);
   write(scmd);
   while ((stat = read()) > 0) {
      if (mainWin->m_displayAll) display_text(msg());
      def = strchr(msg(), '=');
      if (!def) {
         continue;
      }
      /* Pointer to default value */
      *def++ = 0;
      strip_trailing_junk(def);

      if (strcmp(msg(), "job") == 0) {
         if (strcmp(def, job_defs.job_name.toUtf8().data()) != 0) {
            goto bail_out;
         }
         continue;
      }
      if (strcmp(msg(), "pool") == 0) {
         job_defs.pool_name = def;
         continue;
      }
      if (strcmp(msg(), "messages") == 0) {
         job_defs.messages_name = def;
         continue;
      }
      if (strcmp(msg(), "client") == 0) {
         job_defs.client_name = def;
         continue;
      }
      if (strcmp(msg(), "storage") == 0) {
         job_defs.store_name = def;
         continue;
      }
      if (strcmp(msg(), "where") == 0) {
         job_defs.where = def;
         continue;
      }
      if (strcmp(msg(), "level") == 0) {
         job_defs.level = def;
         continue;
      }
      if (strcmp(msg(), "type") == 0) {
         job_defs.type = def;
         continue;
      }
      if (strcmp(msg(), "fileset") == 0) {
         job_defs.fileset_name = def;
         continue;
      }
      if (strcmp(msg(), "catalog") == 0) {
         job_defs.catalog_name = def;
         continue;
      }
      if (strcmp(msg(), "enabled") == 0) {
         job_defs.enabled = *def == '1' ? true : false;
         continue;
      }
   }

   notify(true);
   return true;

bail_out:
   notify(true);
   return false;
}


/*
 * Save user settings associated with this console
 */
void Console::writeSettings()
{
   QFont font = get_font();

   QSettings settings(m_dir->name(), "bat");
   settings.beginGroup("Console");
   settings.setValue("consoleFont", font.family());
   settings.setValue("consolePointSize", font.pointSize());
   settings.setValue("consoleFixedPitch", font.fixedPitch());
   settings.endGroup();
}

/*
 * Read and restore user settings associated with this console
 */
void Console::readSettings()
{ 
   QFont font = get_font();

   QSettings settings(m_dir->name(), "bat");
   settings.beginGroup("Console");
   font.setFamily(settings.value("consoleFont", "Courier").value<QString>());
   font.setPointSize(settings.value("consolePointSize", 10).toInt());
   font.setFixedPitch(settings.value("consoleFixedPitch", true).toBool());
   settings.endGroup();
   m_textEdit->setFont(font);
}

/*
 * Set the console textEdit font
 */
void Console::set_font()
{
   bool ok;
   QFont font = QFontDialog::getFont(&ok, QFont(m_textEdit->font()), this);
   if (ok) {
      m_textEdit->setFont(font);
   }
}

/*
 * Get the console text edit font
 */
const QFont Console::get_font()
{
   return m_textEdit->font();
}

/*
 * Slot for responding to status dir button on button bar
 */
void Console::status_dir()
{
   QString cmd("status dir");
   consoleCommand(cmd);
}

/*
 * Slot for responding to messages button on button bar
 */
void Console::messages()
{
   QString cmd(".messages");
   consoleCommand(cmd);
}

/*
 * Put text into the console window
 */
void Console::display_textf(const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[1000];
   int len;
   va_start(arg_ptr, fmt);
   len = bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   display_text(buf);
}

void Console::display_text(const QString buf)
{
   m_cursor->insertText(buf);
   update_cursor();
}


void Console::display_text(const char *buf)
{
   m_cursor->insertText(buf);
   update_cursor();
}

void Console::display_html(const QString buf)
{
   m_cursor->insertHtml(buf);
   update_cursor();
}

/* Position cursor to end of screen */
void Console::update_cursor()
{
   QApplication::restoreOverrideCursor();
   m_textEdit->moveCursor(QTextCursor::End);
   m_textEdit->ensureCursorVisible();
}

/* 
 * This should be moved into a bSocket class 
 */
char *Console::msg()
{
   if (m_sock) {
      return m_sock->msg;
   }
   return NULL;
}

/* Send a command to the Director */
void Console::write_dir(const char *msg)
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

int Console::write(const QString msg)
{
   return write(msg.toUtf8().data());
}

int Console::write(const char *msg)
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
 * Get to main command prompt -- i.e. abort any subcommand
 */
void Console::beginNewCommand()
{
   for (int i=0; i < 3; i++) {
      write(".");
      while (read() > 0) {
         if (mainWin->m_displayAll) display_text(msg());
      }
      if (m_at_main_prompt) {
         break;
      }
   }
   display_text("\n");
}

void Console::displayToPrompt()
{ 
   int stat = 0;
   if (mainWin->m_commDebug) Pmsg0(000, "DisplaytoPrompt\n");
   while (!m_at_prompt) {
      if ((stat=read()) > 0) {
         display_text(msg());
      }
   }
   if (mainWin->m_commDebug) Pmsg1(000, "endDisplaytoPrompt=%d\n", stat);
}

void Console::discardToPrompt()
{ 
   int stat = 0;
   if (mainWin->m_commDebug) Pmsg0(000, "discardToPrompt\n");
   while (!m_at_prompt) {
      if ((stat=read()) > 0) {
         if (mainWin->m_displayAll) display_text(msg());
      }
   }
   if (mainWin->m_commDebug) Pmsg1(000, "endDisplayToPrompt=%d\n", stat);
}


/* 
 * Blocking read from director
 */
int Console::read()
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
            display_text("\n");
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
         new selectDialog(this);    
         break;
      case BNET_YESNO:
         if (mainWin->m_commDebug) Pmsg0(000, "YESNO\n");
         new yesnoPopUp(this);
         break;
      case BNET_RUN_CMD:
         if (mainWin->m_commDebug) Pmsg0(000, "RUN CMD\n");
         new runCmdPage();
         break;
      case BNET_START_RTREE:
         if (mainWin->m_commDebug) Pmsg0(000, "START RTREE CMD\n");
         new restorePage();
         break;
      case BNET_END_RTREE:
         if (mainWin->m_commDebug) Pmsg0(000, "END RTREE CMD\n");
         break;
      case BNET_ERROR_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "ERROR MSG\n");
         m_sock->recv();              /* get the message */
         display_text(msg());
         QMessageBox::critical(this, "Error", msg(), QMessageBox::Ok);
         break;
      case BNET_WARNING_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "WARNING MSG\n");
         m_sock->recv();              /* get the message */
         display_text(msg());
         QMessageBox::critical(this, "Warning", msg(), QMessageBox::Ok);
         break;
      case BNET_INFO_MSG:
         if (mainWin->m_commDebug) Pmsg0(000, "INFO MSG\n");
         m_sock->recv();              /* get the message */
         display_text(msg());
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
void Console::read_dir(int /* fd */)
{
   if (mainWin->m_commDebug) Pmsg0(000, "read_dir\n");
   while (read() >= 0) {
      display_text(msg());
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
void Console::notify(bool enable) 
{ 
   m_notifier->setEnabled(enable);   
}

void Console::setDirectorTreeItem(QTreeWidgetItem *item)
{
   m_directorTreeItem = item;
}

void Console::setDirRes(DIRRES *dir) 
{ 
   m_dir = dir;
}

/*
 * To have the ability to get the name of the director resource.
 */
void Console::getDirResName(QString &name_returned)
{
   name_returned = m_dir->name();
}

bool Console::is_connectedGui()
{
   if (is_connected()) {
      return true;
   } else {
      QString message("Director ");
      message += " is currently disconnected\n  Please reconnect!!";
      QMessageBox::warning(this, "Bat",
         tr(message.toUtf8().data()), QMessageBox::Ok );
      return false;
   }
}

/*
 * A temporary function to prevent connecting to the director if the director
 * is busy with a restore.
 */
bool Console::preventInUseConnect()
{
   if (!is_connected()) {
      QString message("Director ");
      message += m_dir->name();
      message += " is currently disconnected\n  Please reconnect!!";
      QMessageBox::warning(this, "Bat",
         tr(message.toUtf8().data()), QMessageBox::Ok );
      return false;
   } else if (!m_at_main_prompt){
      QString message("Director ");
      message += m_dir->name();
      message += " is currently busy\n  Please complete restore or other "
                 " operation !!  This is a limitation that will be resolved before a beta"
                 " release.  This is currently an alpha release.";
      QMessageBox::warning(this, "Bat",
         tr(message.toUtf8().data()), QMessageBox::Ok );
      return false;
   } else if (!m_at_prompt){
      QString message("Director ");
      message += m_dir->name();
      message += " is currently not at a prompt\n  Please try again!!";
      QMessageBox::warning(this, "Bat",
         tr(message.toUtf8().data()), QMessageBox::Ok );
      return false;
   } else {
      return true;
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

/* Slot for responding to page selectors status help command */
void Console::consoleHelp()
{
   QString cmd("help");
   consoleCommand(cmd);
}

/* Slot for responding to page selectors reload bacula-dir.conf */
void Console::consoleReload()
{
   QString cmd("reload");
   consoleCommand(cmd);
}

/* Function to get a list of volumes */
void Console::getVolumeList(QStringList &volumeList)
{
   QString query("SELECT VolumeName AS Media FROM Media ORDER BY Media");
   if (mainWin->m_sqlDebug) {
      Pmsg1(000, "Query cmd : %s\n",query.toUtf8().data());
   }
   QStringList results;
   if (sql_cmd(query, results)) {
      QString field;
      QStringList fieldlist;
      /* Iterate through the lines of results. */
      foreach (QString resultline, results) {
         fieldlist = resultline.split("\t");
         volumeList.append(fieldlist[0]);
      } /* foreach resultline */
   } /* if results from query */
}

/* Function to get a list of volumes */
void Console::getStatusList(QStringList &statusLongList)
{
   QString statusQuery("SELECT JobStatusLong FROM Status");
   if (mainWin->m_sqlDebug) {
      Pmsg1(000, "Query cmd : %s\n",statusQuery.toUtf8().data());
   }
   QStringList statusResults;
   if (sql_cmd(statusQuery, statusResults)) {
      QString field;
      QStringList fieldlist;
      /* Iterate through the lines of results. */
      foreach (QString resultline, statusResults) {
         fieldlist = resultline.split("\t");
         statusLongList.append(fieldlist[0]);
      } /* foreach resultline */
   } /* if results from statusquery */
}
