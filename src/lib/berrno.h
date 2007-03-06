/*
 *   Version $Id: berrno.h,v 1.10 2006/11/21 13:20:10 kerns Exp $
 *
 * Kern Sibbald, July MMIV
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

/*
 * Extra bits set to interpret errno value differently from errno
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
 *  must do a GetLastError() to get the error code for formatting.
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
   int code() { return berrno_ & ~(b_errno_exit|b_errno_signal); }
   int code(int stat) { return stat & ~(b_errno_exit|b_errno_signal); }
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
