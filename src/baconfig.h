/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * General header file configurations that apply to
 * all daemons.  System dependent stuff goes here.
 *
 *   Version $Id$
 */


#ifndef _BACONFIG_H
#define _BACONFIG_H 1

/* Bacula common configuration defines */

#undef  TRUE
#undef  FALSE
#define TRUE  1
#define FALSE 0

#ifdef HAVE_TLS
#define have_tls 1
#else
#define have_tls 0
#endif

#ifndef ETIME
#define ETIME ETIMEDOUT
#endif

#define ioctl_req_t long unsigned int

#ifdef PROTOTYPES
# define __PROTO(p)     p
#else
# define __PROTO(p)     ()
#endif

#ifdef DEBUG
#define ASSERT(x) if (!(x)) { \
   char *jcr = NULL; \
   Emsg1(M_ERROR, 0, _("Failed ASSERT: %s\n"), #x); \
   jcr[0] = 0; }
#else
#define ASSERT(x)
#endif

/* Allow printing of NULL pointers */
#define NPRT(x) (x)?(x):_("*None*")
#define NPRTB(x) (x)?(x):""

#if defined(HAVE_WIN32)

#define WIN32_REPARSE_POINT 1
#define WIN32_MOUNT_POINT   2

void InitWinAPIWrapper();

#define  OSDependentInit()    InitWinAPIWrapper()

#define sbrk(x)  0

#define clear_thread_id(x) memset(&(x), 0, sizeof(x))

#if defined(BUILDING_DLL)
#  define DLL_IMP_EXP   _declspec(dllexport)
#elif defined(USING_DLL)
#  define DLL_IMP_EXP   _declspec(dllimport)
#else
#  define DLL_IMP_EXP
#endif

#if defined(USING_CATS)
#  define CATS_IMP_EXP   _declspec(dllimport)
#else
#  define CATS_IMP_EXP
#endif

#else  /* HAVE_WIN32 */

#define clear_thread_id(x) x = 0

#define DLL_IMP_EXP
#define CATS_IMP_EXP

#define  OSDependentInit()

#endif /* HAVE_WIN32 */


#ifdef ENABLE_NLS
   #include <libintl.h>
   #include <locale.h>
   #ifndef _
      #define _(s) gettext((s))
   #endif /* _ */
   #ifndef N_
      #define N_(s) (s)
   #endif /* N_ */
#else /* !ENABLE_NLS */
   #undef _
   #undef N_
   #undef textdomain
   #undef bindtextdomain
   #undef setlocale

   #ifndef _
      #define _(s) (s)
   #endif
   #ifndef N_
      #define N_(s) (s)
   #endif
   #ifndef textdomain
      #define textdomain(d)
   #endif
   #ifndef bindtextdomain
      #define bindtextdomain(p, d)
   #endif
   #ifndef setlocale
      #define setlocale(p, d)
   #endif
#endif /* ENABLE_NLS */


/* Use the following for strings not to be translated */
#define NT_(s) (s)   

/* This should go away! ****FIXME***** */
#define MAXSTRING 500

/* Maximum length to edit time/date */
#define MAX_TIME_LENGTH 50

/* Maximum Name length including EOS */
#define MAX_NAME_LENGTH 128

/* Maximume number of user entered command args */
#define MAX_CMD_ARGS 30

/* All tape operations MUST be a multiple of this */
#define TAPE_BSIZE 1024

#ifdef DEV_BSIZE 
#define B_DEV_BSIZE DEV_BSIZE
#endif

#if !defined(B_DEV_BSIZE) & defined(BSIZE)
#define B_DEV_BSIZE BSIZE
#endif

#ifndef B_DEV_BSIZE
#define B_DEV_BSIZE 512
#endif

/*
 * Set to time limit for other end to respond to
 *  authentication.  Normally 10 minutes is *way*
 *  more than enough. The idea is to keep the Director
 *  from hanging because there is a dead connection on
 *  the other end.
 */
#define AUTH_TIMEOUT 60 * 10

/*
 * Default network buffer size
 */
#define DEFAULT_NETWORK_BUFFER_SIZE (64 * 1024)

