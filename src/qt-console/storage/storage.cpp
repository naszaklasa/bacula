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
 *   Version $Id: storage.cpp 6948 2008-05-11 14:38:19Z kerns $
 *
 *  Storage Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QAbstractEventDispatcher>
#include <QMenu>
#include "bat.h"
#include "storage.h"
#include "label/label.h"
#include "../mount/mount.h"

Storage::Storage()
{
   setupUi(this);
   m_name = "Storage";
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/package-x-generic.png")));

   /* mp_treeWidget, Storage Tree Tree Widget inherited from ui_storage.h */
   m_populated = false;
   m_populating = false;
   m_checkcurwidget = true;
   m_closeable = false;
   m_currentStorage = "";
   /* add context sensitive menu items specific to this classto the page
    * selector tree. m_contextActions is QList of QActions */
   m_contextActions.append(actionRefreshStorage);
   dockPage();
}

Storage::~Storage()
{
}

/*
 * The main meat of the class!!  The function that querries the director and 
 * creates the widgets with appropriate values.
 */
void Storage::populateTree()
{
   if (m_populating)
      return;
   m_populating = true;
   QTreeWidgetItem *storageItem, *topItem;

   if (!m_console->preventInUseConnect())
       return;

   m_checkcurwidget = false;
   mp_treeWidget->clear();
   m_checkcurwidget = true;

   QStringList headerlist = (QStringList() << "Storage Name" << "Storage Id"
       << "Auto Changer");

   topItem = new QTreeWidgetItem(mp_treeWidget);
   topItem->setText(0, "Storage");
   topItem->setData(0, Qt::UserRole, 0);
   topItem->setExpanded(true);

   mp_treeWidget->setColumnCount(headerlist.count());
   mp_treeWidget->setHeaderLabels(headerlist);

   foreach(QString storageName, m_console->storage_list){
      storageItem = new QTreeWidgetItem(topItem);
      storageItem->setText(0, storageName);
      storageItem->setData(0, Qt::UserRole, 1);
      storageItem->setExpanded(true);

      /* Set up query QString and header QStringList */
      QString query("SELECT StorageId AS ID, AutoChanger AS Changer"
               " FROM Storage WHERE");
      query += " Name='" + storageName + "'"
               " ORDER BY Name";

      QStringList results;
      /* This could be a log item */
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "Storage query cmd : %s\n",query.toUtf8().data());
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
                  storageItem->setData(index+1, Qt::UserRole, 1);
                  /* Put media fields under the pool tree item */
                  storageItem->setData(index+1, Qt::UserRole, 1);
                  storageItem->setText(index+1, field);
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
   m_populating = false;
}

/*
 * When the treeWidgetItem in the page selector tree is singleclicked, Make sure
 * The tree has been populated.
 */
void Storage::PgSeltreeWidgetClicked()
{
   if(!m_populated) {
      populateTree();
      createContextMenu();
      m_populated=true;
   }
}

/*
 * Added to set the context menu policy based on currently active treeWidgetItem
 * signaled by currentItemChanged
 */
void Storage::treeItemChanged(QTreeWidgetItem *currentwidgetitem, QTreeWidgetItem *previouswidgetitem )
{
   /* m_checkcurwidget checks to see if this is during a refresh, which will segfault */
   if (m_checkcurwidget) {
      /* The Previous item */
      if (previouswidgetitem) { /* avoid a segfault if first time */
         int treedepth = previouswidgetitem->data(0, Qt::UserRole).toInt();
         if (treedepth == 1){
            mp_treeWidget->removeAction(actionStatusStorageInConsole);
            mp_treeWidget->removeAction(actionLabelStorage);
            mp_treeWidget->removeAction(actionMountStorage);
            mp_treeWidget->removeAction(actionUnMountStorage);
            mp_treeWidget->removeAction(actionUpdateSlots);
            mp_treeWidget->removeAction(actionUpdateSlotsScan);
            mp_treeWidget->removeAction(actionRelease);
         }
      }

      int treedepth = currentwidgetitem->data(0, Qt::UserRole).toInt();
      if (treedepth == 1){
         /* set a hold variable to the storage name in case the context sensitive
          * menu is used */
         m_currentStorage = currentwidgetitem->text(0);
         m_currentAutoChanger = currentwidgetitem->text(2).toInt();
         mp_treeWidget->addAction(actionStatusStorageInConsole);
         mp_treeWidget->addAction(actionLabelStorage);
         mp_treeWidget->addAction(actionMountStorage);
         mp_treeWidget->addAction(actionUnMountStorage);
         mp_treeWidget->addAction(actionRelease);
         QString text;
         text = "Status Storage \"" + m_currentStorage + "\"";
         actionStatusStorageInConsole->setText(text);
         text = "Label media in Storage \"" + m_currentStorage + "\"";
         actionLabelStorage->setText(text);
         text = "Mount media in Storage \"" + m_currentStorage + "\"";
         actionMountStorage->setText(text);
         text = "\"UN\" Mount media in Storage \"" + m_currentStorage + "\"";
         actionUnMountStorage->setText(text);
         text = "Release media in Storage \"" + m_currentStorage + "\"";
         actionRelease->setText(text);
         if (m_currentAutoChanger != 0) {
            mp_treeWidget->addAction(actionUpdateSlots);
            mp_treeWidget->addAction(actionUpdateSlotsScan);
            text = "Barcode Scan media in Storage \"" + m_currentStorage + "\"";
            actionUpdateSlots->setText(text);
            text = "Mount and read scan media in Storage \"" + m_currentStorage + "\"";
            actionUpdateSlotsScan->setText(text);
         }
      }
   }
}

