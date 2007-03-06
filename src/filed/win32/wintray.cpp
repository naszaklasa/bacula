//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the vnc system.
//
//  The vnc system is free software; you can redistribute it and/or modify
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
// If the source code for the vnc system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Bacula
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright 2000-2004, Kern E. Sibbald
//



// Tray

// Implementation of a system tray icon & menu for Bacula

#include "winbacula.h"
#include "winservice.h"
#include <lmcons.h>

// Header

#include "wintray.h"
#include "bacula.h"
#include "jcr.h"

// Constants
#ifdef properties_implemented
const UINT MENU_PROPERTIES_SHOW = RegisterWindowMessage("Bacula.Properties.User.Show");
const UINT MENU_DEFAULT_PROPERTIES_SHOW = RegisterWindowMessage("Bacula.Properties.Default.Show");
#endif
const UINT MENU_ABOUTBOX_SHOW = RegisterWindowMessage("Bacula.AboutBox.Show");
const UINT MENU_STATUS_SHOW = RegisterWindowMessage("Bacula.Status.Show");
const UINT MENU_EVENTS_SHOW = RegisterWindowMessage("Bacula.Events.Show");
const UINT MENU_SERVICEHELPER_MSG = RegisterWindowMessage("Bacula.ServiceHelper.Message");
const UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("Bacula.AddClient.Message");
const char *MENU_CLASS_NAME = "Bacula Tray Icon";

extern void terminate_filed(int sig);
extern char *bac_status(char *buf, int buf_len);
extern int bacstat;

// Implementation

bacMenu::bacMenu()
{
   // Create a dummy window to handle tray icon messages
   WNDCLASSEX wndclass;

   wndclass.cbSize                 = sizeof(wndclass);
   wndclass.style                  = 0;
   wndclass.lpfnWndProc    = bacMenu::WndProc;
   wndclass.cbClsExtra             = 0;
   wndclass.cbWndExtra             = 0;
   wndclass.hInstance              = hAppInstance;
   wndclass.hIcon                  = LoadIcon(NULL, IDI_APPLICATION);
   wndclass.hCursor                = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground  = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wndclass.lpszMenuName   = (const char *) NULL;
   wndclass.lpszClassName  = MENU_CLASS_NAME;
   wndclass.hIconSm                = LoadIcon(NULL, IDI_APPLICATION);

   RegisterClassEx(&wndclass);

   m_hwnd = CreateWindow(MENU_CLASS_NAME,
                           MENU_CLASS_NAME,
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           200, 200,
                           NULL,
                           NULL,
                           hAppInstance,
                           NULL);
   if (m_hwnd == NULL) {
      PostQuitMessage(0);
      return;
   }

   // record which client created this window
   SetWindowLong(m_hwnd, GWL_USERDATA, (LONG) this);

   // Timer to trigger icon updating
   SetTimer(m_hwnd, 1, 5000, NULL);

   // Load the icons for the tray
   m_idle_icon    = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_IDLE));
   m_running_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_RUNNING));
   m_error_icon   = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_JOB_ERROR));
   m_warn_icon    = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_JOB_WARNING));

   // Load the popup menu
   m_hmenu = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_TRAYMENU));

   // Install the tray icon!
   AddTrayIcon();
}

bacMenu::~bacMenu()
{
   // Remove the tray icon
   SendTrayMsg(NIM_DELETE, 0);
        
   // Destroy the loaded menu
   if (m_hmenu != NULL)
      DestroyMenu(m_hmenu);
}

void
bacMenu::AddTrayIcon()
{
   SendTrayMsg(NIM_ADD, bacstat);
}

void
bacMenu::DelTrayIcon()
{
   SendTrayMsg(NIM_DELETE, 0);
}


void
bacMenu::UpdateTrayIcon(int bacstat)
{
   (void *)bac_status(NULL, 0);
   SendTrayMsg(NIM_MODIFY, bacstat);
}

