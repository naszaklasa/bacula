/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxbtreectrl.cpp,v 1.6 2004/07/18 09:29:42 kerns Exp $
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

#include "wxbtreectrl.h"

#include "csprint.h"
#include "wxbmainframe.h"

BEGIN_EVENT_TABLE(wxbTreeCtrl, wxTreeCtrl)
   EVT_LEFT_DCLICK(wxbTreeCtrl::OnDoubleClicked)
   EVT_RIGHT_DOWN(wxbTreeCtrl::OnRightClicked)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxbTREE_MARKED_EVENT)

wxbTreeCtrl::wxbTreeCtrl(
      wxWindow* parent, wxEvtHandler* handler, wxWindowID id, const wxPoint& pos, const wxSize& size): 
            wxTreeCtrl(parent, id, pos, size, wxSUNKEN_BORDER | wxTR_HAS_BUTTONS) {
   this->handler = handler;
}

wxbTreeCtrl::~wxbTreeCtrl() {}

/* 
 * Send mark event if the user double-clicked on the icon.
 */
void wxbTreeCtrl::OnDoubleClicked(wxMouseEvent& event) {
   int flags;
   wxTreeItemId treeid = HitTest(event.GetPosition(), flags);
   if ((flags & wxTREE_HITTEST_ONITEMICON) && (treeid.IsOk())) {
      wxbTreeMarkedEvent evt(GetId(), treeid);

      handler->ProcessEvent(evt);
      
      //No Skip : we don't want this item to be collapsed or expanded
   }
   else {
      event.Skip();
   }
}

/* 
 * Send mark event if the user right clicked on an item.
 */
void wxbTreeCtrl::OnRightClicked(wxMouseEvent& event) {
   int flags;
   wxTreeItemId treeid = HitTest(event.GetPosition(), flags);
   if (treeid.IsOk()) {
      wxbTreeMarkedEvent evt(GetId(), treeid);

      handler->ProcessEvent(evt);
   }
   event.Skip();
}

/* Customized tree event, used for marking events */

wxbTreeMarkedEvent::wxbTreeMarkedEvent(int id, wxTreeItemId& item): wxEvent(id, wxbTREE_MARKED_EVENT) {
   this->item = item;
}

wxbTreeMarkedEvent::~wxbTreeMarkedEvent() {}

wxbTreeMarkedEvent::wxbTreeMarkedEvent(const wxbTreeMarkedEvent& te): wxEvent(te) {
   this->item = te.item;
}

wxEvent *wxbTreeMarkedEvent::Clone() const {
   return new wxbTreeMarkedEvent(*(this));
}

wxTreeItemId wxbTreeMarkedEvent::GetItem() {
   return item;
}
