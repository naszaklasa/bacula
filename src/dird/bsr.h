/*
 *
 *   Bootstrap Record header file
 *
 *	BSR (bootstrap record) handling routines split from 
 *	  ua_restore.c July MMIII
 *
 *     Kern Sibbald, July MMII
 *
 *   Version $Id: bsr.h,v 1.2 2004/04/19 14:27:00 kerns Exp $
 */

/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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
   RBSR *next;			      /* next JobId */
   uint32_t JobId;		      /* JobId this bsr */
   uint32_t VolSessionId;		    
   uint32_t VolSessionTime;
   int	    VolCount;		      /* Volume parameter count */
   VOL_PARAMS *VolParams;	      /* Volume, start/end file/blocks */
   RBSR_FINDEX *fi;		      /* File indexes this JobId */
};
