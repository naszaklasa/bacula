/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"

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
