/*
 *   Version $Id: berrno.h,v 1.7 2005/08/21 08:40:45 kerns Exp $
 *
 * Kern Sibbald, July MMIV
 *
 */
/*
   Copyright (C) 2004-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifdef HAVE_WIN32
#define b_errno_win32  (1<<29)        /* user reserved bit */
#endif
#define b_errno_exit   (1<<28)        /* child exited, exit code returned */
#define b_errno_signal (1<<27)        /* child died, signal code returned */

/*
 * A more generalized way of handling errno that works with Unix, Windows,
 *  and with Bacula bpipes.
 *
 * It works by picking up errno and creating a memory pool buffer
 *  for editing the message. strerror() does the actual editing, and
 *  it is thread safe.
 *
 * If bit 29 in berrno_ is set then it is a Win32 error, and we
 *  must to a GetLastError() to get the error code for formatting.
 * If bit 29 in berrno_ is not set, then it is a Unix errno.
 *
 */
class berrno : public SMARTALLOC {
   POOLMEM *buf_;
   int berrno_;
   void format_win32_message();
public:
   berrno(int pool=PM_EMSG);
   ~berrno();
   const char *strerror();
   const char *strerror(int errnum);
   void set_errno(int errnum);
};

/* Constructor */
inline berrno::berrno(int pool)
{
   berrno_ = errno;
   buf_ = get_pool_memory(pool);
#ifdef HAVE_WIN32
   format_win32_message();
#endif
}

inline berrno::~berrno()
{
   free_pool_memory(buf_);
}

inline const char *berrno::strerror(int errnum)
{
   berrno_ = errnum;
   return berrno::strerror();
}


inline void berrno::set_errno(int errnum)
{
   berrno_ = errnum;
}