void
bacMenu::SendTrayMsg(DWORD msg, int bacstat)
{
   struct s_last_job *job;
   
   // Create the tray icon message
   m_nid.hWnd = m_hwnd;
   m_nid.cbSize = sizeof(m_nid);
   m_nid.uID = IDI_BACULA;                  // never changes after construction
   switch (bacstat) {
   case 0:
      m_nid.hIcon = m_idle_icon;
      break;
   case JS_Running:
      m_nid.hIcon = m_running_icon;
      break;
   case JS_ErrorTerminated:
      m_nid.hIcon = m_error_icon;
      break;
   default:
      if (last_jobs->size() > 0) {
         job = (struct s_last_job *)last_jobs->last();
         if (job->Errors) {
            m_nid.hIcon = m_warn_icon;
         } else {
            m_nid.hIcon = m_idle_icon;
         }
      } else {
         m_nid.hIcon = m_idle_icon;
      }
      break;
   }

   m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
   m_nid.uCallbackMessage = WM_TRAYNOTIFY;


   // Use resource string as tip if there is one
   if (LoadString(hAppInstance, IDI_BACULA, m_nid.szTip, sizeof(m_nid.szTip))) {
       m_nid.uFlags |= NIF_TIP;
   }

   // Try to add the Bacula status to the tip string, if possible
   if (m_nid.uFlags & NIF_TIP) {
       bac_status(m_nid.szTip, sizeof(m_nid.szTip));
   }

   // Send the message
   if (Shell_NotifyIcon(msg, &m_nid)) {
      EnableMenuItem(m_hmenu, ID_CLOSE, MF_ENABLED);
   } else {
      if (!bacService::RunningAsService()) {
         if (msg == NIM_ADD) {
            // The tray icon couldn't be created, so use the Properties dialog
            // as the main program window
         // removed because it causes quit when not running as a
         // service in use with BartPe.
         // PostQuitMessage(0);
         }
      }
   }
}

// Process window messages
LRESULT CALLBACK bacMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   // This is a static method, so we don't know which instantiation we're 
   // dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion 
   // from a newsgroup to get the pseudo-this.
   bacMenu *_this = (bacMenu *) GetWindowLong(hwnd, GWL_USERDATA);

   switch (iMsg) {

   // Every five seconds, a timer message causes the icon to update
   case WM_TIMER:
      // *** HACK for running servicified
      if (bacService::RunningAsService()) {
          // Attempt to add the icon if it's not already there
          _this->AddTrayIcon();
      }

      // Update the icon
      _this->UpdateTrayIcon(bacstat);
     break;

   // STANDARD MESSAGE HANDLING
   case WM_CREATE:
      return 0;

   case WM_COMMAND:
      // User has clicked an item on the tray menu
      switch (LOWORD(wParam)) {
      case ID_STATUS:
         // Show the status dialog
         _this->m_status.Show(TRUE);
         _this->UpdateTrayIcon(bacstat);
         break;

      case ID_EVENTS:
         // Show the Events dialog
         _this->m_events.Show(TRUE);
         _this->UpdateTrayIcon(bacstat);
         break;


      case ID_KILLCLIENTS:
         // Disconnect all currently connected clients
         break;

      case ID_ABOUT:
         // Show the About box
         _this->m_about.Show(TRUE);
         break;

      case ID_CLOSE:
         // User selected Close from the tray menu
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;

      }
      return 0;

   case WM_TRAYNOTIFY:
      // User has clicked on the tray icon or the menu
      {
         // Get the submenu to use as a pop-up menu
         HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

         // What event are we responding to, RMB click?
         if (lParam==WM_RBUTTONUP) {
            if (submenu == NULL) {
                    return 0;
            }

            // Make the first menu item the default (bold font)
            SetMenuDefaultItem(submenu, 0, TRUE);
            
            // Get the current cursor position, to display the menu at
            POINT mouse;
            GetCursorPos(&mouse);

            // There's a "bug"
            // (Microsoft calls it a feature) in Windows 95 that requires calling
            // SetForegroundWindow. To find out more, search for Q135788 in MSDN.
            //
            SetForegroundWindow(_this->m_nid.hWnd);

            // Display the menu at the desired position
            TrackPopupMenu(submenu,
                            0, mouse.x, mouse.y, 0,
                            _this->m_nid.hWnd, NULL);

            return 0;
         }
         
         // Or was there a LMB double click?
         if (lParam==WM_LBUTTONDBLCLK) {
             // double click: execute first menu item
             SendMessage(_this->m_nid.hWnd,
                         WM_COMMAND, 
                         GetMenuItemID(submenu, 0),
                         0);
         }

         return 0;
      }

   case WM_CLOSE:
      terminate_filed(0);
      break;

   case WM_DESTROY:
      // The user wants Bacula to quit cleanly...
      PostQuitMessage(0);
      return 0;

   case WM_QUERYENDSESSION:
      // Are we running as a system service?
      // Or is the system shutting down (in which case we should check anyway!)
      if ((!bacService::RunningAsService()) || (lParam == 0)) {
         // No, so we are about to be killed

         // If there are remote connections then we should verify
         // that the user is happy about killing them.

         // Finally, post a quit message, just in case
         PostQuitMessage(0);
         return TRUE;
      }
      return TRUE;

   
   default:
      if (iMsg == MENU_ABOUTBOX_SHOW) {
         // External request to show our About dialog
         PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_ABOUT, 0), 0);
         return 0;
      }
      if (iMsg == MENU_STATUS_SHOW) {
         // External request to show our status
         PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_STATUS, 0), 0);
         return 0;
      }
   }

   // Message not recognised
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
