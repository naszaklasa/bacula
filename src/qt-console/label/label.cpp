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
 *  Label Page class
 *
 *   Kern Sibbald, February MMVII
 *
 */ 

#include "bat.h"
#include "label.h"
#include <QMessageBox>

labelPage::labelPage()
{
   QString deflt("");
   m_closeable = false;
   showPage(deflt);
}

/*
 * An overload of the constructor to have a default storage show in the
 * combobox on start.  Used from context sensitive in storage class.
 */
labelPage::labelPage(QString &defString)
{
   m_closeable = true;
   showPage(defString);
}

/*
 * moved the constructor code here for the overload.
 */
void labelPage::showPage(QString &defString)
{
   m_name = "Label";
   pgInitialize();
   setupUi(this);
   m_console->notify(false);
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/label.png")));

   storageCombo->addItems(m_console->storage_list);
   int index = storageCombo->findText(defString, Qt::MatchExactly);
   if (index != -1) {
      storageCombo->setCurrentIndex(index);
   }
   poolCombo->addItems(m_console->pool_list);
   connect(okButton, SIGNAL(pressed()), this, SLOT(okButtonPushed()));
   connect(cancelButton, SIGNAL(pressed()), this, SLOT(cancelButtonPushed()));
   connect(automountOnButton, SIGNAL(pressed()), this, SLOT(automountOnButtonPushed()));
   connect(automountOffButton, SIGNAL(pressed()), this, SLOT(automountOffButtonPushed()));
   dockPage();
   setCurrent();
   this->show();
}


void labelPage::okButtonPushed()
{
   QString scmd;
   if (volumeName->text().toUtf8().data()[0] == 0) {
      QMessageBox::warning(this, "No Volume name", "No Volume name given",
                           QMessageBox::Ok, QMessageBox::Ok);
      return;
   }
   this->hide();
   scmd = QString("label volume=\"%1\" pool=\"%2\" storage=\"%3\" slot=%4\n")
                  .arg(volumeName->text())
                  .arg(poolCombo->currentText())
                  .arg(storageCombo->currentText()) 
                  .arg(slotSpin->value());
   if (mainWin->m_commandDebug) {
      Pmsg1(000, "sending command : %s\n",scmd.toUtf8().data());
   }
   m_console->write_dir(scmd.toUtf8().data());
   m_console->displayToPrompt();
   m_console->notify(true);
   closeStackPage();
   mainWin->resetFocus();
}

void labelPage::cancelButtonPushed()
{
   this->hide();
   m_console->notify(true);
   closeStackPage();
   mainWin->resetFocus();
}

/* turn automount on */
void labelPage::automountOnButtonPushed()
{
   QString cmd("automount on");
   consoleCommand(cmd);
}

/* turn automount off */
void labelPage::automountOffButtonPushed()
{
   QString cmd("automount off");
   consoleCommand(cmd);
}