/*
 * Stream definitions. Once defined these must NEVER
 *   change as they go on the storage media.
 * Note, the following streams are passed from the SD to the DIR
 *   so that they may be put into the catalog (actually only the
 *   stat packet part of the attr record is put in the catalog.
 *
 *   STREAM_UNIX_ATTRIBUTES
 *   STREAM_UNIX_ATTRIBUTES_EX
 *   STREAM_MD5_DIGEST
 *   STREAM_SHA1_DIGEST
 *   STREAM_SHA256_DIGEST
 *   STREAM_SHA512_DIGEST
 */
#define STREAM_NONE                         0    /* Reserved Non-Stream */
#define STREAM_UNIX_ATTRIBUTES              1    /* Generic Unix attributes */
#define STREAM_FILE_DATA                    2    /* Standard uncompressed data */
#define STREAM_MD5_SIGNATURE                3    /* deprecated */
#define STREAM_MD5_DIGEST                   3    /* MD5 digest for the file */
#define STREAM_GZIP_DATA                    4    /* GZip compressed file data */
#define STREAM_UNIX_ATTRIBUTES_EX           5    /* Extended Unix attr for Win32 EX - Deprecated */
#define STREAM_SPARSE_DATA                  6    /* Sparse data stream */
#define STREAM_SPARSE_GZIP_DATA             7    /* Sparse gzipped data stream */
#define STREAM_PROGRAM_NAMES                8    /* program names for program data */
#define STREAM_PROGRAM_DATA                 9    /* Data needing program */
#define STREAM_SHA1_SIGNATURE              10    /* deprecated */
#define STREAM_SHA1_DIGEST                 10    /* SHA1 digest for the file */
#define STREAM_WIN32_DATA                  11    /* Win32 BackupRead data */
#define STREAM_WIN32_GZIP_DATA             12    /* Gzipped Win32 BackupRead data */
#define STREAM_MACOS_FORK_DATA             13    /* Mac resource fork */
#define STREAM_HFSPLUS_ATTRIBUTES          14    /* Mac OS extra attributes */
#define STREAM_UNIX_ACCESS_ACL             15    /* Standard ACL attributes on UNIX - Deprecated */
#define STREAM_UNIX_DEFAULT_ACL            16    /* Default ACL attributes on UNIX - Deprecated */
#define STREAM_SHA256_DIGEST               17    /* SHA-256 digest for the file */
#define STREAM_SHA512_DIGEST               18    /* SHA-512 digest for the file */
#define STREAM_SIGNED_DIGEST               19    /* Signed File Digest, ASN.1, DER Encoded */
#define STREAM_ENCRYPTED_FILE_DATA         20    /* Encrypted, uncompressed data */
#define STREAM_ENCRYPTED_WIN32_DATA        21    /* Encrypted, uncompressed Win32 BackupRead data */
#define STREAM_ENCRYPTED_SESSION_DATA      22    /* Encrypted Session Data, ASN.1, DER Encoded */
#define STREAM_ENCRYPTED_FILE_GZIP_DATA    23    /* Encrypted, compressed data */
#define STREAM_ENCRYPTED_WIN32_GZIP_DATA   24    /* Encrypted, compressed Win32 BackupRead data */
#define STREAM_ENCRYPTED_MACOS_FORK_DATA   25    /* Encrypted, uncompressed Mac resource fork */
#define STREAM_PLUGIN_NAME                 26    /* Plugin "file" string */
#define STREAM_PLUGIN_DATA                 27    /* Plugin specific data */

/*
 * Additional Stream definitions. Once defined these must NEVER
 *   change as they go on the storage media.
 *
 * The Stream numbers from 1000-1999 are reserved for ACL and extended attribute streams.
 * Each different platform has its own stream id(s), if a platform supports multiple stream types
 * it should supply different handlers for each type it supports and this should be called
 * from the stream dispatch function. Currently in this reserved space we allocate the
 * different acl streams from 1000 on and the different extended attributes streams from
 * 1999 down. So the two naming spaces grows towards each other.
 */
#define STREAM_ACL_AIX_TEXT              1000    /* AIX specific string representation from acl_get */
#define STREAM_ACL_DARWIN_ACCESS_ACL     1001    /* Darwin (OSX) specific acl_t string representation
                                                  * from acl_to_text (POSIX acl)
                                                  */
#define STREAM_ACL_FREEBSD_DEFAULT_ACL   1002    /* FreeBSD specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for default acls.
                                                  */
