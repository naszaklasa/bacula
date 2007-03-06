/* Some elementary bit manipulations
 *
 *   Kern Sibbald, MM
 *
 *  NOTE:  base 0
 *
 *   Version $Id: bits.h,v 1.4 2004/12/21 16:18:38 kerns Exp $
 */
/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

#ifndef __BITS_H_
#define __BITS_H_

/* number of bytes to hold n bits */
#define nbytes_for_bits(n) ((((n)-1)>>3)+1)

/* test if bit is set */
#define bit_is_set(b, var) (((var)[(b)>>3] & (1<<((b)&0x7))) != 0)

/* set bit */
#define set_bit(b, var) ((var)[(b)>>3] |= (1<<((b)&0x7)))

/* clear bit */
#define clear_bit(b, var) ((var)[(b)>>3] &= ~(1<<((b)&0x7)))

/* clear all bits */
#define clear_all_bits(b, var) memset(var, 0, nbytes_for_bits(b))

/* set range of bits */
#define set_bits(f, l, var) { \
   int i; \
   for (i=f; i<=l; i++)  \
      set_bit(i, var); \
}

/* clear range of bits */
#define clear_bits(f, l, var) { \
   int i; \
   for (i=f; i<=l; i++)  \
      clear_bit(i, var); \
}

#endif /* __BITS_H_ */
