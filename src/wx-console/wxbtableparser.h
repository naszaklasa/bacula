/*
 *
 *   Class used to parse tables received from director in this format :
 *
 *  +---------+---------+-------------------+
 *  | Header1 | Header2 | ...               |
 *  +---------+---------+-------------------+
 *  |  Data11 | Data12  | ...               |
 *  |  ....   |  ...    | ...               |
 *  +---------+---------+-------------------+
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: wxbtableparser.h,v 1.7 2004/12/21 16:19:00 kerns Exp $
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

#ifndef WXBTABLEPARSER_H
#define WXBTABLEPARSER_H

#include "wx/wxprec.h"

#ifdef __BORLANDC__
   #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include "wxbutils.h"

#include <wx/dynarray.h>

/*
 * Allow the use of Object Array (auto-deletion, object returned as themselves
 * and not as pointers)
 */
class wxbArrayString: public wxArrayString, public wxObject {
   public:
      wxbArrayString(int n = 1);
      virtual ~wxbArrayString();
};

WX_DECLARE_OBJARRAY( wxbArrayString, wxbTable );

/*
 * Class used to parse tables received from director. Data can be accessed with
 * the operator [].
 *
 * Example : wxString elem = parser[3][2]; fetches column 2 of element 3.
 */
class wxbTableParser: public wxbTable, public wxbDataParser
{
   public:
      wxbTableParser(bool header = true);
      virtual ~wxbTableParser();

      /*
       *   Receives data to analyse.
       */
      virtual bool Analyse(wxString str, int status);

      /*
       *   Return true table parsing has finished.
       */
      bool hasFinished();

      /*
       *   Returns table header as an array of wxStrings.
       */
      const wxbArrayString& GetHeader();
   private:
      wxbArrayString tableHeader;

      /*
       * 0 - Table has not begun
       * 1 - first +--+ line obtained, header will follow
       * 2 - second +--+ line obtained, data will follow
       * 3 - last +--+ line obtained, table parsing has finished
       */
      int separatorNum;
};

#endif // WXBTABLEPARSER_H
