/*
 *  Bacula low level File I/O routines.  This routine simulates
 *    open(), read(), write(), and close(), but using native routines.
 *    I.e. on Windows, we use Windows APIs.
 *
 *    Kern Sibbald, April MMIII
 *
 *   Version $Id: bfile.c,v 1.30 2004/10/04 20:34:02 kerns Exp $
 *
 */
/*
   Copyright (C) 2003-2004 Kern Sibbald and John Walker

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

#include "bacula.h"
#include "find.h"

/* ===============================================================
 * 
 *	      U N I X	AND   W I N D O W S
 *
 * ===============================================================
 */

int is_win32_stream(int stream)
{
   switch (stream) {
   case STREAM_WIN32_DATA:
   case STREAM_WIN32_GZIP_DATA:
      return 1;
   }
   return 0;
}

const char *stream_to_ascii(int stream)
{
   static char buf[20];

   switch (stream) {
   case STREAM_GZIP_DATA:
      return "GZIP data";
   case STREAM_SPARSE_GZIP_DATA:
      return "GZIP sparse data";
   case STREAM_WIN32_DATA:
      return "Win32 data";
   case STREAM_WIN32_GZIP_DATA:
      return "Win32 GZIP data";
   case STREAM_UNIX_ATTRIBUTES:
      return "File attributes";
   case STREAM_FILE_DATA:
      return "File data";
   case STREAM_MD5_SIGNATURE:
      return "MD5 signature";
   case STREAM_UNIX_ATTRIBUTES_EX:
      return "Extended attributes";
   case STREAM_SPARSE_DATA:
      return "Sparse data";
   case STREAM_PROGRAM_NAMES:
      return "Program names";
   case STREAM_PROGRAM_DATA:
      return "Program data";
   case STREAM_SHA1_SIGNATURE:
      return "SHA1 signature";
   default:
      sprintf(buf, "%d", stream);
      return (const char *)buf;
   }
}



/* ===============================================================
 * 
 *	      W I N D O W S
 *
 * ===============================================================
 */

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)

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
int set_win32_backup(BFILE *bfd) 
{
   /* We enable if possible here */
   bfd->use_backup_api = have_win32_api();
   return bfd->use_backup_api;
}


int set_portable_backup(BFILE *bfd)
{
   bfd->use_backup_api = 0;
   return 1;
}

void set_prog(BFILE *bfd, char *prog, JCR *jcr)
{
   bfd->prog = prog;
   bfd->jcr = jcr;
}

/*
 * Return 1 if we are NOT using Win32 BackupWrite() 
 * return 0 if are
 */
int is_portable_backup(BFILE *bfd) 
{
   return !bfd->use_backup_api;
}

int have_win32_api()
{
   return p_BackupRead && p_BackupWrite;
}



/*
 * Return 1 if we support the stream
 *	  0 if we do not support the stream
 */
int is_stream_supported(int stream)
{
   /* No Win32 backup on this machine */
   switch (stream) {
#ifndef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
      return 0;
#endif
   case STREAM_WIN32_DATA:
   case STREAM_WIN32_GZIP_DATA:
      return have_win32_api();

   /* Known streams */
#ifdef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
#endif
   case STREAM_UNIX_ATTRIBUTES:
   case STREAM_FILE_DATA:
   case STREAM_MD5_SIGNATURE:
   case STREAM_UNIX_ATTRIBUTES_EX:
   case STREAM_SPARSE_DATA:
   case STREAM_PROGRAM_NAMES:
   case STREAM_PROGRAM_DATA:
   case STREAM_SHA1_SIGNATURE:
   case 0:			      /* compatibility with old tapes */
      return 1;
   }
   return 0;
}

HANDLE bget_handle(BFILE *bfd)
{
   return bfd->fh;
}

