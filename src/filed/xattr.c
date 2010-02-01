/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2008-2009 Free Software Foundation Europe e.V.

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
 * Functions to handle Extended Attributes for bacula.
 *
 * Extended Attributes are so OS specific we only restore Extended Attributes if
 * they were saved using a filed on the same platform.
 *
 * Currently we support the following OSes:
 *   - FreeBSD (Extended Attributes)
 *   - Darwin (Extended Attributes)
 *   - Linux (Extended Attributes)
 *   - NetBSD (Extended Attributes)
 *   - Solaris (Extended Attributes and Extensible Attributes)
 *
 *   Written by Marco van Wieringen, November MMVIII
 *
 *   Version $Id$
 */

#include "bacula.h"
#include "filed.h"
#include "xattr.h"

#if !defined(HAVE_XATTR)
/*
 * Entry points when compiled without support for XATTRs or on an unsupported platform.
 */
bool build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return false;
}

bool parse_xattr_stream(JCR *jcr, int stream)
{
   return false;
}
#else
/*
 * Send a XATTR stream to the SD.
 */
static bool send_xattr_stream(JCR *jcr, int stream)
{
   BSOCK *sd = jcr->store_bsock;
   POOLMEM *msgsave;
#ifdef FD_NO_SEND_TEST
   return true;
#endif

   /*
    * Sanity check
    */
   if (jcr->xattr_data_len <= 0)
      return true;

   /*
    * Send header
    */
   if (!sd->fsend("%ld %d 0", jcr->JobFiles, stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return false;
   }

   /*
    * Send the buffer to the storage deamon
    */
   Dmsg1(400, "Backing up XATTR <%s>\n", jcr->xattr_data);
   msgsave = sd->msg;
   sd->msg = jcr->xattr_data;
   sd->msglen = jcr->xattr_data_len;
   if (!sd->send()) {
      sd->msg = msgsave;
      sd->msglen = 0;
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return false;
   }

   jcr->JobBytes += sd->msglen;
   sd->msg = msgsave;
   if (!sd->signal(BNET_EOD)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return false;
   }
   Dmsg1(200, "XATTR of file: %s successfully backed up!\n", jcr->last_fname);

   return true;
}

/*
 * This is a supported OS, See what kind of interface we should use.
 * Start with the generic interface used by most OS-es.
 */
#if defined(HAVE_DARWIN_OS) \
   || defined(HAVE_FREEBSD_OS) \
   || defined(HAVE_LINUX_OS) \
   || defined(HAVE_NETBSD_OS)
       
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

/*
 * OSX doesn't have llistxattr, lgetxattr and lsetxattr but has
 * listxattr, getxattr and setxattr with an extra options argument
 * which mimics the l variants of the functions when we specify
 * XATTR_NOFOLLOW as the options value.
 */
#if defined(HAVE_DARWIN_OS)
   #define llistxattr(path, list, size) listxattr((path), (list), (size), XATTR_NOFOLLOW)
   #define lgetxattr(path, name, value, size) getxattr((path), (name), (value), (size), 0, XATTR_NOFOLLOW)
   #define lsetxattr(path, name, value, size, flags) setxattr((path), (name), (value), (size), (flags), XATTR_NOFOLLOW)
#else
   /*
    * Fallback to the non l-functions when those are not available.
    */
   #if defined(HAVE_GETXATTR) && !defined(HAVE_LGETXATTR)
   #define lgetxattr getxattr
   #endif
   #if defined(HAVE_SETXATTR) && !defined(HAVE_LSETXATTR)
   #define lsetxattr setxattr
   #endif
   #if defined(HAVE_LISTXATTR) && !defined(HAVE_LLISTXATTR)
   #define llistxattr listxattr
   #endif
#endif

static void xattr_drop_internal_table(xattr_t *xattr_value_list)
{
   xattr_t *current_xattr;

   /*
    * Walk the list of xattrs and free allocated memory on traversing.
    */
   for (current_xattr = xattr_value_list;
        current_xattr != (xattr_t *)NULL;
        current_xattr++) {
      /*
       * See if we can shortcut.
       */
      if (current_xattr->magic != XATTR_MAGIC)
         break;

      free(current_xattr->name);

      if (current_xattr->value_length > 0)
         free(current_xattr->value);
   }

   /*
    * Free the array of control structs.
    */
   free(xattr_value_list);
}

/*
 * The xattr stream for OSX, FreeBSD, Linux and NetBSD is a serialized stream of bytes
 * which encodes one or more xattr_t structures.
 *
 * The Serialized stream consists of the following elements:
 *    magic - A magic string which makes it easy to detect any binary incompatabilites
 *    name_length - The length of the following xattr name
 *    name - The name of the extended attribute
 *    value_length - The length of the following xattr data
 *    value - The actual content of the extended attribute
 *
 * This is repeated 1 or more times.
 * 
 */
static uint32_t serialize_xattr_stream(JCR *jcr, uint32_t expected_serialize_len, xattr_t *xattr_value_list)
{
   xattr_t *current_xattr;
   ser_declare;

   /*
    * Make sure the serialized stream fits in the poolmem buffer.
    * We allocate some more to be sure the stream is gonna fit.
    */
   jcr->xattr_data = check_pool_memory_size(jcr->xattr_data, expected_serialize_len + 10);
   ser_begin(jcr->xattr_data, expected_serialize_len + 10);

   /*
    * Walk the list of xattrs and serialize the data.
    */
   for (current_xattr = xattr_value_list; current_xattr != (xattr_t *)NULL; current_xattr++) {
      /*
       * See if we can shortcut.
       */
      if (current_xattr->magic != XATTR_MAGIC)
         break;

      ser_uint32(current_xattr->magic);
      ser_uint32(current_xattr->name_length);
      ser_bytes(current_xattr->name, current_xattr->name_length);

      ser_uint32(current_xattr->value_length);
      ser_bytes(current_xattr->value, current_xattr->value_length);
   }

   ser_end(jcr->xattr_data, expected_serialize_len + 10);
   jcr->xattr_data_len = ser_length(jcr->xattr_data);

   return jcr->xattr_data_len;
}

static bool generic_xattr_build_streams(JCR *jcr, FF_PKT *ff_pkt, int stream)
{
   int count = 0;
   int32_t xattr_list_len,
           xattr_value_len;
   uint32_t expected_serialize_len = 0;
   char *xattr_list, *bp;
   xattr_t *xattr_value_list, *current_xattr;

   /*
    * First get the length of the available list with extended attributes.
    */
   xattr_list_len = llistxattr(jcr->last_fname, NULL, 0);
   if (xattr_list_len < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("llistxattr error on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg2(100, "llistxattr error file=%s ERR=%s\n",
         jcr->last_fname, be.bstrerror());
      return true;                       /* non-fatal return */
   } else if (xattr_list_len == 0) {
      return true;
   }

   /*
    * Allocate room for the extented attribute list.
    */
   xattr_list = (char *)malloc(xattr_list_len + 1);
   memset((caddr_t)xattr_list, 0, xattr_list_len + 1);

   /*
    * Get the actual list of extended attributes names for a file.
    */
   xattr_list_len = llistxattr(jcr->last_fname, xattr_list, xattr_list_len);
   if (xattr_list_len < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("llistxattr error on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg2(100, "llistxattr error file=%s ERR=%s\n",
         jcr->last_fname, be.bstrerror());
      free(xattr_list);
      return true;                    /* non-fatal return */
   }
   xattr_list[xattr_list_len] = '\0';

   /*
    * Count the number of extended attributes on a file.
    */
   bp = xattr_list;
   while ((bp - xattr_list) + 1 < xattr_list_len) {
#if defined(HAVE_LINUX_OS)
      /*
       * On Linux you also get the acls in the extented attribute list.
       * So we check if we are already backing up acls and if we do we
       * don't store the extended attribute with the same info.
       */
      if ((ff_pkt->flags & FO_ACL) == 0 || strcmp(bp, "system.posix_acl_access"))
         count++;
#else
      count++;
#endif

      bp = strchr(bp, '\0') + 1;
   }

   if (count == 0) {
      free(xattr_list);

      return true;
   }

   /*
    * Allocate enough room to hold all extended attributes.
    * After allocating the storage make sure its empty by zeroing it.
    */
   xattr_value_list = (xattr_t *)malloc(count * sizeof(xattr_t));
   memset((caddr_t)xattr_value_list, 0, count * sizeof(xattr_t));

   /*
    * Walk the list of extended attributes names and retrieve the data.
    * We already count the bytes needed for serializing the stream later on.
    */
   current_xattr = xattr_value_list;
   bp = xattr_list;
   while ((bp - xattr_list) + 1 < xattr_list_len) {
#if defined(HAVE_LINUX_OS)
      if (ff_pkt->flags & FO_ACL && !strcmp(bp, "system.posix_acl_access")) {
         bp = strchr(bp, '\0') + 1;
         continue;
      }
#endif

      /*
       * Each xattr valuepair starts with a magic so we can parse it easier.
       */
      current_xattr->magic = XATTR_MAGIC;
      expected_serialize_len += sizeof(current_xattr->magic);

      /*
       * Allocate space for storing the name.
       */
      current_xattr->name_length = strlen(bp);
      current_xattr->name = (char *)malloc(current_xattr->name_length);
      memcpy((caddr_t)current_xattr->name, (caddr_t)bp, current_xattr->name_length);

      expected_serialize_len += sizeof(current_xattr->name_length) + current_xattr->name_length;

      /*
       * First see how long the value is for the extended attribute.
       */
      xattr_value_len = lgetxattr(jcr->last_fname, bp, NULL, 0);
      if (xattr_value_len < 0) {
         berrno be;
         Jmsg2(jcr, M_ERROR, 0, _("lgetxattr error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
         Dmsg2(100, "lgetxattr error file=%s ERR=%s\n",
            jcr->last_fname, be.bstrerror());
         goto bail_out;
      }

      /*
       * Allocate space for storing the value.
       */
      current_xattr->value = (char *)malloc(xattr_value_len);
      memset((caddr_t)current_xattr->value, 0, xattr_value_len);

      xattr_value_len = lgetxattr(jcr->last_fname, bp, current_xattr->value, xattr_value_len);
      if (xattr_value_len < 0) {
         berrno be;
         Jmsg2(jcr, M_ERROR, 0, _("lgetxattr error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
         Dmsg2(100, "lgetxattr error file=%s ERR=%s\n",
            jcr->last_fname, be.bstrerror());
         goto bail_out;
      }

      /*
       * Store the actual length of the value.
       */
      current_xattr->value_length = xattr_value_len;
      expected_serialize_len += sizeof(current_xattr->value_length) + current_xattr->value_length;

      /*
       * Protect ourself against things getting out of hand.
       */
      if (expected_serialize_len >= MAX_XATTR_STREAM) {
         Jmsg2(jcr, M_ERROR, 0, _("Xattr stream on file \"%s\" exceeds maximum size of %d bytes\n"),
            jcr->last_fname, MAX_XATTR_STREAM);
         goto bail_out;
      }
      
      /*
       * Next attribute.
       */
      current_xattr++;
      bp = strchr(bp, '\0') + 1;
   }

   /*
    * Serialize the datastream.
    */
   if (serialize_xattr_stream(jcr, expected_serialize_len, xattr_value_list) < expected_serialize_len) {
      Jmsg1(jcr, M_ERROR, 0, _("Failed to serialize extended attributes on file \"%s\"\n"),
         jcr->last_fname);
      Dmsg1(100, "Failed to serialize extended attributes on file \"%s\"\n",
         jcr->last_fname);
      goto bail_out;
   }

   xattr_drop_internal_table(xattr_value_list);
   free(xattr_list);

   /*
    * Send the datastream to the SD.
    */
   return send_xattr_stream(jcr, stream);

bail_out:
   xattr_drop_internal_table(xattr_value_list);
   free(xattr_list);
   return true;                    /* non-fatal return */
}

static bool generic_xattr_parse_streams(JCR *jcr)
{
   unser_declare;
   xattr_t current_xattr;
   bool retval = true;                /* default non-fatal */

   /*
    * Parse the stream and perform the setxattr calls on the file.
    *
    * Start unserializing the data. We keep on looping while we have not
    * unserialized all bytes in the stream.
    */
   unser_begin(jcr->xattr_data, jcr->xattr_data_len);
   while (unser_length(jcr->xattr_data) < jcr->xattr_data_len) {
      /*
       * First make sure the magic is present. This way we can easily catch corruption.
       * Any missing MAGIC is fatal we do NOT try to continue.
       */
      unser_uint32(current_xattr.magic);
      if (current_xattr.magic != XATTR_MAGIC) {
         Jmsg1(jcr, M_ERROR, 0, _("Illegal xattr stream, no XATTR_MAGIC on file \"%s\"\n"),
            jcr->last_fname);
         Dmsg1(100, "Illegal xattr stream, no XATTR_MAGIC on file \"%s\"\n",
            jcr->last_fname);
         return true;                 /* non-fatal return */
      }

      /*
       * Decode the valuepair. First decode the length of the name.
       */
      unser_uint32(current_xattr.name_length);
      
      /*
       * Allocate room for the name and decode its content.
       */
      current_xattr.name = (char *)malloc(current_xattr.name_length + 1);
      unser_bytes(current_xattr.name, current_xattr.name_length);

      /*
       * The xattr_name needs to be null terminated for lsetxattr.
       */
      current_xattr.name[current_xattr.name_length] = '\0';

      /*
       * Decode the value length.
       */
      unser_uint32(current_xattr.value_length);

      /*
       * Allocate room for the value and decode its content.
       */
      current_xattr.value = (char *)malloc(current_xattr.value_length);
      unser_bytes(current_xattr.value, current_xattr.value_length);

      /*
       * Try to set the extended attribute on the file.
       * If we fail to set this attribute we flag the error but its not fatal,
       * we try to restore the other extended attributes too.
       */
      if (lsetxattr(jcr->last_fname, current_xattr.name, current_xattr.value,
         current_xattr.value_length, 0) != 0) {
         berrno be;
         Jmsg2(jcr, M_ERROR, 0, _("lsetxattr error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
         Dmsg2(100, "lsetxattr error file=%s ERR=%s\n",
            jcr->last_fname, be.bstrerror());
      }

      /*
       * Free the temporary buffers.
       */
      free(current_xattr.name);
      free(current_xattr.value);
   }

   unser_end(jcr->xattr_data, jcr->xattr_data_len);
   return retval;
}

#if defined(HAVE_DARWIN_OS)
static bool darwin_build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return generic_xattr_build_streams(jcr, ff_pkt, STREAM_XATTR_DARWIN);
}

static bool darwin_parse_xattr_stream(JCR *jcr, int stream)
{
   switch (stream) {
   case STREAM_XATTR_DARWIN:
      return generic_xattr_parse_streams(jcr);
   default:
      Jmsg2(jcr, M_WARNING, 0,
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;                    /* non-fatal error */
   }
}
#elif defined(HAVE_FREEBSD_OS)
static bool freebsd_build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return generic_xattr_build_streams(jcr, ff_pkt, STREAM_XATTR_FREEBSD);
}

static bool freebsd_parse_xattr_stream(JCR *jcr, int stream)
{
   switch (stream) {
   case STREAM_XATTR_FREEBSD:
      return generic_xattr_parse_streams(jcr);
   default:
      Jmsg2(jcr, M_WARNING, 0, 
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;                    /* non-fatal error */
   }
}
#elif defined(HAVE_LINUX_OS)
static bool linux_build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return generic_xattr_build_streams(jcr, ff_pkt, STREAM_XATTR_LINUX);
}

static bool linux_parse_xattr_stream(JCR *jcr, int stream)
{
   switch (stream) {
   case STREAM_XATTR_LINUX:
      return generic_xattr_parse_streams(jcr);
   default:
      Jmsg2(jcr, M_WARNING, 0, 
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;                    /* non-fatal error */
   }
}
#elif defined(HAVE_NETBSD_OS)
static bool netbsd_build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return generic_xattr_build_streams(jcr, ff_pkt, STREAM_XATTR_NETBSD);
}

static bool netbsd_parse_xattr_stream(JCR *jcr, int stream)
{
   switch (stream) {
   case STREAM_XATTR_NETBSD:
      return generic_xattr_parse_streams(jcr);
   default:
      Jmsg2(jcr, M_WARNING, 0, 
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;                    /* non-fatal error */
   }
}
#endif
#elif defined(HAVE_SUN_OS)
/*
 * Solaris extended attributes were introduced in Solaris 9
 * by PSARC 1999/209
 *
 * Solaris extensible attributes were introduced in OpenSolaris
 * by PSARC 2007/315 Solaris extensible attributes are also
 * sometimes called extended system attributes.
 *
 * man fsattr(5) on Solaris gives a wealth of info. The most
 * important bits are:
 *
 * Attributes are logically supported as files within the  file
 * system.   The  file  system  is  therefore augmented with an
 * orthogonal name space of file attributes. Any file  (includ-
 * ing  attribute files) can have an arbitrarily deep attribute
 * tree associated with it. Attribute values  are  accessed  by
 * file descriptors obtained through a special attribute inter-
 * face.  This logical view of "attributes as files" allows the
 * leveraging  of  existing file system interface functionality
 * to support the construction, deletion, and  manipulation  of
 * attributes.
 *
 * The special files  "."  and  ".."  retain  their  accustomed
 * semantics within the attribute hierarchy.  The "." attribute
 * file refers to the current directory and the ".."  attribute
 * file  refers to the parent directory.  The unnamed directory
 * at the head of each attribute tree is considered the "child"
 * of  the  file it is associated with and the ".." file refers
 * to the associated file.  For  any  non-directory  file  with
 * attributes,  the  ".." entry in the unnamed directory refers
 * to a file that is not a directory.
 *
 * Conceptually, the attribute model is fully general. Extended
 * attributes  can  be  any  type of file (doors, links, direc-
 * tories, and so forth) and can even have their own attributes
 * (fully  recursive).   As a result, the attributes associated
 * with a file could be an arbitrarily deep directory hierarchy
 * where each attribute could have an equally complex attribute
 * tree associated with it.  Not all implementations  are  able
 * to,  or  want to, support the full model. Implementation are
 * therefore permitted to reject operations that are  not  sup-
 * ported.   For  example,  the implementation for the UFS file
 * system allows only regular files as attributes (for example,
 * no sub-directories) and rejects attempts to place attributes
 * on attributes.
 *
 * The following list details the operations that are  rejected
 * in the current implementation:
 *
 * link                     Any attempt to create links between
 *                          attribute  and  non-attribute space
 *                          is rejected  to  prevent  security-
 *                          related   or   otherwise  sensitive
 *                          attributes from being exposed,  and
 *                          therefore  manipulable,  as regular
 *                          files.
 *
 * rename                   Any  attempt  to   rename   between
 *                          attribute  and  non-attribute space
 *                          is rejected to prevent  an  already
 *                          linked  file from being renamed and
 *                          thereby circumventing the link res-
 *                          triction above.
 *
 * mkdir, symlink, mknod    Any  attempt  to  create  a   "non-
 *                          regular" file in attribute space is
 *                          rejected to reduce the  functional-
 *                          ity,  and  therefore  exposure  and
 *                          risk, of  the  initial  implementa-
 *                          tion.
 *
 * The entire available name space has been allocated to  "gen-
 * eral use" to bring the implementation in line with the NFSv4
 * draft standard [NFSv4]. That standard defines "named  attri-
 * butes"  (equivalent  to Solaris Extended Attributes) with no
 * naming restrictions.  All Sun  applications  making  use  of
 * opaque extended attributes will use the prefix "SUNW".
 *
 */
#ifdef HAVE_SYS_ATTR_H
#include <sys/attr.h>
#endif

#ifdef HAVE_ATTR_H
#include <attr.h>
#endif

#ifdef HAVE_SYS_NVPAIR_H
#include <sys/nvpair.h>
#endif

#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif

/*
 * This is the count of xattrs saved on a certain file, it gets reset
 * on each new file processed and is used to see if we need to send
 * the hidden xattr dir data. We only send that data when we encounter
 * an other xattr on the file.
 */
static int nr_xattr_saved = 0;
static char toplevel_hidden_dir_xattr_data[MAXSTRING];
static int toplevel_hidden_dir_xattr_data_len;

/*
 * This code creates a temporary cache with entries for each xattr which has
 * a link count > 1 (which indicates it has one or more hard linked counterpart(s))
 */
static xattr_link_cache_entry_t *xattr_link_cache_head = NULL,
                                *xattr_link_cache_tail = NULL;

static struct xattr_link_cache_entry *find_xattr_link_cache_entry(ino_t inum)
{
   xattr_link_cache_entry_t *ptr;

   for (ptr = xattr_link_cache_head; ptr != NULL; ptr = ptr->next) {
      if (ptr->inum == inum) {
         return ptr;
      }
   }
   return NULL;
}

static void add_xattr_link_cache_entry(ino_t inum, char *target)
{
   xattr_link_cache_entry_t *ptr;

   ptr = (xattr_link_cache_entry_t *)malloc(sizeof(struct xattr_link_cache_entry));
   memset((caddr_t)ptr, 0, sizeof(struct xattr_link_cache_entry));
   ptr->inum = inum;
   strncpy(ptr->target, target, sizeof(ptr->target));
   if (xattr_link_cache_head == NULL) {
      xattr_link_cache_head = ptr;
   }
   if (xattr_link_cache_tail != NULL) {
      xattr_link_cache_tail->next = ptr;
   }
}

static void drop_xattr_link_cache(void)
{
   xattr_link_cache_entry_t *ptr, *next;

   for (ptr = xattr_link_cache_tail; ptr != NULL; ptr = next) {
      next = ptr->next;
      free(ptr);
   }
   xattr_link_cache_head = NULL;
   xattr_link_cache_tail = NULL;
}

#if defined(HAVE_SYS_NVPAIR_H) && defined(_PC_SATTR_ENABLED)
/*
 * This function returns true if a non default extended system attribute
 * list is associated with fd and returns false when an error has occured
 * or when only extended system attributes other than archive,
 * av_modified or crtime are set.
 *
 * The function returns true for the following cases:
 *
 * - any extended system attribute other than the default attributes
 *   ('archive', 'av_modified' and 'crtime') is set
 * - nvlist has NULL name string
 * - nvpair has data type of 'nvlist'
 * - default data type.
 */
static bool solaris_has_non_transient_extensible_attributes(int fd)
{
   boolean_t value;
   data_type_t type;
   nvlist_t *response;
   nvpair_t *pair;
   f_attr_t fattr;
   char *name;
   bool retval = false;

   if (fgetattr(fd, XATTR_VIEW_READWRITE, &response) != 0) {
      return false;
   }

   pair = NULL;
   while ((pair = nvlist_next_nvpair(response, pair)) != NULL) {
      name = nvpair_name(pair);

      if (name != NULL) {
         fattr = name_to_attr(name);
      } else {
         retval = true;
         goto cleanup;
      }

      type = nvpair_type(pair);
      switch (type) {
      case DATA_TYPE_BOOLEAN_VALUE:
         if (nvpair_value_boolean_value(pair, &value) != 0) {
            continue;
         }
         if (value && fattr != F_ARCHIVE &&
                      fattr != F_AV_MODIFIED) {
            retval = true;
            goto cleanup;
         }
         break;
      case DATA_TYPE_UINT64_ARRAY:
         if (fattr != F_CRTIME) {
            retval = true;
            goto cleanup;
         }
         break;
      case DATA_TYPE_NVLIST:
      default:
         retval = true;
         goto cleanup;
      }
   }

cleanup:
   if (response != NULL) {
      nvlist_free(response);
   }
   return retval;
}
#endif /* HAVE_SYS_NVPAIR_H && _PC_SATTR_ENABLED */

#if defined(HAVE_ACL) && !defined(HAVE_EXTENDED_ACL)
/*
 * See if an acl is a trivial one (e.g. just the stat bits encoded as acl.)
 * There is no need to store those acls as we already store the stat bits too.
 */
static bool acl_is_trivial(int count, aclent_t *entries)
{
   int n;
   aclent_t *ace;

   for (n = 0; n < count; n++) {
      ace = &entries[n];
      if (!(ace->a_type == USER_OBJ ||
            ace->a_type == GROUP_OBJ ||
            ace->a_type == OTHER_OBJ ||
            ace->a_type == CLASS_OBJ))
        return false;
   }
   return true;
}
#endif /* HAVE_ACL && !HAVE_EXTENDED_ACL */

static bool solaris_save_xattr_acl(JCR *jcr, int fd, const char *attrname, char **acl_text)
{
#ifdef HAVE_ACL
#ifdef HAVE_EXTENDED_ACL
   int flags;
   acl_t *aclp = NULL;

   /*
    * See if this attribute has an ACL
    */
   if ((fd != -1 && fpathconf(fd, _PC_ACL_ENABLED) > 0) ||
       pathconf(attrname, _PC_ACL_ENABLED) > 0) {
      /*
       * See if there is a non trivial acl on the file.
       */
      if ((fd != -1 && facl_get(fd, ACL_NO_TRIVIAL, &aclp) != 0) ||
           acl_get(attrname, ACL_NO_TRIVIAL, &aclp) != 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to get acl on xattr %s on file \"%s\": ERR=%s\n"),
            attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "facl_get/acl_get of xattr %s on \"%s\" failed: ERR=%s\n",
            attrname, jcr->last_fname, be.bstrerror());
         return true;                    /* non-fatal */
      }

      if (aclp != NULL) {
#if defined(ACL_SID_FMT)
         /*
          * New format flag added in newer Solaris versions.
          */
         flags = ACL_APPEND_ID | ACL_COMPACT_FMT | ACL_SID_FMT;
#else
         flags = ACL_APPEND_ID | ACL_COMPACT_FMT;
#endif /* ACL_SID_FMT */

         *acl_text = acl_totext(aclp, flags);
         acl_free(aclp);
      } else {
         *acl_text = NULL;
      }
   } else {
      *acl_text = NULL;
   }

   return true;
#else /* HAVE_EXTENDED_ACL */
   int n;
   aclent_t *acls = NULL;

   /*
    * See if this attribute has an ACL
    */
   if (fd != -1) {
      n = facl(fd, GETACLCNT, 0, NULL);
   } else {
      n = acl(attrname, GETACLCNT, 0, NULL);
   }

   if (n >= MIN_ACL_ENTRIES) {
      acls = (aclent_t *)malloc(n * sizeof(aclent_t));
      if ((fd != -1 && facl(fd, GETACL, n, acls) != n) ||
          acl(attrname, GETACL, n, acls) != n) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to get acl on xattr %s on file \"%s\": ERR=%s\n"),
            attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "facl/acl of xattr %s on \"%s\" failed: ERR=%s\n",
            attrname, jcr->last_fname, be.bstrerror());
         free(acls);
         return true;                 /* non-fatal */
      }

      /*
       * See if there is a non trivial acl on the file.
       */
      if (!acl_is_trivial(n, acls)) {
         if ((*acl_text = acltotext(acls, n)) == NULL) {
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Unable to get acl text on xattr %s on file \"%s\": ERR=%s\n"),
               attrname, jcr->last_fname, be.bstrerror());
            Dmsg3(100, "acltotext of xattr %s on \"%s\" failed: ERR=%s\n",
               attrname, jcr->last_fname, be.bstrerror());
            free(acls);
            return true;                 /* non-fatal */
         }
      } else {
         *acl_text = NULL;
      }

     free(acls);
   } else {
      *acl_text = NULL;
   }

   return true;
#endif /* HAVE_EXTENDED_ACL */

#else /* HAVE_ACL */
   return false;                         /* fatal return */
#endif /* HAVE_ACL */
}

