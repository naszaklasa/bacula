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
 * Kern Sibbald, February MMVII
 */

#ifndef _RELABEL_H_
#define _RELABEL_H_

#include <QtGui>
#include "ui_relabel.h"
#include "console.h"

class relabelDialog : public QDialog, public Ui::relabelForm
{
   Q_OBJECT 

public:
   relabelDialog(Console *console, QString &fromVolume);

private slots:
   void accept();
   void reject();

private:
   Console *m_console;
   QString m_fromVolume;
};

#endif /* _RELABEL_H_ */
