/*
 *
 *    Interaction thread between director and the GUI
 *
 *    Nicolas Boichat, April-May 2004
 *
 *    Version $Id: console_thread.h,v 1.11.2.1 2006/04/12 20:49:18 kerns Exp $
 */
/*
   Copyright (C) 2004-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifndef CONSOLE_THREAD_H
#define CONSOLE_THREAD_H

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
