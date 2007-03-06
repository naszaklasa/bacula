/*
 *
 *   Custom tree control, which send "tree marked" events when the user right-
 *   click on a item, or double-click on a mark.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxbtreectrl.h,v 1.5 2004/12/21 16:19:00 kerns Exp $
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

#ifndef WXBTREECTRL_H
#define WXBTREECTRL_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include <wx/treectrl.h>

BEGIN_DECLARE_EVENT_TYPES()
   DECLARE_EVENT_TYPE(wxbTREE_MARKED_EVENT,       618)
END_DECLARE_EVENT_TYPES()

/* Customized tree event, used for marking events */
class wxbTreeMarkedEvent: public wxEvent {
   public:
      wxbTreeMarkedEvent(int id, wxTreeItemId& item);
      ~wxbTreeMarkedEvent();
      wxbTreeMarkedEvent(const wxbTreeMarkedEvent& te);
      virtual wxEvent *Clone() const;

      wxTreeItemId GetItem();
   private:
      wxTreeItemId item;
};

typedef void (wxEvtHandler::*wxTreeMarkedEventFunction)(wxbTreeMarkedEvent&);

#define EVT_TREE_MARKED_EVENT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
	wxbTREE_MARKED_EVENT, id, wxID_ANY, \
	(wxObjectEventFunction)(wxEventFunction)(wxTreeMarkedEventFunction)&fn, \
	(wxObject *) NULL \
    ),

/* Customized tree, which transmit double clicks on images */
class wxbTreeCtrl: public wxTreeCtrl {
   public:
      wxbTreeCtrl(wxWindow* parent, wxEvtHandler* handler, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
      ~wxbTreeCtrl();

   private:
      void OnDoubleClicked(wxMouseEvent& event);
      void OnRightClicked(wxMouseEvent& event);

      wxEvtHandler* handler;

      DECLARE_EVENT_TABLE();
};

#endif // WXBTREECTRL_H