/*
 * Forward declaration for recursive function call.
 */
static bool solaris_save_xattrs(JCR *jcr, const char *xattr_namespace, const char *attr_parent);

/*
 * Save an extended or extensible attribute.
 * This is stored as an opaque stream of bytes with the following encoding:
 *
 * <xattr_name>\0<stat_buffer>\0<acl_string>\0<actual_xattr_data>
 * 
 * or for a hardlinked or symlinked attribute
 *
 * <xattr_name>\0<stat_buffer>\0<xattr_link_source>\0
 *
 * xattr_name can be a subpath relative to the file the xattr is on.
 * stat_buffer is the string representation of the stat struct.
 * acl_string is an acl text when a non trivial acl is set on the xattr.
 * actual_xattr_data is the content of the xattr file.
 */
static bool solaris_save_xattr(JCR *jcr, int fd, const char *xattr_namespace,
                                  const char *attrname, bool toplevel_hidden_dir, int stream)
{
   int cnt;
   int attrfd = -1;
   struct stat st;
   struct xattr_link_cache_entry *xlce;
   char target_attrname[PATH_MAX];
   char link_source[PATH_MAX];
   char *acl_text = NULL;
   char attribs[MAXSTRING];
   char buffer[BUFSIZ];
   bool retval = true;                /* default is non-fatal */

   snprintf(target_attrname, sizeof(target_attrname), "%s%s", xattr_namespace, attrname);

   /*
    * Get the stats of the extended or extensible attribute.
    */
   if (fstatat(fd, attrname, &st, AT_SYMLINK_NOFOLLOW) < 0) {
      berrno be;
      Jmsg3(jcr, M_ERROR, 0, _("Unable to get status on xattr %s on file \"%s\": ERR=%s\n"),
         target_attrname, jcr->last_fname, be.bstrerror());
      Dmsg3(100, "fstatat of xattr %s on \"%s\" failed: ERR=%s\n",
         target_attrname, jcr->last_fname, be.bstrerror());
      goto cleanup;
   }

   /*
    * Based on the filetype perform the correct action. We support most filetypes here, more
    * then the actual implementation on Solaris supports so some code may never get executed
    * due to limitations in the implementation.
    */
   switch (st.st_mode & S_IFMT) {
   case S_IFIFO:
   case S_IFCHR:
   case S_IFBLK:
      /*
       * Get any acl on the xattr.
       */
      if (!solaris_save_xattr_acl(jcr, attrfd, attrname, &acl_text))
         goto cleanup;

      /*
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       * Encode the stat struct into an ASCII representation.
       */
      encode_stat(attribs, &st, 0, stream);
      cnt = snprintf(buffer, sizeof(buffer), "%s%c%s%c%s%c",
                     target_attrname, 0, attribs, 0, (acl_text) ? acl_text : "", 0);
      break;

   case S_IFDIR:
      /*
       * Get any acl on the xattr.
       */
      if (!solaris_save_xattr_acl(jcr, attrfd, attrname, &acl_text))
         goto cleanup;

      /*
       * See if this is the toplevel_hidden_dir being saved.
       */
      if (toplevel_hidden_dir) {
         /*
          * Save the data for later storage when we encounter a real xattr.
          * Encode the stat struct into an ASCII representation and jump out of the function.
          */
         encode_stat(attribs, &st, 0, stream);
         toplevel_hidden_dir_xattr_data_len = snprintf(toplevel_hidden_dir_xattr_data,
                                                       sizeof(toplevel_hidden_dir_xattr_data),
                                                       "%s%c%s%c%s%c",
                                                       target_attrname, 0, attribs, 0,
                                                       (acl_text) ? acl_text : "", 0);
         goto cleanup;
      } else {
         /*
          * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
          * Encode the stat struct into an ASCII representation.
          */
         encode_stat(attribs, &st, 0, stream);
         cnt = snprintf(buffer, sizeof(buffer),
                        "%s%c%s%c%s%c",
                        target_attrname, 0, attribs, 0, (acl_text) ? acl_text : "", 0);
      }
      break;
   case S_IFREG:
      /*
       * If this is a hardlinked file check the inode cache for a hit.
       */
      if (st.st_nlink > 1) {
         /*
          * See if the cache already knows this inode number.
          */
         if ((xlce = find_xattr_link_cache_entry(st.st_ino)) != NULL) {
            /*
             * Generate a xattr encoding with the reference to the target in there.
             */
            encode_stat(attribs, &st, st.st_ino, stream);
            cnt = snprintf(buffer, sizeof(buffer),
                           "%s%c%s%c%s%c",
                           target_attrname, 0, attribs, 0, xlce->target, 0);
            pm_memcpy(jcr->xattr_data, buffer, cnt);
            jcr->xattr_data_len = cnt;
            retval = send_xattr_stream(jcr, stream);

            /*
             * For a hard linked file we are ready now, no need to recursively save the attributes.
             */
            goto cleanup;
         }

         /*
          * Store this hard linked file in the cache.
          * Store the name relative to the top level xattr space.
          */
         add_xattr_link_cache_entry(st.st_ino, target_attrname + 1);
      }

      /*
       * Get any acl on the xattr.
       */
      if (!solaris_save_xattr_acl(jcr, attrfd, attrname, &acl_text)) {
         goto cleanup;
      }

      /*
       * Encode the stat struct into an ASCII representation.
       */
      encode_stat(attribs, &st, 0, stream);
      cnt = snprintf(buffer, sizeof(buffer),
                     "%s%c%s%c%s%c",
                     target_attrname, 0, attribs, 0, (acl_text) ? acl_text : "", 0);

      /*
       * Open the extended or extensible attribute file.
       */
      if ((attrfd = openat(fd, attrname, O_RDONLY)) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to open xattr %s on \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "openat of xattr %s on \"%s\" failed: ERR=%s\n",
            target_attrname, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }
      break;

   case S_IFLNK:
      /*
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       * Encode the stat struct into an ASCII representation.
       */
      if (readlink(attrname, link_source, sizeof(link_source)) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to read symlin %s on \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "readlink of xattr %s on \"%s\" failed: ERR=%s\n",
            target_attrname, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }

      /*
       * Generate a xattr encoding with the reference to the target in there.
       */
      encode_stat(attribs, &st, st.st_ino, stream);
      cnt = snprintf(buffer, sizeof(buffer),
                     "%s%c%s%c%s%c",
                     target_attrname, 0, attribs, 0, link_source, 0);
      pm_memcpy(jcr->xattr_data, buffer, cnt);
      jcr->xattr_data_len = cnt;
      retval = send_xattr_stream(jcr, stream);

      /*
       * For a soft linked file we are ready now, no need to recursively save the attributes.
       */
      goto cleanup;

   default:
      goto cleanup;
   }

   /*
    * See if this is the first real xattr being saved. If it is save the toplevel_hidden_dir attributes first.
    */
   if (nr_xattr_saved == 0) {
      pm_memcpy(jcr->xattr_data, toplevel_hidden_dir_xattr_data, toplevel_hidden_dir_xattr_data_len);
      jcr->xattr_data_len = toplevel_hidden_dir_xattr_data_len;
      retval = send_xattr_stream(jcr, STREAM_XATTR_SOLARIS);
   }

   pm_memcpy(jcr->xattr_data, buffer, cnt);
   jcr->xattr_data_len = cnt;

   /*
    * Only dump the content of regular files.
    */
   switch (st.st_mode & S_IFMT) {
   case S_IFREG:
      if (st.st_size > 0) {
         /*
          * Protect ourself against things getting out of hand.
          */
         if (st.st_size >= MAX_XATTR_STREAM) {
            Jmsg2(jcr, M_ERROR, 0, _("Xattr stream on file \"%s\" exceeds maximum size of %d bytes\n"),
               jcr->last_fname, MAX_XATTR_STREAM);
            goto cleanup;
         }

         while ((cnt = read(attrfd, buffer, sizeof(buffer))) > 0) {
            jcr->xattr_data = check_pool_memory_size(jcr->xattr_data, jcr->xattr_data_len + cnt);
            memcpy(jcr->xattr_data + jcr->xattr_data_len, buffer, cnt);
            jcr->xattr_data_len += cnt;
         }

         if (cnt < 0) {
            Jmsg2(jcr, M_ERROR, 0, _("Unable to read content of xattr %s on file \"%s\"\n"),
               target_attrname, jcr->last_fname);
            Dmsg2(100, "read of data from xattr %s on \"%s\" failed\n",
               target_attrname, jcr->last_fname);
            goto cleanup;
         }
      }
      break;

   default:
      break;
   }

   if (retval) {
      retval = send_xattr_stream(jcr, stream);
      nr_xattr_saved++;
   }

   /*
    * Recursivly call solaris_save_extended_attributes for archiving the attributes
    * available on this extended attribute.
    */
   if (retval) {
      retval = solaris_save_xattrs(jcr, xattr_namespace, attrname);
      
      /*
       * The recursive call could change our working dir so change back to the wanted workdir.
       */
      if (fchdir(fd) < 0) {
         berrno be;
         Jmsg2(jcr, M_ERROR, 0, _("Unable to chdir to xattr space of file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to fchdir to xattr space of file \"%s\" using fd %d: ERR=%s\n",
            jcr->last_fname, fd, be.bstrerror());
         goto cleanup;
      }
   }

cleanup:
   if (acl_text) {
      free(acl_text);
   }
   if (attrfd != -1) {
      close(attrfd);
   }
   return retval;
}

static bool solaris_save_xattrs(JCR *jcr, const char *xattr_namespace, const char *attr_parent)
{
   const char *name;
   int fd, filefd = -1, attrdirfd = -1;
   DIR *dirp;
   struct dirent *dp;
   char current_xattr_namespace[PATH_MAX];
   bool retval = true;                   /* default non-fatal error */
 
   /*
    * Determine what argument to use. Use attr_parent when set
    * (recursive call) or jcr->last_fname for first call. Also save
    * the current depth of the xattr_space we are in.
    */
   if (attr_parent) {
      name = attr_parent;
      if (xattr_namespace) {
         snprintf(current_xattr_namespace, sizeof(current_xattr_namespace), "%s%s/",
                  xattr_namespace, attr_parent);
      } else {
         strncpy(current_xattr_namespace, "/", sizeof(current_xattr_namespace));
      }
   } else {
      name = jcr->last_fname;
      strncpy(current_xattr_namespace, "/", sizeof(current_xattr_namespace));
   }

   /*
    * Open the file on which to save the xattrs read-only.
    */
   if ((filefd = open(name, O_RDONLY | O_NONBLOCK)) < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to open file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg2(100, "Unable to open file \"%s\": ERR=%s\n",
         jcr->last_fname, be.bstrerror());
      goto cleanup;
   }

   /*
    * Open the xattr naming space.
    */
   if ((attrdirfd = openat(filefd, ".", O_RDONLY | O_XATTR)) < 0) {
      switch (errno) {
      case EINVAL:
         /*
          * Gentile way of the system saying this type of xattr layering is not supported.
          * Which is not problem we just forget about this this xattr.
          * But as this is not an error we return a positive return value.
          */
         retval = true;
         break;
      default:
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to open xattr space %s on file \"%s\": ERR=%s\n"),
            name, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to open xattr space %s on file \"%s\": ERR=%s\n",
            name, jcr->last_fname, be.bstrerror());
      }
      goto cleanup;
   }

  /*
   * We need to change into the attribute directory to determine if each of the
   * attributes should be saved.
   */
   if (fchdir(attrdirfd) < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to chdir to xattr space on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg3(100, "Unable to fchdir to xattr space on file \"%s\" using fd %d: ERR=%s\n",
         jcr->last_fname, attrdirfd, be.bstrerror());
      goto cleanup;
   }

   /*
    * Save the data of the toplevel xattr hidden_dir. We save this one before anything
    * else because the readdir returns "." entry after the extensible attr entry.
    * And as we want this entry before anything else we better just save its data.
    */
   if (!attr_parent)
      solaris_save_xattr(jcr, attrdirfd, current_xattr_namespace, ".",
                            true, STREAM_XATTR_SOLARIS);

   if ((fd = dup(attrdirfd)) == -1 ||
       (dirp = fdopendir(fd)) == (DIR *)NULL) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to list the xattr space on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg3(100, "Unable to fdopendir xattr space on file \"%s\" using fd %d: ERR=%s\n",
         jcr->last_fname, fd, be.bstrerror());

      goto cleanup;
   }

   /*
    * Walk the namespace.
    */
   while (dp = readdir(dirp)) {
      /*
       * Skip only the toplevel . dir.
       */
      if (!attr_parent && !strcmp(dp->d_name, "."))
         continue;

      /*
       * Skip all .. directories
       */
      if (!strcmp(dp->d_name, ".."))
         continue;

      Dmsg3(400, "processing extended attribute %s%s on file \"%s\"\n",
         current_xattr_namespace, dp->d_name, jcr->last_fname);

#if defined(HAVE_SYS_NVPAIR_H) && defined(_PC_SATTR_ENABLED)
      /*
       * We are not interested in read-only extensible attributes.
       */
      if (!strcmp(dp->d_name, VIEW_READONLY)) {
         Dmsg3(400, "Skipping readonly extensible attributes %s%s on file \"%s\"\n",
            current_xattr_namespace, dp->d_name, jcr->last_fname);

         continue;
      }

      /*
       * We are only interested in read-write extensible attributes
       * when they contain non-transient values.
       */
      if (!strcmp(dp->d_name, VIEW_READWRITE)) {
         /*
          * Determine if there are non-transient system attributes at the toplevel.
          * We need to provide a fd to the open file.
          */
         if (!solaris_has_non_transient_extensible_attributes(filefd)) {
            Dmsg3(400, "Skipping transient extensible attributes %s%s on file \"%s\"\n",
               current_xattr_namespace, dp->d_name, jcr->last_fname);
            continue;
         }

         /*
          * Save the xattr.
          */
         solaris_save_xattr(jcr, attrdirfd, current_xattr_namespace, dp->d_name,
                               false, STREAM_XATTR_SOLARIS_SYS);
         continue;
      }
#endif /* HAVE_SYS_NVPAIR_H && _PC_SATTR_ENABLED */

      /*
       * Save the xattr.
       */
      solaris_save_xattr(jcr, attrdirfd, current_xattr_namespace, dp->d_name,
                            false, STREAM_XATTR_SOLARIS);
   }

   closedir(dirp);
   retval = true;

