/*
 *
 *    Interaction thread between director and the GUI
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: console_thread.cpp,v 1.22 2004/11/15 16:49:53 nboichat Exp $
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

// http://66.102.9.104/search?q=cache:Djc1mPF3hRoJ:cvs.sourceforge.net/viewcvs.py/audacity/audacity-src/src/AudioIO.cpp%3Frev%3D1.102+macos+x+wxthread&hl=fr

#include "console_thread.h" // class's header file

#include <wx/wxprec.h>

#include <wx/thread.h>
#include <wx/file.h>
#include <bacula.h>
#include <jcr.h>

#include "console_conf.h"

#include "csprint.h"

#ifdef HAVE_WIN32
#include <windows.h>
DWORD  g_platform_id = VER_PLATFORM_WIN32_WINDOWS;
char OK_msg[]   = "2000 OK\n";
char TERM_msg[] = "2999 Terminate\n";
#endif

/* Imported functions */
int authenticate_director(JCR *jcr, DIRRES *director, CONRES *cons);

bool console_thread::inited = false;
bool console_thread::configloaded = false;
wxString console_thread::working_dir = ".";

void console_thread::SetWorkingDirectory(wxString w_dir) {
   if ((w_dir.Last() == '/') || (w_dir.Last() == '\\')) {
      console_thread::working_dir = w_dir.Mid(0, w_dir.Length()-1);
   }
   else {
      console_thread::working_dir = w_dir;
   }
}

void console_thread::InitLib() {
   if (WSA_Init() != 0) {
      csprint("Error while initializing windows sockets...\n");
      inited = false;
      return;
   }
   
   init_stack_dump();
   my_name_is(0, NULL, "wx-console");
   //textdomain("bacula-console");
   working_directory = console_thread::working_dir;
   
   inited = true;
}

void console_thread::FreeLib() {
   if (inited) {
      if (WSACleanup() != 0) {
         csprint("Error while cleaning up windows sockets...\n");
      }
   }
}

wxString console_thread::LoadConfig(wxString configfile) {
   if (!inited) {
      InitLib();
      if (!inited)
         return "Error while initializing library.";
   }
   
   free_config_resources();
   
   MSGS* msgs = (MSGS *)malloc(sizeof(MSGS));
   memset(msgs, 0, sizeof(MSGS));
   for (int i=1; i<=M_MAX; i++) {
#ifndef WIN32
      add_msg_dest(msgs, MD_STDOUT, i, NULL, NULL);
#endif
      add_msg_dest(msgs, MD_SYSLOG, i, NULL, NULL);
      add_msg_dest(msgs, MD_CONSOLE, i, NULL, NULL);
   }
   
   init_msg(NULL, msgs);
   init_console_msg(console_thread::working_dir);

   if (!parse_config(configfile.c_str(), 0)) {
      configloaded = false;
      wxFile file(console_thread::working_dir + "/wx-console.conmsg");
      if (!file.IsOpened())
         return "Unable to retrieve error message.";
      wxString err = "";
      wxChar buffer[513];
      off_t len;
      while ((len = file.Read(buffer, 512)) > -1) {
         buffer[len] = (wxChar)0;
         err += buffer;
         if (file.Eof())
            break;
      }
      file.Close();
      term_msg();
      wxRemoveFile(console_thread::working_dir + "/wx-console.conmsg");
      return err;
   }
   
   term_msg();
   wxRemoveFile(console_thread::working_dir + "/wx-console.conmsg");
   init_msg(NULL, NULL);
   
   configloaded = true;
   
   return "";
}

// class constructor
console_thread::console_thread() {
   UA_sock = NULL;
   choosingdirector = false;
}

// class destructor
console_thread::~console_thread() {
   if (UA_sock) {
      bnet_sig(UA_sock, BNET_TERMINATE); /* send EOF */
      bnet_close(UA_sock);
      UA_sock = NULL;
   }
}

/*
 * Thread entry point
 */
