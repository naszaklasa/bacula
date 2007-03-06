/*

	Error checking memory allocator

     Version $Id: alloc.c,v 1.4 2004/12/21 16:18:38 kerns Exp $
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

#ifdef NEEDED

#include <stdio.h>
#include <stdlib.h>

#ifdef TESTERR
#undef NULL
#define NULL  buf
#endif

/*LINTLIBRARY*/

#define V	 (void)

#ifdef SMARTALLOC

extern void *sm_malloc();

/*  SM_ALLOC  --  Allocate buffer and signal on error  */

void *sm_alloc(char *fname, int lineno, unsigned int nbytes)
{
	void *buf;

	if ((buf = sm_malloc(fname, lineno, nbytes)) != NULL) {
	   return buf;
	}
	V fprintf(stderr, "\nBoom!!!  Memory capacity exceeded.\n");
	V fprintf(stderr, "  Requested %u bytes at line %d of %s.\n",
	   nbytes, lineno, fname);
	abort();
	/*NOTREACHED*/
}
#else

/*  ALLOC  --  Allocate buffer and signal on error  */

void *alloc(unsigned int nbytes)
{
	void *buf;

	if ((buf = malloc(nbytes)) != NULL) {
	   return buf;
	}
	V fprintf(stderr, "\nBoom!!!  Memory capacity exceeded.\n");
	V fprintf(stderr, "  Requested %u bytes.\n", nbytes);
	abort();
	/*NOTREACHED*/
}
#endif
#endif