#define STREAM_ACL_FREEBSD_ACCESS_ACL    1003    /* FreeBSD specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for access acls.
                                                  */
#define STREAM_ACL_HPUX_ACL_ENTRY        1004    /* HPUX specific acl_entry string representation
                                                  * from acltostr (POSIX acl)
                                                  */
#define STREAM_ACL_IRIX_DEFAULT_ACL      1005    /* IRIX specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for default acls.
                                                  */
#define STREAM_ACL_IRIX_ACCESS_ACL       1006    /* IRIX specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for access acls.
                                                  */
#define STREAM_ACL_LINUX_DEFAULT_ACL     1007    /* Linux specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for default acls.
                                                  */
#define STREAM_ACL_LINUX_ACCESS_ACL      1008    /* Linux specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for access acls.
                                                  */
#define STREAM_ACL_TRU64_DEFAULT_ACL     1009    /* Tru64 specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for default acls.
                                                  */
#define STREAM_ACL_TRU64_DEFAULT_DIR_ACL 1010    /* Tru64 specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for default acls.
                                                  */
#define STREAM_ACL_TRU64_ACCESS_ACL      1011    /* Tru64 specific acl_t string representation
                                                  * from acl_to_text (POSIX acl) for access acls.
                                                  */
#define STREAM_ACL_SOLARIS_ACLENT        1012    /* Solaris specific aclent_t string representation
                                                  * from acltotext or acl_totext (POSIX acl)
                                                  */
#define STREAM_ACL_SOLARIS_ACE           1013    /* Solaris specific ace_t string representation from
                                                  * from acl_totext (NFSv4 or ZFS acl)
                                                  */
#define STREAM_XATTR_SOLARIS_SYS         1994    /* Solaris specific extensible attributes or
                                                  * otherwise named extended system attributes.
                                                  */
#define STREAM_XATTR_SOLARIS             1995    /* Solaris specific extented attributes */
#define STREAM_XATTR_DARWIN              1996    /* Darwin (OSX) specific extended attributes */
#define STREAM_XATTR_FREEBSD             1997    /* FreeBSD specific extended attributes */
#define STREAM_XATTR_LINUX               1998    /* Linux specific extended attributes */
#define STREAM_XATTR_NETBSD              1999    /* NetBSD specific extended attributes */

/*
 *  File type (Bacula defined).
 *  NOTE!!! These are saved in the Attributes record on the tape, so
 *          do not change them. If need be, add to them.
 *
 *  This is stored as 32 bits on the Volume, but only FT_MASK (16) bits are
 *    used for the file type. The upper bits are used to indicate
 *    additional optional fields in the attribute record.
 */
#define FT_MASK       0xFFFF          /* Bits used by FT (type) */
#define FT_LNKSAVED   1               /* hard link to file already saved */
#define FT_REGE       2               /* Regular file but empty */
#define FT_REG        3               /* Regular file */
#define FT_LNK        4               /* Soft Link */
#define FT_DIREND     5               /* Directory at end (saved) */
#define FT_SPEC       6               /* Special file -- chr, blk, fifo, sock */
#define FT_NOACCESS   7               /* Not able to access */
#define FT_NOFOLLOW   8               /* Could not follow link */
#define FT_NOSTAT     9               /* Could not stat file */
#define FT_NOCHG     10               /* Incremental option, file not changed */
#define FT_DIRNOCHG  11               /* Incremental option, directory not changed */
#define FT_ISARCH    12               /* Trying to save archive file */
#define FT_NORECURSE 13               /* No recursion into directory */
#define FT_NOFSCHG   14               /* Different file system, prohibited */
#define FT_NOOPEN    15               /* Could not open directory */
#define FT_RAW       16               /* Raw block device */
#define FT_FIFO      17               /* Raw fifo device */
/* The DIRBEGIN packet is sent to the FD file processing routine so
 * that it can filter packets, but otherwise, it is not used
 * or saved */
#define FT_DIRBEGIN  18               /* Directory at beginning (not saved) */
#define FT_INVALIDFS 19               /* File system not allowed for */
#define FT_INVALIDDT 20               /* Drive type not allowed for */
#define FT_REPARSE   21               /* Win NTFS reparse point */
#define FT_PLUGIN    22               /* Plugin generated filename */
#define FT_DELETED   23               /* Deleted file entry */

