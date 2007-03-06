/*
 * Bacula MD5 definitions
 *
 *  Kern Sibbald, 2001
 *
 *   Version $Id: md5.h,v 1.3.28.1 2006/01/07 11:18:11 kerns Exp $
 */
/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
   MA 02111-1307, USA.

 */

#ifndef __BMD5_H
#define __BMD5_H

#define MD5HashSize 16

struct MD5Context {
   uint32_t buf[4];
   uint32_t bits[2];
   uint8_t  in[64];
};

typedef struct MD5Context MD5Context;

extern void MD5Init(struct MD5Context *ctx);
extern void MD5Update(struct MD5Context *ctx, unsigned char *buf, unsigned len);
extern void MD5Final(unsigned char digest[16], struct MD5Context *ctx);
extern void MD5Transform(uint32_t buf[4], uint32_t in[16]);

#endif /* !__BMD5_H */
