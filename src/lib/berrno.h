/*
 *   Version $Id: berrno.h,v 1.5 2004/09/19 18:56:26 kerns Exp $
 */

/*
   Copyright (C) 2004 Kern Sibbald and John Walker

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

   Kern Sibbald, July MMIV

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