/* Definitions for upper part of type word (see above). */
#define AR_DATA_STREAM (1<<16)        /* Data stream id present */

/*
 * Tape label types -- stored in catalog
 */
#define B_BACULA_LABEL 0
#define B_ANSI_LABEL   1
#define B_IBM_LABEL    2

/* Size of File Address stored in STREAM_SPARSE_DATA. Do NOT change! */
#define SPARSE_FADDR_SIZE (sizeof(uint64_t))

/* Size of crypto length stored at head of crypto buffer. Do NOT change! */
#define CRYPTO_LEN_SIZE ((int)sizeof(uint32_t))


/* This is for dumb compilers/libraries like Solaris. Linux GCC
 * does it correctly, so it might be worthwhile
 * to remove the isascii(c) with ifdefs on such
 * "smart" systems.
 */
#define B_ISSPACE(c) (isascii((int)(c)) && isspace((int)(c)))
#define B_ISALPHA(c) (isascii((int)(c)) && isalpha((int)(c)))
#define B_ISUPPER(c) (isascii((int)(c)) && isupper((int)(c)))
#define B_ISDIGIT(c) (isascii((int)(c)) && isdigit((int)(c)))

/* For multiplying by 10 with shift and addition */
#define B_TIMES10(d) ((d<<3)+(d<<1))


typedef void (HANDLER)();
typedef int (INTHANDLER)();

#ifdef SETPGRP_VOID
# define SETPGRP_ARGS(x, y) /* No arguments */
#else
# define SETPGRP_ARGS(x, y) (x, y)
#endif

#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFM) == S_IFLNK)
#endif

/* Added by KES to deal with Win32 systems */
#ifndef S_ISWIN32
#define S_ISWIN32 020000
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#ifndef MODE_RW
#define MODE_RW 0666
#endif

#if defined(HAVE_WIN32)
typedef int64_t   boffset_t;
#else
typedef off_t     boffset_t;
#endif

/* These probably should be subroutines */
#define Pw(x) \
   do { int errstat; if ((errstat=rwl_writelock(&(x)))) \
      e_msg(__FILE__, __LINE__, M_ABORT, 0, "Write lock lock failure. ERR=%s\n",\
           strerror(errstat)); \
   } while(0)

#define Vw(x) \
   do { int errstat; if ((errstat=rwl_writeunlock(&(x)))) \
         e_msg(__FILE__, __LINE__, M_ABORT, 0, "Write lock unlock failure. ERR=%s\n",\
           strerror(errstat)); \
   } while(0)

#define LockRes()   b_LockRes(__FILE__, __LINE__)
#define UnlockRes() b_UnlockRes(__FILE__, __LINE__)

#ifdef DEBUG_MEMSET
#define memset(a, v, n) b_memset(__FILE__, __LINE__, a, v, n)
void b_memset(const char *file, int line, void *mem, int val, size_t num);
#endif


/*
 * The digit following Dmsg and Emsg indicates the number of substitutions in
 * the message string. We need to do this kludge because non-GNU compilers
 * do not handle varargs #defines.
 */
/* Debug Messages that are printed */
#ifdef DEBUG
#define Dmsg0(lvl, msg)             if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg)
#define Dmsg1(lvl, msg, a1)         if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)     if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8) if ((lvl)<=debug_level) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg9(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9) if ((lvl)<=debug_level) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define Dmsg10(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) if ((lvl)<=debug_level) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) if ((lvl)<=debug_level) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Dmsg12(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) if ((lvl)<=debug_level) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#define Dmsg13(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) if ((lvl)<=debug_level) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)
#else
#define Dmsg0(lvl, msg)
#define Dmsg1(lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Dmsg12(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#define Dmsg13(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)
#endif /* DEBUG */

