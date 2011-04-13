/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2007 Free Software Foundation Europe e.V.

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
/*
 *
 *   Main frame
 *
 *    Nicolas Boichat, July 2004
 *
 *    Version $Id$
 */

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"

#include "wxbmainframe.h" // class's header file

#include "wxbrestorepanel.h"

#include "wxbconfigfileeditor.h"

#include "csprint.h"

#include "wxwin16x16.xpm"

#include <wx/arrimpl.cpp>

#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/config.h>

#include <wx/filename.h>

#undef Yield /* MinGW defines Yield */

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
   // menu items
   Minimal_Quit = 1,

   // it is important for the id corresponding to the "About" command to have
   // this standard value as otherwise it won't be handled properly under Mac
   // (where it is special and put into the "Apple" menu)
   Minimal_About = wxID_ABOUT,
   
   ChangeConfigFile = 2,
   EditConfigFile = 3,
   MenuConnect = 4,
   MenuDisconnect = 5,
   TypeText = 6,
   SendButton = 7,
   Thread = 8
};

/*
 *   wxbTHREAD_EVENT declaration, used by csprint
 */

DEFINE_EVENT_TYPE(wxbTHREAD_EVENT)

typedef void (wxEvtHandler::*wxThreadEventFunction)(wxbThreadEvent&);

#define EVT_THREAD_EVENT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxbTHREAD_EVENT, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction)(wxThreadEventFunction)&fn, \
        (wxObject *) NULL \
    ),

// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(wxbMainFrame, wxFrame)
   EVT_MENU(Minimal_Quit,  wxbMainFrame::OnQuit)
   EVT_MENU(Minimal_About, wxbMainFrame::OnAbout)
   EVT_MENU(ChangeConfigFile, wxbMainFrame::OnChangeConfig)
   EVT_MENU(EditConfigFile, wxbMainFrame::OnEditConfig)
   EVT_MENU(MenuConnect, wxbMainFrame::OnConnect)
   EVT_MENU(MenuDisconnect, wxbMainFrame::OnDisconnect)
   EVT_TEXT_ENTER(TypeText, wxbMainFrame::OnEnter)
   EVT_THREAD_EVENT(Thread, wxbMainFrame::OnPrint)
   EVT_BUTTON(SendButton, wxbMainFrame::OnEnter)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxbThreadEvent
// ----------------------------------------------------------------------------

/*
 *  wxbThreadEvent constructor
 */
wxbThreadEvent::wxbThreadEvent(int id): wxEvent(id, wxbTHREAD_EVENT) {
   m_eventObject = NULL;
}

/*
 *  wxbThreadEvent destructor
 */
wxbThreadEvent::~wxbThreadEvent()
{
   if (m_eventObject != NULL) {
      delete m_eventObject;
   }
}

/*
 *  wxbThreadEvent copy constructor
 */
wxbThreadEvent::wxbThreadEvent(const wxbThreadEvent& te)
{
   this->m_eventType = te.m_eventType;
   this->m_id = te.m_id;
   if (te.m_eventObject != NULL) {
      this->m_eventObject = new wxbPrintObject(*((wxbPrintObject*)te.m_eventObject));
   }
   else {
      this->m_eventObject = NULL;
   }
   this->m_skipped = te.m_skipped;
   this->m_timeStamp = te.m_timeStamp;
}

/*
 *  Must be implemented (abstract in wxEvent)
 */
wxEvent* wxbThreadEvent::Clone() const
{
   return new wxbThreadEvent(*this);
}

/*
 *  Gets the wxbPrintObject attached to this event, containing data sent by director
 */
wxbPrintObject* wxbThreadEvent::GetEventPrintObject()
{
   return (wxbPrintObject*)m_eventObject;
}

/*
 *  Sets the wxbPrintObject attached to this event
 */
