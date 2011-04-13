/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

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
/*
 *
 *   wxbDataParser, class that receives and analyses data
 *
 *    Nicolas Boichat, April-July 2004
 *
 *    Version $Id$
 */

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"
#include "wxbutils.h"
#include "wxbmainframe.h"
#include "csprint.h"
#include "wxbtableparser.h"

/* A macro named Yield is defined under MinGW */
#undef Yield

bool wxbUtils::inited = false;

wxString wxbUtils::ConvertToPrintable(wxString& str) {
   /* FIXME : Unicode support should be added to fix this problem */
#if (wxUSE_UNICODE == 0) && __WXGTK20__
   wxString strnew(str);
   /* Convert the string to something printable without unicode */
   for (unsigned int i = 0; i < strnew.Length(); i++) {
      /* If the character is not ASCII, print a ? */
      if (((unsigned char)strnew[i] > (unsigned char)127)) {
         strnew[i] = '?';
      }
   }
   return strnew;
#else
   return str;
#endif   
}

/* Sleeps during milliseconds (wrapper for wxUsleep (2.4) or wxMilliSleep (2.6)) */
void wxbUtils::MilliSleep(unsigned long milliseconds) {
#if wxCHECK_VERSION(2, 6, 0)
   ::wxMilliSleep(milliseconds);
#else
   ::wxUsleep(milliseconds);
#endif
}

/* Initialization */
void wxbUtils::Init() {
   inited = true;
}

/* Reset state */
void wxbUtils::Reset() {
   inited = false;
}

/* Parse a table in tableParser */
wxbTableParser* wxbUtils::CreateAndWaitForParser(wxString cmd) {
   wxbTableParser* tableParser = new wxbTableParser();

   wxbMainFrame::GetInstance()->Send(cmd);

   //time_t base = wxDateTime::Now().GetTicks();
   while (!tableParser->hasFinished()) {
      //innerThread->Yield();
      wxTheApp->Yield(true);
      wxbUtils::MilliSleep(100);
      //if (base+15 < wxDateTime::Now().GetTicks()) break;
   }
   return tableParser;
}

/* Run a command, and waits until prompt result is fully received,
 * if keepresults is true, returns a valid pointer to a wxbPromptParser
 * containing the data. */
wxbPromptParser* wxbUtils::WaitForPrompt(wxString cmd, bool keepresults) {
   wxbPromptParser* promptParser = new wxbPromptParser();
   
   wxbMainFrame::GetInstance()->Send(cmd);
    
   //time_t base = wxDateTime::Now().GetTicks();
   while (!promptParser->hasFinished()) {
      //innerThread->Yield();
      wxTheApp->Yield(true);
      wxbUtils::MilliSleep(100);
      //if (base+15 < wxDateTime::Now().GetTicks()) break;
   }
     
   if (keepresults) {
      return promptParser;
   }
   else {
      delete promptParser;
      return NULL;
   }  
}

/* Run a command, and waits until result is fully received. */
wxbDataTokenizer* wxbUtils::WaitForEnd(wxString cmd, bool keepresults, bool linebyline) {
   wxbDataTokenizer* datatokenizer = new wxbDataTokenizer(linebyline);

   wxbMainFrame::GetInstance()->Send(cmd);
   
   //wxbMainFrame::GetInstance()->Print("(<WFE)", CS_DEBUG);
   
   //time_t base = wxDateTime::Now().GetTicks();
   while (!datatokenizer->hasFinished()) {
      //innerThread->Yield();
      wxTheApp->Yield(true);
      wxbUtils::MilliSleep(100);
      //if (base+15 < wxDateTime::Now().GetTicks()) break;
   }
   
   //wxbMainFrame::GetInstance()->Print("(>WFE)", CS_DEBUG);
   
   if (keepresults) {
      return datatokenizer;
   }
   else {
      delete datatokenizer;
      return NULL;
   }
}


/* Creates a new wxbDataParser, and register it in wxbMainFrame */
wxbDataParser::wxbDataParser(bool lineanalysis) {
   wxbMainFrame::GetInstance()->Register(this);
   this->lineanalysis = lineanalysis;
//   buffer = "";
}

/* Destroy a wxbDataParser, and unregister it in wxbMainFrame */
wxbDataParser::~wxbDataParser() {
   wxbMainFrame::GetInstance()->Unregister(this);
}

/*
 * Receives director information, forwarded by wxbMainFrame, and sends it
 * line by line to the virtual function Analyse.
 */
