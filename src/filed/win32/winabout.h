/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

   This file was inspired by the VNC Win32 code by ATT

   Kern E. Sibbald, 2000
*/


/* Object implementing the About dialog for Bacula */

class bacAbout;

#ifndef _WINABOUT_H_
#define _WINABOUT_H_ 1

/* Define the bacAbout class */
class bacAbout
{
public:
   bacAbout();
  ~bacAbout();

   /* The dialog box window proc */
   static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   void Show(BOOL show);

   /* Object local storage */
   bool visible;
};

#endif
