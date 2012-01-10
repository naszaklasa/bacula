/*
 *
 *   wxbPanel for restoring files
 *
 *    Nicolas Boichat, April-May 2004
 *
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

#ifndef WXBRESTOREPANEL_H
#define WXBRESTOREPANEL_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include "wxbtableparser.h"

#include <wx/thread.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/splitter.h>

#include "wxbutils.h"

#include "wxbconfigpanel.h"
#include "wxbtreectrl.h"
#include "wxblistctrl.h"

WX_DECLARE_LIST(wxEvent, wxbEventList);

/* Class for storing directory entries (results from dir commands). */
class wxbDirEntry {
public:
   wxString perm;
   wxString nlink;
   wxString user;
   wxString group;
   wxString size;
   wxString date;
   int marked; /* 0 - Not Marked, 1 - Marked, 2 - Some file under is marked */
   wxString fullname; /* full name with path */
   wxString filename; /* only filename, no path */
};

/*
 * wxbPanel for restoring files
 */
class wxbRestorePanel : public wxbPanel
{
   public:
      wxbRestorePanel(wxWindow* parent);
      ~wxbRestorePanel();

      /* wxbPanel overloadings */
      virtual wxString GetTitle();
      virtual void EnablePanel(bool enable = true);

   private:
/* Commands called by events handler */

      /* The main button has been clicked */
      void CmdStart();

      /* The cancel button has been clicked */
      void CmdCancel();

      /* Apply configuration changes */
      void CmdConfigApply();

      /* Cancel restore */
      void CmdConfigCancel();

       /* List jobs for a specified client */
      void CmdListJobs();

      /* List files and directories for a specified tree item */
      void CmdList(wxTreeItemId item);

      /* Mark a treeitem (directory) or several listitems (file or directory),
       * state defines if it should mark (1), unmark (0), or switch state (-1) */
      void CmdMark(wxTreeItemId treeitem, long* listitems, int listsize, int state = -1);

/* General functions and variables */
      //bool ended; /* The last command send has finished */

      //long filemessages; /* When restoring, number of files restored */
      long totfilemessages; /* When restoring, number of files to be restored */
      wxString jobid;

      /* Run a dir command, and waits until result is fully received.
       * If recurse is true, update the children too. */
      void UpdateTreeItem(wxTreeItemId item, bool updatelist, bool recurse);

      /* Parse dir command results. */
      int ParseList(wxString line, wxbDirEntry* entry);

      /* Sets a list item state, and update its parents and children if it is a directory */
      void SetListItemState(long listitem, int newstate);

      /* Sets a tree item state, and update its children, parents and list (if necessary) */
      void SetTreeItemState(wxTreeItemId item, int newstate);

      /* Update a tree item parents' state */
      void UpdateTreeItemState(wxTreeItemId item);

      /* Refresh the whole tree. */
      void RefreshTree();

      /* Refresh file list */
      void RefreshList();

      /* Update first config, adapting settings to the job name selected */
      void UpdateFirstConfig();

      /* Update second config */
      bool UpdateSecondConfig(wxbDataTokenizer* dt);

/* Status related */
      enum status_enum
      {
         disabled,    // The panel is not activatable
         activable,   // The panel is activable, but not activated
         entered,     // The panel is activated
         choosing,    // The user is choosing files to restore
         listing,     // Dir listing is in progress
         configuring, // The user is configuring restore process
         restoring,   // Bacula is restoring files
         finished     // Retore done (state will change in activable)
      };

      status_enum status;

      /* Cancelled status :
       *  - 0 - !cancelled
       *  - 1 - will be cancelled
       *  - 2 - has been cancelled */
      int cancelled;

      /* Set current status by enabling/disabling components */
      void SetStatus(status_enum newstatus);

/* UI related */
      bool working; // A command is running, discard GUI events
      void SetWorking(bool working);
      bool IsWorking();
      bool markWhenCommandDone; //If an item should be (un)marked after the current listing/marking is done
      wxTreeItemId currentTreeItem; // Currently selected tree item

      /* Enable or disable config controls status */
      void EnableConfig(bool enable);

/* Event handling */
//      wxbEventList* pendingEvents; /* Stores event sent while working */ //EVTQUEUE
//      bool processing; /* True if pendingEvents is being processed */ //EVTQUEUE

//      virtual void AddPendingEvent(wxEvent& event);
//      virtual bool ProcessEvent(wxEvent& event); //EVTQUEUE

      void OnStart(wxCommandEvent& event);
      void OnCancel(wxCommandEvent& event);

      void OnTreeChanging(wxTreeEvent& event);
      void OnTreeExpanding(wxTreeEvent& event);
      void OnTreeChanged(wxTreeEvent& event);
      void OnTreeMarked(wxbTreeMarkedEvent& event);
      void OnTreeAdd(wxCommandEvent& event);
      void OnTreeRemove(wxCommandEvent& event);
      void OnTreeRefresh(wxCommandEvent& event);

      void OnListMarked(wxbListMarkedEvent& event);
      void OnListActivated(wxListEvent& event);
      void OnListChanged(wxListEvent& event);
      void OnListAdd(wxCommandEvent& event);
      void OnListRemove(wxCommandEvent& event);
      void OnListRefresh(wxCommandEvent& event);

      void OnConfigUpdated(wxCommandEvent& event);
      void OnConfigOk(wxCommandEvent& WXUNUSED(event));
      void OnConfigApply(wxCommandEvent& WXUNUSED(event));
      void OnConfigCancel(wxCommandEvent& WXUNUSED(event));

/* Components */
      wxBoxSizer *centerSizer; /* Center sizer */
      wxSplitterWindow *treelistPanel; /* Panel which contains tree and list */
      wxbConfigPanel *configPanel; /* Panel which contains initial restore options */
      wxbConfigPanel *restorePanel; /* Panel which contains final restore options */

      wxImageList* imagelist; //image list for tree and list

      wxButton* start;
      wxButton* cancel;

      wxbTreeCtrl* tree;
      wxButton* treeadd;
      wxButton* treeremove;
      wxButton* treerefresh;

      wxbListCtrl* list;
      wxButton* listadd;
      wxButton* listremove;
      wxButton* listrefresh;

      wxGauge* gauge;

      long cfgUpdated; //keeps which config fields have been updated

      friend class wxbSplitterWindow;

      DECLARE_EVENT_TABLE();
};

#endif // WXBRESTOREPANEL_H
