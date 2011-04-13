/*
 *
 *    csprint header file, used by console_thread to send events back to the GUI.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

#ifndef CSPRINT_H
#define CSPRINT_H

#define CS_DATA          1 /* data has been received */
#define CS_END           2 /* no data to receive anymore */
#define CS_PROMPT        3 /* prompt signal received */
#define CS_CONNECTED     4 /* the socket is now connected */
#define CS_DISCONNECTED  5 /* the socket is now disconnected */

#define CS_REMOVEPROMPT  6 /* remove the prompt (#), when automatic messages are comming */

#define CS_DEBUG        10 /* used to print debug messages */
#define CS_TERMINATED   99 /* used to signal that the thread is terminated */

/* function called by console_thread to send events back to the GUI */
class wxString;

void csprint(const char* str, int status=CS_DATA);
void csprint(wxString str, int status=CS_DATA);

#endif