cleanup:
   if (attrdirfd != -1)
      close(attrdirfd);
   if (filefd != -1)
      close(filefd);
   return retval;
}

#ifdef HAVE_ACL
static bool solaris_restore_xattr_acl(JCR *jcr, int fd, const char *attrname, char *acl_text)
{
#ifdef HAVE_EXTENDED_ACL
   int error;
   acl_t *aclp = NULL;

   if ((error = acl_fromtext(acl_text, &aclp)) != 0) {
      Jmsg1(jcr, M_ERROR, 0, _("Unable to convert acl from text on file \"%s\"\n"),
               jcr->last_fname);
      return true;                    /* non-fatal */
   }

   if ((fd != -1 && facl_set(fd, aclp) != 0) ||
        acl_set(attrname, aclp) != 0) {
      berrno be;
      Jmsg3(jcr, M_ERROR, 0, _("Unable to restore acl of xattr %s on file \"%s\": ERR=%s\n"),
         attrname, jcr->last_fname, be.bstrerror());
      Dmsg3(100, "Unable to restore acl of xattr %s on file \"%s\": ERR=%s\n",
         attrname, jcr->last_fname, be.bstrerror());
      return true;                    /* non-fatal */
   }

   if (aclp) {
      acl_free(aclp);
   }
   return true;

#else /* HAVE_EXTENDED_ACL */
   int n;
   aclent_t *acls = NULL;

   acls = aclfromtext(acl_text, &n);
   if (!acls) {
      if ((fd != -1 && facl(fd, SETACL, n, acls) != 0) ||
           acl(attrname, SETACL, n, acls) != 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to restore acl of xattr %s on file \"%s\": ERR=%s\n"),
            attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to restore acl of xattr %s on file \"%s\": ERR=%s\n",
            attrname, jcr->last_fname, be.bstrerror());
         return true;                   /* non-fatal */
      }
   }

   if (acls) {
      free(acls);
   }
   return true;

