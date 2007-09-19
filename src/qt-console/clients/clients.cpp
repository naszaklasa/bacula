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
 *   Version $Id: clients.cpp 5372 2007-08-17 12:17:04Z kerns $
 *
 *  Clients Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QAbstractEventDispatcher>
#include <QMenu>
#include "bat.h"
#include "clients/clients.h"
#include "run/run.h"

Clients::Clients()
{
   setupUi(this);
   m_name = "Clients";
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/network-server.png")));

   /* mp_treeWidget, Storage Tree Tree Widget inherited from ui_client.h */
   m_populated = false;
   m_checkcurwidget = true;
   m_closeable = false;
   /* add context sensitive menu items specific to this classto the page
    * selector tree. m_contextActions is QList of QActions */
   m_contextActions.append(actionRefreshClients);
   createContextMenu();
   dockPage();
}

Clients::~Clients()
{
}

/*
 * The main meat of the class!!  The function that querries the director and 
 * creates the widgets with appropriate values.
 */
void Clients::populateTree()
{
   QTreeWidgetItem *clientItem, *topItem;

   if (!m_console->preventInUseConnect())
      return;
   m_checkcurwidget = false;
   mp_treeWidget->clear();
   m_checkcurwidget = true;

   QStringList headerlist = (QStringList() << "Client Name" << "File Retention"
       << "Job Retention" << "AutoPrune" << "ClientId" << "Uname" );

   topItem = new QTreeWidgetItem(mp_treeWidget);
   topItem->setText(0, "Clients");
   topItem->setData(0, Qt::UserRole, 0);
   topItem->setExpanded(true);

   mp_treeWidget->setColumnCount(headerlist.count());
   mp_treeWidget->setHeaderLabels(headerlist);

   foreach (QString clientName, m_console->client_list){
      clientItem = new QTreeWidgetItem(topItem);
      clientItem->setText(0, clientName);
      clientItem->setData(0, Qt::UserRole, 1);
      clientItem->setExpanded(true);

      /* Set up query QString and header QStringList */
      QString query("");
      query += "SELECT FileRetention, JobRetention, AutoPrune, ClientId, Uname"
           " FROM Client"
           " WHERE ";
      query += " Name='" + clientName + "'";
      query += " ORDER BY Name";

      QStringList results;
      /* This could be a log item */
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "Clients query cmd : %s\n",query.toUtf8().data());
      }
      if (m_console->sql_cmd(query, results)) {
         int resultCount = results.count();
         if (resultCount == 1){
            QString resultline;
            QString field;
            QStringList fieldlist;
            /* there will only be one of these */
            foreach (resultline, results) {
               fieldlist = resultline.split("\t");
               int index = 0;
               /* Iterate through fields in the record */
               foreach (field, fieldlist) {
                  field = field.trimmed();  /* strip leading & trailing spaces */
                  clientItem->setData(index+1, Qt::UserRole, 1);
                  /* Put media fields under the pool tree item */
                  clientItem->setData(index+1, Qt::UserRole, 1);
                  clientItem->setText(index+1, field);
                  index++;
               }
            }
         }
      }
   }
   /* Resize the columns */
   for(int cnter=0; cnter<headerlist.size(); cnter++) {
      mp_treeWidget->resizeColumnToContents(cnter);
   }
}

/*
 * When the treeWidgetItem in the page selector tree is singleclicked, Make sure
 * The tree has been populated.
 */
void Clients::PgSeltreeWidgetClicked()
{
   if(!m_populated) {
      populateTree();
      m_populated=true;
   }
}

/*
 * Added to set the context menu policy based on currently active treeWidgetItem
 * signaled by currentItemChanged
 */
void Clients::treeItemChanged(QTreeWidgetItem *currentwidgetitem, QTreeWidgetItem *previouswidgetitem )
{
   /* m_checkcurwidget checks to see if this is during a refresh, which will segfault */
   if (m_checkcurwidget) {
      /* The Previous item */
      if (previouswidgetitem) { /* avoid a segfault if first time */
         int treedepth = previouswidgetitem->data(0, Qt::UserRole).toInt();
         if (treedepth == 1){
            mp_treeWidget->removeAction(actionListJobsofClient);
            mp_treeWidget->removeAction(actionStatusClientInConsole);
            mp_treeWidget->removeAction(actionPurgeJobs);
            mp_treeWidget->removeAction(actionPrune);
         }
      }

      int treedepth = currentwidgetitem->data(0, Qt::UserRole).toInt();
      if (treedepth == 1){
         /* set a hold variable to the client name in case the context sensitive
          * menu is used */
         m_currentlyselected=currentwidgetitem->text(0);
         mp_treeWidget->addAction(actionListJobsofClient);
         mp_treeWidget->addAction(actionStatusClientInConsole);
         mp_treeWidget->addAction(actionPurgeJobs);
         mp_treeWidget->addAction(actionPrune);
      }
   }
}

/* 
 * Setup a context menu 
 * Made separate from populate so that it would not create context menu over and
 * over as the tree is repopulated.
 */
void Clients::createContextMenu()
{
   mp_treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
   mp_treeWidget->addAction(actionRefreshClients);
   connect(mp_treeWidget, SIGNAL(
           currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(treeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
   /* connect to the action specific to this pages class */
   connect(actionRefreshClients, SIGNAL(triggered()), this,
                SLOT(populateTree()));
   connect(actionListJobsofClient, SIGNAL(triggered()), this,
                SLOT(showJobs()));
   connect(actionStatusClientInConsole, SIGNAL(triggered()), this,
                SLOT(consoleStatusClient()));
   connect(actionPurgeJobs, SIGNAL(triggered()), this,
                SLOT(consolePurgeJobs()));
   connect(actionPrune, SIGNAL(triggered()), this,
                SLOT(prune()));
}

/*
 * Function responding to actionListJobsofClient which calls mainwin function
 * to create a window of a list of jobs of this client.
 */
void Clients::showJobs()
{
   QTreeWidgetItem *parentItem = mainWin->getFromHash(this);
   mainWin->createPageJobList("", m_currentlyselected, "", "", parentItem);
}

/*
 * Function responding to actionListJobsofClient which calls mainwin function
 * to create a window of a list of jobs of this client.
 */
void Clients::consoleStatusClient()
{
   QString cmd("status client=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void Clients::currentStackItem()
{
   if(!m_populated) {
      populateTree();
      /* Create the context menu for the client tree */
      m_populated=true;
   }
}

/*
 * Function responding to actionPurgeJobs 
 */
void Clients::consolePurgeJobs()
{
   if (QMessageBox::warning(this, tr("Bat"),
      tr("Are you sure you want to purge ??  !!!.\n"
"The Purge command will delete associated Catalog database records from Jobs and"
" Volumes without considering the retention period. Purge  works only on the"
" Catalog database and does not affect data written to Volumes. This command can"
" be dangerous because you can delete catalog records associated with current"
" backups of files, and we recommend that you do not use it unless you know what"
" you are doing.\n\n"
" Is there any way I can get you to Click cancel here.  You really don't want to do"
" this\n\n"
      "Press OK to proceed with the purge operation?"),
      QMessageBox::Ok | QMessageBox::Cancel)
      == QMessageBox::Cancel) { return; }

   QString cmd("purge jobs client=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Function responding to actionPrune
 */
void Clients::prune()
{
   new prunePage("", m_currentlyselected);
}
