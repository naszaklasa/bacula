/*
 *  Bacula errno handler
 *
 *    berrno is a simplistic errno handler that works for
 *      Unix, Win32, and Bacula bpipes.
 *
 *    See berrno.h for how to use berrno.
 *
 *   Kern Sibbald, July MMIV
 *
 *   Version $Id: berrno.c,v 1.6 2005/08/21 08:40:45 kerns Exp $
 *
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

#include "bacula.h"

#ifndef HAVE_WIN32
extern const char *get_signal_name(int sig);
extern int num_execvp_errors;
extern int execvp_errors[];
#endif

const char *berrno::strerror()
{
   int stat = 0;
#ifdef HAVE_WIN32
   if (berrno_ & b_errno_win32) {
      return (const char *)buf_;
   }
#else
   if (berrno_ & b_errno_exit) {
      stat = (berrno_ & ~b_errno_exit);       /* remove bit */
      if (stat == 0) {
         return _("Child exited normally.");    /* this really shouldn't happen */
      } else {
         /* Maybe an execvp failure */
         if (stat >= 200) {
            if (stat < 200 + num_execvp_errors) {
               berrno_ = execvp_errors[stat - 200];
            } else {
               return _("Unknown error during program execvp");
            }
         } else {
            Mmsg(&buf_, _("Child exited with code %d"), stat);
            return buf_;
         }
         /* If we drop out here, berrno_ is set to an execvp errno */
      }
   }
   if (berrno_ & b_errno_signal) {
      stat = (berrno_ & ~b_errno_signal);        /* remove bit */
      Mmsg(&buf_, _("Child died from signal %d: %s"), stat, get_signal_name(stat));
      return buf_;
   }
#endif
   /* Normal errno */
   if (bstrerror(berrno_, buf_, 1024) < 0) {
      return _("Invalid errno. No error message possible.");
   }
   return buf_;
}

void berrno::format_win32_message()
{
#ifdef HAVE_WIN32
   LPVOID msg;
   if (berrno_ & b_errno_win32) {
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          GetLastError(),
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&msg,
          0,
          NULL);

      pm_strcpy(&buf_, (const char *)msg);
      LocalFree(msg);
   }
#endif
}

#ifdef TEST_PROGRAM

int main()
{
}
#endif
