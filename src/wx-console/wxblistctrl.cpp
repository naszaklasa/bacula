/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxblistctrl.cpp,v 1.7 2005/07/08 10:03:25 kerns Exp $
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
