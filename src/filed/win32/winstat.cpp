/*
 * Bacula File daemon Status Dialog box
 *
 *  Inspired from the VNC code by ATT.
 *
 * Copyright (2000) Kern E. Sibbald
 *
 */
/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

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

#include "winbacula.h"
#include "winstat.h"

extern void FillStatusBox(HWND hwnd, int id_list);

bacStatus::bacStatus()
{
   visible = FALSE;
}

bacStatus::~bacStatus()
{
}


/* Dialog box handling functions */
void
bacStatus::Show(BOOL show)
{
   if (show && !visible) {
      DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_STATUS), NULL,
          (DLGPROC)DialogProc, (LONG)this);
   }
}

BOOL CALLBACK
bacStatus::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   /* Get class pointer from user data */
   bacStatus *_this = (bacStatus *)GetWindowLong(hwnd, GWL_USERDATA);

   switch (uMsg) {
   case WM_INITDIALOG:
      /* Set class pointer in user data */
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (bacStatus *)lParam;

      /* show the dialog */
      SetForegroundWindow(hwnd);

      /* Update every 5 seconds */
      SetTimer(hwnd, 1, 5000, NULL); 
      _this->visible = TRUE;
      FillStatusBox(hwnd, IDC_LIST);
      return TRUE;

   case WM_TIMER:
      FillStatusBox(hwnd, IDC_LIST);
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDCANCEL:
      case IDOK:
         KillTimer(hwnd, 1);
         EndDialog(hwnd, TRUE);
         _this->visible = FALSE;
         return TRUE;
      }
      break;

   case WM_DESTROY:
      KillTimer(hwnd, 1);
      EndDialog(hwnd, FALSE);
      _this->visible = FALSE;
      return TRUE;
   }
   return 0;
}
