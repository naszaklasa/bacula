/*
 *
 *   wxbDataParser, class that receives and analyses data
 *
 *    Nicolas Boichat, April-July 2004
 *
 *    Version $Id: wxbutils.cpp,v 1.8 2004/07/18 09:29:42 kerns Exp $
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
 
#include "wxbutils.h"

#include "wxbmainframe.h"

#include "csprint.h"

#include "wxbtableparser.h"

/* A macro named Yield is defined under MinGW */
#undef Yield

bool wxbUtils::inited = false;

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
      ::wxUsleep(100);
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
      ::wxUsleep(100);
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
      ::wxUsleep(100);
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
         ret = Analyse(buffer << "\n", CS_DATA);
         buffer = "";
         for (unsigned int i = 1; i < str.Length(); i++) {
            ret = Analyse("\n", status);
         }
      }
      else {
         wxStringTokenizer tkz(str, "\n", 
            (wxStringTokenizerMode)(wxTOKEN_RET_DELIMS | wxTOKEN_RET_EMPTY | wxTOKEN_RET_EMPTY_ALL));
   
         while ( tkz.HasMoreTokens() ) {
            buffer << tkz.GetNextToken();
            if (buffer.Length() != 0) {
               if ((buffer.GetChar(buffer.Length()-1) == '\n') ||
                  (buffer.GetChar(buffer.Length()-1) == '\r')) {
                  ret = Analyse(buffer, status);
                  buffer = "";
               }
            }
         }
      }
   
      if (buffer == "$ ") { // Restore console
         ret = Analyse(buffer, status);
         buffer = "";
      }
   
      if (status != CS_DATA) {
         if (buffer.Length() != 0) {
            ret = Analyse(buffer, CS_DATA);
         }
         buffer = "";
         ret = Analyse("", status);
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

   if (str != "") {
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
   introStr = "";
   choices = NULL;
   numerical = false;
   questionStr = "";
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
         questionStr = "";
         introStr = "";
      }
      int i;
      long num;
      
      if (((i = str.Find(": ")) > 0) && (str.Mid(0, i).Trim(false).ToLong(&num))) { /* List element */
         if (!choices) {
            choices = new wxArrayString();
            choices->Add(""); /* index 0 is never used by multiple choice questions */
            numerical = true;
         }
         
         if ((long)choices->GetCount() != num) { /* new choice has begun */
            delete choices;
            choices = new wxArrayString();
            choices->Add("", num); /* fill until this number */
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
      if (prompt = ((status == CS_PROMPT) && (questionStr != "$ "))) { // && (str.Find(": ") == str.Length())
         if (introStr.Last() == '\n') {
            introStr.RemoveLast();
         }
         if ((introStr != "") && (questionStr == "")) {
            questionStr = introStr;
            introStr = "";
         }
         
         if ((!choices) && (questionStr.Find("(yes/mod/no)") > -1)) {
            choices = new wxArrayString();
            choices->Add("yes");
            choices->Add("mod");
            choices->Add("no");
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
         questionStr = "";
         introStr = "";
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