#endif /* HAVE_EXTENDED_ACL */

}
#endif /* HAVE_ACL */

static bool solaris_restore_xattrs(JCR *jcr, bool is_extensible)
{
   int fd, filefd = -1, attrdirfd = -1, attrfd = -1;
   int used_bytes, total_bytes, cnt;
   char *bp, *target_attrname, *attribs;
   char *linked_target = NULL;
   char *acl_text = NULL;
   char *data = NULL;
   int32_t inum;
   struct stat st;
   struct timeval times[2];
   bool retval = true;             /* default non-fatal */

   /*
    * Parse the xattr stream. First the part that is the same for all xattrs.
    */
   used_bytes = 0;
   total_bytes = jcr->xattr_data_len;

   /*
    * The name of the target xattr has a leading / we are not interested
    * in that so skip it when decoding the string. We always start a the /
    * of the xattr space anyway.
    */
   target_attrname = jcr->xattr_data + 1;
   if ((bp = strchr(target_attrname, '\0')) == (char *)NULL ||
       (used_bytes = (bp - jcr->xattr_data)) >= (total_bytes - 1)) {
      goto parse_error;
   }
   attribs = ++bp;

   /*
    * Open the file on which to restore the xattrs read-only.
    */
   if ((filefd = open(jcr->last_fname, O_RDONLY | O_NONBLOCK)) < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to open file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg2(100, "Unable to open file \"%s\": ERR=%s\n",
         jcr->last_fname, be.bstrerror());
      goto cleanup;
   }

   /*
    * Open the xattr naming space and make it the current working dir.
    */
   if ((attrdirfd = openat(filefd, ".", O_RDONLY | O_XATTR)) < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to open xattr space on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg2(100, "Unable to open xattr space on file \"%s\": ERR=%s\n",
         jcr->last_fname, be.bstrerror());
      goto cleanup;
   }

   if (fchdir(attrdirfd) < 0) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Unable to chdir to xattr space on file \"%s\": ERR=%s\n"),
         jcr->last_fname, be.bstrerror());
      Dmsg3(100, "Unable to fchdir to xattr space on file \"%s\" using fd %d: ERR=%s\n",
         jcr->last_fname, attrdirfd, be.bstrerror());
      goto cleanup;
   }

   /*
    * Try to open the correct xattr subdir based on the target_attrname given.
    * e.g. check if its a subdir attrname. Each / in the string makes us go
    * one level deeper.
    */
   while ((bp = strchr(target_attrname, '/')) != (char *)NULL) {
      *bp = '\0';

      if ((fd = open(target_attrname, O_RDONLY | O_NONBLOCK)) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to open xattr %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to open xattr %s on file \"%s\": ERR=%s\n",
            target_attrname, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }

      close(filefd);
      filefd = fd;

      /*
       * Open the xattr naming space.
       */
      if ((fd = openat(filefd, ".", O_RDONLY | O_XATTR)) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to open xattr space %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to open xattr space %s on file \"%s\": ERR=%s\n",
            target_attrname, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }

      close(attrdirfd);
      attrdirfd = fd;

      /*
       * Make the xattr space our current workingdir.
       */
      if (fchdir(attrdirfd) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to chdir to xattr space %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg4(100, "Unable to fchdir to xattr space %s on file \"%s\" using fd %d: ERR=%s\n",
            target_attrname, jcr->last_fname, attrdirfd, be.bstrerror());
         goto cleanup;
      }

      target_attrname = ++bp;
   }

   /*
    * Decode the attributes from the stream.
    */
   decode_stat(attribs, &st, &inum);

   /*
    * Decode the next field (acl_text).
    */
   if ((bp = strchr(attribs, '\0')) == (char *)NULL ||
       (used_bytes = (bp - jcr->xattr_data)) >= (total_bytes - 1)) {
      goto parse_error;
   }
   acl_text = ++bp;

   /*
    * Based on the filetype perform the correct action. We support most filetypes here, more
    * then the actual implementation on Solaris supports so some code may never get executed
    * due to limitations in the implementation.
    */
   switch (st.st_mode & S_IFMT) {
   case S_IFIFO:
      /*
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       */
      unlinkat(attrdirfd, target_attrname, 0);
      if (mkfifo(target_attrname, st.st_mode) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to mkfifo xattr %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to mkfifo xattr %s on file \"%s\": ERR=%s\n",
            target_attrname,  jcr->last_fname, be.bstrerror());
         goto cleanup;
      }
      break;
   case S_IFCHR:
   case S_IFBLK:
      /*
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       */
      unlinkat(attrdirfd, target_attrname, 0);
      if (mknod(target_attrname, st.st_mode, st.st_rdev) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to mknod xattr %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to mknod xattr %s on file \"%s\": ERR=%s\n",
            target_attrname,  jcr->last_fname, be.bstrerror());
         goto cleanup;
      }
      break;
   case S_IFDIR:
      /*
       * If its not the hidden_dir create the entry.
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       */
      if (strcmp(target_attrname, ".")) {
         unlinkat(attrdirfd, target_attrname, AT_REMOVEDIR);
         if (mkdir(target_attrname, st.st_mode) < 0) {
            berrno be;
            Jmsg3(jcr, M_WARNING, 0, _("Unable to mkdir xattr %s on file \"%s\": ERR=%s\n"),
               target_attrname, jcr->last_fname, be.bstrerror());
            Dmsg3(100, "Unable to mkdir xattr %s on file \"%s\": ERR=%s\n",
               target_attrname,  jcr->last_fname, be.bstrerror());
            goto cleanup;
         }
      }
      break;
   case S_IFREG:
      /*
       * See if this is a hard linked file. e.g. inum != 0
       */
      if (inum != 0) {
         linked_target = bp;

         unlinkat(attrdirfd, target_attrname, 0);
         if (link(linked_target, target_attrname) < 0) {
            berrno be;
            Jmsg4(jcr, M_ERROR, 0, _("Unable to link xattr %s to %s on file \"%s\": ERR=%s\n"),
               target_attrname, linked_target, jcr->last_fname, be.bstrerror());
            Dmsg4(100, "Unable to link xattr %s to %s on file \"%s\": ERR=%s\n",
               target_attrname, linked_target, jcr->last_fname, be.bstrerror());
            goto cleanup;
         }

         /*
          * Successfully restored xattr.
          */
         retval = true;
         goto cleanup;
      } else {
         if ((bp = strchr(acl_text, '\0')) == (char *)NULL ||
             (used_bytes = (bp - jcr->xattr_data)) >= total_bytes) {
            goto parse_error;
         }

         if (used_bytes < (total_bytes - 1))
            data = ++bp;

         /*
          * Restore the actual xattr.
          */
         if (!is_extensible) {
            unlinkat(attrdirfd, target_attrname, 0);
         }

         if ((attrfd = openat(attrdirfd, target_attrname, O_RDWR | O_CREAT | O_TRUNC, st.st_mode)) < 0) {
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Unable to open xattr %s on file \"%s\": ERR=%s\n"),
               target_attrname, jcr->last_fname, be.bstrerror());
            Dmsg3(100, "Unable to open xattr %s on file \"%s\": ERR=%s\n",
               target_attrname, jcr->last_fname, be.bstrerror());
            goto cleanup;
         }
      }

      /*
       * Restore the actual data.
       */
      if (st.st_size > 0) {
         used_bytes = (data - jcr->xattr_data);
         cnt = total_bytes - used_bytes;

         /*
          * Do a sanity check, the st.st_size should be the same as the number of bytes
          * we have available as data of the stream.
          */
         if (cnt != st.st_size) {
            Jmsg2(jcr, M_ERROR, 0, _("Unable to restore data of xattr %s on file \"%s\": Not all data available in xattr stream\n"),
               target_attrname, jcr->last_fname);
            Dmsg2(100, "Unable to restore data of xattr %s on file \"%s\": Not all data available in xattr stream\n",
               target_attrname, jcr->last_fname);
            goto cleanup;
         }

         while (cnt > 0) {
            cnt = write(attrfd, data, cnt);
            if (cnt < 0) {
               berrno be;
               Jmsg3(jcr, M_ERROR, 0, _("Unable to restore data of xattr %s on file \"%s\": ERR=%s\n"),
                  target_attrname, jcr->last_fname, be.bstrerror());
               Dmsg3(100, "Unable to restore data of xattr %s on file \"%s\": ERR=%s\n",
                  target_attrname, jcr->last_fname, be.bstrerror());
               goto cleanup;
            }

            used_bytes += cnt;
            data += cnt;
            cnt = total_bytes - used_bytes;
         }
      }
      break;
   case S_IFLNK:
      /*
       * The current implementation of xattr on Solaris doesn't support this, but if it ever does we are prepared.
       */
      linked_target = bp;

      if (symlink(linked_target, target_attrname) < 0) {
         berrno be;
         Jmsg4(jcr, M_ERROR, 0, _("Unable to symlink xattr %s to %s on file \"%s\": ERR=%s\n"),
            target_attrname, linked_target, jcr->last_fname, be.bstrerror());
         Dmsg4(100, "Unable to symlink xattr %s to %s on file \"%s\": ERR=%s\n",
            target_attrname, linked_target, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }

      /*
       * Successfully restored xattr.
       */
      retval = true;
      goto cleanup;
   default:
      goto cleanup;
   }

   /*
    * Restore owner and acl for non extensible attributes.
    */
   if (!is_extensible) {
      if (fchownat(attrdirfd, target_attrname, st.st_uid, st.st_gid, AT_SYMLINK_NOFOLLOW) < 0) {
         switch (errno) {
         case EINVAL:
            /*
             * Gentile way of the system saying this type of xattr layering is not supported.
             * But as this is not an error we return a positive return value.
             */
            retval = true;
            break;
         default:
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Unable to restore owner of xattr %s on file \"%s\": ERR=%s\n"),
               target_attrname, jcr->last_fname, be.bstrerror());
            Dmsg3(100, "Unable to restore owner of xattr %s on file \"%s\": ERR=%s\n",
               target_attrname, jcr->last_fname, be.bstrerror());
         }
         goto cleanup;
      }
   }