void wxbThreadEvent::SetEventPrintObject(wxbPrintObject* object)
{
   m_eventObject = (wxObject*)object;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

wxbMainFrame *wxbMainFrame::frame = NULL;

/*
 *  Singleton constructor
 */
wxbMainFrame* wxbMainFrame::CreateInstance(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
{
   frame = new wxbMainFrame(title, pos, size, style);
   return frame;
}

/*
 *  Returns singleton instance
 */
wxbMainFrame* wxbMainFrame::GetInstance()
{
   return frame;
}

/*
 *  Private destructor
 */
wxbMainFrame::~wxbMainFrame()
{
   wxConfig::Get()->Write(wxT("/Position/X"), (long)GetPosition().x);
   wxConfig::Get()->Write(wxT("/Position/Y"), (long)GetPosition().y);
   wxConfig::Get()->Write(wxT("/Size/Width"), (long)GetSize().GetWidth());
   wxConfig::Get()->Write(wxT("/Size/Height"), (long)GetSize().GetHeight());

   if (ct != NULL) { // && (!ct->IsRunning())
      ct->Delete();
   }
   frame = NULL;
}

/*
 *  Private constructor
 */
wxbMainFrame::wxbMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
      : wxFrame(NULL, -1, title, pos, size, style)
{
   lockedbyconsole = false;
   
   ct = NULL;
   
   promptparser = NULL;

   // set the frame icon
   SetIcon(wxIcon(wxwin16x16_xpm));

#if wxUSE_MENUS
   // create a menu bar
   menuFile = new wxMenu;

   // the "About" item should be in the help menu
   wxMenu *helpMenu = new wxMenu;
   helpMenu->Append(Minimal_About, _("&About...\tF1"), _("Show about dialog"));

   menuFile->Append(MenuConnect, _("Connect"), _("Connect to the director"));
   menuFile->Append(MenuDisconnect, _("Disconnect"), _("Disconnect of the director"));
   menuFile->AppendSeparator();
   menuFile->Append(ChangeConfigFile, _("Change of configuration file"), _("Change your default configuration file"));
   menuFile->Append(EditConfigFile, _("Edit your configuration file"), _("Edit your configuration file"));
   menuFile->AppendSeparator();
   menuFile->Append(Minimal_Quit, _("E&xit\tAlt-X"), _("Quit this program"));

   // now append the freshly created menu to the menu bar...
   wxMenuBar *menuBar = new wxMenuBar();
   menuBar->Append(menuFile, _("&File"));
   menuBar->Append(helpMenu, _("&Help"));

   // ... and attach this menu bar to the frame
   SetMenuBar(menuBar);
#endif // wxUSE_MENUS

   CreateStatusBar(1);
   
   SetStatusText(wxString::Format(_("Welcome to bacula bwx-console %s (%s)!\n"), wxT(VERSION), wxT(BDATE)));

   wxPanel* global = new wxPanel(this, -1);

   notebook = new wxNotebook(global, -1);

   /* Console */

   wxPanel* consolePanel = new wxPanel(notebook, -1);
   notebook->AddPage(consolePanel, _("Console"));

   consoleCtrl = new wxTextCtrl(consolePanel,-1,wxT(""),wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
   wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
#if defined __WXGTK12__ && !defined __WXGTK20__ // Fix for "chinese" fonts under gtk+ 1.2
   font.SetDefaultEncoding(wxFONTENCODING_ISO8859_1);
   consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
   Print(_("Warning : Unicode is disabled because you are using wxWidgets for GTK+ 1.2.\n"), CS_DEBUG);
#else 
   consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
#if (wxUSE_UNICODE == 0) && __WXGTK20__
   Print(_("Warning : There is a problem with wxWidgets for GTK+ 2.0 without Unicode support when handling non-ASCII filenames: Every non-ASCII character in such filenames will be replaced by an interrogation mark.\nIf this behaviour disturbs you, please build bwx-console against a Unicode version of wxWidgets for GTK+ 2.0.\n---\n"), CS_DEBUG);   
#endif
#endif

   helpCtrl = new wxStaticText(consolePanel, -1, _("Type your command below:"));

   wxFlexGridSizer *consoleSizer = new wxFlexGridSizer(4, 1, 0, 0);
   consoleSizer->AddGrowableCol(0);
   consoleSizer->AddGrowableRow(0);

   typeCtrl = new wxbHistoryTextCtrl(helpCtrl, consolePanel,TypeText,wxT(""),wxDefaultPosition,wxSize(200,20));
   sendButton = new wxButton(consolePanel, SendButton, _("Send"));
   
   wxFlexGridSizer *typeSizer = new wxFlexGridSizer(1, 2, 0, 0);
   typeSizer->AddGrowableCol(0);
   typeSizer->AddGrowableRow(0);

   //typeSizer->Add(new wxStaticText(consolePanel, -1, _("Command: ")), 0, wxALIGN_CENTER | wxALL, 0);
   typeSizer->Add(typeCtrl, 1, wxEXPAND | wxALL, 0);
   typeSizer->Add(sendButton, 1, wxEXPAND | wxLEFT, 5);

   consoleSizer->Add(consoleCtrl, 1, wxEXPAND | wxALL, 0);
   consoleSizer->Add(new wxStaticLine(consolePanel, -1), 0, wxEXPAND | wxALL, 0);
   consoleSizer->Add(helpCtrl, 1, wxEXPAND | wxALL, 2);
   consoleSizer->Add(typeSizer, 0, wxEXPAND | wxALL, 2);

   consolePanel->SetAutoLayout( TRUE );
   consolePanel->SetSizer( consoleSizer );
   consoleSizer->SetSizeHints( consolePanel );

   // Creates the list of panels which are included in notebook, and that need to receive director information

   panels = new wxbPanel* [2];
   panels[0] = new wxbRestorePanel(notebook);
   panels[1] = NULL;

   for (int i = 0; panels[i] != NULL; i++) {
      notebook->AddPage(panels[i], panels[i]->GetTitle());
   }

   wxBoxSizer* globalSizer = new wxBoxSizer(wxHORIZONTAL);

#if wxCHECK_VERSION(2, 6, 0)
   globalSizer->Add(notebook, 1, wxEXPAND, 0);
#else
   globalSizer->Add(new wxNotebookSizer(notebook), 1, wxEXPAND, 0);
#endif

   global->SetSizer( globalSizer );
   globalSizer->SetSizeHints( global );

   wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

   sizer->Add(global, 1, wxEXPAND | wxALL, 0);
   SetAutoLayout(true);
   SetSizer( sizer );
   sizer->SetSizeHints( this );
   this->SetSize(size);
   EnableConsole(false);
   
   consoleBuffer = wxT("");
   
   configfile = wxT("");
}

/*
 *  Starts the thread interacting with the director
 *  If config is not empty, uses this config file.
 */
void wxbMainFrame::StartConsoleThread(const wxString& config) {
   menuFile->Enable(MenuConnect, false);
   menuFile->Enable(MenuDisconnect, false);
   menuFile->Enable(ChangeConfigFile, false);
   menuFile->Enable(EditConfigFile, false);

   if (ct != NULL) {
      ct->Delete();
      ct = NULL;
      wxTheApp->Yield();
   }
   if (promptparser == NULL) {
      promptparser = new wxbPromptParser();      
   }
   
   if (config == wxT("")) {
      configfile = wxT("");
      
      if (((wxTheApp->argc % 2) != 1)) {
         Print(_("Error while parsing command line arguments, using defaults.\n"), CS_DEBUG);
         Print(_("Usage: bwx-console [-c configfile] [-w tmp]\n"), CS_DEBUG);
      }
      else {
         for (int c = 1; c < wxTheApp->argc; c += 2) {
            if ((wxTheApp->argc >= c+2) && (wxString(wxTheApp->argv[c]) == wxT("-c"))) {
               configfile = wxTheApp->argv[c+1];
            }
            if ((wxTheApp->argc >= c+2) && (wxString(wxTheApp->argv[c]) == wxT("-w"))) {
               console_thread::SetWorkingDirectory(wxTheApp->argv[c+1]);
            }
            if (wxTheApp->argv[c][0] != '-') {
               Print(_("Error while parsing command line arguments, using defaults.\n"), CS_DEBUG);
               Print(_("Usage: bwx-console [-c configfile] [-w tmp]\n"), CS_DEBUG);
               break;
            }
         }
      }
      
      if (configfile == wxT("")) {
         wxConfig::Set(new wxConfig(wxT("bwx-console"), wxT("bacula")));
         if (!wxConfig::Get()->Read(wxT("/ConfigFile"), &configfile)) {
#ifdef HAVE_MACOSX
            wxFileName filename(::wxGetHomeDir());
            filename.MakeAbsolute();
            configfile = filename.GetLongPath();
            if (!IsPathSeparator(configfile.Last())) {
               configfile += '/';
            }
            configfile += L"Library/Preferences/org.bacula.wxconsole.conf";
#else
            wxFileName filename(::wxGetCwd(), wxT("bwx-console.conf"));
            filename.MakeAbsolute();
            configfile = filename.GetLongPath();
#ifdef HAVE_WIN32
            configfile.Replace(wxT("\\"), wxT("/"));
#endif //HAVE_WIN32
#endif //HAVE_MACOSX
            wxConfig::Get()->Write(wxT("/ConfigFile"), configfile);
   
            int answer = wxMessageBox(
                              wxString::Format(_(
                              "It seems that it is the first time you run bwx-console.\nThis file (%s) has been choosen as default configuration file.\nDo you want to edit it? (if you click No you will have to select another file)"),
                              configfile.c_str()),
                              _("First run"),
                              wxYES_NO | wxICON_QUESTION, this);
            if (answer == wxYES) {
               wxbConfigFileEditor(this, configfile).ShowModal();
            }
         }
      }
   }
   else {
      configfile = config;
   }
   
   wxString err = console_thread::LoadConfig(configfile);
   
   while (err != wxT("")) {
      int answer = wxMessageBox(
                        wxString::Format(_(
                           "Unable to read %s\nError: %s\nDo you want to choose another one? (Press no to edit this file)"),
                           configfile.c_str(), err.c_str()),
                        _("Unable to read configuration file"),
                        wxYES_NO | wxCANCEL | wxICON_ERROR, this);
      if (answer == wxNO) {
         wxbConfigFileEditor(this, configfile).ShowModal();
         err = console_thread::LoadConfig(configfile);
      }
      else if (answer == wxCANCEL) {
         frame = NULL;
         Close(true);
         return;
      }
      else { // (answer == wxYES)
         configfile = wxFileSelector(_("Please choose a configuration file to use"));
         if ( !configfile.empty() ) {
            err = console_thread::LoadConfig(configfile);
         }
         else {
            frame = NULL;
            Close(true);
            return;
         }
      }
      
      if ((err == wxT("")) && (config == wxT(""))) {
         answer = wxMessageBox(
                           _("This configuration file has been successfully read, use it as default?"),
                           _("Configuration file read successfully"),
                           wxYES_NO | wxICON_QUESTION, this);
         if (answer == wxYES) {
              wxConfigBase::Get()->Write(wxT("/ConfigFile"), configfile);
         }
         break;
      }
   }
   
   // former was csprint
   Print(wxString::Format(_("Using this configuration file: %s\n"), configfile.c_str()), CS_DEBUG);
   
   ct = new console_thread();
   ct->Create();
   ct->Run();
   SetStatusText(_("Connecting to the director..."));
}

/* Register a new wxbDataParser */
void wxbMainFrame::Register(wxbDataParser* dp) {
   parsers.Add(dp);
}
   
/* Unregister a wxbDataParser */
void wxbMainFrame::Unregister(wxbDataParser* dp) {
   int index;
   if ((index = parsers.Index(dp)) != wxNOT_FOUND) {
      parsers.RemoveAt(index);
   }
   else {
      Print(_("Failed to unregister a data parser !"), CS_DEBUG);
   }
}

// event handlers

void wxbMainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
   Print(_("Quitting.\n"), CS_DEBUG);
   if (ct != NULL) {
      ct->Delete();
      ct = NULL;
      wxTheApp->Yield();
   }
   console_thread::FreeLib();
   frame = NULL;
   wxTheApp->Yield();
   Close(TRUE);
}

void wxbMainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   wxString msg;
   msg.Printf(_("Welcome to Bacula bwx-console.\n"
     "Written by Nicolas Boichat <nicolas@boichat.ch>\n"
     "Copyright (C), 2005-2007 Free Software Foundation Europe, e.V.\n"));

   wxMessageBox(msg, _("About Bacula bwx-console"), wxOK | wxICON_INFORMATION, this);
}

void wxbMainFrame::OnChangeConfig(wxCommandEvent& event) {
   wxString oriconfigfile;
   wxConfig::Get()->Read(wxT("/ConfigFile"), &oriconfigfile);
   wxString configfile = wxFileSelector(_("Please choose your default configuration file"));
   if ( !configfile.empty() ) {
      if (oriconfigfile != configfile) {
         int answer = wxMessageBox(
                           _("Use this configuration file as default?"),
                           _("Configuration file"),
                           wxYES_NO | wxICON_QUESTION, this);
         if (answer == wxYES) {
              wxConfigBase::Get()->Write(wxT("/ConfigFile"), configfile);
              wxConfigBase::Get()->Flush();
              StartConsoleThread(wxT(""));
              return;
         }
      }
   
      StartConsoleThread(configfile);
   }
}

void wxbMainFrame::OnEditConfig(wxCommandEvent& event) {
   wxString configfile;
   wxConfig::Get()->Read(wxT("/ConfigFile"), &configfile);
   int stat = wxbConfigFileEditor(this, configfile).ShowModal();
   if (stat == wxOK) {
      StartConsoleThread(configfile);
   }
}

void wxbMainFrame::OnConnect(wxCommandEvent& event) {
   StartConsoleThread(configfile);
}

void wxbMainFrame::OnDisconnect(wxCommandEvent& event) {
   if (ct != NULL) {
      ct->Delete();
      ct = NULL;
   }
}

void wxbMainFrame::OnEnter(wxCommandEvent& WXUNUSED(event))
{
   lockedbyconsole = true;
   DisablePanels();
   typeCtrl->HistoryAdd(typeCtrl->GetValue());
   wxString str = typeCtrl->GetValue() + wxT("\n");
   Send(str);
}

/*
 *  Called when data is arriving from director
 */
void wxbMainFrame::OnPrint(wxbThreadEvent& event) {
   wxbPrintObject* po = event.GetEventPrintObject();

   Print(po->str, po->status);
}

/*
 *  Prints data received from director to the console, and forwards it to the panels
 */
void wxbMainFrame::Print(wxString str, int status)
{
   if (lockedbyconsole) {
      EnableConsole(false);
   }
   
   if (status == CS_REMOVEPROMPT) {
      if (consoleCtrl->GetLastPosition() > 0) {
         consoleCtrl->Remove(consoleCtrl->GetLastPosition()-1, consoleCtrl->GetLastPosition()+1);
      }
      return;
   }
   
   if (status == CS_TERMINATED) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = wxT("");
      SetStatusText(_("Console thread terminated."));
#ifdef HAVE_WIN32
      consoleCtrl->PageDown();
#else
      consoleCtrl->ScrollLines(1);
#endif
      ct = NULL;
      DisablePanels();
      int answer = wxMessageBox( _("Connection to the director lost. Quit program?"), 
                                 _("Connection lost"),
                        wxYES_NO | wxICON_EXCLAMATION, this);
      if (answer == wxYES) {
         frame = NULL;
         Close(true);
      }
      menuFile->Enable(MenuConnect, true);
      menuFile->SetLabel(MenuConnect, _("Connect"));
      menuFile->SetHelpString(MenuConnect, _("Connect to the director"));
      menuFile->Enable(MenuDisconnect, false);
      menuFile->Enable(ChangeConfigFile, true);
      menuFile->Enable(EditConfigFile, true);
      return;
   }
   
   if (status == CS_CONNECTED) {
      SetStatusText(_("Connected to the director."));
      typeCtrl->ClearCommandList();
      bool parsed = false;
      int retries = 3;
      wxbDataTokenizer* dt = wxbUtils::WaitForEnd(wxT(".help"), true);
      while (true) {
         int i, j;
         wxString str;
         for (i = 0; i < (int)dt->GetCount(); i++) {
            str = (*dt)[i];
            str.RemoveLast();
            if ((j = str.Find(' ')) > -1) {
               typeCtrl->AddCommand(str.Mid(0, j), str.Mid(j+1));
               parsed = true;
            }
         }
         retries--;
         if ((parsed) || (!retries))
            break;
         dt = wxbUtils::WaitForEnd(wxT(""), true);
      }
      EnablePanels();
      menuFile->Enable(MenuConnect, true);
      menuFile->SetLabel(MenuConnect, _("Reconnect"));
      menuFile->SetHelpString(MenuConnect, _("Reconnect to the director"));
      menuFile->Enable(MenuDisconnect, true);
      menuFile->Enable(ChangeConfigFile, true);
      menuFile->Enable(EditConfigFile, true);
      return;
   }
   if (status == CS_DISCONNECTED) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = wxT("");
#ifdef HAVE_WIN32
      consoleCtrl->PageDown();
#else
      consoleCtrl->ScrollLines(1);
#endif
      SetStatusText(_("Disconnected of the director."));
      DisablePanels();
      return;
   }
      
   // CS_DEBUG is often sent by panels, 
   // and resend it to them would sometimes cause infinite loops
   
   /* One promptcaught is normal, so we must have two true Print values to be
    * sure that the prompt has effectively been caught.
    */
   int promptcaught = -1;
   
   if (status != CS_DEBUG) {
      for (unsigned int i = 0; i < parsers.GetCount(); i++) {
         promptcaught += parsers[i]->Print(str, status) ? 1 : 0;
      }
         
      if ((status == CS_PROMPT) && (promptcaught < 1) && (promptparser->isPrompt())) {
         Print(_("Unexpected question has been received.\n"), CS_DEBUG);
//         Print(wxString("(") << promptparser->getIntroString() << "/-/" << promptparser->getQuestionString() << ")\n", CS_DEBUG);
         
         wxString message;
         if (promptparser->getIntroString() != wxT("")) {
            message << promptparser->getIntroString() << wxT("\n");
         }
         message << promptparser->getQuestionString();
         
         if (promptparser->getChoices()) {
            wxString *choices = new wxString[promptparser->getChoices()->GetCount()];
            int *numbers = new int[promptparser->getChoices()->GetCount()];
            int n = 0;
            
            for (unsigned int i = 0; i < promptparser->getChoices()->GetCount(); i++) {
               if ((*promptparser->getChoices())[i] != wxT("")) {
                  choices[n] = (*promptparser->getChoices())[i];
                  numbers[n] = i;
                  n++;
               }
            }
            
            int res = ::wxGetSingleChoiceIndex(message,
               _("bwx-console: unexpected director's question."), n, choices, this);
            if (res == -1) { //Cancel pressed
               Send(wxT(".\n"));
            }
            else {
               if (promptparser->isNumericalChoice()) {
                  Send(wxString() << numbers[res] << wxT("\n"));
               }
               else {
                  Send(wxString() << choices[res] << wxT("\n"));
               }
            }
            delete[] choices;
            delete[] numbers;
         }
         else {
            Send(::wxGetTextFromUser(message,
               _("bwx-console: unexpected director's question."),
               wxT(""), this) + wxT("\n"));
         }
      }
   }
      
   if (status == CS_END) {
      if (lockedbyconsole) {
         EnablePanels();
         lockedbyconsole = false;
      }
      str = wxT("#");
   }

   if (status == CS_DEBUG) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = wxT("");
