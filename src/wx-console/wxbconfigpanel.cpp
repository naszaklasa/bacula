/*
 *
 *   Config panel, used to specify parameters (for example clients, filesets... in restore)
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxbconfigpanel.cpp,v 1.6 2004/07/18 09:29:42 kerns Exp $
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

#include "wxbconfigpanel.h"

#include <wx/arrimpl.cpp>

WX_DEFINE_OBJARRAY(wxbConfig);

/* Create a new config parameter */
wxbConfigParam::wxbConfigParam(wxString title, wxWindowID id, wxbConfigType type, wxString value) {
   this->title = title;
   this->id = id;
   this->type = type;
   this->value = value;
   this->values = NULL;
   this->nvalues = 0;
   this->choicectrl = NULL;
   this->textctrl = NULL;
   this->statictext = NULL;
}

wxbConfigParam::wxbConfigParam(wxString title, wxWindowID id, wxbConfigType type, int n, wxString* values) {
   this->title = title;
   this->id = id;
   this->type = type;
   this->values = new wxString[n];
   for (int i = 0; i < n; i++) {
      this->values[i] = values[i];
   }
   this->nvalues = n;
   this->choicectrl = NULL;
   this->textctrl = NULL;
   this->statictext = NULL;
}

wxbConfigParam::~wxbConfigParam() {
   if (values) delete[] values;
   if (choicectrl) delete choicectrl;
   if (textctrl) delete textctrl;
   if (statictext) delete statictext;
}
  
void wxbConfigParam::AddControl(wxWindow* parent, wxSizer* sizer) {
   sizer->Add(new wxStaticText(parent, -1, title + ": ", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL);
   switch (type) {
   case text:
      statictext = new wxStaticText(parent, -1, value, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
      sizer->Add(statictext, 1, wxEXPAND | wxADJUST_MINSIZE);
      break;
   case modifiableText:
      textctrl = new wxTextCtrl(parent, id, value, wxDefaultPosition, wxDefaultSize);
      sizer->Add(textctrl, 1, wxEXPAND | wxADJUST_MINSIZE);
      break;
   case choice:
      choicectrl = new wxChoice(parent, id, wxDefaultPosition, wxSize(150, 20), nvalues, values);
      sizer->Add(choicectrl, 1, wxEXPAND);
      break;
   }
}

wxString wxbConfigParam::GetTitle() {
   return title;
}

wxString wxbConfigParam::GetValue() {
   switch (type) {
   case text:
      return (statictext != NULL) ? statictext->GetLabel() : wxString("");
      break;
   case modifiableText:
      return (textctrl != NULL) ? textctrl->GetValue() : wxString("");      
      break;
   case choice:
      return (choicectrl != NULL) ? choicectrl->GetStringSelection() : wxString("");
      break;      
   }
   return "";
}

void wxbConfigParam::SetValue(wxString str) {
   switch (type) {
   case text:
      if (statictext)
         statictext->SetLabel(str);
      break;
   case modifiableText:
      if (textctrl)
         textctrl->SetValue(str);      
      break;
   case choice:
      if (choicectrl) {
         int k;
         for (k = 0; k < choicectrl->GetCount(); k++) {
            if (str == choicectrl->GetString(k)) {
               choicectrl->SetSelection(k);
               break;
            }
         }
         if (k == choicectrl->GetCount()) { // Should never happen
            choicectrl->Append(str);
            choicectrl->SetSelection(k);
         }
      }
      break;      
   }
}

int wxbConfigParam::GetIndex() {
   if (choicectrl) {
      return choicectrl->GetSelection();
   }
   return -1;
}

void wxbConfigParam::SetIndex(int ind) {
   if (choicectrl) {
      choicectrl->SetSelection(ind);
   }
}

void wxbConfigParam::Clear() {
   if (choicectrl) {
      choicectrl->Clear();
   }
}

void wxbConfigParam::Add(wxString value) {
   if (choicectrl) {
      choicectrl->Append(value);
   }
}

/* Creates a new config panel, config must be allocated with new */
wxbConfigPanel::wxbConfigPanel(wxWindow* parent, wxbConfig* config, wxString title,
      wxWindowID ok, wxWindowID cancel, wxWindowID apply): wxPanel(parent) {

   this->config = config;

   wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
   
   mainSizer->Add(
      new wxStaticText(this, -1, title, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER),
            0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

   wxFlexGridSizer* cfgSizer = new wxFlexGridSizer(config->GetCount()+1, 2, 8, 5);
   unsigned int i;
   for (i = 0; i < config->GetCount(); i++) {
      (*config)[i].AddControl(this, cfgSizer);
   }
   mainSizer->Add(cfgSizer, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 5);
   
   wxBoxSizer* restoreBottomSizer = new wxBoxSizer(wxHORIZONTAL);
   
   cfgOk = new wxButton(this, ok, "OK", wxDefaultPosition, wxSize(70, 25));
   restoreBottomSizer->Add(cfgOk, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

   if (apply != -1) {
      cfgApply = new wxButton(this, apply, "Apply", wxDefaultPosition, wxSize(70, 25));
      restoreBottomSizer->Add(cfgApply, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);
   }
   else {
      cfgApply = NULL;
   }

   cfgCancel = new wxButton(this, cancel, "Cancel", wxDefaultPosition, wxSize(70, 25));
   restoreBottomSizer->Add(cfgCancel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
   
   mainSizer->Add(restoreBottomSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 5);
   
   SetSizer(mainSizer);
   mainSizer->SetSizeHints(this);
   
   last = 0;
}

wxbConfigPanel::~wxbConfigPanel() {
   delete config;
}
   
void wxbConfigPanel::SetRowString(const char* title, wxString value) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].SetValue(value);
   }
}

wxString wxbConfigPanel::GetRowString(const char* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      return (*config)[i].GetValue();
   }
   return "";
}

void wxbConfigPanel::SetRowSelection(const char* title, int ind) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].SetIndex(ind);
   }
}

int wxbConfigPanel::GetRowSelection(const char* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      return (*config)[i].GetIndex();
   }
   return -1;
}

void wxbConfigPanel::ClearRowChoices(const char* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].Clear();
   }  
}

void wxbConfigPanel::AddRowChoice(const char* title, wxString value) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].Add(value);
   }  
}

int wxbConfigPanel::FindRow(const char* title) {
   unsigned int i;
   
   for (i = last; i < config->GetCount(); i++) {
      if ((*config)[i].GetTitle() == title) {
         last = i;
         return i;
      }
   }
   
   for (i = 0; i < last; i++) {
      if ((*config)[i].GetTitle() == title) {
         last = i;
         return i;
      }
   }
   
   last = 0;
   return -1;
}

void wxbConfigPanel::EnableApply(bool enable) {
   cfgOk->Enable(!enable);
   if (cfgApply) cfgApply->Enable(enable);
}
