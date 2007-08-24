/*
 * Properties we use for getting and setting ACLs.
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

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

#ifndef _BACULA_ACL_
#define _BACULA_ACL_

/* For shorter ACL strings when possible, define BACL_WANT_SHORT_ACLS */
/* #define BACL_WANT_SHORT_ACLS */

/* For numeric user/group ids when possible, define BACL_WANT_NUMERIC_IDS */
/* #define BACL_WANT_NUMERIC_IDS */

/* We support the following types of ACLs */
#define BACL_TYPE_NONE        0x000
#define BACL_TYPE_ACCESS      0x001
#define BACL_TYPE_DEFAULT     0x002

#define BACL_CAP_NONE         0x000    /* No special capabilities */
#define BACL_CAP_DEFAULTS     0x001    /* Has default ACLs for directories */
#define BACL_CAP_DEFAULTS_DIR 0x002    /* Default ACLs must be read separately */

/* Set BACL_CAP (always) and BACL_ENOTSUP (when used) for various OS */
#if defined(HAVE_FREEBSD_OS)
#define BACL_CAP              (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_DARWIN_OS)
#define BACL_CAP              BACL_CAP_NONE
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_HPUX_OS)
#define BACL_CAP              BACL_CAP_NONE
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_IRIX_OS)
#define BACL_CAP              (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP          ENOSYS
#elif defined(HAVE_LINUX_OS) 
#define BACL_CAP              (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#define BACL_ENOTSUP          ENOTSUP
#elif defined(HAVE_OSF1_OS)
#define BACL_CAP              (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
/* #define BACL_ENOTSUP       ENOTSUP */     /* Don't know */
#define BACL_CAP              (BACL_CAP_DEFAULTS|BACL_CAP_DEFAULTS_DIR)
#elif defined(HAVE_SUN_OS)
#define BACL_CAP              BACL_CAP_DEFAULTS
#else
#define BACL_CAP              BACL_CAP_NONE  /* nothing special */
#endif

#endif
