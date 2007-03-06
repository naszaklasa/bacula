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
 *    Version $Id: wxbtableparser.cpp,v 1.6 2004/08/06 15:14:36 nboichat Exp $
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
         wxStringTokenizer tkz(str, "|", wxTOKEN_STRTOK);

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
