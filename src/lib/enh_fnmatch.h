/* Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.

NOTE: The canonical source of this file is maintained with the GNU C Library.
Bugs can be reported to bug-glibc@prep.ai.mit.edu.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.	*/

/* Modified version of fnmatch.h - Robert Nelson */

#ifndef _ENH_FNMATCH_H

#define _ENH_FNMATCH_H	1

#ifdef	__cplusplus
extern "C" {
#endif

#if defined (__cplusplus) || (defined (__STDC__) && __STDC__)
#undef	__P
#define __P(protos)	protos
#else /* Not C++ or ANSI C.  */
#undef	__P
#define __P(protos)	()
/* We can get away without defining `const' here only because in this file
   it is used only inside the prototype for `fnmatch', which is elided in
   non-ANSI C where `const' is problematical.  */
#endif /* C++ or ANSI C.  */


/* Match STRING against the filename pattern PATTERN,
   returning zero if it matches, FNM_NOMATCH if not.  */
extern int enh_fnmatch __P ((const char *__pattern, const char *__string,
			 int __flags));

#ifdef	__cplusplus
}
#endif

#endif /* fnmatch.h */
