/*
 *  Bacula low level File I/O routines.  This routine simulates
 *    open(), read(), write(), and close(), but using native routines.
 *    I.e. on Windows, we use Windows APIs.
 *
 *     Kern Sibbald May MMIII
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

#ifndef __BFILE_H
#define __BFILE_H

/*  =======================================================
 *
 *   W I N D O W S 
 *
 *  =======================================================
 */
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)

#include <windows.h>
#include "winapi.h"

enum {
   BF_CLOSED,
   BF_READ,                           /* BackupRead */
   BF_WRITE                           /* BackupWrite */
};

/* In bfile.c */

/* Basic low level I/O file packet */
struct BFILE {
   int use_backup_api;                /* set if using BackupRead/Write */
   int mode;                          /* set if file is open */
   HANDLE fh;                         /* Win32 file handle */
   int fid;                           /* fd if doing Unix style */
   LPVOID lpContext;                  /* BackupRead/Write context */
   POOLMEM *errmsg;                   /* error message buffer */
   DWORD rw_bytes;                    /* Bytes read or written */
   DWORD lerror;                      /* Last error code */
   int berrno;                        /* errno */
   char *prog;                        /* reader/writer program if any */
   BPIPE *bpipe;                      /* pipe for reader */
   JCR *jcr;                          /* jcr for editing job codes */
};      

HANDLE bget_handle(BFILE *bfd);

#else   /* Linux/Unix systems */

/*  =======================================================
 *
 *   U N I X 
 *
 *  =======================================================
 */

/* Basic low level I/O file packet */
struct BFILE {
   int fid;                           /* file id on Unix */
   int berrno;
   char *prog;                        /* reader/writer program if any */
   BPIPE *bpipe;                      /* pipe for reader */
   JCR *jcr;                          /* jcr for editing job codes */
};      

#endif

void    binit(BFILE *bfd);
int     is_bopen(BFILE *bfd);
int     set_win32_backup(BFILE *bfd);
int     set_portable_backup(BFILE *bfd);
void    set_prog(BFILE *bfd, char *prog, JCR *jcr);
int     have_win32_api();
int     is_portable_backup(BFILE *bfd);
int     is_stream_supported(int stream);
int     is_win32_stream(int stream);
char   *xberror(BFILE *bfd);          /* DO NOT USE  -- use berrno class */
int     bopen(BFILE *bfd, const char *fname, int flags, mode_t mode);
int     bclose(BFILE *bfd);
ssize_t bread(BFILE *bfd, void *buf, size_t count);
ssize_t bwrite(BFILE *bfd, void *buf, size_t count);
off_t   blseek(BFILE *bfd, off_t offset, int whence);
const char   *stream_to_ascii(int stream);

#endif /* __BFILE_H */