#ifdef HAVE_ACL
   if (acl_text && *acl_text)
      if (!solaris_restore_xattr_acl(jcr, attrfd, target_attrname, acl_text))
         goto cleanup;
#endif /* HAVE_ACL */

   /*
    * For a non extensible attribute restore access and modification time on the xattr.
    */
   if (!is_extensible) {
      times[0].tv_sec = st.st_atime;
      times[0].tv_usec = 0;
      times[1].tv_sec = st.st_mtime;
      times[1].tv_usec = 0;

      if (futimesat(attrdirfd, target_attrname, times) < 0) {
         berrno be;
         Jmsg3(jcr, M_ERROR, 0, _("Unable to restore filetimes of xattr %s on file \"%s\": ERR=%s\n"),
            target_attrname, jcr->last_fname, be.bstrerror());
         Dmsg3(100, "Unable to restore filetimes of xattr %s on file \"%s\": ERR=%s\n",
            target_attrname, jcr->last_fname, be.bstrerror());
         goto cleanup;
      }
   }

   /*
    * Successfully restored xattr.
    */
   retval = true;
   goto cleanup;

parse_error:
   Jmsg1(jcr, M_ERROR, 0, _("Illegal xattr stream, failed to parse xattr stream on file \"%s\"\n"),
      jcr->last_fname);
   Dmsg1(100, "Illegal xattr stream, failed to parse xattr stream on file \"%s\"\n",
      jcr->last_fname);

