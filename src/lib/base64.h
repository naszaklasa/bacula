/*
 *   Generic base 64 input and output routines
 *
 *    Written by Kern E. Sibbald, March MM.
 *
 *   Version $Id: base64.h,v 1.2.2.2 2006/01/07 11:18:11 kerns Exp $
 */

/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

/* Maximum size of len bytes after base64 encoding */
#define BASE64_SIZE(len) (((len + 3 - (len % 3)) / 3) * 4)
