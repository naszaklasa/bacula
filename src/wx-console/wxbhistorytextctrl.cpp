/*
 *
 *   Text control with an history of commands typed
 *
 *    Nicolas Boichat, July 2004
 *
 *    Version $Id: wxbhistorytextctrl.cpp,v 1.5 2004/08/12 23:15:39 nboichat Exp $
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
   history.Add("");
}

void wxbHistoryTextCtrl::AddCommand(wxString cmd, wxString description) {
   commands[cmd] = description;
}

void wxbHistoryTextCtrl::ClearCommandList() {
   commands.clear();
}

void wxbHistoryTextCtrl::HistoryAdd(wxString cmd) {
   if (cmd == "") return;
   index = history.Count();
   history[index-1] = cmd;
   history.Add("");
}

void wxbHistoryTextCtrl::SetValue(const wxString& value) {
   if (value == "") {
      help->SetLabel("Type your command below:");
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
   else if (GetValue() != "") {
      wxbCommands::iterator it;
      wxString key;
      wxString helptext = "Unknown command.";
      int found = 0;      
      for( it = commands.begin(); it != commands.end(); ++it ) {         
         if (it->first.Find(GetValue()) == 0) {
            found++;
            if (found > 2) {
               helptext += " " + it->first;
            }
            else if (found > 1) {
               helptext = "Possible completions: " + key + " " + it->first;
            }
            else { // (found == 1)
               helptext = it->first + ": " + it->second;
               key = it->first;
            }
         }
         else if (GetValue().Find(it->first) == 0) {
            helptext = it->first + ": " + it->second;
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
      help->SetLabel("Type your command below:");
      event.Skip();
   }
}