cleanup:
   if (attrfd != -1) {
      close(attrfd);
   }
   if (attrdirfd != -1) {
      close(attrdirfd);
   }
   if (filefd != -1) {
      close(filefd);
   }
   return retval;
}

static bool solaris_extract_xattr(JCR *jcr, int stream)
{
   char cwd[PATH_MAX];
   bool is_extensible = false;
   bool retval;

   /*
    * First make sure we can restore xattr on the filesystem.
    */
   switch (stream) {
#if defined(HAVE_SYS_NVPAIR_H) && defined(_PC_SATTR_ENABLED)
   case STREAM_XATTR_SOLARIS_SYS:
      if (pathconf(jcr->last_fname, _PC_SATTR_ENABLED) <= 0) {
         Qmsg1(jcr, M_WARNING, 0,
               _("Failed to restore extensible attributes on file \"%s\"\n"),
               jcr->last_fname);
         Dmsg1(100, "Unable to restore extensible attributes on file \"%s\", filesystem doesn't support this\n",
            jcr->last_fname);
         return true;
      }

      is_extensible = true;
      break;
#endif
   case STREAM_XATTR_SOLARIS:
      if (pathconf(jcr->last_fname, _PC_XATTR_ENABLED) <= 0) {
         Qmsg1(jcr, M_WARNING, 0,
               _("Failed to restore extended attributes on file \"%s\"\n"),
               jcr->last_fname);
         Dmsg1(100, "Unable to restore extended attributes on file \"%s\", filesystem doesn't support this\n",
            jcr->last_fname);
         return true;
      }
      break;
   default:
      return true;
   }

   /*
    * As we change the cwd in the restore function save the current cwd
    * for restore after return from the solaris_restore_xattrs function.
    */
   getcwd(cwd, sizeof(cwd));
   retval = solaris_restore_xattrs(jcr, is_extensible);
   chdir(cwd);
   return retval;
}

