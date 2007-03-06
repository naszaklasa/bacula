/*
 *
 *   Text control with an history of commands typed
 *
 *    Nicolas Boichat, July 2004
 *
 *    Version $Id: wxbhistorytextctrl.h,v 1.4.6.1 2005/04/12 21:31:25 kerns Exp $
 */
/*
   Copyright (C) 2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
	 const wxString& value = "", const wxPoint& pos = wxDefaultPosition,
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