#ifdef TRACE_FILE
#define Tmsg0(lvl, msg)             t_msg(__FILE__, __LINE__, lvl, msg)
#define Tmsg1(lvl, msg, a1)         t_msg(__FILE__, __LINE__, lvl, msg, a1)
#define Tmsg2(lvl, msg, a1, a2)     t_msg(__FILE__, __LINE__, lvl, msg, a1, a2)
#define Tmsg3(lvl, msg, a1, a2, a3) t_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3)
#define Tmsg4(lvl, msg, arg1, arg2, arg3, arg4) t_msg(__FILE__, __LINE__, lvl, msg, arg1, arg2, arg3, arg4)
#define Tmsg5(lvl, msg, a1, a2, a3, a4, a5) t_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5)
#define Tmsg6(lvl, msg, a1, a2, a3, a4, a5, a6) t_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6)
#define Tmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7) t_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Tmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8) t_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Tmsg9(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9) t_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define Tmsg10(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) t_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#define Tmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) t_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Tmsg12(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) t_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#define Tmsg13(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) t_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)
#else
#define Tmsg0(lvl, msg)
#define Tmsg1(lvl, msg, a1)
#define Tmsg2(lvl, msg, a1, a2)
#define Tmsg3(lvl, msg, a1, a2, a3)
#define Tmsg4(lvl, msg, arg1, arg2, arg3, arg4)
#define Tmsg5(lvl, msg, a1, a2, a3, a4, a5)
#define Tmsg6(lvl, msg, a1, a2, a3, a4, a5, a6)
#define Tmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Tmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Tmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Tmsg12(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#define Tmsg13(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)
#endif /* TRACE_FILE */



/* Messages that are printed (uses d_msg) */
#define Pmsg0(lvl, msg)             p_msg(__FILE__, __LINE__, lvl, msg)
#define Pmsg1(lvl, msg, a1)         p_msg(__FILE__, __LINE__, lvl, msg, a1)
#define Pmsg2(lvl, msg, a1, a2)     p_msg(__FILE__, __LINE__, lvl, msg, a1, a2)
#define Pmsg3(lvl, msg, a1, a2, a3) p_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3)
#define Pmsg4(lvl, msg, arg1, arg2, arg3, arg4) p_msg(__FILE__, __LINE__, lvl, msg, arg1, arg2, arg3, arg4)
#define Pmsg5(lvl, msg, a1, a2, a3, a4, a5) p_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5)
#define Pmsg6(lvl, msg, a1, a2, a3, a4, a5, a6) p_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6)
#define Pmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7) p_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Pmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8) p_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Pmsg9(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define Pmsg10(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#define Pmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Pmsg12(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#define Pmsg13(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)
#define Pmsg14(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) p_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14)


/* Daemon Error Messages that are delivered according to the message resource */
#define Emsg0(typ, lvl, msg)             e_msg(__FILE__, __LINE__, typ, lvl, msg)
#define Emsg1(typ, lvl, msg, a1)         e_msg(__FILE__, __LINE__, typ, lvl, msg, a1)
#define Emsg2(typ, lvl, msg, a1, a2)     e_msg(__FILE__, __LINE__, typ, lvl, msg, a1, a2)
#define Emsg3(typ, lvl, msg, a1, a2, a3) e_msg(__FILE__, __LINE__, typ, lvl, msg, a1, a2, a3)
#define Emsg4(typ, lvl, msg, a1, a2, a3, a4) e_msg(__FILE__, __LINE__, typ, lvl, msg, a1, a2, a3, a4)
#define Emsg5(typ, lvl, msg, a1, a2, a3, a4, a5) e_msg(__FILE__, __LINE__, typ, lvl, msg, a1, a2, a3, a4, a5)
#define Emsg6(typ, lvl, msg, a1, a2, a3, a4, a5, a6) e_msg(__FILE__, __LINE__, typ, lvl, msg, a1, a2, a3, a4, a5, a6)

/* Job Error Messages that are delivered according to the message resource */
#define Jmsg0(jcr, typ, lvl, msg)             j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg)
#define Jmsg1(jcr, typ, lvl, msg, a1)         j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1)
#define Jmsg2(jcr, typ, lvl, msg, a1, a2)     j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2)
#define Jmsg3(jcr, typ, lvl, msg, a1, a2, a3) j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3)
#define Jmsg4(jcr, typ, lvl, msg, a1, a2, a3, a4) j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4)
#define Jmsg5(jcr, typ, lvl, msg, a1, a2, a3, a4, a5) j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4, a5)
#define Jmsg6(jcr, typ, lvl, msg, a1, a2, a3, a4, a5, a6) j_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4, a5, a6)