void* console_thread::Entry() {
   if (!inited) {
      csprint("Error : Library not initialized\n");
      csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);
      #ifdef HAVE_WIN32
         Exit();
      #endif
      return NULL;
   }
   
   if (!configloaded) {
      csprint("Error : No configuration file loaded\n");
      csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);
      #ifdef HAVE_WIN32
         Exit();
      #endif
      return NULL;
   }
   
   csprint("Connecting...\n");
  
   int count = 0;
   DIRRES* res[16]; /* Maximum 16 directors */
   
   LockRes();
   DIRRES* dir;
   foreach_res(dir, R_DIRECTOR) {
      res[count] = dir;
      count++;
      if (count == 16) {
         break;
      }
   }
   UnlockRes();
   
   if (count == 0) {
      csprint("Error : No director defined in config file.\n");
      csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);
      #ifdef HAVE_WIN32
         Exit();
      #endif
      return NULL;
   }
   else if (count == 1) {
      directorchoosen = 1;
   }
   else {
      while (true) {
         csprint("Multiple directors found in your config file.\n");
         for (int i = 0; i < count; i++) {
            if (i < 9) {
               csprint(wxString("    ") << (i+1) << ": " << res[i]->hdr.name << "\n");
            }
            else {
               csprint(wxString("   ") <<  (i+1) << ": " << res[i]->hdr.name << "\n");
            }
         }
         csprint(wxString("Please choose a director (1-") << count << ") : ");
         csprint(NULL, CS_PROMPT);
         choosingdirector = true;
         directorchoosen = -1;
         while(directorchoosen == -1) {
            bmicrosleep(0, 2000);
            Yield();
         }      
         choosingdirector = false;
         if (directorchoosen != 0) {
            break;
         }
      }
   }

   memset(&jcr, 0, sizeof(jcr));
   
   jcr.dequeuing = 1; /* TODO: catch messages */

   UA_sock = bnet_connect(&jcr, 3, 3, "Director daemon",
      res[directorchoosen-1]->address, NULL, res[directorchoosen-1]->DIRport, 0);
      
   if (UA_sock == NULL) {
      csprint("Failed to connect to the director\n");
      csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);
      #ifdef HAVE_WIN32
         Exit();
      #endif
      return NULL;
   }

   csprint("Connected\n");

   jcr.dir_bsock = UA_sock;
   LockRes();
   /* If cons==NULL, default console will be used */
   CONRES *cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)NULL);
   UnlockRes();
   if (!authenticate_director(&jcr, res[directorchoosen-1], cons)) {
      csprint("ERR=");
      csprint(UA_sock->msg);
      csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);
      #ifdef HAVE_WIN32
         Exit();
      #endif
      return NULL;
   }
   
   csprint(NULL, CS_CONNECTED);
   
   Write(".messages\n");

   int stat;

   /* main loop */
   while(!TestDestroy()) {   /* Tests if thread has been ended */
      if ((stat = bnet_recv(UA_sock)) >= 0) {
         csprint(UA_sock->msg);
      }
      else if (stat == BNET_SIGNAL) {
         if (UA_sock->msglen == BNET_PROMPT) {
            csprint(NULL, CS_PROMPT);
         }
         else if (UA_sock->msglen == BNET_EOD) {
            csprint(NULL, CS_END);
         }
         else if (UA_sock->msglen == BNET_HEARTBEAT) {
            bnet_sig(UA_sock, BNET_HB_RESPONSE);
            csprint("<< Heartbeat signal received, answered. >>\n", CS_DEBUG);
         }
         else {
            csprint("<< Unexpected signal received : ", CS_DEBUG);
            csprint(bnet_sig_to_ascii(UA_sock), CS_DEBUG);
            csprint(">>\n", CS_DEBUG);
         }
      }
      else { /* BNET_HARDEOF || BNET_ERROR */
         csprint(NULL, CS_END);
         break;
      }
           
      if (is_bnet_stop(UA_sock)) {
         csprint(NULL, CS_END);
         break;            /* error or term */
      }
   }
   
   csprint(NULL, CS_DISCONNECTED);

   csprint("Connection terminated\n");
   
   UA_sock = NULL;

   csprint(NULL, CS_TERMINATED);

   #ifdef HAVE_WIN32
      Exit();
   #endif
   
   return NULL;
}

void console_thread::Write(const char* str) {
   if (UA_sock) {
       UA_sock->msglen = strlen(str);
       pm_strcpy(&UA_sock->msg, str);
       bnet_send(UA_sock);
   }
   else if (choosingdirector) {
      wxString number = str;
      number.RemoveLast(); /* Removes \n */
      long val;
      if (number.ToLong(&val)) {
         directorchoosen = (int)val;
      }
      else {
         directorchoosen = 0;
      }
   }
}

void console_thread::Delete() {
   Write("quit\n");
   if (UA_sock) {
      bnet_sig(UA_sock, BNET_TERMINATE); /* send EOF */
      bnet_close(UA_sock);
      UA_sock = NULL;
      /*csprint(NULL, CS_END);
      csprint(NULL, CS_DISCONNECTED);
      csprint(NULL, CS_TERMINATED);*/
   }
}