#ifdef HAVE_WIN32
      consoleCtrl->PageDown();
#else
      consoleCtrl->ScrollLines(1);
#endif
      consoleCtrl->SetDefaultStyle(wxTextAttr(wxColour(0, 128, 0)));
   }
   else {
      consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));
   }
   consoleBuffer << wxbUtils::ConvertToPrintable(str);
   if (status == CS_PROMPT) {
      if (lockedbyconsole) {
         EnableConsole(true);
      }
      //consoleBuffer << wxT("<P>");
   }
   
   if ((status == CS_END) || (status == CS_PROMPT) || (str.Find(wxT("\n")) > -1)) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = wxT("");

#ifdef HAVE_WIN32
      consoleCtrl->PageDown();
#else
      consoleCtrl->ScrollLines(1);
#endif
   }
   
   //consoleCtrl->ShowPosition(consoleCtrl->GetLastPosition());
   
   /*if (status != CS_DEBUG) {
      consoleCtrl->AppendText("@");
   }*/
   //consoleCtrl->SetInsertionPointEnd();
}

/*
 *  Sends data to the director
 */
void wxbMainFrame::Send(wxString str)
{
   if (ct != NULL) {
      /* wxString may contain everything in UNICODE
       * -> convert to UTF-8 and send to director
       */
      ct->Write (str.mb_str(wxConvUTF8));
      typeCtrl->SetValue(wxT(""));
      consoleCtrl->SetDefaultStyle(wxTextAttr(*wxRED));
      consoleCtrl->AppendText(wxbUtils::ConvertToPrintable(str));      
      //consoleCtrl->PageDown();
   }
   
/*   if ((consoleCtrl->GetNumberOfLines()-1) > nlines) {
      nlines = consoleCtrl->GetNumberOfLines()-1;
   }
   
   consoleCtrl->ShowPosition(nlines);*/
}

