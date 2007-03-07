/*
 *  Version $Id: version.h 4325 2007-03-06 20:02:31Z kerns $
 */

#undef  VERSION
#define VERSION "2.0.3"
#define BDATE   "06 March 2007"
#define LSMDATE "06Mar07"

#define PROG_COPYRIGHT "Copyright (C) %d-2007 Free Software Foundation Europe e.V.\n"
#define BYEAR "2007"       /* year for copyright messages in progs */

/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

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


/* Debug flags */
#undef  DEBUG
#define DEBUG 1
#define TRACEBACK 1
#define SMCHECK
#define TRACE_FILE 1

/* If this is set stdout will not be closed on startup */
/* #define DEVELOPER 1 */

#define DATA_ENCRYPTION 1

#define USE_BSNPRINTF 1

/* Turn on the following flag to enable batch attribute inserts
 *  in the catalog.  This gives a large speedup.
 */
/* #define HAVE_BATCH_FILE_INSERT 1 */


/* Debug flags not normally turned on */

/* #define TRACE_JCR_CHAIN 1 */
/* #define TRACE_RES 1 */
/* #define DEBUG_MEMSET 1 */
/* #define DEBUG_MUTEX 1 */

/*
 * Set SMALLOC_SANITY_CHECK to zero to turn off, otherwise
 *  it is the maximum memory malloced before Bacula will
 *  abort.  Except for debug situations, this should be zero
 */
#define SMALLOC_SANITY_CHECK 0  /* 500000000  0.5 GB max */


/* Check if header of tape block is zero before writing */
/* #define DEBUG_BLOCK_ZEROING 1 */

/* #define FULL_DEBUG 1 */   /* normally on for testing only */

/* Turn this on ONLY if you want all Dmsg() to append to the
 *   trace file. Implemented mainly for Win32 ...
 */
/*  #define SEND_DMSG_TO_FILE 1 */


/* The following are turned on for performance testing */
/*  
 * If you turn on the NO_ATTRIBUTES_TEST and rebuild, the SD
 *  will receive the attributes from the FD, will write them
 *  to disk, then when the data is written to tape, it will
 *  read back the attributes, but they will not be sent to
 *  the Director. So this will eliminate: 1. the comm time
 *  to send the attributes to the Director. 2. the time it
 *  takes the Director to put them in the catalog database.
 */
/* #define NO_ATTRIBUTES_TEST 1 */

/* 
* If you turn on NO_TAPE_WRITE_TEST and rebuild, the SD
*  will do all normal actions, but will not write to the
*  Volume.  Note, this means a lot of functions such as
*  labeling will not work, so you must use it only when 
*  Bacula is going to append to a Volume. This will eliminate
*  the time it takes to write to the Volume (not the time
*  it takes to do any positioning).
*/
/* #define NO_TAPE_WRITE_TEST 1 */

/*
 * If you turn on FD_NO_SEND_TEST and rebuild, the FD will
 *  not send any attributes or data to the SD. This will
 *  eliminate the comm time sending to the SD.
 */
/* #define FD_NO_SEND_TEST 1 */
