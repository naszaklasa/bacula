/*
 *
 *    Interaction thread between director and the GUI
 *
 *    Nicolas Boichat, April-May 2004
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

#ifndef CONSOLE_THREAD_H
#define CONSOLE_THREAD_H

#ifndef ENABLE_NLS
#undef setlocale
#undef textdomain
#undef bindtextdomain
#endif


#include <wx/wxprec.h>

#include <wx/string.h>
#include <wx/thread.h> // inheriting class's header file
#include <wx/intl.h> /* We need to have _ and N_ defined by wxWidgets before Bacula tries to redefine them */
#include "bacula.h"
#include "jcr.h"

/**
 * Console thread, does interaction between bacula routines and the GUI
 */
class console_thread : public wxThread
{
   public:
      // class constructor
      console_thread();
      // class destructor
      ~console_thread();

      void* Entry();
      void Write(const char* str);
      virtual void Delete();

      static void InitLib();
      static void FreeLib();
      static wxString LoadConfig(wxString configfile);
      static void SetWorkingDirectory(wxString w_dir);
   private:
      static bool inited;
      static bool configloaded;

      bool choosingdirector;

      static wxString working_dir;

      int directorchoosen;

      BSOCK* UA_sock;
      JCR jcr;
};

int pm_cst_strcpy(POOLMEM **pm, const char *str);

#endif // CONSOLE_THREAD_H