/* Enable panels */
void wxbMainFrame::EnablePanels() {
   for (int i = 0; panels[i] != NULL; i++) {
      panels[i]->EnablePanel(true);
   }
   EnableConsole(true);
}

/* Disable panels, except the one passed as parameter */
void wxbMainFrame::DisablePanels(void* except) {
   for (int i = 0; panels[i] != NULL; i++) {
      if (panels[i] != except) {
         panels[i]->EnablePanel(false);
      }
      else {
         panels[i]->EnablePanel(true);
      }
   }
   if (this != except) {
      EnableConsole(false);
   }
}

/* Enable or disable console typing */
void wxbMainFrame::EnableConsole(bool enable) {
   typeCtrl->Enable(enable);
   sendButton->Enable(enable);
   if (enable) {
      typeCtrl->SetFocus();
   }
}

/*
 *  Used by csprint, which is called by console thread.
 *
 *  In GTK and perhaps X11, only the main thread is allowed to interact with
 *  graphical components, so by firing an event, the main loop will call OnPrint.
 *
 *  Calling OnPrint directly from console thread produces "unexpected async replies".
 */
void firePrintEvent(wxString str, int status)
{
   wxbPrintObject* po = new wxbPrintObject(str, status);

   wxbThreadEvent evt(Thread);
   evt.SetEventPrintObject(po);
   
   if (wxbMainFrame::GetInstance()) {
      wxbMainFrame::GetInstance()->AddPendingEvent(evt);
   }
}

//wxString csBuffer; /* Temporary buffer for receiving data from console thread */

/*
 *  Called by console thread, this function forwards data line by line and end
 *  signals to the GUI.
 */

void csprint(wxString str, int status)
{
   firePrintEvent(str, status);  
}


void csprint(const char* str, int status)
{
   if (str != 0) {
      firePrintEvent(wxString(str,wxConvUTF8), status);      
   }
   else {
      firePrintEvent(wxT(""), status);
   }
}
