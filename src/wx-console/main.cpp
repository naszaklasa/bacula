/*
 *
 *    Bacula wx-console application
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: main.cpp,v 1.17 2005/08/19 09:34:49 nboichat Exp $
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

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "config.h"

#include <wx/wxprec.h>
#include <wx/config.h>
#include <wx/intl.h>

#include "wxbmainframe.h"

#include "csprint.h"

void InitWinAPIWrapper();

/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) { return 1; }
int generate_job_event(JCR *jcr, const char *event) { return 1; }


// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

class MyApp : public wxApp
{
public:
   virtual bool OnInit();
};

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_APP(MyApp)

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
   /* wxWidgets internationalisation */
   wxLocale m_locale;
   m_locale.Init();
   m_locale.AddCatalog(wxT("bacula"));
   wxLocale::AddCatalogLookupPathPrefix(wxT(LOCALEDIR));

   long posx, posy, sizex, sizey;
   int displayx, displayy;
   InitWinAPIWrapper();
   wxConfig::Get()->Read(wxT("/Position/X"), &posx, 50);
   wxConfig::Get()->Read(wxT("/Position/Y"), &posy, 50);
   wxConfig::Get()->Read(wxT("/Size/Width"), &sizex, 780);
   wxConfig::Get()->Read(wxT("/Size/Height"), &sizey, 500);
   
   wxDisplaySize(&displayx, &displayy);
   
   /* Check if we are on the screen... */
   if ((posx+sizex > displayx) || (posy+sizey > displayy)) {
      /* Try to move the top-left corner first */
      posx = 50;
      posy = 50;
      if ((posx+sizex > displayx) || (posy+sizey > displayy)) {
         posx = 25;
         posy = 25;
         sizex = displayx - 50;
         sizey = displayy - 50;
      }
   }

   wxbMainFrame *frame = wxbMainFrame::CreateInstance(_("Bacula wx-console"),
                         wxPoint(posx, posy), wxSize(sizex, sizey));

   frame->Show(TRUE);

   frame->Print(wxString::Format(_("Welcome to bacula wx-console %s (%s)!\n"), wxT(VERSION), wxT(BDATE)), CS_DEBUG);

   frame->StartConsoleThread(wxT(""));
   
   return TRUE;
}

#ifndef HAVE_WIN32
void InitWinAPIWrapper() { };
#endif