/* Queued Job Error Messages that are delivered according to the message resource */
#define Qmsg0(jcr, typ, lvl, msg)             q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg)
#define Qmsg1(jcr, typ, lvl, msg, a1)         q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1)
#define Qmsg2(jcr, typ, lvl, msg, a1, a2)     q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2)
#define Qmsg3(jcr, typ, lvl, msg, a1, a2, a3) q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3)
#define Qmsg4(jcr, typ, lvl, msg, a1, a2, a3, a4) q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4)
#define Qmsg5(jcr, typ, lvl, msg, a1, a2, a3, a4, a5) q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4, a5)
#define Qmsg6(jcr, typ, lvl, msg, a1, a2, a3, a4, a5, a6) q_msg(__FILE__, __LINE__, jcr, typ, lvl, msg, a1, a2, a3, a4, a5, a6)


/* Memory Messages that are edited into a Pool Memory buffer */
#define Mmsg0(buf, msg)             m_msg(__FILE__, __LINE__, buf, msg)
#define Mmsg1(buf, msg, a1)         m_msg(__FILE__, __LINE__, buf, msg, a1)
#define Mmsg2(buf, msg, a1, a2)     m_msg(__FILE__, __LINE__, buf, msg, a1, a2)
#define Mmsg3(buf, msg, a1, a2, a3) m_msg(__FILE__, __LINE__, buf, msg, a1, a2, a3)
#define Mmsg4(buf, msg, a1, a2, a3, a4) m_msg(__FILE__, __LINE__, buf, msg, a1, a2, a3, a4)
#define Mmsg5(buf, msg, a1, a2, a3, a4, a5) m_msg(__FILE__, __LINE__, buf, msg, a1, a2, a3, a4, a5)
#define Mmsg6(buf, msg, a1, a2, a3, a4, a5, a6) m_msg(__FILE__, __LINE__, buf, msg, a1, a2, a3, a4, a5, a6)
#define Mmsg7(buf, msg, a1, a2, a3, a4, a5, a6, a7) m_msg(__FILE__, __LINE__, buf, msg, a1, a2, a3, a4, a5, a6)
#define Mmsg8(buf,msg,a1,a2,a3,a4,a5,a6,a7,a8) m_msg(__FILE__,__LINE__,buf,msg,a1,a2,a3,a4,a5,a6)
#define Mmsg11(buf,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) m_msg(__FILE__,__LINE__,buf,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#define Mmsg15(buf,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) m_msg(__FILE__,__LINE__,buf,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)

class POOL_MEM;
/* Edit message into Pool Memory buffer -- no __FILE__ and __LINE__ */
int  Mmsg(POOLMEM **msgbuf, const char *fmt,...);
int  Mmsg(POOLMEM *&msgbuf, const char *fmt,...);
int  Mmsg(POOL_MEM &msgbuf, const char *fmt,...);


class JCR;
void d_msg(const char *file, int line, int level, const char *fmt,...);
void p_msg(const char *file, int line, int level, const char *fmt,...);
void e_msg(const char *file, int line, int type, int level, const char *fmt,...);
void j_msg(const char *file, int line, JCR *jcr, int type, utime_t mtime, const char *fmt,...);
void q_msg(const char *file, int line, JCR *jcr, int type, utime_t mtime, const char *fmt,...);
int  m_msg(const char *file, int line, POOLMEM **msgbuf, const char *fmt,...);
int  m_msg(const char *file, int line, POOLMEM *&pool_buf, const char *fmt, ...);


/* Use our strdup with smartalloc */
#ifndef HAVE_WXCONSOLE
#undef strdup
#define strdup(buf) bad_call_on_strdup_use_bstrdup(buf)
#else 
/* Groan, WxWidgets has its own way of doing NLS so cleanup */
#ifndef ENABLE_NLS
#undef _
#undef setlocale
#undef textdomain
#undef bindtextdomain
#endif  
#endif

/* Use our fgets which handles interrupts */
#undef fgets
#define fgets(x,y,z) bfgets((x), (y), (z))

/* Use our sscanf, which is safer and works with known sizes */
#define sscanf bsscanf

#ifdef DEBUG
#define bstrdup(str) strcpy((char *)b_malloc(__FILE__,__LINE__,strlen((str))+1),(str))
#else
#define bstrdup(str) strcpy((char *)bmalloc(strlen((str))+1),(str))
#endif

#ifdef DEBUG
#define bmalloc(size) b_malloc(__FILE__, __LINE__, (size))
#endif

