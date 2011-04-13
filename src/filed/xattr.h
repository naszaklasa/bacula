/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2009 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#ifndef __XATTR_H
#define __XATTR_H

/*
 * Magic used in the magic field of the xattr struct.
 * This way we can see we encounter a valid xattr struct.
 */
#define XATTR_MAGIC 0x5C5884

/*
 * Internal representation of an extended attribute.
 */
struct xattr_t {
   uint32_t magic;
   uint32_t name_length;
   char *name;
   uint32_t value_length;
   char *value;
};

/*
 * Internal representation of an extended attribute hardlinked file.
 */
struct xattr_link_cache_entry_t {
   uint32_t inum;
   char target[PATH_MAX];
};

/*
 * Internal tracking data.
 */
struct xattr_data_t {
   POOLMEM *content;
   uint32_t content_length;
   uint32_t nr_errors;
   uint32_t nr_saved;
   alist *link_cache;
};

/*
 * Maximum size of the XATTR stream this prevents us from blowing up the filed.
 */
#define MAX_XATTR_STREAM  (1 * 1024 * 1024) /* 1 Mb */

/*
 * Upperlimit on a xattr internal buffer
 */
#define XATTR_BUFSIZ	1024

#endif
