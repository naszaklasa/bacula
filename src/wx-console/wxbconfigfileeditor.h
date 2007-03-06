/*
 *
 *    Configuration file editor
 *
 *    Nicolas Boichat, May 2004
 *
 *    Version $Id: wxbconfigfileeditor.h,v 1.5 2005/08/27 14:33:39 nboichat Exp $
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

#include "wx/wxprec.h"
#include "wx/wx.h"
#include <wx/dialog.h>

#include <wx/textctrl.h>

class wxbConfigFileEditor : public wxDialog {
public:
	wxbConfigFileEditor(wxWindow* parent, wxString filename);
	virtual ~wxbConfigFileEditor();
private:
   wxString filename;

   wxTextCtrl* textCtrl;

   bool firstpaint;

   void OnSave(wxCommandEvent& event);
   void OnQuit(wxCommandEvent& event);
   void OnPaint(wxPaintEvent& event);

   DECLARE_EVENT_TABLE()
};
