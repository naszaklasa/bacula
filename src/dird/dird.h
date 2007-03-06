/*
 * Includes specific to the Director
 *
 *     Kern Sibbald, December MM
 *
 *    Version $Id: dird.h,v 1.13 2006/11/21 13:20:08 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

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

#include "lib/runscript.h"
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