int bopen(BFILE *bfd, const char *fname, int flags, mode_t mode)
{
   POOLMEM *win32_fname;
   DWORD dwaccess, dwflags, dwshare;

   /* Convert to Windows path format */
   win32_fname = get_pool_memory(PM_FNAME);
   unix_name_to_win32(&win32_fname, (char *)fname);

   if (flags & O_CREAT) {	      /* Create */
      if (bfd->use_backup_api) {
	 dwaccess = GENERIC_WRITE|FILE_ALL_ACCESS|WRITE_OWNER|WRITE_DAC|ACCESS_SYSTEM_SECURITY; 	       
	 dwflags = FILE_FLAG_BACKUP_SEMANTICS;
      } else {
	 dwaccess = GENERIC_WRITE;
	 dwflags = 0;
      }
      bfd->fh = CreateFile(win32_fname,
	     dwaccess,		      /* Requested access */
	     0, 		      /* Shared mode */
	     NULL,		      /* SecurityAttributes */
	     CREATE_ALWAYS,	      /* CreationDisposition */
	     dwflags,		      /* Flags and attributes */
	     NULL);		      /* TemplateFile */
      bfd->mode = BF_WRITE;

   } else if (flags & O_WRONLY) {     /* Open existing for write */
      if (bfd->use_backup_api) {
	 dwaccess = GENERIC_WRITE|WRITE_OWNER|WRITE_DAC;
	 dwflags = FILE_FLAG_BACKUP_SEMANTICS;
      } else {
	 dwaccess = GENERIC_WRITE;
	 dwflags = 0;
      }
      bfd->fh = CreateFile(win32_fname,
	     dwaccess,		      /* Requested access */
	     0, 		      /* Shared mode */
	     NULL,		      /* SecurityAttributes */
	     OPEN_EXISTING,	      /* CreationDisposition */
	     dwflags,		      /* Flags and attributes */
	     NULL);		      /* TemplateFile */
      bfd->mode = BF_WRITE;

   } else {			      /* Read */
      if (bfd->use_backup_api) {
	 dwaccess = GENERIC_READ|READ_CONTROL|ACCESS_SYSTEM_SECURITY;
	 dwflags = FILE_FLAG_BACKUP_SEMANTICS;
	 dwshare = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
      } else {
	 dwaccess = GENERIC_READ;
	 dwflags = 0;
	 dwshare = FILE_SHARE_READ|FILE_SHARE_WRITE;
      }
      bfd->fh = CreateFile(win32_fname,
	     dwaccess,		      /* Requested access */
	     dwshare,		      /* Share modes */
	     NULL,		      /* SecurityAttributes */
	     OPEN_EXISTING,	      /* CreationDisposition */
	     dwflags,		      /* Flags and attributes */
	     NULL);		      /* TemplateFile */
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
   free_pool_memory(win32_fname);
   return bfd->mode == BF_CLOSED ? -1 : 1;
}

/* 
 * Returns  0 on success
 *	   -1 on error
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
	      buf,		      /* buffer */
	      (DWORD)0, 	      /* bytes to read */
	      &bfd->rw_bytes,	      /* bytes read */
	      1,		      /* Abort */
	      1,		      /* ProcessSecurity */
	      &bfd->lpContext)) {     /* Read context */
	 errno = b_errno_win32;
	 stat = -1;
      } 
   } else if (bfd->use_backup_api && bfd->mode == BF_WRITE) {
      BYTE buf[10];
      if (!bfd->lpContext && !p_BackupWrite(bfd->fh,   
	      buf,		      /* buffer */
	      (DWORD)0, 	      /* bytes to read */
	      &bfd->rw_bytes,	      /* bytes written */
	      1,		      /* Abort */
	      1,		      /* ProcessSecurity */
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
 *	     0	       on EOF
 *	    -1	       on error
 */
ssize_t bread(BFILE *bfd, void *buf, size_t count)
{
   bfd->rw_bytes = 0;

   if (bfd->use_backup_api) {
      if (!p_BackupRead(bfd->fh,
	   (BYTE *)buf,
	   count,
	   &bfd->rw_bytes,
	   0,				/* no Abort */
	   1,				/* Process Security */
	   &bfd->lpContext)) {		/* Context */
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
	   0,				/* No abort */
	   1,				/* Process Security */
	   &bfd->lpContext)) {		/* Context */
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

int is_bopen(BFILE *bfd)
{
   return bfd->mode != BF_CLOSED;
}

off_t blseek(BFILE *bfd, off_t offset, int whence)
{
   /* ****FIXME**** this must be implemented if we want to read Win32 Archives */
   return -1;
}

#else  /* Unix systems */

/* ===============================================================
 * 
 *	      U N I X
 *
 * ===============================================================
 */
void binit(BFILE *bfd)
{
   memset(bfd, 0, sizeof(BFILE));
   bfd->fid = -1;
}

int have_win32_api()
{ 
   return 0;			      /* no can do */
} 

/*
 * Enables using the Backup API (win32_data).
 *   Returns 1 if function worked
 *   Returns 0 if failed (i.e. do not have Backup API on this machine)
 */
int set_win32_backup(BFILE *bfd) 
{
   return 0;			      /* no can do */
}


int set_portable_backup(BFILE *bfd)
{
   return 1;			      /* no problem */
}

/*
 * Return 1 if we are writing in portable format
 * return 0 if not
 */
int is_portable_backup(BFILE *bfd) 
{
   return 1;			      /* portable by definition */
}

void set_prog(BFILE *bfd, char *prog, JCR *jcr)
{
   bfd->prog = prog;
   bfd->jcr = jcr;
}


int is_stream_supported(int stream)
{
   /* No Win32 backup on this machine */
   switch (stream) {
#ifndef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
#endif
   case STREAM_WIN32_DATA:
   case STREAM_WIN32_GZIP_DATA:
      return 0;

   /* Known streams */
#ifdef HAVE_LIBZ
   case STREAM_GZIP_DATA:
   case STREAM_SPARSE_GZIP_DATA:
#endif
   case STREAM_UNIX_ATTRIBUTES:
   case STREAM_FILE_DATA:
   case STREAM_MD5_SIGNATURE:
   case STREAM_UNIX_ATTRIBUTES_EX:
   case STREAM_SPARSE_DATA:
   case STREAM_PROGRAM_NAMES:
   case STREAM_PROGRAM_DATA:
   case STREAM_SHA1_SIGNATURE:
   case 0:			      /* compatibility with old tapes */
      return 1;

   }
   return 0;
}

int bopen(BFILE *bfd, const char *fname, int flags, mode_t mode)
{
   /* Open reader/writer program */
   if (bfd->prog) {
      POOLMEM *ecmd = get_pool_memory(PM_FNAME);
      ecmd = edit_job_codes(bfd->jcr, ecmd, bfd->prog, fname);
      const char *pmode;
      if (flags & O_RDONLY) {
         pmode = "r";
      } else {
         pmode = "w";
      }
      bfd->bpipe = open_bpipe(ecmd, 0, pmode);
      if (bfd->bpipe == NULL) {
	 bfd->berrno = errno;
	 bfd->fid = -1;
	 free_pool_memory(ecmd);
	 return -1;
      }
      free_pool_memory(ecmd);
      if (flags & O_RDONLY) {
	 bfd->fid = fileno(bfd->bpipe->rfd);
      } else {
	 bfd->fid = fileno(bfd->bpipe->wfd);
      }
      errno = 0;
      return bfd->fid;
   }

   /* Normal file open */
   bfd->fid = open(fname, flags, mode);
   bfd->berrno = errno;
   Dmsg1(400, "Open file %d\n", bfd->fid);
   errno = bfd->berrno;
   return bfd->fid;
}

int bclose(BFILE *bfd)
{ 
   int stat;  
   Dmsg1(400, "Close file %d\n", bfd->fid);
   if (bfd->fid == -1) {
      return 0;
   }
   /* Close reader/writer program */
   if (bfd->prog && bfd->bpipe) {
      stat = close_bpipe(bfd->bpipe);
      bfd->berrno = errno;
      bfd->fid = -1;
      bfd->bpipe = NULL;
      return stat;
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
   stat = read(bfd->fid, buf, count);
   bfd->berrno = errno;
   return stat;
}

ssize_t bwrite(BFILE *bfd, void *buf, size_t count)
{
   ssize_t stat;
   stat = write(bfd->fid, buf, count);
   bfd->berrno = errno;
   return stat;
}

int is_bopen(BFILE *bfd)
{
   return bfd->fid >= 0;
}

off_t blseek(BFILE *bfd, off_t offset, int whence)
{
    off_t pos;
    pos = lseek(bfd->fid, offset, whence);
    bfd->berrno = errno;
    return pos;
}

/* DO NOT USE */
char *xberror(BFILE *bfd)
{
    return strerror(bfd->berrno);
}

#endif