/*
 * Replace codes needed in both file routines and non-file routines
 * Job replace codes -- in "replace"
 */
#define REPLACE_ALWAYS   'a'
#define REPLACE_IFNEWER  'w'
#define REPLACE_NEVER    'n'
#define REPLACE_IFOLDER  'o'

/* This probably should be done on a machine by machine basis, but it works */
/* This is critical for the smartalloc routines to properly align memory */
#define ALIGN_SIZE (sizeof(double))
#define BALIGN(x) (((x) + ALIGN_SIZE - 1) & ~(ALIGN_SIZE -1))


/* =============================================================
 *               OS Dependent defines
 * ============================================================= 
 */

#ifndef HAVE_FSEEKO
/* Bad news. This OS cannot handle 64 bit fseeks and ftells */
#define fseeko fseek
#define ftello ftell
#endif

#if defined (__digital__) && defined (__unix__)
/* Tru64 - it does have fseeko and ftello , but since ftell/fseek are also 64 bit */
/* take this 'shortcut' */
#define fseeko fseek
#define ftello ftell
#undef  ioctl_req_t
#define ioctl_req_t int
#endif


#ifdef HAVE_OSF1_OS
#undef  ioctl_req_t
#define ioctl_req_t int
#endif

#ifdef HAVE_SUN_OS
   /*
    * On Solaris 2.5, threads are not timesliced by default, so we need to
    * explictly increase the conncurrency level.
    */
#include <thread.h>
#define set_thread_concurrency(x)  thr_setconcurrency(x)
extern int thr_setconcurrency(int);
#define SunOS 1
#undef  ioctl_req_t
#define ioctl_req_t int

#else


/* Not needed on most systems */
#define set_thread_concurrency(x)

#endif

#if defined(HAVE_DARWIN_OS) || defined(HAVE_OSF1_OS)
/* Apparently someone forgot to wrap getdomainname as a C function */
extern "C" int getdomainname(char *name, int len);
#endif



#if defined(HAVE_WIN32)
#define DEFAULT_CONFIGDIR "C:\\Documents and Settings\\All Users\\Application Data\\Bacula"
#define PathSeparator '\\'

inline bool IsPathSeparator(int ch) { return ch == '/' || ch == '\\'; }
inline char *first_path_separator(char *path) { return strpbrk(path, "/\\"); }
inline const char *first_path_separator(const char *path) { return strpbrk(path, "/\\"); }

extern void pause_msg(const char *file, const char *func, int line, const char *msg);
#define pause(msg) if (debug_level) pause_msg(__FILE__, __func__, __LINE__, (msg))

#else
#define PathSeparator '/'
/* Define Winsock functions if we aren't on Windows */

#define WSA_Init() 0 /* 0 = success */
#define WSACleanup() 0 /* 0 = success */

inline bool IsPathSeparator(int ch) { return ch == '/'; }
inline char *first_path_separator(char *path) { return strchr(path, '/'); }
inline const char *first_path_separator(const char *path) { return strchr(path, '/'); }
#define pause(msg)
#endif


/* HP-UX 11 specific workarounds */

#ifdef HAVE_HPUX_OS
# undef h_errno
extern int h_errno;
/* the {get,set}domainname() functions exist in HPUX's libc.
 * the configure script detects that correctly.
 * the problem is no system headers declares the prototypes for these functions
 * this is done below
 */
extern "C" int getdomainname(char *name, int namelen);
extern "C" int setdomainname(char *name, int namelen);
#undef  ioctl_req_t
#define ioctl_req_t int
#endif /* HAVE_HPUX_OS */


#ifdef HAVE_OSF1_OS
extern "C" int fchdir(int filedes);
extern "C" long gethostid(void);
extern "C" int mknod ( const char *path, int mode, dev_t device );
#undef  ioctl_req_t
#define ioctl_req_t int
#endif


/* Disabled because it breaks internationalisation...
#undef HAVE_SETLOCALE
#ifdef HAVE_SETLOCALE
#include <locale.h>
#else
#define setlocale(x, y) ("ANSI_X3.4-1968")
#endif
#ifdef HAVE_NL_LANGINFO
#include <langinfo.h>
#else
#define nl_langinfo(x) ("ANSI_X3.4-1968")
#endif
*/

#endif /* _BACONFIG_H */
