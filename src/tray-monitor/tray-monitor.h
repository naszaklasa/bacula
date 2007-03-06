/*
 * Includes specific to the tray monitor
 *
 *     Nicolas Boichat, August MMIV
 *
 *    Version $Id: tray-monitor.h,v 1.6.6.1 2005/04/12 21:31:23 kerns Exp $
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

#include <gtk/gtk.h>

#include "tray_conf.h"

#include "jcr.h"

enum stateenum {
   idle = 0,
   running = 1,
   warn = 2,
   error = 3
};

struct monitoritem {
   rescode type; /* R_DIRECTOR, R_CLIENT or R_STORAGE */
   void* resource; /* DIRRES*, CLIENT* or STORE* */
   BSOCK *D_sock;
   stateenum state;
   stateenum oldstate;
   GtkWidget* image;
   GtkWidget* label;
};
