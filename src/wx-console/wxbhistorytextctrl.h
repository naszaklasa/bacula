/*
 *
 *   Text control with an history of commands typed
 *
 *    Nicolas Boichat, July 2004
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

#ifndef WXBHISTORYTEXTCTRL
#define WXBHISTORYTEXTCTRL

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include <wx/treectrl.h>
#include <wx/hashmap.h>

WX_DECLARE_STRING_HASH_MAP( wxString, wxbCommands );

class wxbHistoryTextCtrl: public wxTextCtrl {
   public:
      wxbHistoryTextCtrl(wxStaticText* help, wxWindow* parent, wxWindowID id,
         const wxString& value = wxT(""), const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         const wxValidator& validator = wxDefaultValidator,
         const wxString& name = wxTextCtrlNameStr);

      void HistoryAdd(wxString cmd);

      void AddCommand(wxString cmd, wxString description);
      void ClearCommandList();

      virtual void SetValue(const wxString& value);
   private:
      wxArrayString history;
      wxbCommands commands;
      int index;
      wxStaticText* help;

      void OnKeyUp(wxKeyEvent& event);
      void OnKeyDown(wxKeyEvent& event);

      DECLARE_EVENT_TABLE();
};

#endif //WXBHISTORYTEXTCTRL