/* 
 * Setup a context menu 
 * Made separate from populate so that it would not create context menu over and
 * over as the tree is repopulated.
 */
void Storage::createContextMenu()
{
   mp_treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
   mp_treeWidget->addAction(actionRefreshStorage);
   connect(mp_treeWidget, SIGNAL(
           currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(treeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
   /* connect to the action specific to this pages class */
   connect(actionRefreshStorage, SIGNAL(triggered()), this,
                SLOT(populateTree()));
   connect(actionStatusStorageInConsole, SIGNAL(triggered()), this,
                SLOT(consoleStatusStorage()));
   connect(actionLabelStorage, SIGNAL(triggered()), this,
                SLOT(consoleLabelStorage()));
   connect(actionMountStorage, SIGNAL(triggered()), this,
                SLOT(consoleMountStorage()));
   connect(actionUnMountStorage, SIGNAL(triggered()), this,
                SLOT(consoleUnMountStorage()));
   connect(actionUpdateSlots, SIGNAL(triggered()), this,
                SLOT(consoleUpdateSlots()));
   connect(actionUpdateSlotsScan, SIGNAL(triggered()), this,
                SLOT(consoleUpdateSlotsScan()));
   connect(actionRelease, SIGNAL(triggered()), this,
                SLOT(consoleRelease()));
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void Storage::currentStackItem()
{
   if(!m_populated) {
      populateTree();
      /* Create the context menu for the storage tree */
      createContextMenu();
      m_populated=true;
   }
}

/*
 *  Functions to respond to local context sensitive menu sending console commands
 *  If I could figure out how to make these one function passing a string, Yaaaaaa
 */
void Storage::consoleStatusStorage()
{
   QString cmd("status storage=");
   cmd += m_currentStorage;
   consoleCommand(cmd);
}

/* Label Media populating current storage by default */
void Storage::consoleLabelStorage()
{
   new labelPage(m_currentStorage);
}

/* Mount currently selected storage */
void Storage::consoleMountStorage()
{
   if (m_currentAutoChanger == 0){
      /* no autochanger, just execute the command in the console */
      QString cmd("mount storage=");
      cmd += m_currentStorage;
      consoleCommand(cmd);
   } else {
      setConsoleCurrent();
      /* if this storage is an autochanger, lets ask for the slot */
      new mountDialog(m_console, m_currentStorage);
   }
}

/* Unmount Currently selected storage */
void Storage::consoleUnMountStorage()
{
   QString cmd("umount storage=");
   cmd += m_currentStorage;
   consoleCommand(cmd);
}

/* Update Slots */
void Storage::consoleUpdateSlots()
{
   QString cmd("update slots storage=");
   cmd += m_currentStorage;
   consoleCommand(cmd);
}

/* Update Slots Scan*/
void Storage::consoleUpdateSlotsScan()
{
   QString cmd("update slots scan storage=");
   cmd += m_currentStorage;
   consoleCommand(cmd);
}

/* Release a tape in the drive */
void Storage::consoleRelease()
{
   QString cmd("release storage=");
   cmd += m_currentStorage;
   consoleCommand(cmd);
}
