/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2009 Free Software Foundation Europe e.V.

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

#ifndef _BACULA_XATTR_
#define _BACULA_XATTR_

/*
 * Magic used in the magic field of the xattr struct.
 * This way we can see we encounter a valid xattr struct.
 */
#define XATTR_MAGIC 0x5C5884

/*
 * Internal representation of an extended attribute.
 */
typedef struct xattr {
   uint32_t magic;
   uint32_t name_length;
   char *name;
   uint32_t value_length;
   char *value;
} xattr_t;

/*
 * Internal representation of an extended attribute hardlinked file.
 */
typedef struct xattr_link_cache_entry {
   uint32_t inum;
   char target[PATH_MAX];
   struct xattr_link_cache_entry *next;
} xattr_link_cache_entry_t;

/*
 * Maximum size of the XATTR stream this prevents us from blowing up the filed.
 */
#define MAX_XATTR_STREAM  (1 * 1024 * 1024) /* 1 Mb */

#endif
