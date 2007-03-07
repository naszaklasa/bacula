/*
 *  Bacula low level File I/O routines.  This routine simulates
 *    open(), read(), write(), and close(), but using native routines.
 *    I.e. on Windows, we use Windows APIs.
 *
 *    Kern Sibbald, April MMIII
 *
 *   Version $Id: bfile.c 3676 2006-11-21 20:14:47Z kerns $
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2006 Free Software Foundation Europe e.V.

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

#include "bacula.h"
#include "find.h"

bool    (*python_set_prog)(JCR *jcr, const char *prog) = NULL;
int     (*python_open)(BFILE *bfd, const char *fname, int flags, mode_t mode) = NULL;
int     (*python_close)(BFILE *bfd) = NULL;
ssize_t (*python_read)(BFILE *bfd, void *buf, size_t count) = NULL;
ssize_t (*python_write)(BFILE *bfd, void *buf, size_t count) = NULL;

#ifdef HAVE_DARWIN_OS
#include <sys/paths.h>
#endif

/* ===============================================================
 *
 *            U N I X   AND   W I N D O W S
 *
 * ===============================================================
 */

bool is_win32_stream(int stream)
{
   switch (stream) {
   case STREAM_WIN32_DATA:
   case STREAM_WIN32_GZIP_DATA:
   case STREAM_ENCRYPTED_WIN32_DATA:
   case STREAM_ENCRYPTED_WIN32_GZIP_DATA:
      return true;
   }
   return false;
}

const char *stream_to_ascii(int stream)
{
   static char buf[20];

   switch (stream) {
   case STREAM_GZIP_DATA:
      return _("GZIP data");
   case STREAM_SPARSE_GZIP_DATA:
      return _("GZIP sparse data");
   case STREAM_WIN32_DATA:
      return _("Win32 data");
   case STREAM_WIN32_GZIP_DATA:
      return _("Win32 GZIP data");
   case STREAM_UNIX_ATTRIBUTES:
      return _("File attributes");
   case STREAM_FILE_DATA:
      return _("File data");
   case STREAM_MD5_DIGEST:
      return _("MD5 digest");
   case STREAM_UNIX_ATTRIBUTES_EX:
      return _("Extended attributes");
   case STREAM_SPARSE_DATA:
      return _("Sparse data");
   case STREAM_PROGRAM_NAMES:
      return _("Program names");
   case STREAM_PROGRAM_DATA:
      return _("Program data");
   case STREAM_SHA1_DIGEST:
      return _("SHA1 digest");
   case STREAM_MACOS_FORK_DATA:
      return _("HFS+ resource fork");
   case STREAM_HFSPLUS_ATTRIBUTES:
      return _("HFS+ Finder Info");
   case STREAM_SHA256_DIGEST:
      return _("SHA256 digest");
   case STREAM_SHA512_DIGEST:
      return _("SHA512 digest");
   case STREAM_SIGNED_DIGEST:
      return _("Signed digest");
   case STREAM_ENCRYPTED_FILE_DATA:
      return _("Encrypted File data");
   case STREAM_ENCRYPTED_FILE_GZIP_DATA:
      return _("Encrypted GZIP data");
   case STREAM_ENCRYPTED_WIN32_DATA:
      return _("Encrypted Win32 data");
   case STREAM_ENCRYPTED_WIN32_GZIP_DATA:
      return _("Encrypted Win32 GZIP data");
   case STREAM_ENCRYPTED_MACOS_FORK_DATA:
      return _("Encrypted HFS+ resource fork");
   default:
      sprintf(buf, "%d", stream);
      return (const char *)buf;
   }
}

   
void int64_LE2BE(int64_t* pBE, const int64_t v)
{
   /* convert little endian to big endian */
   if (htonl(1) != 1L) { /* no work if on little endian machine */
           memcpy(pBE, &v, sizeof(int64_t));
   } else {
           int i;
           uint8_t rv[sizeof(int64_t)];
           uint8_t *pv = (uint8_t *) &v;

           for (i = 0; i < 8; i++) {
              rv[i] = pv[7 - i];
           }
           memcpy(pBE, &rv, sizeof(int64_t));
   }    
}


void int32_LE2BE(int32_t* pBE, const int32_t v)
{
   /* convert little endian to big endian */
   if (htonl(1) != 1L) { /* no work if on little endian machine */
           memcpy(pBE, &v, sizeof(int32_t));
   } else {
           int i;
           uint8_t rv[sizeof(int32_t)];
           uint8_t *pv = (uint8_t *) &v;

           for (i = 0; i < 4; i++) {
              rv[i] = pv[3 - i];
           }
           memcpy(pBE, &rv, sizeof(int32_t));
   }    
}