bool wxbDataParser::Print(wxString str, int status) {
   bool ret = false;
   if (lineanalysis) {
      bool allnewline = true;
      for (unsigned int i = 0; i < str.Length(); i++) {
         if (!(allnewline = (str.GetChar(i) == '\n')))
            break;
      }
   
      if (allnewline) {
         ret = Analyse(buffer << wxT("\n"), CS_DATA);
         buffer = wxT("");
         for (unsigned int i = 1; i < str.Length(); i++) {
            ret = Analyse(wxT("\n"), status);
         }
      }
      else {
         wxStringTokenizer tkz(str, wxT("\n"), 
            (wxStringTokenizerMode)(wxTOKEN_RET_DELIMS | wxTOKEN_RET_EMPTY | wxTOKEN_RET_EMPTY_ALL));
   
         while ( tkz.HasMoreTokens() ) {
            buffer << tkz.GetNextToken();
            if (buffer.Length() != 0) {
               if ((buffer.GetChar(buffer.Length()-1) == '\n') ||
                  (buffer.GetChar(buffer.Length()-1) == '\r')) {
                  ret = Analyse(buffer, status);
                  buffer = wxT("");
               }
            }
         }
      }
   
      if (buffer == wxT("$ ")) { // Restore console
         ret = Analyse(buffer, status);
         buffer = wxT("");
      }
   
      if (status != CS_DATA) {
         if (buffer.Length() != 0) {
            ret = Analyse(buffer, CS_DATA);
         }
         buffer = wxT("");
         ret = Analyse(wxT(""), status);
      }
   }
   else {
      ret = Analyse(wxString(str), status);
   }
   return ret;
}

/* Creates a new wxbDataTokenizer */
wxbDataTokenizer::wxbDataTokenizer(bool linebyline): wxbDataParser(linebyline), wxArrayString() {
   finished = false;
}

/* Destroy a wxbDataTokenizer */
wxbDataTokenizer::~wxbDataTokenizer() {

}

/*
 *   Receives director information, forwarded by wxbMainFrame.
 */
bool wxbDataTokenizer::Analyse(wxString str, int status) {
   finished = ((status == CS_END) || (status == CS_PROMPT) || (status == CS_DISCONNECTED));

   if (str != wxT("")) {
      Add(str);
   }
   return false;
}

/* Returns true if the last signal received was an end signal, 
 * indicating that no more data is available */
bool wxbDataTokenizer::hasFinished() {
   return finished;
}

/* Creates a new wxbDataTokenizer */
wxbPromptParser::wxbPromptParser(): wxbDataParser(false) {
   finished = false;
   prompt = false;
   introStr = wxT("");
   choices = NULL;
   numerical = false;
   questionStr = wxT("");
}

/* Destroy a wxbDataTokenizer */
wxbPromptParser::~wxbPromptParser() {
   if (choices) {
      delete choices;
   }
}

/*
 *   Receives director information, forwarded by wxbMainFrame.
 */
bool wxbPromptParser::Analyse(wxString str, int status) {
   if (status == CS_DATA) {
      if (finished || prompt) { /* New question */
         finished = false;
         prompt = false;
         if (choices) {
            delete choices;
            choices = NULL;
            numerical = false;
         }
         questionStr = wxT("");
         introStr = wxT("");
      }
      int i;
      long num;
      
      if (((i = str.Find(wxT(": "))) > 0) && (str.Mid(0, i).Trim(false).ToLong(&num))) { /* List element */
         if (!choices) {
            choices = new wxArrayString();
            choices->Add(wxT("")); /* index 0 is never used by multiple choice questions */
            numerical = true;
         }
         
         if ((long)choices->GetCount() != num) { /* new choice has begun */
            delete choices;
            choices = new wxArrayString();
            choices->Add(wxT(""), num); /* fill until this number */
            numerical = true;
         }
         
         choices->Add(str.Mid(i+2).RemoveLast());
      }
      else if (!choices) { /* Introduction, no list received yet */
         introStr << questionStr;
         questionStr = wxString(str);
      }
      else { /* List receveived, get the question now */
         introStr << questionStr;
         questionStr = wxString(str);
      }
   }
   else {
      finished = ((status == CS_PROMPT) || (status == CS_END) || (status == CS_DISCONNECTED));
      if (prompt = ((status == CS_PROMPT) && (questionStr != wxT("$ ")))) { // && (str.Find(": ") == str.Length())
         if (introStr.Last() == '\n') {
            introStr.RemoveLast();
         }
         if ((introStr != wxT("")) && (questionStr == wxT(""))) {
            questionStr = introStr;
            introStr = wxT("");
         }
         
         if ((!choices) && (questionStr.Find(wxT("(yes/mod/no)")) > -1)) {
            choices = new wxArrayString();
            choices->Add(wxT("yes"));
            choices->Add(wxT("mod"));
            choices->Add(wxT("no"));
            numerical = false;
         }
         return true;
      }
      else { /* ended or (dis)connected */
         if (choices) {
            delete choices;
            choices = NULL;
            numerical = false;
         }
         questionStr = wxT("");
         introStr = wxT("");
      }
   }
   return false;
}

/* Returns true if the last signal received was an prompt signal, 
 * indicating that the answer must be sent */
bool wxbPromptParser::hasFinished() {
   return finished;
}

/* Returns true if the last message received is a prompt message */
bool wxbPromptParser::isPrompt() {
   return prompt;
}

/* Returns multiple choice question's introduction */
wxString wxbPromptParser::getIntroString() {
   return introStr;
}

/* Returns question string */
wxString wxbPromptParser::getQuestionString() {
   return questionStr;
}

/* Returns a wxArrayString containing the indexed choices we have
 * to answer the question, or NULL if this question is not a multiple
 * choice one. */
wxArrayString* wxbPromptParser::getChoices() {
   return choices;
}

/* Returns true if the expected answer to the choice list is a number,
 * false if it is a string (for example yes/mod/no). */
bool wxbPromptParser::isNumericalChoice() {
   return numerical;
}
