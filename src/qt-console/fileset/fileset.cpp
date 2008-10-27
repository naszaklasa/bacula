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
 *   Version $Id: fileset.cpp 6948 2008-05-11 14:38:19Z kerns $
 *
 *  FileSet Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QAbstractEventDispatcher>
#include <QMenu>
#include "bat.h"
#include "fileset/fileset.h"

FileSet::FileSet()
{
   setupUi(this);
   m_name = "FileSets";
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/system-file-manager.png")));

   /* mp_treeWidget, FileSet Tree Tree Widget inherited from ui_fileset.h */
   m_populated = false;
   m_populating = false;
   m_checkcurwidget = true;
   m_closeable = false;
   readSettings();
   /* add context sensitive menu items specific to this classto the page
    * selector tree. m_contextActions is QList of QActions */
   m_contextActions.append(actionRefreshFileSet);
   dockPage();
}

FileSet::~FileSet()
{
   writeSettings();
}

/*
 * The main meat of the class!!  The function that querries the director and 
 * creates the widgets with appropriate values.
 */
void FileSet::populateTree()
{
   if (m_populating)
      return;
   m_populating = true;

   QTreeWidgetItem *filesetItem, *topItem;

   if (!m_console->preventInUseConnect())
       return;

   m_checkcurwidget = false;
   mp_treeWidget->clear();
   m_checkcurwidget = true;

   QStringList headerlist = (QStringList() << "  FileSet Name  " << "FileSet Id"
       << "Create Time");

   topItem = new QTreeWidgetItem(mp_treeWidget);
   topItem->setText(0, "FileSet");
   topItem->setData(0, Qt::UserRole, 0);
   topItem->setExpanded(true);

   mp_treeWidget->setColumnCount(headerlist.count());
   mp_treeWidget->setHeaderLabels(headerlist);

   foreach(QString filesetName, m_console->fileset_list) {
      filesetItem = new QTreeWidgetItem(topItem);
      filesetItem->setText(0, filesetName);
      filesetItem->setData(0, Qt::UserRole, 1);
      filesetItem->setExpanded(true);

      /* Set up query QString and header QStringList */
      QString query("");
      query += "SELECT FileSet AS Name, FileSetId AS Id, CreateTime"
           " FROM FileSet"
           " WHERE ";
      query += " FileSet='" + filesetName + "'";
      query += " ORDER BY CreateTime DESC LIMIT 1";

      QStringList results;
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "FileSet query cmd : %s\n",query.toUtf8().data());
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
                  filesetItem->setData(index, Qt::UserRole, 1);
                  /* Put media fields under the pool tree item */
                  filesetItem->setData(index, Qt::UserRole, 1);
                  filesetItem->setText(index, field);
                  index++;
               }
            }
         }
      }
   }
   /* Resize the columns */
   for (int cnter=0; cnter<headerlist.size(); cnter++) {
      mp_treeWidget->resizeColumnToContents(cnter);
   }
   m_populating = false;
}

/*
 * When the treeWidgetItem in the page selector tree is singleclicked, Make sure
 * The tree has been populated.
 */
void FileSet::PgSeltreeWidgetClicked()
{
   if (!m_populated) {
      populateTree();
      createContextMenu();
      m_populated = true;
   }
}

/*
 * Added to set the context menu policy based on currently active treeWidgetItem
 * signaled by currentItemChanged
 */
void FileSet::treeItemChanged(QTreeWidgetItem *currentwidgetitem, 
                              QTreeWidgetItem *previouswidgetitem )
{
   /* m_checkcurwidget checks to see if this is during a refresh, which will segfault */
   if (m_checkcurwidget) {
      /* The Previous item */
      if (previouswidgetitem) { /* avoid a segfault if first time */
         int treedepth = previouswidgetitem->data(0, Qt::UserRole).toInt();
         if (treedepth == 1) {
            mp_treeWidget->removeAction(actionStatusFileSetInConsole);
            mp_treeWidget->removeAction(actionShowJobs);
         }
      }

      int treedepth = currentwidgetitem->data(0, Qt::UserRole).toInt();
      if (treedepth == 1){
         /* set a hold variable to the fileset name in case the context sensitive
          * menu is used */
         m_currentlyselected=currentwidgetitem->text(0);
         mp_treeWidget->addAction(actionStatusFileSetInConsole);
         mp_treeWidget->addAction(actionShowJobs);
      }
   }
}

/* 
 * Setup a context menu 
 * Made separate from populate so that it would not create context menu over and
 * over as the tree is repopulated.
 */
void FileSet::createContextMenu()
{
   mp_treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
   mp_treeWidget->addAction(actionRefreshFileSet);
   connect(mp_treeWidget, SIGNAL(
           currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(treeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
   /* connect to the action specific to this pages class */
   connect(actionRefreshFileSet, SIGNAL(triggered()), this,
                SLOT(populateTree()));
   connect(actionStatusFileSetInConsole, SIGNAL(triggered()), this,
                SLOT(consoleStatusFileSet()));
   connect(actionShowJobs, SIGNAL(triggered()), this,
                SLOT(showJobs()));
}

/*
 * Function responding to actionListJobsofFileSet which calls mainwin function
 * to create a window of a list of jobs of this fileset.
 */
void FileSet::consoleStatusFileSet()
{
   QString cmd("status fileset=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void FileSet::currentStackItem()
{
   if(!m_populated) {
      populateTree();
      /* Create the context menu for the fileset tree */
      createContextMenu();
      m_populated=true;
   }
}

/*
 * Save user settings associated with this page
 */
void FileSet::writeSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("FileSet");
   settings.setValue("geometry", saveGeometry());
   settings.endGroup();
}

/*
 * Read and restore user settings associated with this page
 */
void FileSet::readSettings()
{ 
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("FileSet");
   restoreGeometry(settings.value("geometry").toByteArray());
   settings.endGroup();
}

/*
 * Create a JobList object pre-populating a fileset
 */
void FileSet::showJobs()
{
   QTreeWidgetItem *parentItem = mainWin->getFromHash(this);
   mainWin->createPageJobList("", "", "", m_currentlyselected, parentItem);
}
