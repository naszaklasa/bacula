/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2009 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
 
/*
 *  Select dialog class
 *
 *   Kern Sibbald, March MMVII
 *
 *  $Id: select.cpp 8775 2009-04-30 16:57:18Z bartleyd2 $
 */ 

#include "bat.h"
#include "textinput.h"

/*
 * Read input text box
 */
textInputDialog::textInputDialog(Console *console, int conn) 
{
   m_conn = conn;
   QDateTime dt;

   m_console = console;
   setupUi(this);
   setAttribute(Qt::WA_DeleteOnClose);
   labelWidget->setText(m_console->returnFromPrompt(m_conn));
   this->show();
}

void textInputDialog::accept()
{
   this->hide();
   m_console->write_dir(m_conn, lineEdit->text().toUtf8().data());
   m_console->displayToPrompt(m_conn);
   mainWin->resetFocus();
   m_console->displayToPrompt(m_conn);
   m_console->notify(m_conn, true);
   this->close();
}


void textInputDialog::reject()
{
   this->hide();
   mainWin->set_status(tr(" Canceled"));
   mainWin->resetFocus();
   m_console->beginNewCommand(m_conn);
   m_console->notify(m_conn, true);
   this->close();
}

