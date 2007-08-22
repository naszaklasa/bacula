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
 *  Select dialog class
 *
 *   Kern Sibbald, March MMVII
 *
 *  $Id: select.cpp 5257 2007-07-28 10:36:28Z kerns $
 */ 

#include "bat.h"
#include "select.h"

/*
 * Read the items for the selection
 */
selectDialog::selectDialog(Console *console) 
{
   QDateTime dt;
   int stat;
   QListWidgetItem *item;
   int row = 0;

   m_console = console;
   setupUi(this);
   connect(listBox, SIGNAL(currentRowChanged(int)), this, SLOT(index_change(int)));
   setAttribute(Qt::WA_DeleteOnClose);
   m_console->read();                 /* get title */
   labelWidget->setText(m_console->msg());
   while ((stat=m_console->read()) > 0) {
      item = new QListWidgetItem;
      item->setText(m_console->msg());
      listBox->insertItem(row++, item);
   }
   m_console->displayToPrompt();
   this->show();
}

void selectDialog::accept()
{
   char cmd[100];

   this->hide();
   bsnprintf(cmd, sizeof(cmd), "%d", m_index+1);
   m_console->write_dir(cmd);
   m_console->displayToPrompt();
   this->close();
   mainWin->resetFocus();
   m_console->displayToPrompt();

}


void selectDialog::reject()
{
   this->hide();
   mainWin->set_status(" Canceled");
   this->close();
   mainWin->resetFocus();
   m_console->beginNewCommand();
}

/*
 * Called here when the jobname combo box is changed.
 *  We load the default values for the new job in the
 *  other combo boxes.
 */
void selectDialog::index_change(int index)
{
   m_index = index;
}

/*
 * Handle yesno PopUp when Bacula asks a yes/no question.
 */
/*
 * Read the items for the selection
 */
yesnoPopUp::yesnoPopUp(Console *console) 
{
   QMessageBox msgBox;

   setAttribute(Qt::WA_DeleteOnClose);
   console->read();                 /* get yesno question */
   msgBox.setWindowTitle("Bat Question");
   msgBox.setText(console->msg());
   msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
   console->displayToPrompt();
   switch (msgBox.exec()) {
   case QMessageBox::Yes:
      console->write_dir("yes");
      break;
   case QMessageBox::No:
      console->write_dir("no");
      break;
   }
   console->displayToPrompt();
   mainWin->resetFocus();
}
