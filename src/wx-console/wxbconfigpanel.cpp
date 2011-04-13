/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

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
 *
 *   Config panel, used to specify parameters (for example clients, filesets... in restore)
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
 */

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"
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
   sizer->Add(new wxStaticText(parent, -1, title + wxT(": "), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), 0, wxALIGN_CENTER_VERTICAL);
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
#if defined __WXGTK20__ /* Choices are taller under GTK+-2.0 */
      wxSize size = wxSize(150, 25);
#else
      wxSize size = wxSize(150, 20);
#endif
      choicectrl = new wxChoice(parent, id, wxDefaultPosition, size, nvalues, values);
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
      return (statictext != NULL) ? statictext->GetLabel() : wxString(wxT(""));
      break;
   case modifiableText:
      return (textctrl != NULL) ? textctrl->GetValue() : wxString(wxT(""));      
      break;
   case choice:
      return (choicectrl != NULL) ? choicectrl->GetStringSelection() : wxString(wxT(""));
      break;      
   }
   return wxT("");
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
         for (k = 0; k < (int)choicectrl->GetCount(); k++) {
            if (str == choicectrl->GetString(k)) {
               choicectrl->SetSelection(k);
               break;
            }
         }
         if (k == (int)choicectrl->GetCount()) { // Should never happen
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

int wxbConfigParam::GetCount() {
   if (choicectrl) {
      return choicectrl->GetCount();
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
   
   cfgOk = new wxButton(this, ok, _("OK"), wxDefaultPosition, wxSize(70, 25));
   restoreBottomSizer->Add(cfgOk, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

   if (apply != -1) {
      cfgApply = new wxButton(this, apply, _("Apply"), wxDefaultPosition, wxSize(70, 25));
      restoreBottomSizer->Add(cfgApply, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);
   }
   else {
      cfgApply = NULL;
   }

   cfgCancel = new wxButton(this, cancel, _("Cancel"), wxDefaultPosition, wxSize(70, 25));
   restoreBottomSizer->Add(cfgCancel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
   
   mainSizer->Add(restoreBottomSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL, 5);
   
   SetSizer(mainSizer);
   mainSizer->SetSizeHints(this);
   
   last = 0;
}

wxbConfigPanel::~wxbConfigPanel() {
   delete config;
}
   
void wxbConfigPanel::SetRowString(const wxChar* title, wxString value) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].SetValue(value);
   }
}

wxString wxbConfigPanel::GetRowString(const wxChar* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      return (*config)[i].GetValue();
   }
   return wxT("");
}

void wxbConfigPanel::SetRowSelection(const wxChar* title, int ind) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].SetIndex(ind);
   }
}

int wxbConfigPanel::GetRowSelection(const wxChar* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      return (*config)[i].GetIndex();
   }
   return -1;
}

void wxbConfigPanel::ClearRowChoices(const wxChar* title) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].Clear();
   }  
}

void wxbConfigPanel::AddRowChoice(const wxChar* title, wxString value) {
   int i;
   if ((i = FindRow(title)) > -1) {
      (*config)[i].Add(value);
   }  
}

int wxbConfigPanel::FindRow(const wxChar* title) {
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

int wxbConfigPanel::GetRowCount(const wxChar* title)
{
   int i;
   if ((i = FindRow(title)) > -1) {
      return (*config)[i].GetCount();
   }  

   return -1;
}

void wxbConfigPanel::EnableApply(bool enable) {
   cfgOk->Enable(!enable);
   if (cfgApply) cfgApply->Enable(enable);
}