bool processWin32BackupAPIBlock (BFILE *bfd, void *pBuffer, ssize_t dwSize)
{
   /* pByte contains the buffer 
      dwSize the len to be processed.  function assumes to be
      called in successive incremental order over the complete
      BackupRead stream beginning at pos 0 and ending at the end.
    */

   PROCESS_WIN32_BACKUPAPIBLOCK_CONTEXT* pContext = &(bfd->win32DecompContext);
   bool bContinue = false;
   int64_t dwDataOffset = 0;
   int64_t dwDataLen;

   /* Win32 Stream Header size without name of stream.
    * = sizeof (WIN32_STREAM_ID)- sizeof(WCHAR*); 
    */
   int32_t dwSizeHeader = 20; 

   do {
      if (pContext->liNextHeader >= dwSize) {
         dwDataLen = dwSize-dwDataOffset;
         bContinue = false; /* 1 iteration is enough */
      } else {
         dwDataLen = pContext->liNextHeader-dwDataOffset;
         bContinue = true; /* multiple iterations may be necessary */
      }

      /* flush */
      /* copy block of real DATA */
      if (pContext->bIsInData) {
         if (bwrite(bfd, ((char *)pBuffer)+dwDataOffset, dwDataLen) != (ssize_t)dwDataLen)
            return false;
      }

      if (pContext->liNextHeader < dwSize) {/* is a header in this block ? */
         int32_t dwOffsetTarget;
         int32_t dwOffsetSource;

         if (pContext->liNextHeader < 0) {
            /* start of header was before this block, so we
             * continue with the part in the current block 
             */
            dwOffsetTarget = -pContext->liNextHeader;
            dwOffsetSource = 0;
         } else {
            /* start of header is inside of this block */
            dwOffsetTarget = 0;
            dwOffsetSource = pContext->liNextHeader;
         }

         int32_t dwHeaderPartLen = dwSizeHeader-dwOffsetTarget;
         bool bHeaderIsComplete;

         if (dwHeaderPartLen <= dwSize-dwOffsetSource) {
            /* header (or rest of header) is completely available
               in current block 
             */
            bHeaderIsComplete = true;
         } else {
            /* header will continue in next block */
            bHeaderIsComplete = false;
            dwHeaderPartLen = dwSize-dwOffsetSource;
         }

         /* copy the available portion of header to persistent copy */
         memcpy(((char *)&pContext->header_stream)+dwOffsetTarget, ((char *)pBuffer)+dwOffsetSource, dwHeaderPartLen);

         /* recalculate position of next header */
         if (bHeaderIsComplete) {
            /* convert stream name size (32 bit little endian) to machine type */
            int32_t dwNameSize; 
            int32_LE2BE (&dwNameSize, pContext->header_stream.dwStreamNameSize);
            dwDataOffset = dwNameSize+pContext->liNextHeader+dwSizeHeader;

            /* convert stream size (64 bit little endian) to machine type */
            int64_LE2BE (&(pContext->liNextHeader), pContext->header_stream.Size);
            pContext->liNextHeader += dwDataOffset;

            pContext->bIsInData = pContext->header_stream.dwStreamId == WIN32_BACKUP_DATA;
            if (dwDataOffset == dwSize)
               bContinue = false;
         } else {
            /* stop and continue with next block */
            bContinue = false;
            pContext->bIsInData = false;
         }
      }
   } while (bContinue);

   /* set "NextHeader" relative to the beginning of the next block */
   pContext->liNextHeader-= dwSize;

   return TRUE;
}



/* ===============================================================
 *
 *            W I N D O W S
 *
 * ===============================================================
 */

#if defined(HAVE_WIN32)

void unix_name_to_win32(POOLMEM **win32_name, char *name);
extern "C" HANDLE get_osfhandle(int fd);


void binit(BFILE *bfd)
{
   memset(bfd, 0, sizeof(BFILE));
   bfd->fid = -1;
   bfd->mode = BF_CLOSED;
   bfd->use_backup_api = have_win32_api();
}

/*
 * Enables using the Backup API (win32_data).
 *   Returns 1 if function worked
 *   Returns 0 if failed (i.e. do not have Backup API on this machine)
 */
