/*
 *
 *   Text control with an history of commands typed
 *
 *    Nicolas Boichat, July 2004
 *
 *    Version $Id: wxbhistorytextctrl.cpp,v 1.10 2005/08/18 21:45:18 nboichat Exp $
 */
/*
   Copyright (C) 2004-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */
 
#include "wxbhistorytextctrl.h"

BEGIN_EVENT_TABLE(wxbHistoryTextCtrl, wxTextCtrl)
   EVT_KEY_DOWN(wxbHistoryTextCtrl::OnKeyDown)
   EVT_KEY_UP(wxbHistoryTextCtrl::OnKeyUp)
END_EVENT_TABLE()

wxbHistoryTextCtrl::wxbHistoryTextCtrl(wxStaticText* help, 
      wxWindow* parent, wxWindowID id, 
      const wxString& value, const wxPoint& pos, 
      const wxSize& size , 
      const wxValidator& validator, 
      const wxString& name): 
         wxTextCtrl(parent, id, value, pos, size, 
            wxTE_PROCESS_ENTER, validator, name) {
   this->help = help;
   index = 0;
   history.Add(wxT(""));
}

void wxbHistoryTextCtrl::AddCommand(wxString cmd, wxString description) {
   commands[cmd] = description;
}

void wxbHistoryTextCtrl::ClearCommandList() {
   commands.clear();
}

void wxbHistoryTextCtrl::HistoryAdd(wxString cmd) {
   if (cmd == wxT("")) return;
   index = history.Count();
   history[index-1] = cmd;
   history.Add(wxT(""));
}

void wxbHistoryTextCtrl::SetValue(const wxString& value) {
   if (value == wxT("")) {
      help->SetLabel(_("Type your command below:"));
   }
   wxTextCtrl::SetValue(value);
}

void wxbHistoryTextCtrl::OnKeyDown(wxKeyEvent& event) {
   if (event.m_keyCode == WXK_TAB) {

   }
   else {
      event.Skip();
   }
}

void wxbHistoryTextCtrl::OnKeyUp(wxKeyEvent& event) {
   if (event.m_keyCode == WXK_UP) {
      if (index > 0) {
         if (index == ((int)history.Count()-1)) {
            history[index] = GetValue();
         }
         index--;
         SetValue(history[index]);
         SetInsertionPointEnd();
      }
   }
   else if (event.m_keyCode == WXK_DOWN) {
      if (index < ((int)history.Count()-1)) {
         index++;
         SetValue(history[index]);
         SetInsertionPointEnd();
      }      
   }
   else if (GetValue() != wxT("")) {
      wxbCommands::iterator it;
      wxString key;
      wxString helptext = _("Unknown command.");
      int found = 0;      
      for( it = commands.begin(); it != commands.end(); ++it ) {         
         if (it->first.Find(GetValue()) == 0) {
            found++;
            if (found > 2) {
               helptext += wxT(" ") + it->first;
            }
            else if (found > 1) {
               helptext = _("Possible completions: ") + key + wxT(" ") + it->first;
            }
            else { // (found == 1)
               helptext = it->first + wxT(": ") + it->second;
               key = it->first;
            }
         }
         else if (GetValue().Find(it->first) == 0) {
            helptext = it->first + wxT(": ") + it->second;
            found = 0;
            break;
         }
      }
      
      help->SetLabel(helptext);
            
      if (event.m_keyCode == WXK_TAB) {
         if (found == 1) {
            SetValue(key);
            SetInsertionPointEnd();
         }
      }
      else {
         event.Skip();
      }
   }
   else {
      help->SetLabel(_("Type your command below:"));
      event.Skip();
   }
}
