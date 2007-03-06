/*
 *  Version $Id: version.h,v 1.554.2.54 2006/06/08 14:46:46 kerns Exp $
 */

#undef  VERSION
#define VERSION "1.38.10"
#define BDATE   "08 June 2006"
#define LSMDATE "08Jun06"

/* Debug flags */
#undef  DEBUG
#define DEBUG 1
#define TRACEBACK 1
#define SMCHECK
#define TRACE_FILE 1

/* If this is set stdout will not be closed on startup */
/* #define DEVELOPER 1 */

/* #define USE_BSNPRINTF */

/* Debug flags not normally turned on */

/* #define FILE_SEEK 1 */  

/* #define TRACE_JCR_CHAIN 1 */
/* #define TRACE_RES 1 */
/* #define DEBUG_MEMSET 1 */
/* #define DEBUG_MUTEX 1 */

/* Check if header of tape block is zero before writing */
#define DEBUG_BLOCK_ZEROING 1

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