bool set_win32_backup(BFILE *bfd)
{
   /* We enable if possible here */
   bfd->use_backup_api = have_win32_api();
   return bfd->use_backup_api;
}


bool set_portable_backup(BFILE *bfd)
{
   bfd->use_backup_api = false;
   return true;
}

bool set_prog(BFILE *bfd, char *prog, JCR *jcr)
{
   bfd->prog = prog;
   bfd->jcr = jcr;
   return false;
}

/*
 * Return 1 if we are NOT using Win32 BackupWrite()
 * return 0 if are
 */
bool is_portable_backup(BFILE *bfd)
{
   return !bfd->use_backup_api;
}

bool have_win32_api()
{
   return p_BackupRead && p_BackupWrite;
}


/*
 * Return true  if we support the stream
 *        false if we do not support the stream
 *
 *  This code is running under Win32, so we
 *    do not need #ifdef on MACOS ...
 */
bool is_restore_stream_supported(int stream)
{
   switch (stream) {

/* Streams known not to be supported */
#ifndef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
   case STREAM_WIN32_GZIP_DATA:
#endif
   case STREAM_MACOS_FORK_DATA:
   case STREAM_HFSPLUS_ATTRIBUTES:
   case STREAM_ENCRYPTED_MACOS_FORK_DATA:
      return false;

   /* Known streams */
#ifdef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
   case STREAM_WIN32_GZIP_DATA:
#endif
   case STREAM_WIN32_DATA:
   case STREAM_UNIX_ATTRIBUTES:
   case STREAM_FILE_DATA:
   case STREAM_MD5_DIGEST:
   case STREAM_UNIX_ATTRIBUTES_EX:
   case STREAM_SPARSE_DATA:
   case STREAM_PROGRAM_NAMES:
   case STREAM_PROGRAM_DATA:
   case STREAM_SHA1_DIGEST:
#ifdef HAVE_SHA2
   case STREAM_SHA256_DIGEST:
   case STREAM_SHA512_DIGEST:
#endif
#ifdef HAVE_CRYPTO
   case STREAM_SIGNED_DIGEST:
   case STREAM_ENCRYPTED_FILE_DATA:
   case STREAM_ENCRYPTED_FILE_GZIP_DATA:
   case STREAM_ENCRYPTED_WIN32_DATA:
   case STREAM_ENCRYPTED_WIN32_GZIP_DATA:
#endif
   case 0:                            /* compatibility with old tapes */
      return true;
   }
   return false;
}

HANDLE bget_handle(BFILE *bfd)
{
   return bfd->fh;
}

