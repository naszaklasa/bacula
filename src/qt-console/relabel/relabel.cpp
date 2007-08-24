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
 *  Label Dialog class
 *
 *   Kern Sibbald, February MMVII
 *
 */ 

#include "bat.h"
#include "relabel.h"
#include <QMessageBox>

/*
 * An overload of the constructor to have a default storage show in the
 * combobox on start.  Used from context sensitive in storage class.
 */
relabelDialog::relabelDialog(Console *console, QString &fromVolume)
{
   m_console = console;
   m_fromVolume = fromVolume;
   m_console->notify(false);
   setupUi(this);
   storageCombo->addItems(console->storage_list);
   poolCombo->addItems(console->pool_list);
   volumeName->setText(fromVolume);
   QString fromText("From Volume : ");
   fromText += fromVolume;
   fromLabel->setText(fromText);
   this->show();
}


void relabelDialog::accept()
{
   QString scmd;
   if (volumeName->text().toUtf8().data()[0] == 0) {
      QMessageBox::warning(this, "No Volume name", "No Volume name given",
                           QMessageBox::Ok, QMessageBox::Ok);
      return;
   }
   if (m_fromVolume == volumeName->text().toUtf8()) {
      QMessageBox::warning(this, "New name must be different", "New name must be different",
                           QMessageBox::Ok, QMessageBox::Ok);
      return;
   }

   this->hide();
   scmd = QString("relabel storage=\"%1\" oldvolume=\"%2\" volume=\"%3\" pool=\"%4\" slot=%5")
                  .arg(storageCombo->currentText())
                  .arg(m_fromVolume)
                  .arg(volumeName->text())
                  .arg(poolCombo->currentText())
                  .arg(slotSpin->value());
   if (mainWin->m_commandDebug) {
      Pmsg1(000, "sending command : %s\n",scmd.toUtf8().data());
   }
   m_console->write_dir(scmd.toUtf8().data());
   m_console->displayToPrompt();
   m_console->notify(true);
   delete this;
   mainWin->resetFocus();
}

void relabelDialog::reject()
{
   this->hide();
   m_console->notify(true);
   delete this;
   mainWin->resetFocus();
}
