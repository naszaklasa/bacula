/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

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
 *   Version $Id: pages.cpp 5257 2007-07-28 10:36:28Z kerns $
 *
 *   Dirk Bartley, March 2007
 */

#include "pages.h"
#include "bat.h"

/*
 * dockPage
 * This function is intended to be called from within the Pages class to pull
 * a window from floating to in the stack widget.
 */

void Pages::dockPage()
{
   /* These two lines are for making sure if it is being changed from a window
    * that it has the proper window flag and parent.
    */
   setWindowFlags(Qt::Widget);

   /* This was being done already */
   m_parent->addWidget(this);

   /* Set docked flag */
   m_docked = true;
   mainWin->stackedWidget->setCurrentWidget(this);
   /* lets set the page selectors action for docking or undocking */
   setContextMenuDockText();
}

/*
 * undockPage
 * This function is intended to be called from within the Pages class to put
 * a window from the stack widget to a floating window.
 */

void Pages::undockPage()
{
   /* Change from a stacked widget to a normal window */
   m_parent->removeWidget(this);
   setWindowFlags(Qt::Window);
   show();
   /* Clear docked flag */
   m_docked = false;
   /* The window has been undocked, lets change the context menu */
   setContextMenuDockText();
}

/*
 * This function is intended to be called with the subclasses.  When it is 
 * called the specific sublclass does not have to be known to Pages.  When it 
 * is called this function will change the page from it's current state of being
 * docked or undocked and change it to the other.
 */

void Pages::togglePageDocking()
{
   if (m_docked) {
      undockPage();
   } else {
      dockPage();
   }
}

/*
 * This function is because I wanted for some reason to keep it protected but still 
 * give any subclasses the ability to find out if it is currently stacked or not.
 */
bool Pages::isDocked()
{
   return m_docked;
}

/*
 * To keep m_closeable protected as well
 */
bool Pages::isCloseable()
{
   return m_closeable;
}

/*
 * When a window is closed, this slot is called.  The idea is to put it back in the
 * stack here, and it works.  I wanted to get it to the top of the stack so that the
 * user immediately sees where his window went.  Also, if he undocks the window, then
 * closes it with the tree item highlighted, it may be confusing that the highlighted
 * treewidgetitem is not the stack item in the front.
 */

void Pages::closeEvent(QCloseEvent* event)
{
   /* A Widget was closed, lets toggle it back into the window, and set it in front. */
   dockPage();

   /* this fixes my woes of getting the widget to show up on top when closed */
   event->ignore();

   /* Set the current tree widget item in the Page Selector window to the item 
    * which represents "this" 
    * Which will also bring "this" to the top of the stacked widget */
   setCurrent();
}

/*
 * The next three are virtual functions.  The idea here is that each subclass will have the
 * built in virtual function to override if the programmer wants to populate the window
 * when it it is first clicked.
 */
void Pages::PgSeltreeWidgetClicked()
{
}

/*
 *  Virtual function which is called when this page is visible on the stack.
 *  This will be overridden by classes that want to populate if they are on the
 *  top.
 */
void Pages::currentStackItem()
{
}

/*
 * Function to close the stacked page and remove the widget from the
 * Page selector window
 */
void Pages::closeStackPage()
{
   /* First get the tree widget item and destroy it */
   QTreeWidgetItem *item=mainWin->getFromHash(this);
   /* remove the QTreeWidgetItem <-> page from the hash */
   mainWin->hashRemove(item, this);
   /* remove the item from the page selector by destroying it */
   delete item;
   /* remove this */
   delete this;
}

/*
 * Function to set members from the external mainwin and it's overload being
 * passed a specific QTreeWidgetItem to be it's parent on the tree
 */
void Pages::pgInitialize()
{
   pgInitialize(NULL);
}

void Pages::pgInitialize(QTreeWidgetItem *parentTreeWidgetItem)
{
   m_parent = mainWin->stackedWidget;
   m_console = mainWin->currentConsole();

   if (!parentTreeWidgetItem) {
      parentTreeWidgetItem = m_console->directorTreeItem();
   }

   QTreeWidgetItem *item = new QTreeWidgetItem(parentTreeWidgetItem);
   QString name; 
   treeWidgetName(name);
   item->setText(0, name);
   mainWin->hashInsert(item, this);
   setTitle();
}

/*
 * Virtual Function to return a name
 * All subclasses should override this function
 */
void Pages::treeWidgetName(QString &name)
{
   name = m_name;
}

/*
 * Function to simplify executing a console command and bringing the
 * console to the front of the stack
 */
void Pages::consoleCommand(QString &command)
{
   /*if (!m_console->is_connectedGui())
       return;*/
   if (!m_console->preventInUseConnect()) {
       return;
   }
   consoleInput(command);
}

/*
 * Function to simplify executing a console command, but does not
 *  check for the connection in use.  We need this so that we can
 *  *always* enter command from the command line.
 */
void Pages::consoleInput(QString &command)
{
   /* Bring this director's console to the front of the stack */
   setConsoleCurrent();
   QString displayhtml("<font color=\"blue\">");
   displayhtml += command + "</font>\n";
   m_console->display_html(displayhtml);
   m_console->display_text("\n");
   m_console->write_dir(command.toUtf8().data());
   m_console->displayToPrompt();
}

/*
 * Function for handling undocked windows becoming active.
 * Change the currently selected item in the page selector window to the now
 * active undocked window.  This will also make the console for the undocked
 * window m_currentConsole.
 */
void Pages::changeEvent(QEvent *event)
{
   if ((event->type() ==  QEvent::ActivationChange) && (isActiveWindow())) {
      setCurrent();
   }
}

/*
 * Function to simplify getting the name of the class and the director into
 * the caption or title of the window
 */
void Pages::setTitle()
{
   QString title, director;
   treeWidgetName(title);
   m_console->getDirResName(director);
   title += " of Director ";
   title += director;
   setWindowTitle(title);
}

/*
 * Bring the current directors console window to the top of the stack.
 */
void Pages::setConsoleCurrent()
{
   mainWin->treeWidget->setCurrentItem(mainWin->getFromHash(m_console));
}

/*
 * Bring this window to the top of the stack.
 */
void Pages::setCurrent()
{
   mainWin->treeWidget->setCurrentItem(mainWin->getFromHash(this));
}

/*
 * Function to set the text of the toggle dock context menu when page and
 * widget item are NOT known.  
 */
void Pages::setContextMenuDockText()
{
   QTreeWidgetItem *item = mainWin->getFromHash(this);
   QString docktext("");
   if (isDocked()) {
       docktext += "UnDock ";
   } else {
       docktext += "ReDock ";
   }
   docktext += item->text(0) += " Window";
      
   mainWin->actionToggleDock->setText(docktext);
   setTreeWidgetItemDockColor();
}

/*
 * Function to set the color of the tree widget item based on whether it is
 * docked or not.
 */
void Pages::setTreeWidgetItemDockColor()
{
   QTreeWidgetItem* item = mainWin->getFromHash(this);
   if (item) {
      if (item->text(0) != "Console") {
         if (isDocked()) {
         /* Set the brush to blue if undocked */
            QBrush blackBrush(Qt::black);
            item->setForeground(0, blackBrush);
         } else {
         /* Set the brush back to black if docked */
            QBrush blueBrush(Qt::blue);
            item->setForeground(0, blueBrush);
         }
      }
   }
}
