/*
 *
 *   Bootstrap Record header file
 *
 *      BSR (bootstrap record) handling routines split from
 *        ua_restore.c July MMIII
 *
 *     Kern Sibbald, July MMII
 *
 *   Version $Id: bsr.h,v 1.5 2005/09/04 15:47:12 kerns Exp $
 */

/*
   Copyright (C) 2002-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */


/* FileIndex entry in restore bootstrap record */
struct RBSR_FINDEX {
   RBSR_FINDEX *next;
   int32_t findex;
   int32_t findex2;
};

/*
 * Restore bootstrap record -- not the real one, but useful here
 *  The restore bsr is a chain of BSR records (linked by next).
 *  Each BSR represents a single JobId, and within it, it
 *    contains a linked list of file indexes for that JobId.
 *    The complete_bsr() routine, will then add all the volumes
 *    on which the Job is stored to the BSR.
 */
struct RBSR {
   RBSR *next;                        /* next JobId */
   JobId_t JobId;                     /* JobId this bsr */
   uint32_t VolSessionId;
   uint32_t VolSessionTime;
   int      VolCount;                 /* Volume parameter count */
   VOL_PARAMS *VolParams;             /* Volume, start/end file/blocks */
   RBSR_FINDEX *fi;                   /* File indexes this JobId */
};
