/*
 *
 *   Class used to parse tables received from director in this format :
 *
 *  +---------+---------+-------------------+
 *  | Header1 | Header2 | ...               |
 *  +---------+---------+-------------------+
 *  |  Data11 | Data12  | ...               |
 *  |  ....   | ...     | ...               |
 *  +---------+---------+-------------------+
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2009 Free Software Foundation Europe e.V.

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

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"

#include "wxbtableparser.h" // class's header file

#include "csprint.h"

#include <wx/tokenzr.h>

#include "wxbmainframe.h"

#include <wx/arrimpl.cpp>

WX_DEFINE_OBJARRAY(wxbTable);

wxbArrayString::wxbArrayString(int n) : wxArrayString(), wxObject() {
   Alloc(n);
}

wxbArrayString::~wxbArrayString() {
   
}

/*
 *   wxbTableParser constructor
 */
wxbTableParser::wxbTableParser(bool header) : wxbTable(), wxbDataParser(true) {
   separatorNum = header ? 0 : 2;
   tableHeader = wxbArrayString();
}

/*
 *   wxbTableParser destructor
 */
wxbTableParser::~wxbTableParser() {

}

/*
 *   Returns table header as an array of wxStrings.
 */
const wxbArrayString& wxbTableParser::GetHeader() {
   return tableHeader;
}

/*
 *   Receives data to analyse.
 */
bool wxbTableParser::Analyse(wxString str, int status) {
   if ((status == CS_END) && (separatorNum > 0)) {
      separatorNum = 3;
   }

   if (separatorNum == 3) return false;

   if (str.Length() > 4) {
      if ((str.GetChar(0) == '+') && (str.GetChar(str.Length()-2) == '+') && (str.GetChar(str.Length()-1) == '\n')) {
         separatorNum++;
         return false;
      }

      if ((str.GetChar(0) == '|') && (str.GetChar(str.Length()-2) == '|') && (str.GetChar(str.Length()-1) == '\n')) {
         str.RemoveLast();
         wxStringTokenizer tkz(str, wxT("|"), wxTOKEN_STRTOK);

         if (separatorNum == 1) {
            while ( tkz.HasMoreTokens() ) {
               tableHeader.Add(tkz.GetNextToken().Trim(true).Trim(false));
            }
         }
         else if (separatorNum == 2) {
            wxbArrayString tablerow(tableHeader.GetCount());
            while ( tkz.HasMoreTokens() ) {
               tablerow.Add(tkz.GetNextToken().Trim(true).Trim(false));
            }
            Add(tablerow);
         }
      }
   }
   return false;
}

/*
 *   Return true table parsing has finished.
 */
bool wxbTableParser::hasFinished() {
   return (separatorNum == 3);
}
