#ifndef _RESTORE_H_
#define _RESTORE_H_

/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2007 Free Software Foundation Europe e.V.

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
 *   Version $Id$
 *
 *  Kern Sibbald, February 2007
 */

#include <QtGui>
#include "pages.h"
#include "ui_brestore.h"
#include "ui_restore.h"
#include "ui_prerestore.h"

enum {
   R_NONE,
   R_JOBIDLIST,
   R_JOBDATETIME
};

/*
 * The pre-restore dialog selects the Job/Client to be restored
 * It really could use considerable enhancement.
 */
class prerestorePage : public Pages, public Ui::prerestoreForm
{
   Q_OBJECT 

public:
   prerestorePage();
   prerestorePage(QString &data, unsigned int);

private slots:
   void okButtonPushed();
   void cancelButtonPushed();
   void job_name_change(int index);
   void recentChanged(int);
   void jobRadioClicked(bool);
   void jobidsRadioClicked(bool);
   void jobIdEditFinished();

private:
   int m_conn;
   int jobdefsFromJob(QStringList &, QString &);
   void buildPage();
   bool checkJobIdList();
   QString m_dataIn;
   unsigned int m_dataInType;
};

/*  
 * The restore dialog is brought up once we are in the Bacula
 * restore tree routines.  It handles putting up a GUI tree
 * representation of the files to be restored.
 */
class restorePage : public Pages, public Ui::restoreForm
{
   Q_OBJECT 

public:
   restorePage(int conn);
   ~restorePage();
   void fillDirectory();
   char *get_cwd();
   bool cwd(const char *);

private slots:
   void okButtonPushed();
   void cancelButtonPushed();
   void fileDoubleClicked(QTreeWidgetItem *item, int column);
   void directoryItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
   void upButtonPushed();
   void unmarkButtonPushed();
   void markButtonPushed();
   void addDirectory(QString &);

private:
   int m_conn;
   void writeSettings();
   void readSettings();
   QString m_cwd;
   QHash<QString, QTreeWidgetItem *> m_dirPaths;
   QHash<QTreeWidgetItem *,QString> m_dirTreeItems;
   QRegExp m_rx;
   QString m_splitText;
};


class bRestore : public Pages, public Ui::bRestoreForm
{
   Q_OBJECT 

public:
   bRestore();
   ~bRestore();

public slots:

private:

};

#endif /* _RESTORE_H_ */
