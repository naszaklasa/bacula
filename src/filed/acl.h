/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * Properties we use for getting and setting ACLs.
 */

#ifndef _BACULA_ACL_
#define _BACULA_ACL_

/* For shorter ACL strings when possible, define BACL_WANT_SHORT_ACLS */
/* #define BACL_WANT_SHORT_ACLS */

/* For numeric user/group ids when possible, define BACL_WANT_NUMERIC_IDS */
/* #define BACL_WANT_NUMERIC_IDS */

/*
 * We support the following types of ACLs
 */
typedef enum {
   BACL_TYPE_NONE = 0,
   BACL_TYPE_ACCESS = 1,
   BACL_TYPE_DEFAULT = 2,
   BACL_TYPE_DEFAULT_DIR = 3,
   BACL_TYPE_EXTENDED = 4
} bacl_type;

/*
 * This value is used as ostype when we encounter a invalid acl type.
 * The way the code is build this should never happen.
 */
#if !defined(ACL_TYPE_NONE)
#define ACL_TYPE_NONE 0x0
#endif

#if defined(HAVE_FREEBSD_OS)
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_DARWIN_OS)
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_HPUX_OS)
#define BACL_ENOTSUP          EOPNOTSUPP
#elif defined(HAVE_IRIX_OS)
#define BACL_ENOTSUP          ENOSYS
#elif defined(HAVE_LINUX_OS) 
#define BACL_ENOTSUP          ENOTSUP
#endif

#endif
