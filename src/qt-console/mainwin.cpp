/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2008 Free Software Foundation Europe e.V.

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
 *   Version $Id: mainwin.cpp 7475 2008-08-14 07:21:08Z kerns $
 *
 *  Main Window control for bat (qt-console)
 *
 *   Kern Sibbald, January MMVII
 *
 */ 

#include "bat.h"
#include "version.h"
#include "joblist/joblist.h"
#include "storage/storage.h"
#include "fileset/fileset.h"
#include "label/label.h"
#include "run/run.h"
#include "pages.h"
#include "restore/restore.h"
#include "medialist/medialist.h"
#include "joblist/joblist.h"
#include "clients/clients.h"
#include "restore/restoretree.h"
#include "help/help.h"
#include "jobs/jobs.h"
#ifdef HAVE_QWT
#include "jobgraphs/jobplot.h"
#endif

/* 
 * Daemon message callback
 */
void message_callback(int /* type */, char *msg)
{
   QMessageBox::warning(mainWin, "Bat", msg, QMessageBox::Ok);
}

MainWin::MainWin(QWidget *parent) : QMainWindow(parent)
{
   m_isClosing = false;
   m_dtformat = "yyyy-MM-dd HH:mm:ss";
   mainWin = this;
   setupUi(this);                     /* Setup UI defined by main.ui (designer) */
   register_message_callback(message_callback);
   readPreferences();
   treeWidget->clear();
   treeWidget->setColumnCount(1);
   treeWidget->setHeaderLabel("Select Page");
   treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

   createPages();

   resetFocus();

   createConnections();

   this->show();

   readSettings();

   foreach(Console *console, m_consoleHash) {
      console->connect_dir();
   }
   m_currentConsole = (Console*)getFromHash(m_firstItem);
   m_currentConsole->setCurrent();
   if (m_miscDebug) {
      QString directoryResourceName;
      m_currentConsole->getDirResName(directoryResourceName);
      Pmsg1(000, "Setting initial window to %s\n", directoryResourceName.toUtf8().data());
   }
}

void MainWin::createPages()
{
   DIRRES *dir;
   QTreeWidgetItem *item, *topItem;
   m_firstItem = NULL;

   LockRes();
   foreach_res(dir, R_DIRECTOR) {

      /* Create console tree stacked widget item */
      m_currentConsole = new Console(stackedWidget);
      m_currentConsole->setDirRes(dir);
      m_currentConsole->readSettings();

      /* The top tree item representing the director */
      topItem = new QTreeWidgetItem(treeWidget);
      topItem->setText(0, dir->name());
      topItem->setIcon(0, QIcon(":images/server.png"));
      /* Set background to grey for ease of identification of inactive Director */
      QBrush greyBrush(Qt::lightGray);
      topItem->setBackground(0, greyBrush);
      m_currentConsole->setDirectorTreeItem(topItem);
      m_consoleHash.insert(topItem, m_currentConsole);

      /* Create Tree Widget Item */
      item = new QTreeWidgetItem(topItem);
      item->setText(0, "Console");
      if (!m_firstItem){ m_firstItem = item; }
      item->setIcon(0,QIcon(QString::fromUtf8(":images/utilities-terminal.png")));

      /* insert the cosole and tree widget item into the hashes */
      hashInsert(item, m_currentConsole);

      /* Set Color of treeWidgetItem for the console
      * It will be set to green in the console class if the connection is made.
      */
      QBrush redBrush(Qt::red);
      item->setForeground(0, redBrush);
      m_currentConsole->dockPage();

      /*
       * Create instances in alphabetic order of the rest 
       *  of the classes that will by default exist under each Director.  
       */
//      new bRestore();
      new Clients();
      new FileSet();
      new Jobs();
      createPageJobList("", "", "", "", NULL);

#ifdef HAVE_QWT
      JobPlotPass pass;
      pass.use = false;
      new JobPlot(NULL, pass);
#endif
      new MediaList();
      new Storage();
      new restoreTree();

      treeWidget->expandItem(topItem);
      stackedWidget->setCurrentWidget(m_currentConsole);
   }
   UnlockRes();
}

