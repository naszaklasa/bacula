/*
 *
 *   wxbDataParser, class that receives and analyses data
 *   wxbPanel, main frame's notebook panels
 *
 *    Nicolas Boichat, April-July 2004
 *
 *   Version $Id: wxbutils.h,v 1.5.10.1 2005/04/12 21:31:25 kerns Exp $
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

#ifndef WXBUTILS_H
#define WXBUTILS_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

class wxbTableParser;
class wxbDataParser;
class wxbDataTokenizer;
class wxbPromptParser;

/*
 *  General functions
 */
class wxbUtils
{
   public:
      /* Initialization */
      static void Init();

      /* Reset state */
      static void Reset();

      /* Parse a table in tableParser */
      static wxbTableParser* CreateAndWaitForParser(wxString cmd);

      /* Run a command, and waits until result is fully received,
       * if keepresults is true, returns a valid pointer to a wxbDataTokenizer
       * containing the data. */
      static wxbDataTokenizer* WaitForEnd(wxString cmd, bool keepresults = false, bool linebyline = true);

      /* Run a command, and waits until prompt result is fully received,
       * if keepresults is true, returns a valid pointer to a wxbPromptParser
       * containing the data. */
      static wxbPromptParser* WaitForPrompt(wxString cmd, bool keepresults = false);

   private:
      static bool inited;
};

/*
 *  abstract class that can receive director information.
 */
class wxbDataParser
{
   public:
      /* Creates a new wxbDataParser, and register it in wxbMainFrame
       * lineanalysis : indicate if date is to be analysed line by line (true)
       * or packet by packet (false).
       */
      wxbDataParser(bool lineanalysis);

      /* Destroy a wxbDataParser, and unregister it in wxbMainFrame */
      virtual ~wxbDataParser();

      /*
       * Receives director information, forwarded by wxbMainFrame, and sends it
       * line by line to the virtual function Analyse.
       *
       * Returns true if status == CS_PROMPT and the message has been parsed
       * correctly.
       */
      bool Print(wxString str, int status);

      /*
       *   Receives data to analyse.
       */
      virtual bool Analyse(wxString str, int status) = 0;
   private:
      bool lineanalysis;
      wxString buffer;
};

/*
 *  abstract panel that can receive director information.
 */
class wxbPanel : public wxPanel
{
   public:
      wxbPanel(wxWindow* parent) : wxPanel(parent) {}

      /*
       *   Tab title in the notebook.
       */
      virtual wxString GetTitle() = 0;

      /*
       *   Enable or disable this panel
       */
      virtual void EnablePanel(bool enable = true) = 0;
};

/*
 *  Receives director information, and splits it by line.
 *
 * datatokenizer[0] retrieves first line
 */
class wxbDataTokenizer: public wxbDataParser, public wxArrayString
{
   public:
      /* Creates a new wxbDataTokenizer */
      wxbDataTokenizer(bool linebyline);

      /* Destroy a wxbDataTokenizer */
      virtual ~wxbDataTokenizer();

      /*
       *   Receives data to analyse.
       */
      virtual bool Analyse(wxString str, int status);

      /* Returns true if the last signal received was an end signal,
       * indicating that no more data is available */
      bool hasFinished();

   private:
      bool finished;
};

/*
 *  Receives director information, and check if the last messages are questions.
 */
class wxbPromptParser: public wxbDataParser
{
   public:
      /* Creates a new wxbDataTokenizer */
      wxbPromptParser();

      /* Destroy a wxbDataTokenizer */
      virtual ~wxbPromptParser();

      /*
       *   Receives data to analyse.
       */
      virtual bool Analyse(wxString str, int status);

      /* Returns true if the last signal received was an prompt signal,
       * or an end signal */
      bool hasFinished();

      /* Returns true if the last message received is a prompt message */
      bool isPrompt();

      /* Returns multiple choice question's introduction */
      wxString getIntroString();

      /* Returns question string */
      wxString getQuestionString();

      /* Returns a wxArrayString containing the indexed choices we have
       * to answer the question, or NULL if this question is not a multiple
       * choice one. */
      wxArrayString* getChoices();

      /* Returns true if the expected answer to the choice list is a number,
       * false if it is a string (for example yes/mod/no). */
      bool isNumericalChoice();

   private:
      bool finished;
      bool prompt;
      bool numerical;
      wxString introStr;
      wxArrayString* choices;
      wxString questionStr;
};

#endif // WXBUTILS_H
