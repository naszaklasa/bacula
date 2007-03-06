/*
 *   attr.h Definition of attributes packet for unpacking from tape
 *
 *    Kern Sibbald, June MMIII
 *
 *   Version $Id: attr.h,v 1.4.4.1 2006/06/28 20:39:22 kerns Exp $
 */
/*
   Copyright (C) 2003-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

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
