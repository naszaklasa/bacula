/*
 *
 *   Main frame header file
 *
 *    Nicolas Boichat, July 2004
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

#ifndef WXBMAINFRAME_H
#define WXBMAINFRAME_H

#include "wx/wxprec.h"

#ifdef __BORLANDC__
   #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#include <wx/notebook.h>

#include <wx/dynarray.h>

#include "console_thread.h"

#include "wxbutils.h"

#include "wxbhistorytextctrl.h"

WX_DEFINE_ARRAY(wxbDataParser*, wxbDataParsers);

// ----------------------------------------------------------------------------
// wxbPrintObject - Used by wxbThreadEvent to contain data sent by director
// ----------------------------------------------------------------------------

class wxbPrintObject: public wxObject {
   public:
      wxString str;
      int status;
      wxbPrintObject(wxString str, int status): wxObject() {
         this->str = str;
         this->status = status;
      }

      wxbPrintObject(const wxbPrintObject& pe) {
         this->str = pe.str;
         this->status = pe.status;
      }
};

// ----------------------------------------------------------------------------
// wxbThreadEvent - Event used by wxbTHREAD_EVENT
// ----------------------------------------------------------------------------

DECLARE_LOCAL_EVENT_TYPE(wxbTHREAD_EVENT, -1)

class wxbThreadEvent: public wxEvent {
   public:
      wxbThreadEvent(int id);
      ~wxbThreadEvent();
      wxbThreadEvent(const wxbThreadEvent& te);
      virtual wxEvent *Clone() const;
      wxbPrintObject* GetEventPrintObject();
      void SetEventPrintObject(wxbPrintObject* object);
};

// Define a new frame type: this is going to be our main frame
class wxbMainFrame : public wxFrame
{
public:
   /* this class is a singleton */
   static wxbMainFrame* CreateInstance(const wxString& title, const wxPoint& pos, const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);
   static wxbMainFrame* GetInstance();

   /* event handlers (these functions should _not_ be virtual) */
   void OnQuit(wxCommandEvent& event);
   void OnAbout(wxCommandEvent& event);
   void OnChangeConfig(wxCommandEvent& event);
   void OnEditConfig(wxCommandEvent& event);
   void OnConnect(wxCommandEvent& event);
   void OnDisconnect(wxCommandEvent& event);
   void OnEnter(wxCommandEvent& event);
   void OnPrint(wxbThreadEvent& event);

   /* Enable and disable panels */
   void EnablePanels();
   void DisablePanels(void* except = NULL);

   void EnableConsole(bool enable = true);

   /*
    *  Prints data received from director to the console,
    *  and forwards it to the panels
    */
   void Print(wxString str, int status);

   /* Sends data to the director */
   void Send(wxString str);

   /*
    *  Starts the thread interacting with the director
    *  If config is not empty, uses this config file.
    */
   void StartConsoleThread(const wxString& config);

   /* Register a new wxbDataParser */
   void Register(wxbDataParser* dp);

   /* Unregister a wxbDataParser */
   void Unregister(wxbDataParser* dp);

   console_thread* ct; /* thread interacting with the director */

private:
   /* private constructor, singleton */
   wxbMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style);
   ~wxbMainFrame();

   static wxbMainFrame *frame; /* this */

   wxMenu *menuFile;

   wxNotebook *notebook; /* main notebook */
   wxTextCtrl *consoleCtrl; /* wxTextCtrl containing graphical console */
   wxStaticText *helpCtrl; /* wxStaticText showing help messages */
   wxbHistoryTextCtrl *typeCtrl; /* wxbHistoryTextCtrl for console user input */
   wxButton *sendButton; /* wxButton used to send data */

   wxbPanel **panels; /* panels array, contained in the notebook */
   wxbDataParsers parsers; /* Data parsers, which need to receive director informations */

   wxbPromptParser* promptparser; /* prompt parser catching uncatched questions */

   bool lockedbyconsole; /* true if the panels have been locked by something typed in the console */

   wxString configfile; /* configfile used */

   wxString consoleBuffer; /* Buffer used to print in the console line by line */

   // any class wishing to process wxWindows events must use this macro
   DECLARE_EVENT_TABLE()
};

#endif // WXBMAINFRAME_H
