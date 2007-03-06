/*
 * tls.h TLS support functions
 *
 * Author: Landon Fuller <landonf@threerings.net>
 *
 * Version $Id: tls.h,v 1.1 2005/04/22 08:09:35 landonf Exp $
 *
 * Copyright (C) 2005 Kern Sibbald
 *
 * This file was contributed to the Bacula project by Landon Fuller
 * and Three Rings Design, Inc.
 *
 * Three Rings Design, Inc. has been granted a perpetual, worldwide,
 * non-exclusive, no-charge, royalty-free, irrevocable copyright
 * license to reproduce, prepare derivative works of, publicly
 * display, publicly perform, sublicense, and distribute the original
 * work contributed by Three Rings Design, Inc. and its employees to
 * the Bacula project in source or object form.
 *
 * If you wish to license contributions from Three Rings Design, Inc,
 * under an alternate open source license please contact
 * Landon Fuller <landonf@threerings.net>.
 */
/*  
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

#ifndef __TLS_H_
#define __TLS_H_

/*
 * Opaque TLS Context Structure.
 * New TLS Connections are manufactured from this context.
 */
typedef struct TLS_Context TLS_CONTEXT;

/* Opaque TLS Connection Structure */
typedef struct TLS_Connection TLS_CONNECTION;

/* PEM Decryption Passphrase Callback */
typedef int (TLS_PEM_PASSWD_CB) (char *buf, int size, const void *userdata);

#endif /* __TLS_H_ */
