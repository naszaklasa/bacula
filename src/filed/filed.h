/*
 * Bacula File Daemon specific configuration and defines
 *
 *     Kern Sibbald, Jan MMI 
 *
 *   Version $Id: filed.h,v 1.9 2004/08/22 17:37:44 nboichat Exp $
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

#define FILE_DAEMON 1
#include "filed_conf.h"
#include "findlib/find.h"
#include "jcr.h"
#include "protos.h"                   /* file daemon prototypes */
#ifdef HAVE_LIBZ
#include <zlib.h>                     /* compression headers */
#else
#define uLongf uint32_t
#endif

extern const int win32_client;              /* Are we running on Windows? */

extern CLIENT *me;                    /* "Global" Client resource */
