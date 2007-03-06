/*
 *
 *    csprint header file, used by console_thread to send events back to the GUI.
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id: csprint.h,v 1.6 2004/07/18 09:33:26 kerns Exp $
 */
/*
   Copyright (C) 2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#ifndef CSPRINT_H
#define CSPRINT_H

#define CS_DATA          1 /* data has been received */
#define CS_END           2 /* no data to receive anymore */
#define CS_PROMPT        3 /* prompt signal received */
#define CS_CONNECTED     4 /* the socket is now connected */
#define CS_DISCONNECTED  5 /* the socket is now disconnected */
#define CS_DEBUG        10 /* used to print debug messages */
#define CS_TERMINATED   99 /* used to signal that the thread is terminated */

/* function called by console_thread to send events back to the GUI */
void csprint(const char* str, int status=CS_DATA);

#endif
