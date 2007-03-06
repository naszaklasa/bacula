/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxbtreectrl.cpp,v 1.8 2005/07/08 10:03:27 kerns Exp $
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
