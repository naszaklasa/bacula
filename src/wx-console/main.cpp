/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

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
 *    Bacula bwx-console application
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
 */


// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"

#include <wx/wxprec.h>
#include <wx/config.h>
#include <wx/intl.h>
#include "wxbmainframe.h"
#include "csprint.h"


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
   OSDependentInit();
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

   wxbMainFrame *frame = wxbMainFrame::CreateInstance(_("Bacula bwx-console"),
                         wxPoint(posx, posy), wxSize(sizex, sizey));

   frame->Show(TRUE);

   frame->Print(wxString::Format(_("Welcome to bacula bwx-console %s (%s)!\n"), wxT(VERSION), wxT(BDATE)), CS_DEBUG);

   frame->StartConsoleThread(wxT(""));
   
   return TRUE;
}
