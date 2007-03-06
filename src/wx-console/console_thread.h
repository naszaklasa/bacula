/*
 *
 *    Interaction thread between director and the GUI
 *
 *    Nicolas Boichat, April-May 2004
 *
 *    Version $Id: console_thread.h,v 1.8.4.1 2005/04/12 21:31:24 kerns Exp $
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

#ifndef CONSOLE_THREAD_H
#define CONSOLE_THREAD_H

#include <wx/wxprec.h>

#include <wx/thread.h> // inheriting class's header file
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
