/*
 *
 *    Configuration file editor
 *
 *    Nicolas Boichat, May 2004
 *
 *    Version $Id: wxbconfigfileeditor.cpp,v 1.3 2004/07/18 09:29:42 kerns Exp $
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

#include "wxbconfigfileeditor.h"

#include <wx/file.h>
#include <wx/filename.h>

enum
{
   Save = 1,
   Quit = 2
};

BEGIN_EVENT_TABLE(wxbConfigFileEditor, wxDialog)
   EVT_BUTTON(Save, wxbConfigFileEditor::OnSave)
   EVT_BUTTON(Quit, wxbConfigFileEditor::OnQuit)
END_EVENT_TABLE()

wxbConfigFileEditor::wxbConfigFileEditor(wxWindow* parent, wxString filename):
      wxDialog(parent, -1, "Config file editor", wxDefaultPosition, wxSize(500, 300),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
   this->filename = filename;
   
   textCtrl = new wxTextCtrl(this,-1,"",wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_RICH | wxTE_DONTWRAP);
   wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
#if defined __WXGTK12__ && !defined __WXGTK20__ // Fix for "chinese" fonts under gtk+ 1.2
   font.SetDefaultEncoding(wxFONTENCODING_ISO8859_1);
#endif
   textCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));

   wxFlexGridSizer *mainSizer = new wxFlexGridSizer(2, 1, 0, 0);
   mainSizer->AddGrowableCol(0);
   mainSizer->AddGrowableRow(0);
   
   wxBoxSizer *bottomsizer = new wxBoxSizer(wxHORIZONTAL);
   bottomsizer->Add(new wxButton(this, Save, "Save and close"), 0, wxALL, 10);
   bottomsizer->Add(new wxButton(this, Quit, "Close without saving"), 0, wxALL, 10);
   
   mainSizer->Add(textCtrl, 1, wxEXPAND);
   mainSizer->Add(bottomsizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
   
   this->SetSizer(mainSizer);
   
   wxFileName filen(filename);
   
   if (!filen.FileExists()) {
      (*textCtrl) << "#\n";
      (*textCtrl) << "# Bacula wx-console Configuration File\n";
      (*textCtrl) << "#\n";
      (*textCtrl) << "\n";
      (*textCtrl) << "Director {\n";
      (*textCtrl) << "  Name = <hostname>-dir\n";
      (*textCtrl) << "  DIRport = 9101\n";
      (*textCtrl) << "  address = <hostname>\n";
      (*textCtrl) << "  Password = \"<dir_password>\"\n";
      (*textCtrl) << "}\n";
   }
   else {
      wxFile file(filename);
      wxChar buffer[2049];
      off_t len;
      while ((len = file.Read(buffer, 2048)) > -1) {
         buffer[len] = (wxChar)0;
         (*textCtrl) << buffer;
         if (file.Eof())
            break;
      }
      file.Close();
   }
}

wxbConfigFileEditor::~wxbConfigFileEditor() {
   
}

void wxbConfigFileEditor::OnSave(wxCommandEvent& event) {
   wxFile file(filename, wxFile::write);
   if (!file.IsOpened()) {
      wxMessageBox(wxString("Unable to write to ") << filename << "\n", "Error while saving",
                        wxOK | wxICON_ERROR, this);
      EndModal(wxCANCEL);
      return;
   }
   
   file.Write(textCtrl->GetValue());
   
   file.Flush();
   file.Close();
   
   EndModal(wxOK);
}

void wxbConfigFileEditor::OnQuit(wxCommandEvent& event) {
   EndModal(wxCANCEL);
}
