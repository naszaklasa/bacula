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

#include "wxbtreectrl.h"

#include "csprint.h"
#include "wxbmainframe.h"

BEGIN_EVENT_TABLE(wxbTreeCtrl, wxTreeCtrl)
   EVT_LEFT_DCLICK(wxbTreeCtrl::OnDoubleClicked)
   EVT_RIGHT_DOWN(wxbTreeCtrl::OnRightClicked)
END_EVENT_TABLE()

DEFINE_LOCAL_EVENT_TYPE(wxbTREE_MARKED_EVENT)

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
