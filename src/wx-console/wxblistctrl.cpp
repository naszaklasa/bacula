/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxblistctrl.cpp,v 1.5 2004/07/18 09:29:42 kerns Exp $
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

#include "wxblistctrl.h"

#include "csprint.h"
#include "wxbmainframe.h"

BEGIN_EVENT_TABLE(wxbListCtrl, wxListCtrl)
   EVT_LEFT_DCLICK(wxbListCtrl::OnDoubleClicked)
   EVT_RIGHT_DOWN(wxbListCtrl::OnRightClicked)
END_EVENT_TABLE()

DEFINE_LOCAL_EVENT_TYPE(wxbLIST_MARKED_EVENT)

wxbListCtrl::wxbListCtrl(
      wxWindow* parent, wxEvtHandler* handler, wxWindowID id, const wxPoint& pos, const wxSize& size): 
            wxListCtrl(parent, id, pos, size, wxLC_REPORT | wxSUNKEN_BORDER) {
   this->handler = handler;
}

wxbListCtrl::~wxbListCtrl() {}

/* 
 * Send mark event if the user double-clicked on the icon.
 */
void wxbListCtrl::OnDoubleClicked(wxMouseEvent& event) {
   if (event.GetX() < GetColumnWidth(0)) {
      wxbListMarkedEvent evt(GetId());

      handler->ProcessEvent(evt);
      
      //No Skip : we don't want to go in this directory (if it is a directory)
   }
   else {
      event.Skip();
   }
}

/* 
 * Send mark event if the user right clicked on an item.
 */
void wxbListCtrl::OnRightClicked(wxMouseEvent& event) {
   wxbListMarkedEvent evt(GetId());

   handler->ProcessEvent(evt);
}

/* Customized tree event, used for marking events */

wxbListMarkedEvent::wxbListMarkedEvent(int id): wxEvent(id, wxbLIST_MARKED_EVENT) {}

wxbListMarkedEvent::~wxbListMarkedEvent() {}

wxbListMarkedEvent::wxbListMarkedEvent(const wxbListMarkedEvent& te): wxEvent(te) {}

wxEvent *wxbListMarkedEvent::Clone() const {
   return new wxbListMarkedEvent(*(this));
}