/*
 * create an instance of the the joblist class on the stack
 */
void MainWin::createPageJobList(const QString &media, const QString &client,
              const QString &job, const QString &fileset, QTreeWidgetItem *parentTreeWidgetItem)
{
   QTreeWidgetItem *holdItem;

   /* save current tree widget item in case query produces no results */
   holdItem = treeWidget->currentItem();
   JobList* joblist = new JobList(media, client, job, fileset, parentTreeWidgetItem);
   /* If this is a query of jobs on a specific media */
   if ((media != "") || (client != "") || (job != "") || (fileset != "")) {
      joblist->setCurrent();
      /* did query produce results, if not close window and set back to hold */
      if (joblist->m_resultCount == 0) {
         joblist->closeStackPage();
         treeWidget->setCurrentItem(holdItem);
      }
   }
}

/*
 * Handle up and down arrow keys for the command line
 *  history.
 */
void MainWin::keyPressEvent(QKeyEvent *event)
{
   if (m_cmd_history.size() == 0) {
      event->ignore();
      return;
   }
   switch (event->key()) {
   case Qt::Key_Down:
      if (m_cmd_last < 0 || m_cmd_last >= (m_cmd_history.size()-1)) {
         event->ignore();
         return;
      }
      m_cmd_last++;
      break;
   case Qt::Key_Up:
      if (m_cmd_last == 0) {
         event->ignore();
         return;
      }
      if (m_cmd_last < 0 || m_cmd_last > (m_cmd_history.size()-1)) {
         m_cmd_last = m_cmd_history.size() - 1;
      } else {
         m_cmd_last--;
      }
      break;
   default:
      event->ignore();
      return;
   }
   lineEdit->setText(m_cmd_history[m_cmd_last]);
}

