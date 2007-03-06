/*
 * Includes specific to the Director
 *
 *     Kern Sibbald, December MM
 *
 *    Version $Id: dird.h,v 1.7.14.1 2005/04/05 17:23:54 kerns Exp $
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

#include "dird_conf.h"

#define DIRECTOR_DAEMON 1

#include "cats/cats.h"

#include "jcr.h"
#include "bsr.h"
#include "ua.h"
#include "protos.h"

#include "jobq.h"

/* Globals that dird.c exports */
extern DIRRES *director;                     /* Director resource */
extern int FDConnectTimeout;
extern int SDConnectTimeout;

/* From job.c */
void dird_free_jcr(JCR *jcr);
void dird_free_jcr_pointers(JCR *jcr);
