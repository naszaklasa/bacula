/*
 * Bacula File daemon Status Dialog box
 *
 *  Inspired from the VNC code by ATT.
 *
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#include "bacula.h"
#include "winbacula.h"
#include "winstat.h"
#include "winres.h"

extern void output_status(void sendit(const char *msg, int len, void *sarg), void *arg);

bacStatus::bacStatus()
{
   m_bVisible = FALSE;
   m_hTextDisplay = NULL;
}

bacStatus::~bacStatus()
{
}

void
bacStatus::DisplayString(const char *msg, int len, void *context)
{
   /* Get class pointer from user data */
   bacStatus *_this = (bacStatus *)context;
   const char *pStart;
   const char *pCurrent;

   for (pStart = msg, pCurrent = msg; ; pCurrent++) {
      if (*pCurrent == '\n' || *pCurrent == '\0') {
         int lenSubstring = pCurrent - pStart;
         if (lenSubstring > 0) {
            char *pSubString = (char *)alloca(lenSubstring + 1);
            bstrncpy(pSubString, pStart, lenSubstring + 1);

            SendMessage(_this->m_hTextDisplay, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(_this->m_hTextDisplay, EM_REPLACESEL, 0, (LPARAM)pSubString);
         }
         
         if (*pCurrent == '\n') {
            SendMessage(_this->m_hTextDisplay, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(_this->m_hTextDisplay, EM_REPLACESEL, 0, (LONG)"\r\n");
         }

         if (*pCurrent == '\0'){
            break;
         }
         pStart = pCurrent + 1;
      }
   }
}

void 
bacStatus::UpdateDisplay()
{
   if (m_hTextDisplay != NULL) {
      long  lHorizontalPos = GetScrollPos(m_hTextDisplay, SB_HORZ);
      long  lVerticalPos = GetScrollPos(m_hTextDisplay, SB_VERT);
      long  selStart, selEnd;

      SendMessage(m_hTextDisplay, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

      SetWindowText(m_hTextDisplay, "");

      output_status(DisplayString, this);

      SendMessage(m_hTextDisplay, EM_SETSEL, (WPARAM)selStart, (LPARAM)selEnd);
      SendMessage(m_hTextDisplay, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, lHorizontalPos), 0);
      SendMessage(m_hTextDisplay, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, lVerticalPos), 0);
   }
}

/* Dialog box handling functions */
void
bacStatus::Show(BOOL show)
{
   if (show && !m_bVisible) {
      DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_STATUS), NULL,
          (DLGPROC)DialogProc, (LONG)this);
   }
}

void
bacStatus::ResizeChildren(HWND hDlg, WORD wWidth, WORD wHeight)
{
   if (m_hTextDisplay != NULL) {
      HWND  hwndButton = GetDlgItem(hDlg, IDOK);
      RECT  rcWindow;

      GetWindowRect(hwndButton, &rcWindow);

      LONG  lButtonWidth = rcWindow.right - rcWindow.left;
      LONG  lButtonHeight = rcWindow.bottom - rcWindow.top;

      MoveWindow(m_hTextDisplay, 8, 8, wWidth - lButtonWidth - 24, wHeight - 16, TRUE);
      MoveWindow(hwndButton, wWidth - lButtonWidth - 8, 8, lButtonWidth, lButtonHeight, TRUE);
   }
}


BOOL CALLBACK
bacStatus::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   /* Get class pointer from user data */
   bacStatus *_this = (bacStatus *)GetWindowLong(hDlg, GWL_USERDATA);

   switch (uMsg) {
   case WM_INITDIALOG:
      /* Set class pointer in user data */
      SetWindowLong(hDlg, GWL_USERDATA, lParam);
      _this = (bacStatus *)lParam;
      _this->m_hTextDisplay = GetDlgItem(hDlg, IDC_TEXTDISPLAY);

      /* show the dialog */
      SetForegroundWindow(hDlg);

      /* Update every 5 seconds */
      SetTimer(hDlg, 1, 5000, NULL); 
      _this->m_bVisible = TRUE;
      _this->UpdateDisplay();
      return TRUE;

   case WM_TIMER:
      _this->UpdateDisplay();
      return TRUE;

   case WM_SIZE:
      _this->ResizeChildren(hDlg, LOWORD(lParam), HIWORD(lParam));
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDCANCEL:
      case IDOK:
         KillTimer(hDlg, 1);
         EndDialog(hDlg, TRUE);
         _this->m_bVisible = FALSE;
         return TRUE;
      }
      break;

   case WM_DESTROY:
      _this->m_hTextDisplay = NULL;
      KillTimer(hDlg, 1);
      EndDialog(hDlg, FALSE);
      _this->m_bVisible = FALSE;
      return TRUE;
   }
   return 0;
}