void MainWin::createConnections()
{
   /* Connect signals to slots */
   connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(input_line()));
   connect(actionAbout_bat, SIGNAL(triggered()), this, SLOT(about()));
   connect(actionBat_Help, SIGNAL(triggered()), this, SLOT(help()));
   connect(treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, 
           SLOT(treeItemClicked(QTreeWidgetItem *, int)));
   connect(treeWidget, SIGNAL(
           currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(treeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
   connect(stackedWidget, SIGNAL(currentChanged(int)),
           this, SLOT(stackItemChanged(int)));
   connect(actionQuit, SIGNAL(triggered()), app, SLOT(closeAllWindows()));
   connect(actionLabel, SIGNAL(triggered()), this,  SLOT(labelButtonClicked()));
   connect(actionRun, SIGNAL(triggered()), this,  SLOT(runButtonClicked()));
   connect(actionEstimate, SIGNAL(triggered()), this,  SLOT(estimateButtonClicked()));
   connect(actionBrowse, SIGNAL(triggered()), this,  SLOT(browseButtonClicked()));
#ifdef HAVE_QWT
   connect(actionJobPlot, SIGNAL(triggered()), this,  SLOT(jobPlotButtonClicked()));
#endif
   connect(actionRestore, SIGNAL(triggered()), this,  SLOT(restoreButtonClicked()));
   connect(actionUndock, SIGNAL(triggered()), this,  SLOT(undockWindowButton()));
   connect(actionToggleDock, SIGNAL(triggered()), this,  SLOT(toggleDockContextWindow()));
   connect(actionClosePage, SIGNAL(triggered()), this,  SLOT(closePage()));
   connect(actionPreferences, SIGNAL(triggered()), this,  SLOT(setPreferences()));
}

/* 
 * Reimplementation of QWidget closeEvent virtual function   
 */
void MainWin::closeEvent(QCloseEvent *event)
{
   m_isClosing = true;
   writeSettings();
   /* close all non console pages, this will call settings in destructors */
   while (m_consoleHash.count() < m_pagehash.count()) {
      foreach(Pages *page, m_pagehash) {
         if (page !=  page->console()) {
            QTreeWidgetItem* pageSelectorTreeWidgetItem = mainWin->getFromHash(page);
            if (pageSelectorTreeWidgetItem->childCount() == 0) {
               page->console()->setCurrent();
               page->closeStackPage();
            }
         }
      }
   }
   foreach(Console *console, m_consoleHash){
      console->writeSettings();
      console->terminate();
      console->closeStackPage();
   }
   event->accept();
}

void MainWin::writeSettings()
{
   QSettings settings("bacula.org", "bat");

   settings.beginGroup("MainWin");
   settings.setValue("winSize", size());
   settings.setValue("winPos", pos());
   settings.setValue("state", saveState());
   settings.endGroup();
}

void MainWin::readSettings()
{ 
   QSettings settings("bacula.org", "bat");

   settings.beginGroup("MainWin");
   resize(settings.value("winSize", QSize(1041, 801)).toSize());
   move(settings.value("winPos", QPoint(200, 150)).toPoint());
   restoreState(settings.value("state").toByteArray());
   settings.endGroup();
}

/*
 * This subroutine is called with an item in the Page Selection window
 *   is clicked 
 */
void MainWin::treeItemClicked(QTreeWidgetItem *item, int /*column*/)
{
   /* Is this a page that has been inserted into the hash  */
   if (getFromHash(item)) {
      Pages* page = getFromHash(item);
      int stackindex=stackedWidget->indexOf(page);

      if (stackindex >= 0) {
         stackedWidget->setCurrentWidget(page);
      }
      /* run the virtual function in case this class overrides it */
      page->PgSeltreeWidgetClicked();
   }
}

/*
 * Called with a change of the highlighed tree widget item in the page selector.
 */
void MainWin::treeItemChanged(QTreeWidgetItem *currentitem, QTreeWidgetItem *previousitem)
{
   if (m_isClosing) return; /* if closing the application, do nothing here */

   Pages *previousPage, *nextPage;
   Console *previousConsole = NULL;
   Console *nextConsole;

   /* remove all actions before adding actions appropriate for new page */
   foreach(QAction* pageAction, treeWidget->actions()) {
      treeWidget->removeAction(pageAction);
   }

   /* first determine the next item */

   /* knowing the treeWidgetItem, get the page from the hash */
   nextPage = getFromHash(currentitem);
   nextConsole = m_consoleHash.value(currentitem);
   /* Is this a page that has been inserted into the hash  */
   if (nextPage) {
      nextConsole = nextPage->console();
      /* then is it a treeWidgetItem representing a director */
   } else if (nextConsole) {
      /* let the next page BE the console */
      nextPage = nextConsole;
   } else {
      /* Should never get here */
      nextPage = NULL;
      nextConsole = NULL;
   }
          
   /* The Previous item */

   /* this condition prevents a segfault.  The first time there is no previousitem*/
   if (previousitem) {
      /* knowing the treeWidgetItem, get the page from the hash */
      previousPage = getFromHash(previousitem);
      previousConsole = m_consoleHash.value(previousitem);
      if (previousPage) {
         previousConsole = previousPage->console();
      } else if (previousConsole) {
         previousPage = previousConsole;
      }
      if ((previousPage) || (previousConsole)) {
         if (nextConsole != previousConsole) {
            /* remove connections to the current console */
            disconnect(actionConnect, SIGNAL(triggered()), previousConsole, SLOT(connect_dir()));
            disconnect(actionStatusDir, SIGNAL(triggered()), previousConsole, SLOT(status_dir()));
            disconnect(actionMessages, SIGNAL(triggered()), previousConsole, SLOT(messages()));
            disconnect(actionSelectFont, SIGNAL(triggered()), previousConsole, SLOT(set_font()));
            QTreeWidgetItem *dirItem = previousConsole->directorTreeItem();
            QBrush greyBrush(Qt::lightGray);
            dirItem->setBackground(0, greyBrush);
         }
      }
   }

   /* process the current (next) item */
   
   if ((nextPage) || (nextConsole)) {
      if (nextConsole != previousConsole) {
         /* make connections to the current console */
         m_currentConsole = nextConsole;
         connect(actionConnect, SIGNAL(triggered()), m_currentConsole, SLOT(connect_dir()));
         connect(actionSelectFont, SIGNAL(triggered()), m_currentConsole, SLOT(set_font()));
         connect(actionStatusDir, SIGNAL(triggered()), m_currentConsole, SLOT(status_dir()));
         connect(actionMessages, SIGNAL(triggered()), m_currentConsole, SLOT(messages()));
         /* Set director's tree widget background to magenta for ease of identification */
         QTreeWidgetItem *dirItem = m_currentConsole->directorTreeItem();
         QBrush magentaBrush(Qt::magenta);
         dirItem->setBackground(0, magentaBrush);
      }
      /* set the value for the currently active console */
      int stackindex = stackedWidget->indexOf(nextPage);
   
      /* Is this page currently on the stack or is it undocked */
      if (stackindex >= 0) {
         /* put this page on the top of the stack */
         stackedWidget->setCurrentIndex(stackindex);
      } else {
         /* it is undocked, raise it to the front */
         nextPage->raise();
      }
      /* for the page selectors menu action to dock or undock, set the text */
      nextPage->setContextMenuDockText();

      treeWidget->addAction(actionToggleDock);
      /* if this page is closeable, and it has no childern, then add that action */
      if ((nextPage->isCloseable()) && (currentitem->child(0) == NULL))
         treeWidget->addAction(actionClosePage);

      /* Add the actions to the Page Selectors tree widget that are part of the
       * current items list of desired actions regardless of whether on top of stack*/
      treeWidget->addActions(nextPage->m_contextActions);
   }
}

void MainWin::labelButtonClicked() 
{
   new labelPage();
}

void MainWin::runButtonClicked() 
{
   new runPage("");
}

void MainWin::estimateButtonClicked() 
{
   new estimatePage();
}

void MainWin::browseButtonClicked() 
{
   new restoreTree();
}

void MainWin::restoreButtonClicked() 
{
   new prerestorePage();
}

void MainWin::jobPlotButtonClicked()
{
#ifdef HAVE_QWT
   JobPlotPass pass;
   pass.use = false;
   new JobPlot(NULL, pass);
#endif
}

/*
 * The user just finished typing a line in the command line edit box
 */
void MainWin::input_line()
{
   QString cmdStr = lineEdit->text();    /* Get the text */
   lineEdit->clear();                    /* clear the lineEdit box */
   if (m_currentConsole->is_connected()) {
      /* Use consoleInput to allow typing anything */
      m_currentConsole->consoleInput(cmdStr);
   } else {
      set_status("Director not connected. Click on connect button.");
   }
   m_cmd_history.append(cmdStr);
   m_cmd_last = -1;
   if (treeWidget->currentItem() != getFromHash(m_currentConsole))
      m_currentConsole->setCurrent();
}


void MainWin::about()
{
   QMessageBox::about(this, tr("About bat"),
      tr("<br><h2>bat " VERSION " (" BDATE "), by Dirk H Bartley and Kern Sibbald</h2>"
         "<p>Copyright &copy; 2007-" BYEAR " Free Software Foundation Europe e.V."
         "<p>The <b>bat</b> is an administrative console"
         " interface to the Director."));
}

void MainWin::help()
{
   Help::displayFile("index.html");
}

void MainWin::set_statusf(const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[1000];
   int len;
   va_start(arg_ptr, fmt);
   len = bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   set_status(buf);
}

void MainWin::set_status_ready()
{
   set_status(" Ready");
}

void MainWin::set_status(const QString &str)
{
   statusBar()->showMessage(str);
}

void MainWin::set_status(const char *buf)
{
   statusBar()->showMessage(buf);
}

/*
 * Function to respond to the button bar button to undock
 */
void MainWin::undockWindowButton()
{
   Pages* page = (Pages*)stackedWidget->currentWidget();
   page->togglePageDocking();
}

/*
 * Function to respond to action on page selector context menu to toggle the 
 * dock status of the window associated with the page selectors current
 * tree widget item.
 */
void MainWin::toggleDockContextWindow()
{
   QTreeWidgetItem *currentitem = treeWidget->currentItem();
   
   /* Is this a page that has been inserted into the hash  */
   if (getFromHash(currentitem)) {
      Pages* page = getFromHash(currentitem);
      page->togglePageDocking();
   }
}

/*
 * This function is called when the stack item is changed.  Call
 * the virtual function here.  Avoids a window being undocked leaving
 * a window at the top of the stack unpopulated.
 */
void MainWin::stackItemChanged(int)
{
   if (m_isClosing) return; /* if closing the application, do nothing here */
   Pages* page = (Pages*)stackedWidget->currentWidget();
   /* run the virtual function in case this class overrides it */
   page->currentStackItem();
}

/*
 * Function to simplify insertion of QTreeWidgetItem <-> Page association
 * into a double direction hash.
 */
void MainWin::hashInsert(QTreeWidgetItem *item, Pages *page)
{
   m_pagehash.insert(item, page);
   m_widgethash.insert(page, item);
}

/*
 * Function to simplify removal of QTreeWidgetItem <-> Page association
 * into a double direction hash.
 */
void MainWin::hashRemove(QTreeWidgetItem *item, Pages *page)
{
   /* I had all sorts of return status checking code here.  Do we have a log
    * level capability in bat.  I would have left it in but it used printf's
    * and it should really be some kind of log level facility ???
    * ******FIXME********/
   m_pagehash.remove(item);
   m_widgethash.remove(page);
}

/*
 * Function to retrieve a Page* when the item in the page selector's tree is
 * known.
 */
Pages* MainWin::getFromHash(QTreeWidgetItem *item)
{
   return m_pagehash.value(item);
}

/*
 * Function to retrieve the page selectors tree widget item when the page is
 * known.
 */
QTreeWidgetItem* MainWin::getFromHash(Pages *page)
{
   return m_widgethash.value(page);
}

/*
 * Function to respond to action on page selector context menu to close the
 * current window.
 */
void MainWin::closePage()
{
   QTreeWidgetItem *currentitem = treeWidget->currentItem();
   
   /* Is this a page that has been inserted into the hash  */
   if (getFromHash(currentitem)) {
      Pages* page = getFromHash(currentitem);
      if (page->isCloseable()) {
         page->closeStackPage();
      }
   }
}

/* Quick function to return the current console */
Console *MainWin::currentConsole()
{
   return m_currentConsole;
}
/* Quick function to return the tree item for the director */
QTreeWidgetItem *MainWin::currentTopItem()
{
   return m_currentConsole->directorTreeItem();
}

/* Preferences menu item clicked */
void MainWin::setPreferences()
{
   prefsDialog prefs;
   prefs.commDebug->setCheckState(m_commDebug ? Qt::Checked : Qt::Unchecked);
   prefs.displayAll->setCheckState(m_displayAll ? Qt::Checked : Qt::Unchecked);
   prefs.sqlDebug->setCheckState(m_sqlDebug ? Qt::Checked : Qt::Unchecked);
   prefs.commandDebug->setCheckState(m_commandDebug ? Qt::Checked : Qt::Unchecked);
   prefs.miscDebug->setCheckState(m_miscDebug ? Qt::Checked : Qt::Unchecked);
   prefs.recordLimit->setCheckState(m_recordLimitCheck ? Qt::Checked : Qt::Unchecked);
   prefs.recordSpinBox->setValue(m_recordLimitVal);
   prefs.daysLimit->setCheckState(m_daysLimitCheck ? Qt::Checked : Qt::Unchecked);
   prefs.daysSpinBox->setValue(m_daysLimitVal);
   prefs.checkMessages->setCheckState(m_checkMessages ? Qt::Checked : Qt::Unchecked);
   prefs.checkMessagesSpin->setValue(m_checkMessagesInterval);
   prefs.executeLongCheckBox->setCheckState(m_longList ? Qt::Checked : Qt::Unchecked);
   prefs.rtPopDirCheckBox->setCheckState(m_rtPopDirDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtDirCurICCheckBox->setCheckState(m_rtDirCurICDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtDirICCheckBox->setCheckState(m_rtDirICDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtFileTabICCheckBox->setCheckState(m_rtFileTabICDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtVerTabICCheckBox->setCheckState(m_rtVerTabICDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtUpdateFTCheckBox->setCheckState(m_rtUpdateFTDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtUpdateVTCheckBox->setCheckState(m_rtUpdateVTDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtChecksCheckBox->setCheckState(m_rtChecksDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtIconStateCheckBox->setCheckState(m_rtIconStateDebug ? Qt::Checked : Qt::Unchecked);
   prefs.rtRestore1CheckBox->setCheckState(m_rtRestore1Debug ? Qt::Checked : Qt::Unchecked);
   prefs.rtRestore2CheckBox->setCheckState(m_rtRestore2Debug ? Qt::Checked : Qt::Unchecked);
   prefs.rtRestore3CheckBox->setCheckState(m_rtRestore3Debug ? Qt::Checked : Qt::Unchecked);
   prefs.exec();
}

/* Preferences dialog */
prefsDialog::prefsDialog()
{
   setupUi(this);
}

void prefsDialog::accept()
{
   this->hide();
   mainWin->m_commDebug = this->commDebug->checkState() == Qt::Checked;
   mainWin->m_displayAll = this->displayAll->checkState() == Qt::Checked;
   mainWin->m_sqlDebug = this->sqlDebug->checkState() == Qt::Checked;
   mainWin->m_commandDebug = this->commandDebug->checkState() == Qt::Checked;
   mainWin->m_miscDebug = this->miscDebug->checkState() == Qt::Checked;
   mainWin->m_recordLimitCheck = this->recordLimit->checkState() == Qt::Checked;
   mainWin->m_recordLimitVal = this->recordSpinBox->value();
   mainWin->m_daysLimitCheck = this->daysLimit->checkState() == Qt::Checked;
   mainWin->m_daysLimitVal = this->daysSpinBox->value();
   mainWin->m_checkMessages = this->checkMessages->checkState() == Qt::Checked;
   mainWin->m_checkMessagesInterval = this->checkMessagesSpin->value();
   mainWin->m_longList = this->executeLongCheckBox->checkState() == Qt::Checked;

   mainWin->m_rtPopDirDebug = this->rtPopDirCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtDirCurICDebug = this->rtDirCurICCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtDirICDebug = this->rtDirICCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtFileTabICDebug = this->rtFileTabICCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtVerTabICDebug = this->rtVerTabICCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtUpdateFTDebug = this->rtUpdateFTCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtUpdateVTDebug = this->rtUpdateVTCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtChecksDebug = this->rtChecksCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtIconStateDebug = this->rtIconStateCheckBox->checkState() == Qt::Checked;
   mainWin->m_rtRestore1Debug = this->rtRestore1CheckBox->checkState() == Qt::Checked;
   mainWin->m_rtRestore2Debug = this->rtRestore2CheckBox->checkState() == Qt::Checked;
   mainWin->m_rtRestore3Debug = this->rtRestore3CheckBox->checkState() == Qt::Checked;

   QSettings settings("www.bacula.org", "bat");
   settings.beginGroup("Debug");
   settings.setValue("commDebug", mainWin->m_commDebug);
   settings.setValue("displayAll", mainWin->m_displayAll);
   settings.setValue("sqlDebug", mainWin->m_sqlDebug);
   settings.setValue("commandDebug", mainWin->m_commandDebug);
   settings.setValue("miscDebug", mainWin->m_miscDebug);
   settings.endGroup();
   settings.beginGroup("JobList");
   settings.setValue("recordLimitCheck", mainWin->m_recordLimitCheck);
   settings.setValue("recordLimitVal", mainWin->m_recordLimitVal);
   settings.setValue("daysLimitCheck", mainWin->m_daysLimitCheck);
   settings.setValue("daysLimitVal", mainWin->m_daysLimitVal);
   settings.endGroup();
   settings.beginGroup("Messages");
   settings.setValue("checkMessages", mainWin->m_checkMessages);
   settings.setValue("checkMessagesInterval", mainWin->m_checkMessagesInterval);
   settings.endGroup();
   settings.beginGroup("Misc");
   settings.setValue("longList", mainWin->m_longList);
   settings.endGroup();
   settings.beginGroup("RestoreTree");
   settings.setValue("rtPopDirDebug", mainWin->m_rtPopDirDebug);
   settings.setValue("rtDirCurICDebug", mainWin->m_rtDirCurICDebug);
   settings.setValue("rtDirCurICRetDebug", mainWin->m_rtDirICDebug);
   settings.setValue("rtFileTabICDebug", mainWin->m_rtFileTabICDebug);
   settings.setValue("rtVerTabICDebug", mainWin->m_rtVerTabICDebug);
   settings.setValue("rtUpdateFTDebug", mainWin->m_rtUpdateFTDebug);
   settings.setValue("rtUpdateVTDebug", mainWin->m_rtUpdateVTDebug);
   settings.setValue("rtChecksDebug", mainWin->m_rtChecksDebug);
   settings.setValue("rtIconStateDebug", mainWin->m_rtIconStateDebug);
   settings.setValue("rtRestore1Debug", mainWin->m_rtRestore1Debug);
   settings.setValue("rtRestore2Debug", mainWin->m_rtRestore2Debug);
   settings.setValue("rtRestore3Debug", mainWin->m_rtRestore3Debug);
   settings.endGroup();
   foreach(Console *console, mainWin->m_consoleHash) {
      console->startTimer();
   }
}

void prefsDialog::reject()
{
   this->hide();
   mainWin->set_status("Canceled");
}

/* read preferences for the prefences dialog box */
void MainWin::readPreferences()
{
   QSettings settings("www.bacula.org", "bat");
   settings.beginGroup("Debug");
   m_commDebug = settings.value("commDebug", false).toBool();
   m_displayAll = settings.value("displayAll", false).toBool();
   m_sqlDebug = settings.value("sqlDebug", false).toBool();
   m_commandDebug = settings.value("commandDebug", false).toBool();
   m_miscDebug = settings.value("miscDebug", false).toBool();
   settings.endGroup();
   settings.beginGroup("JobList");
   m_recordLimitCheck = settings.value("recordLimitCheck", true).toBool();
   m_recordLimitVal = settings.value("recordLimitVal", 150).toInt();
   m_daysLimitCheck = settings.value("daysLimitCheck", false).toBool();
   m_daysLimitVal = settings.value("daysLimitVal", 28).toInt();
   settings.endGroup();
   settings.beginGroup("Messages");
   m_checkMessages = settings.value("checkMessages", false).toBool();
   m_checkMessagesInterval = settings.value("checkMessagesInterval", 28).toInt();
   settings.endGroup();
   settings.beginGroup("Misc");
   m_longList = settings.value("longList", false).toBool();
   settings.endGroup();
   settings.beginGroup("RestoreTree");
   m_rtPopDirDebug = settings.value("rtPopDirDebug", false).toBool();
   m_rtDirCurICDebug = settings.value("rtDirCurICDebug", false).toBool();
   m_rtDirICDebug = settings.value("rtDirCurICRetDebug", false).toBool();
   m_rtFileTabICDebug = settings.value("rtFileTabICDebug", false).toBool();
   m_rtVerTabICDebug = settings.value("rtVerTabICDebug", false).toBool();
   m_rtUpdateFTDebug = settings.value("rtUpdateFTDebug", false).toBool();
   m_rtUpdateVTDebug = settings.value("rtUpdateVTDebug", false).toBool();
   m_rtChecksDebug = settings.value("rtChecksDebug", false).toBool();
   m_rtIconStateDebug = settings.value("rtIconStateDebug", false).toBool();
   m_rtRestore1Debug = settings.value("rtRestore1Debug", false).toBool();
   m_rtRestore2Debug = settings.value("rtRestore2Debug", false).toBool();
   m_rtRestore3Debug = settings.value("rtRestore3Debug", false).toBool();
   settings.endGroup();
}