int bopen(BFILE *bfd, const char *fname, int flags, mode_t mode)
{
   POOLMEM *win32_fname;
   POOLMEM *win32_fname_wchar;

   DWORD dwaccess, dwflags, dwshare;

   /* Convert to Windows path format */
   win32_fname = get_pool_memory(PM_FNAME);
   win32_fname_wchar = get_pool_memory(PM_FNAME);
   
   unix_name_to_win32(&win32_fname, (char *)fname);

   if (!(p_CreateFileA || p_CreateFileW))
      return 0;

   if (p_CreateFileW && p_MultiByteToWideChar)
      make_win32_path_UTF8_2_wchar(&win32_fname_wchar, fname);

   if (flags & O_CREAT) {             /* Create */
      if (bfd->use_backup_api) {
         dwaccess = GENERIC_WRITE|FILE_ALL_ACCESS|WRITE_OWNER|WRITE_DAC|ACCESS_SYSTEM_SECURITY;
         dwflags = FILE_FLAG_BACKUP_SEMANTICS;
      } else {
         dwaccess = GENERIC_WRITE;
         dwflags = 0;
      }

      // unicode or ansii open for create write
      if (p_CreateFileW && p_MultiByteToWideChar) {   
         bfd->fh = p_CreateFileW((LPCWSTR)win32_fname_wchar,
                dwaccess,                /* Requested access */
                0,                       /* Shared mode */
                NULL,                    /* SecurityAttributes */
                CREATE_ALWAYS,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */
      } else {
         bfd->fh = p_CreateFileA(win32_fname,
                dwaccess,                /* Requested access */
                0,                       /* Shared mode */
                NULL,                    /* SecurityAttributes */
                CREATE_ALWAYS,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */
      }

      bfd->mode = BF_WRITE;

   } else if (flags & O_WRONLY) {     /* Open existing for write */
      if (bfd->use_backup_api) {
         dwaccess = GENERIC_WRITE|WRITE_OWNER|WRITE_DAC;
         dwflags = FILE_FLAG_BACKUP_SEMANTICS;
      } else {
         dwaccess = GENERIC_WRITE;
         dwflags = 0;
      }

      // unicode or ansii open for open existing write
      if (p_CreateFileW && p_MultiByteToWideChar) {   
         bfd->fh = p_CreateFileW((LPCWSTR)win32_fname_wchar,
                dwaccess,                /* Requested access */
                0,                       /* Shared mode */
                NULL,                    /* SecurityAttributes */
                OPEN_EXISTING,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */
      } else {
         bfd->fh = p_CreateFileA(win32_fname,
                dwaccess,                /* Requested access */
                0,                       /* Shared mode */
                NULL,                    /* SecurityAttributes */
                OPEN_EXISTING,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */

      }

      bfd->mode = BF_WRITE;

   } else {                           /* Read */
      if (bfd->use_backup_api) {
         dwaccess = GENERIC_READ|READ_CONTROL|ACCESS_SYSTEM_SECURITY;
         dwflags = FILE_FLAG_BACKUP_SEMANTICS;
         dwshare = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
      } else {
         dwaccess = GENERIC_READ;
         dwflags = 0;
         dwshare = FILE_SHARE_READ|FILE_SHARE_WRITE;
      }

      // unicode or ansii open for open existing read
      if (p_CreateFileW && p_MultiByteToWideChar) {   
         bfd->fh = p_CreateFileW((LPCWSTR)win32_fname_wchar,
                dwaccess,                /* Requested access */
                dwshare,                 /* Share modes */
                NULL,                    /* SecurityAttributes */
                OPEN_EXISTING,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */
      } else {
         bfd->fh = p_CreateFileA(win32_fname,
                dwaccess,                /* Requested access */
                dwshare,                 /* Share modes */
                NULL,                    /* SecurityAttributes */
                OPEN_EXISTING,           /* CreationDisposition */
                dwflags,                 /* Flags and attributes */
                NULL);                   /* TemplateFile */
      }

      bfd->mode = BF_READ;
   }

   if (bfd->fh == INVALID_HANDLE_VALUE) {
      bfd->lerror = GetLastError();
      bfd->berrno = b_errno_win32;
      errno = b_errno_win32;
      bfd->mode = BF_CLOSED;
   }
   bfd->errmsg = NULL;
   bfd->lpContext = NULL;
   bfd->win32DecompContext.bIsInData = false;
   bfd->win32DecompContext.liNextHeader = 0;
   free_pool_memory(win32_fname_wchar);
   free_pool_memory(win32_fname);
   return bfd->mode == BF_CLOSED ? -1 : 1;
}

/*
 * Returns  0 on success
 *         -1 on error
 */
int bclose(BFILE *bfd)
{
   int stat = 0;

   if (bfd->errmsg) {
      free_pool_memory(bfd->errmsg);
      bfd->errmsg = NULL;
   }
   if (bfd->mode == BF_CLOSED) {
      return 0;
   }
   if (bfd->use_backup_api && bfd->mode == BF_READ) {
      BYTE buf[10];
      if (!bfd->lpContext && !p_BackupRead(bfd->fh,
              buf,                    /* buffer */
              (DWORD)0,               /* bytes to read */
              &bfd->rw_bytes,         /* bytes read */
              1,                      /* Abort */
              1,                      /* ProcessSecurity */
              &bfd->lpContext)) {     /* Read context */
         errno = b_errno_win32;
         stat = -1;
      }
   } else if (bfd->use_backup_api && bfd->mode == BF_WRITE) {
      BYTE buf[10];
      if (!bfd->lpContext && !p_BackupWrite(bfd->fh,
              buf,                    /* buffer */
              (DWORD)0,               /* bytes to read */
              &bfd->rw_bytes,         /* bytes written */
              1,                      /* Abort */
              1,                      /* ProcessSecurity */
              &bfd->lpContext)) {     /* Write context */
         errno = b_errno_win32;
         stat = -1;
      }
   }
   if (!CloseHandle(bfd->fh)) {
      stat = -1;
      errno = b_errno_win32;
   }
   bfd->mode = BF_CLOSED;
   bfd->lpContext = NULL;
   return stat;
}

/* Returns: bytes read on success
 *           0         on EOF
 *          -1         on error
 */
ssize_t bread(BFILE *bfd, void *buf, size_t count)
{
   bfd->rw_bytes = 0;

   if (bfd->use_backup_api) {
      if (!p_BackupRead(bfd->fh,
           (BYTE *)buf,
           count,
           &bfd->rw_bytes,
           0,                           /* no Abort */
           1,                           /* Process Security */
           &bfd->lpContext)) {          /* Context */
         bfd->lerror = GetLastError();
         bfd->berrno = b_errno_win32;
         errno = b_errno_win32;
         return -1;
      }
   } else {
      if (!ReadFile(bfd->fh,
           buf,
           count,
           &bfd->rw_bytes,
           NULL)) {
         bfd->lerror = GetLastError();
         bfd->berrno = b_errno_win32;
         errno = b_errno_win32;
         return -1;
      }
   }

   return (ssize_t)bfd->rw_bytes;
}

ssize_t bwrite(BFILE *bfd, void *buf, size_t count)
{
   bfd->rw_bytes = 0;

   if (bfd->use_backup_api) {
      if (!p_BackupWrite(bfd->fh,
           (BYTE *)buf,
           count,
           &bfd->rw_bytes,
           0,                           /* No abort */
           1,                           /* Process Security */
           &bfd->lpContext)) {          /* Context */
         bfd->lerror = GetLastError();
         bfd->berrno = b_errno_win32;
         errno = b_errno_win32;
         return -1;
      }
   } else {
      if (!WriteFile(bfd->fh,
           buf,
           count,
           &bfd->rw_bytes,
           NULL)) {
         bfd->lerror = GetLastError();
         bfd->berrno = b_errno_win32;
         errno = b_errno_win32;
         return -1;
      }
   }
   return (ssize_t)bfd->rw_bytes;
}

bool is_bopen(BFILE *bfd)
{
   return bfd->mode != BF_CLOSED;
}

boffset_t blseek(BFILE *bfd, boffset_t offset, int whence)
{
   LONG  offset_low = (LONG)offset;
   LONG  offset_high = (LONG)(offset >> 32);
   DWORD dwResult;

   dwResult = SetFilePointer(bfd->fh, offset_low, &offset_high, whence);

   if (dwResult == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
      return (boffset_t)-1;
   }

   return ((boffset_t)offset_high << 32) | dwResult;
}

#else  /* Unix systems */

/* ===============================================================
 *
 *            U N I X
 *
 * ===============================================================
 */
void binit(BFILE *bfd)
{
   memset(bfd, 0, sizeof(BFILE));
   bfd->fid = -1;
}

bool have_win32_api()
{
   return false;                       /* no can do */
}

/*
 * Enables using the Backup API (win32_data).
 *   Returns true  if function worked
 *   Returns false if failed (i.e. do not have Backup API on this machine)
 */
bool set_win32_backup(BFILE *bfd)
{
   return false;                       /* no can do */
}


bool set_portable_backup(BFILE *bfd)
{
   return true;                        /* no problem */
}

/*
 * Return true  if we are writing in portable format
 * return false if not
 */
bool is_portable_backup(BFILE *bfd)
{
   return true;                       /* portable by definition */
}

bool set_prog(BFILE *bfd, char *prog, JCR *jcr)
{
#ifdef HAVE_PYTHON
   if (bfd->prog && strcmp(prog, bfd->prog) == 0) {
      return true;                    /* already setup */
   }

   if (python_set_prog(jcr, prog)) {
      Dmsg1(000, "Set prog=%s\n", prog);
      bfd->prog = prog;
      bfd->jcr = jcr;
      return true;
   }
#endif
   Dmsg0(000, "No prog set\n");
   bfd->prog = NULL;
   return false;

}

/* 
 * This code is running on a non-Win32 machine 
 */
bool is_restore_stream_supported(int stream)
{
   /* No Win32 backup on this machine */
     switch (stream) {
#ifndef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
   case STREAM_WIN32_GZIP_DATA:    
#endif
#ifndef HAVE_DARWIN_OS
   case STREAM_MACOS_FORK_DATA:
   case STREAM_HFSPLUS_ATTRIBUTES:
#endif
      return false;

   /* Known streams */
#ifdef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
   case STREAM_WIN32_GZIP_DATA:    
#endif
   case STREAM_WIN32_DATA:
   case STREAM_UNIX_ATTRIBUTES:
   case STREAM_FILE_DATA:
   case STREAM_MD5_DIGEST:
   case STREAM_UNIX_ATTRIBUTES_EX:
   case STREAM_SPARSE_DATA:
   case STREAM_PROGRAM_NAMES:
   case STREAM_PROGRAM_DATA:
   case STREAM_SHA1_DIGEST:
#ifdef HAVE_SHA2
   case STREAM_SHA256_DIGEST:
   case STREAM_SHA512_DIGEST:
#endif
#ifdef HAVE_CRYPTO
   case STREAM_SIGNED_DIGEST:
   case STREAM_ENCRYPTED_FILE_DATA:
   case STREAM_ENCRYPTED_FILE_GZIP_DATA:
   case STREAM_ENCRYPTED_WIN32_DATA:
   case STREAM_ENCRYPTED_WIN32_GZIP_DATA:
#endif
#ifdef HAVE_DARWIN_OS
   case STREAM_MACOS_FORK_DATA:
   case STREAM_HFSPLUS_ATTRIBUTES:
#ifdef HAVE_CRYPTO
   case STREAM_ENCRYPTED_MACOS_FORK_DATA:
#endif /* HAVE_CRYPTO */
#endif /* HAVE_DARWIN_OS */
   case 0:   /* compatibility with old tapes */
      return true;

   }
   return false;
}

int bopen(BFILE *bfd, const char *fname, int flags, mode_t mode)
{
   /* Open reader/writer program */
   if (bfd->prog) {
      Dmsg1(000, "Open file %d\n", bfd->fid);
      return python_open(bfd, fname, flags, mode);
   }

   /* Normal file open */
   Dmsg1(400, "open file %s\n", fname);
   /* We use fnctl to set O_NOATIME if requested to avoid open error */
   bfd->fid = open(fname, flags & ~O_NOATIME, mode);
   /* Set O_NOATIME if possible */
   if (bfd->fid != -1 && flags & O_NOATIME) {
      int oldflags = fcntl(bfd->fid, F_GETFL, 0);
      if (oldflags == -1) {
         bfd->berrno = errno;
         close(bfd->fid);
         bfd->fid = -1;
      } else {
         int ret = fcntl(bfd->fid, F_SETFL, oldflags | O_NOATIME);
        /* EPERM means setting O_NOATIME was not allowed  */
         if (ret == -1 && errno != EPERM) {
            bfd->berrno = errno;
            close(bfd->fid);
            bfd->fid = -1;
         }
      }
   }
   bfd->berrno = errno;
   Dmsg1(400, "Open file %d\n", bfd->fid);
   errno = bfd->berrno;

   bfd->win32DecompContext.bIsInData = false;
   bfd->win32DecompContext.liNextHeader = 0;

   return bfd->fid;
}

#ifdef HAVE_DARWIN_OS
/* Open the resource fork of a file. */
int bopen_rsrc(BFILE *bfd, const char *fname, int flags, mode_t mode)
{
   POOLMEM *rsrc_fname;

   rsrc_fname = get_pool_memory(PM_FNAME);
   pm_strcpy(rsrc_fname, fname);
   pm_strcat(rsrc_fname, _PATH_RSRCFORKSPEC);
   bopen(bfd, rsrc_fname, flags, mode);
   free_pool_memory(rsrc_fname);
   return bfd->fid;
}
#endif


int bclose(BFILE *bfd)
{
   int stat;

   Dmsg1(400, "Close file %d\n", bfd->fid);

   /* Close reader/writer program */
   if (bfd->prog) {
      return python_close(bfd);
   }

   if (bfd->fid == -1) {
      return 0;
   }

   /* Close normal file */
   stat = close(bfd->fid);
   bfd->berrno = errno;
   bfd->fid = -1;
   return stat;
}

ssize_t bread(BFILE *bfd, void *buf, size_t count)
{
   ssize_t stat;

   if (bfd->prog) {
      return python_read(bfd, buf, count);
   }
   stat = read(bfd->fid, buf, count);
   bfd->berrno = errno;
   return stat;
}

ssize_t bwrite(BFILE *bfd, void *buf, size_t count)
{
   ssize_t stat;

   if (bfd->prog) {
      return python_write(bfd, buf, count);
   }
   stat = write(bfd->fid, buf, count);
   bfd->berrno = errno;
   return stat;
}

bool is_bopen(BFILE *bfd)
{
   return bfd->fid >= 0;
}

boffset_t blseek(BFILE *bfd, boffset_t offset, int whence)
{
    boffset_t pos;
    pos = (boffset_t)lseek(bfd->fid, (off_t)offset, whence);
    bfd->berrno = errno;
    return pos;
}

#endif