static int solaris_build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   char cwd[PATH_MAX];
   bool retval = true;

   /*
    * First see if extended attributes or extensible attributes are present.
    * If not just pretend things went ok.
    */
   if (pathconf(jcr->last_fname, _PC_XATTR_EXISTS) > 0) {
      nr_xattr_saved = 0;

      /*
       * As we change the cwd in the save function save the current cwd
       * for restore after return from the solaris_save_xattrs function.
       */
      getcwd(cwd, sizeof(cwd));
      retval = solaris_save_xattrs(jcr, NULL, NULL);
      chdir(cwd);
      drop_xattr_link_cache();
   }

   return retval;
}

static bool solaris_parse_xattr_stream(JCR *jcr, int stream)
{
   switch (stream) {
#if defined(HAVE_SYS_NVPAIR_H) && defined(_PC_SATTR_ENABLED)
   case STREAM_XATTR_SOLARIS_SYS:
#endif
   case STREAM_XATTR_SOLARIS:
      return solaris_extract_xattr(jcr, stream);
   default:
      Jmsg2(jcr, M_WARNING, 0,
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;                    /* non-fatal error */
   }
}
#endif

bool build_xattr_streams(JCR *jcr, FF_PKT *ff_pkt)
{
#if defined(HAVE_SUN_OS)
   return solaris_build_xattr_streams(jcr, ff_pkt);
#elif defined(HAVE_DARWIN_OS)
   return darwin_build_xattr_streams(jcr, ff_pkt);
#elif defined(HAVE_FREEBSD_OS)
   return freebsd_build_xattr_streams(jcr, ff_pkt);
#elif defined(HAVE_LINUX_OS)
   return linux_build_xattr_streams(jcr, ff_pkt);
#elif defined(HAVE_NETBSD_OS)
   return netbsd_build_xattr_streams(jcr, ff_pkt);
#endif
}

bool parse_xattr_stream(JCR *jcr, int stream)
{
   /*
    * Based on the stream being passed in dispatch to the right function
    * for parsing and restoring a specific xattr. The platform determines
    * which streams are recognized and parsed and which are handled by
    * the default case and ignored. As only one of the platform defines
    * is true per compile we never end up with duplicate switch values.
    */
   switch (stream) {
#if defined(HAVE_SUN_OS)
#if defined(HAVE_SYS_NVPAIR_H) && defined(_PC_SATTR_ENABLED)
   case STREAM_XATTR_SOLARIS_SYS:
#endif
   case STREAM_XATTR_SOLARIS:
      return solaris_parse_xattr_stream(jcr, stream);
#elif defined(HAVE_DARWIN_OS)
   case STREAM_XATTR_DARWIN:
      return darwin_parse_xattr_stream(jcr, stream);
#elif defined(HAVE_FREEBSD_OS)
   case STREAM_XATTR_FREEBSD:
      return freebsd_parse_xattr_stream(jcr, stream);
#elif defined(HAVE_LINUX_OS)
   case STREAM_XATTR_LINUX:
      return linux_parse_xattr_stream(jcr, stream);
#elif defined(HAVE_NETBSD_OS)
   case STREAM_XATTR_NETBSD:
      return netbsd_parse_xattr_stream(jcr, stream);
#endif
   default:
      /*
       * Issue a warning and discard the message. But pretend the restore was ok.
       */
      Qmsg2(jcr, M_WARNING, 0,
         _("Can't restore Extended Attributes of %s - incompatible xattr stream encountered - %d\n"),
         jcr->last_fname, stream);
      return true;
   } /* end switch (stream) */
}

#endif
