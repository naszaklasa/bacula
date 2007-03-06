/*
 *   attr.h Definition of attributes packet for unpacking from tape
 * 
 *    Kern Sibbald, June MMIII
 *
 *   Version $Id: attr.h,v 1.3 2003/07/30 09:58:01 kerns Exp $
 */

/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

struct ATTR {
   int32_t stream;                    /* attribute stream id */
   int32_t data_stream;               /* id of data stream to follow */
   int32_t type;                      /* file type FT */
   int32_t file_index;                /* file index */
   int32_t LinkFI;                    /* file index to data if hard link */
   struct stat statp;                 /* decoded stat packet */
   POOLMEM *attrEx;                   /* extended attributes if any */
   POOLMEM *ofname;                   /* output filename */
   POOLMEM *olname;                   /* output link name */
   /*
    * Note the following three variables point into the 
    *  current BSOCK record, so they are invalid after    
    *  the next socket read!
    */
   char *attr;                        /* attributes position */
   char *fname;                       /* filename */
   char *lname;                       /* link name if any */
};
