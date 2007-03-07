/*
 * BootStrap record definition -- for restoring files.
 *
 *    Kern Sibbald, June 2002
 *
 *   Version $Id: bsr.h 3673 2006-11-21 17:03:47Z kerns $
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/


#ifndef __BSR_H
#define __BSR_H 1

/*
 * List of Volume names to be read by Storage daemon.
 *  Formed by Storage daemon from BSR
 */
struct VOL_LIST {
   VOL_LIST *next;
   char VolumeName[MAX_NAME_LENGTH];
   char MediaType[MAX_NAME_LENGTH];
   char device[MAX_NAME_LENGTH];   /* ***FIXME*** use alist here */
   int Slot;
   uint32_t start_file;
};


/*
 * !!!!!!!!!!!!!!!!!! NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!                                               !!!
 * !!!   All records must have a pointer to          !!!
 * !!!   the next item as the first item defined.    !!!
 * !!!                                               !!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

struct BSR_VOLUME {
   BSR_VOLUME *next;
   char VolumeName[MAX_NAME_LENGTH];
   char MediaType[MAX_NAME_LENGTH];
   char device[MAX_NAME_LENGTH];   /* ***FIXME*** use alist here */
   int32_t       Slot;                /* Slot */
};

struct BSR_CLIENT {
   BSR_CLIENT *next;
   char ClientName[MAX_NAME_LENGTH];
};

struct BSR_SESSID {
   BSR_SESSID *next;
   uint32_t sessid;
   uint32_t sessid2;
};

struct BSR_SESSTIME {
   BSR_SESSTIME *next;
   uint32_t sesstime;
   bool done;                         /* local done */
};

struct BSR_VOLFILE {
   BSR_VOLFILE *next;
   uint32_t sfile;                    /* start file */
   uint32_t efile;                    /* end file */
   bool done;                         /* local done */
};

struct BSR_VOLBLOCK {
   BSR_VOLBLOCK *next;
   uint32_t sblock;                   /* start block */
   uint32_t eblock;                   /* end block */
   bool done;                         /* local done */
};


struct BSR_FINDEX {
   BSR_FINDEX *next;
   int32_t findex;                    /* start file index */
   int32_t findex2;                   /* end file index */
   bool done;                         /* local done */
};

struct BSR_JOBID {
   BSR_JOBID *next;
   uint32_t JobId;
   uint32_t JobId2;
};

struct BSR_JOBTYPE {
   BSR_JOBTYPE *next;
   uint32_t JobType;
};

struct BSR_JOBLEVEL {
   BSR_JOBLEVEL *next;
   uint32_t JobLevel;
};

struct BSR_JOB {
   BSR_JOB *next;
   char Job[MAX_NAME_LENGTH];
   bool done;                         /* local done */
};

struct BSR_STREAM {
   BSR_STREAM *next;
   int32_t stream;                    /* stream desired */
};

struct BSR {
   BSR          *next;                /* pointer to next one */
   BSR          *root;                /* root bsr */
   bool          reposition;          /* set when any bsr is marked done */
   bool          mount_next_volume;   /* set when next volume should be mounted */
   bool          done;                /* set when everything found for this bsr */
   bool          use_fast_rejection;  /* set if fast rejection can be used */
   bool          use_positioning;     /* set if we can position the archive */
   BSR_VOLUME   *volume;
   uint32_t      count;               /* count of files to restore this bsr */
   uint32_t      found;               /* count of restored files this bsr */
   BSR_VOLFILE  *volfile;
   BSR_VOLBLOCK *volblock;
   BSR_SESSTIME *sesstime;
   BSR_SESSID   *sessid;
   BSR_JOBID    *JobId;
   BSR_JOB      *job;
   BSR_CLIENT   *client;
   BSR_FINDEX   *FileIndex;
   BSR_JOBTYPE  *JobType;
   BSR_JOBLEVEL *JobLevel;
   BSR_STREAM   *stream;
};


#endif
