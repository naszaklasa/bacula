//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Bacula
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright (2000) Kern E. Sibbald
//



// winMenu

// This class handles creation of a system-tray icon & menu

class bacMenu;

#if (!defined(_win_bacMENU))
#define _win_bacMENU

#include <lmcons.h>
#include "winabout.h"
#include "winstat.h"
#include "winevents.h"

// Constants
extern const UINT MENU_ABOUTBOX_SHOW;
extern const UINT MENU_STATUS_SHOW;
extern const UINT MENU_EVENTS_SHOW;
extern const UINT MENU_SERVICEHELPER_MSG;
extern const UINT MENU_ADD_CLIENT_MSG;
extern const char *MENU_CLASS_NAME;

// The tray menu class itself
class bacMenu
{
public:
   bacMenu();
   ~bacMenu();
protected:
   // Tray icon handling
   void AddTrayIcon();
   void DelTrayIcon();
   void UpdateTrayIcon(int battstat);
   void SendTrayMsg(DWORD msg, int battstat);

   // Message handler for the tray window
   static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

   // Fields
protected:

   // About dialog for this server
   bacAbout  m_about;

   // Status dialog for this server
   bacStatus m_status;

   bacEvents m_events;

   HWND  m_hwnd;
   HMENU m_hmenu;
   NOTIFYICONDATA m_nid;

   // The icon handles
   HICON m_idle_icon;
   HICON m_running_icon;
   HICON m_error_icon;
   HICON m_warn_icon;
};


#endif
