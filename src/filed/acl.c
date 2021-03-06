/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2012 Free Software Foundation Europe e.V.

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
/**
 * Functions to handle ACLs for bacula.
 *
 * Currently we support the following OSes:
 *   - AIX (pre-5.3 and post 5.3 acls, acl_get and aclx_get interface)
 *   - Darwin
 *   - FreeBSD (POSIX and NFSv4/ZFS acls)
 *   - HPUX
 *   - IRIX
 *   - Linux
 *   - Solaris (POSIX and NFSv4/ZFS acls)
 *   - Tru64
 *
 * We handle two different types of ACLs: access and default ACLS.
 * On most systems that support default ACLs they only apply to directories.
 *
 * On some systems (eg. linux and FreeBSD) we must obtain the two ACLs
 * independently, while others (eg. Solaris) provide both in one call.
 *
 * The Filed saves ACLs in their native format and uses different streams
 * for all different platforms. Currently we only allow ACLs to be restored
 * which were saved in the native format of the platform they are extracted
 * on. Later on we might add conversion functions for mapping from one
 * platform to an other or allow restores of systems that use the same
 * native format.
 *
 * Its also interesting to see what the exact format of acl text is on
 * certain platforms and if they use they same encoding we might allow
 * different platform streams to be decoded on an other similar platform.
 *
 *   Original written by Preben 'Peppe' Guldberg, December 2004
 *   Major rewrite by Marco van Wieringen, November 2008
 *   Major overhaul by Marco van Wieringen, January 2012
 */
  
#include "bacula.h"
#include "filed.h"
  
#if !defined(HAVE_ACL)
/**
 * Entry points when compiled without support for ACLs or on an unsupported platform.
 */
bacl_exit_code build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   return bacl_exit_fatal;
}

bacl_exit_code parse_acl_streams(JCR *jcr,
                                 int stream,
                                 char *content,
                                 uint32_t content_length)
{
   return bacl_exit_fatal;
}
#else
/**
 * Send an ACL stream to the SD.
 */
static bacl_exit_code send_acl_stream(JCR *jcr, int stream)
{
   BSOCK *sd = jcr->store_bsock;
   POOLMEM *msgsave;
#ifdef FD_NO_SEND_TEST
   return bacl_exit_ok;
#endif

   /*
    * Sanity check
    */
   if (jcr->acl_data->u.build->content_length <= 0) {
      return bacl_exit_ok;
   }

   /*
    * Send header
    */
   if (!sd->fsend("%ld %d 0", jcr->JobFiles, stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return bacl_exit_fatal;
   }

   /*
    * Send the buffer to the storage deamon
    */
   Dmsg1(400, "Backing up ACL <%s>\n", jcr->acl_data->u.build->content);
   msgsave = sd->msg;
   sd->msg = jcr->acl_data->u.build->content;
   sd->msglen = jcr->acl_data->u.build->content_length + 1;
   if (!sd->send()) {
      sd->msg = msgsave;
      sd->msglen = 0;
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return bacl_exit_fatal;
   }

   jcr->JobBytes += sd->msglen;
   sd->msg = msgsave;
   if (!sd->signal(BNET_EOD)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return bacl_exit_fatal;
   }

   Dmsg1(200, "ACL of file: %s successfully backed up!\n", jcr->last_fname);
   return bacl_exit_ok;
}

/*
 * First the native ACLs.
 */
#if defined(HAVE_ACL)
#if defined(HAVE_AIX_OS)

#if defined(HAVE_EXTENDED_ACL)

#include <sys/access.h>
#include <sys/acl.h>

static bool acl_is_trivial(struct acl *acl)
{
   return (acl_last(acl) != acl->acl_ext ? false : true);
}

static bool acl_nfs4_is_trivial(nfs4_acl_int_t *acl)
{
#if 0
   return (acl->aclEntryN > 0 ? false : true);
#else
   int i;
   int count = acl->aclEntryN;
   nfs4_ace_int_t *ace;

   for (i = 0; i < count; i++) {
      ace = &acl->aclEntry[i];
      if (!((ace->flags & ACE4_ID_SPECIAL) != 0 &&
            (ace->aceWho.special_whoid == ACE4_WHO_OWNER ||
             ace->aceWho.special_whoid == ACE4_WHO_GROUP ||
             ace->aceWho.special_whoid == ACE4_WHO_EVERYONE) &&
             ace->aceType == ACE4_ACCESS_ALLOWED_ACE_TYPE &&
             ace->aceFlags == 0 &&
            (ace->aceMask & ~(ACE4_READ_DATA |
                              ACE4_LIST_DIRECTORY |
                              ACE4_WRITE_DATA |
                              ACE4_ADD_FILE |
                              ACE4_EXECUTE)) == 0)) {
         return false;
      }
   }
   return true;
#endif
}

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[3] = {
   STREAM_ACL_AIX_TEXT,
   STREAM_ACL_AIX_AIXC,
   STREAM_ACL_AIX_NFS4
};
static int os_default_acl_streams[1] = {
   -1
};

static bacl_exit_code aix_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   berrno be;
   mode_t mode;
   acl_type_t type;
   size_t aclsize, acltxtsize;
   bacl_exit_code retval = bacl_exit_error;
   POOLMEM *aclbuf = get_pool_memory(PM_MESSAGE);

   /*
    * First see how big the buffers should be.
    */
   memset(&type, 0, sizeof(acl_type_t));
   type.u64 = ACL_ANY;
   if (aclx_get(jcr->last_fname, GET_ACLINFO_ONLY, &type, NULL, &aclsize, &mode) < 0) {
      switch (errno) {
      case ENOENT:
         retval = bacl_exit_ok;
         goto bail_out;
      case ENOSYS:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_SAVE_NATIVE flag so we skip ACL saves on all other files
          * on the same filesystem. The BACL_FLAG_SAVE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
         retval = bacl_exit_ok;
         goto bail_out;
      default:
         Mmsg2(jcr->errmsg,
               _("aclx_get error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "aclx_get error file=%s ERR=%s\n",
               jcr->last_fname, be.bstrerror());
         goto bail_out;
      }
   }

   /*
    * Make sure the buffers are big enough.
    */
   aclbuf = check_pool_memory_size(aclbuf, aclsize + 1);

   /*
    * Retrieve the ACL info.
    */
   if (aclx_get(jcr->last_fname, 0, &type, aclbuf, &aclsize, &mode) < 0) {
      switch (errno) {
      case ENOENT:
         retval = bacl_exit_ok;
         goto bail_out;
      default:
         Mmsg2(jcr->errmsg,
               _("aclx_get error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "aclx_get error file=%s ERR=%s\n",
               jcr->last_fname, be.bstrerror());
         goto bail_out;
      }
   }

   /*
    * See if the acl is non trivial.
    */
   switch (type.u64) {
   case ACL_AIXC:
      if (acl_is_trivial((struct acl *)aclbuf)) {
         retval = bacl_exit_ok;
         goto bail_out;
      }
      break;
   case ACL_NFS4:
      if (acl_nfs4_is_trivial((nfs4_acl_int_t *)aclbuf)) {
         retval = bacl_exit_ok;
         goto bail_out;
      }
      break;
   default:
      Mmsg2(jcr->errmsg,
            _("Unknown acl type encountered on file \"%s\": %ld\n"),
            jcr->last_fname, type.u64);
      Dmsg2(100, "Unknown acl type encountered on file \"%s\": %ld\n",
            jcr->last_fname, type.u64);
      goto bail_out;
   }

   /*
    * We have a non-trivial acl lets convert it into some ASCII form.
    */
   acltxtsize = sizeof_pool_memory(jcr->acl_data->u.build->content);
   if (aclx_printStr(jcr->acl_data->u.build->content, &acltxtsize, aclbuf,
                     aclsize, type, jcr->last_fname, 0) < 0) {
      switch (errno) {
      case ENOSPC:
         /*
          * Our buffer is not big enough, acltxtsize should be updated with the value
          * the aclx_printStr really need. So we increase the buffer and try again.
          */
         jcr->acl_data->u.build->content =
         check_pool_memory_size(jcr->acl_data->u.build->content, acltxtsize + 1);
         if (aclx_printStr(jcr->acl_data->u.build->content, &acltxtsize, aclbuf,
                           aclsize, type, jcr->last_fname, 0) < 0) {
            Mmsg1(jcr->errmsg,
                  _("Failed to convert acl into text on file \"%s\"\n"),
                  jcr->last_fname);
            Dmsg2(100, "Failed to convert acl into text on file \"%s\": %ld\n",
                  jcr->last_fname, type.u64);
            goto bail_out;
         }
         break;
      default:
         Mmsg1(jcr->errmsg,
               _("Failed to convert acl into text on file \"%s\"\n"),
               jcr->last_fname);
         Dmsg2(100, "Failed to convert acl into text on file \"%s\": %ld\n",
               jcr->last_fname, type.u64);
         goto bail_out;
      }
   }

   jcr->acl_data->u.build->u.build->content_length = strlen(jcr->acl_data->u.build->content) + 1;
   switch (type.u64) {
   case ACL_AIXC:
      retval = send_acl_stream(jcr, STREAM_ACL_AIX_AIXC);
   case ACL_NFS4:
      retval = send_acl_stream(jcr, STREAM_ACL_AIX_NFS4);
   }

bail_out:
   free_pool_memory(aclbuf);

   return retval;
}

/*
 * See if a specific type of ACLs are supported on the filesystem
 * the file is located on.
 */
static inline bool aix_query_acl_support(JCR *jcr,
                                         uint64_t aclType,
                                         acl_type_t *pacl_type_info)
{
   unsigned int i;
   acl_types_list_t acl_type_list;
   size_t acl_type_list_len = sizeof(acl_types_list_t);

   memset(&acl_type_list, 0, sizeof(acl_type_list));
   if (aclx_gettypes(jcr->last_fname, &acl_type_list, &acl_type_list_len)) {
      return false;
   }

   for (i = 0; i < acl_type_list.num_entries; i++) {
      if (acl_type_list.entries[i].u64 == aclType) {
         memcpy(pacl_type_info, acl_type_list.entries + i, sizeof(acl_type_t));
         return true;
      }
   }
   return false;
}

static bacl_exit_code aix_parse_acl_streams(JCR *jcr,
                                            int stream,
                                            char *content,
                                            uint32_t content_length)
{
   int cnt;
   berrno be;
   acl_type_t type;
   size_t aclsize;
   bacl_exit_code retval = bacl_exit_error;
   POOLMEM *aclbuf = get_pool_memory(PM_MESSAGE);

   switch (stream) {
   case STREAM_ACL_AIX_TEXT:
      /*
       * Handle the old stream using the old system call for now.
       */
      if (acl_put(jcr->last_fname, content, 0) != 0) {
         retval = bacl_exit_error;
         goto bail_out;
      }
      retval = bacl_exit_ok;
      goto bail_out;
   case STREAM_ACL_AIX_AIXC:
      if (!aix_query_acl_support(jcr, ACL_AIXC, &type)) {
         Mmsg1(jcr->errmsg,
               _("Trying to restore POSIX acl on file \"%s\" on filesystem without AIXC acl support\n"),
               jcr->last_fname);
         goto bail_out;
      }
      break;
   case STREAM_ACL_AIX_NFS4:
      if (!aix_query_acl_support(jcr, ACL_NFS4, &type)) {
         Mmsg1(jcr->errmsg,
               _("Trying to restore NFSv4 acl on file \"%s\" on filesystem without NFS4 acl support\n"),
               jcr->last_fname);
         goto bail_out;
      }
      break;
   default:
      goto bail_out;
   } /* end switch (stream) */

   /*
    * Set the acl buffer to an initial size. For now we set it
    * to the same size as the ASCII representation.
    */
   aclbuf = check_pool_memory_size(aclbuf, content_length);
   aclsize = content_length;
   if (aclx_scanStr(content, aclbuf, &aclsize, type) < 0) {
      switch (errno) {
      case ENOSPC:
         /*
          * The buffer isn't big enough. The man page doesn't say that aclsize
          * is updated to the needed size as what is done with aclx_printStr.
          * So for now we try to increase the buffer a maximum of 3 times
          * and retry the conversion.
          */
         for (cnt = 0; cnt < 3; cnt++) {
            aclsize = 2 * aclsize;
            aclbuf = check_pool_memory_size(aclbuf, aclsize);

            if (aclx_scanStr(content, aclbuf, &aclsize, type) == 0) {
               break;
            }

            /*
             * See why we failed this time, ENOSPC retry if max retries not met,
             * otherwise abort.
             */
            switch (errno) {
            case ENOSPC:
               if (cnt < 3) {
                  continue;
               }
               /* FALL THROUGH */
            default:
               Mmsg2(jcr->errmsg,
                     _("aclx_scanStr error on file \"%s\": ERR=%s\n"),
                     jcr->last_fname, be.bstrerror());
               Dmsg2(100, "aclx_scanStr error file=%s ERR=%s\n",
                     jcr->last_fname, be.bstrerror());
               goto bail_out;
            }
         }
         break;
      default:
         Mmsg2(jcr->errmsg,
               _("aclx_scanStr error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "aclx_scanStr error file=%s ERR=%s\n",
               jcr->last_fname, be.bstrerror());
      }
   }

   if (aclx_put(jcr->last_fname, SET_ACL, type, aclbuf, aclsize, 0) < 0) {
      switch (errno) {
      case ENOENT:
         retval = bacl_exit_ok;
         goto bail_out;
      case ENOSYS:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
          * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_RESTORE_NATIVE;
         retval = bacl_exit_ok;
         goto bail_out;
      default:
         Mmsg2(jcr->errmsg,
               _("aclx_put error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "aclx_put error file=%s ERR=%s\n",
               jcr->last_fname, be.bstrerror());
         goto bail_out;
      }
   }

   retval = bacl_exit_ok;

bail_out:
   free_pool_memory(aclbuf);

   return retval;
}

#else /* HAVE_EXTENDED_ACL */

#include <sys/access.h>

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[1] = {
   STREAM_ACL_AIX_TEXT
};
static int os_default_acl_streams[1] = {
   -1
};

static bacl_exit_code aix_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   char *acl_text;

   if ((acl_text = acl_get(jcr->last_fname)) != NULL) {
      jcr->acl_data->u.build->content_length =
      pm_strcpy(jcr->acl_data->u.build->content, acl_text);
      actuallyfree(acl_text);
      return send_acl_stream(jcr, STREAM_ACL_AIX_TEXT);
   }
   return bacl_exit_error;
}

static bacl_exit_code aix_parse_acl_streams(JCR *jcr,
                                            int stream,
                                            char *content,
                                            uint32_t content_length)
{
   if (acl_put(jcr->last_fname, content, 0) != 0) {
      return bacl_exit_error;
   }
   return bacl_exit_ok;
}
#endif /* HAVE_EXTENDED_ACL */

/*
 * For this OS setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      aix_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      aix_parse_acl_streams;

#elif defined(HAVE_DARWIN_OS) || \
      defined(HAVE_FREEBSD_OS) || \
      defined(HAVE_IRIX_OS) || \
      defined(HAVE_OSF1_OS) || \
      defined(HAVE_LINUX_OS)

#include <sys/types.h>

#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#else
#error "configure failed to detect availability of sys/acl.h"
#endif

/*
 * On IRIX we can get shortened ACLs
 */
#if defined(HAVE_IRIX_OS) && defined(BACL_WANT_SHORT_ACLS)
#define acl_to_text(acl,len)     acl_to_short_text((acl), (len))
#endif

/*
 * On Linux we can get numeric and/or shorted ACLs
 */
#if defined(HAVE_LINUX_OS)
#if defined(BACL_WANT_SHORT_ACLS) && defined(BACL_WANT_NUMERIC_IDS)
#define BACL_ALTERNATE_TEXT            (TEXT_ABBREVIATE|TEXT_NUMERIC_IDS)
#elif defined(BACL_WANT_SHORT_ACLS)
#define BACL_ALTERNATE_TEXT            TEXT_ABBREVIATE
#elif defined(BACL_WANT_NUMERIC_IDS)
#define BACL_ALTERNATE_TEXT            TEXT_NUMERIC_IDS
#endif
#ifdef BACL_ALTERNATE_TEXT
#include <acl/libacl.h>
#define acl_to_text(acl,len)     (acl_to_any_text((acl), NULL, ',', BACL_ALTERNATE_TEXT))
#endif
#endif

/*
 * On FreeBSD we can get numeric ACLs
 */
#if defined(HAVE_FREEBSD_OS)
#if defined(BACL_WANT_NUMERIC_IDS)
#define BACL_ALTERNATE_TEXT            ACL_TEXT_NUMERIC_IDS
#endif
#ifdef BACL_ALTERNATE_TEXT
#define acl_to_text(acl,len)     (acl_to_text_np((acl), (len), BACL_ALTERNATE_TEXT))
#endif
#endif

/*
 * Some generic functions used by multiple OSes.
 */
static acl_type_t bac_to_os_acltype(bacl_type acltype)
{
   acl_type_t ostype;

   switch (acltype) {
   case BACL_TYPE_ACCESS:
      ostype = ACL_TYPE_ACCESS;
      break;
   case BACL_TYPE_DEFAULT:
      ostype = ACL_TYPE_DEFAULT;
      break;
#ifdef HAVE_ACL_TYPE_NFS4
      /*
       * FreeBSD has an additional acl type named ACL_TYPE_NFS4.
       */
   case BACL_TYPE_NFS4:
      ostype = ACL_TYPE_NFS4;
      break;
#endif
#ifdef HAVE_ACL_TYPE_DEFAULT_DIR
   case BACL_TYPE_DEFAULT_DIR:
      /*
       * TRU64 has an additional acl type named ACL_TYPE_DEFAULT_DIR.
       */
      ostype = ACL_TYPE_DEFAULT_DIR;
      break;
#endif
#ifdef HAVE_ACL_TYPE_EXTENDED
   case BACL_TYPE_EXTENDED:
      /*
       * MacOSX has an additional acl type named ACL_TYPE_EXTENDED.
       */
      ostype = ACL_TYPE_EXTENDED;
      break;
#endif
   default:
      /*
       * This should never happen, as the per OS version function only tries acl
       * types supported on a certain platform.
       */
      ostype = (acl_type_t)ACL_TYPE_NONE;
      break;
   }
   return ostype;
}

static int acl_count_entries(acl_t acl)
{
   int count = 0;
#if defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_LINUX_OS)
   acl_entry_t ace;
   int entry_available;

   entry_available = acl_get_entry(acl, ACL_FIRST_ENTRY, &ace);
   while (entry_available == 1) {
      count++;
      entry_available = acl_get_entry(acl, ACL_NEXT_ENTRY, &ace);
   }
#elif defined(HAVE_IRIX_OS)
   count = acl->acl_cnt;
#elif defined(HAVE_OSF1_OS)
   count = acl->acl_num;
#elif defined(HAVE_DARWIN_OS)
   acl_entry_t ace;
   int entry_available;

   entry_available = acl_get_entry(acl, ACL_FIRST_ENTRY, &ace);
   while (entry_available == 0) {
      count++;
      entry_available = acl_get_entry(acl, ACL_NEXT_ENTRY, &ace);
   }
#endif
   return count;
}

#if !defined(HAVE_DARWIN_OS)
/*
 * See if an acl is a trivial one (e.g. just the stat bits encoded as acl.)
 * There is no need to store those acls as we already store the stat bits too.
 */
static bool acl_is_trivial(acl_t acl)
{
  /*
   * acl is trivial if it has only the following entries:
   * "user::",
   * "group::",
   * "other::"
   */
   acl_entry_t ace;
   acl_tag_t tag;
#if defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_LINUX_OS)
   int entry_available;

   entry_available = acl_get_entry(acl, ACL_FIRST_ENTRY, &ace);
   while (entry_available == 1) {
      /*
       * Get the tag type of this acl entry.
       * If we fail to get the tagtype we call the acl non-trivial.
       */
      if (acl_get_tag_type(ace, &tag) < 0)
         return true;
      /*
       * Anything other the ACL_USER_OBJ, ACL_GROUP_OBJ or ACL_OTHER breaks the spell.
       */
      if (tag != ACL_USER_OBJ &&
          tag != ACL_GROUP_OBJ &&
          tag != ACL_OTHER)
         return false;
      entry_available = acl_get_entry(acl, ACL_NEXT_ENTRY, &ace);
   }
   return true;
#elif defined(HAVE_IRIX_OS)
   int n;

   for (n = 0; n < acl->acl_cnt; n++) {
      ace = &acl->acl_entry[n];
      tag = ace->ae_tag;

      /*
       * Anything other the ACL_USER_OBJ, ACL_GROUP_OBJ or ACL_OTHER breaks the spell.
       */
      if (tag != ACL_USER_OBJ &&
          tag != ACL_GROUP_OBJ &&
          tag != ACL_OTHER_OBJ)
         return false;
   }
   return true;
#elif defined(HAVE_OSF1_OS)
   int count;

   ace = acl->acl_first;
   count = acl->acl_num;

   while (count > 0) {
      tag = ace->entry->acl_type;
      /*
       * Anything other the ACL_USER_OBJ, ACL_GROUP_OBJ or ACL_OTHER breaks the spell.
       */
      if (tag != ACL_USER_OBJ &&
          tag != ACL_GROUP_OBJ &&
          tag != ACL_OTHER)
         return false;
      /*
       * On Tru64, perm can also contain non-standard bits such as
       * PERM_INSERT, PERM_DELETE, PERM_MODIFY, PERM_LOOKUP, ...
       */
      if ((ace->entry->acl_perm & ~(ACL_READ | ACL_WRITE | ACL_EXECUTE)))
         return false;
      ace = ace->next;
      count--;
   }
   return true;
#endif
}
#endif

/**
 * Generic wrapper around acl_get_file call.
 */
static bacl_exit_code generic_get_acl_from_os(JCR *jcr, bacl_type acltype)
{
   acl_t acl;
   acl_type_t ostype;
   char *acl_text;
   berrno be;
   bacl_exit_code retval = bacl_exit_ok;

   ostype = bac_to_os_acltype(acltype);
   acl = acl_get_file(jcr->last_fname, ostype);
   if (acl) {
      /**
       * From observation, IRIX's acl_get_file() seems to return a
       * non-NULL acl with a count field of -1 when a file has no ACL
       * defined, while IRIX's acl_to_text() returns NULL when presented
       * with such an ACL. 
       *
       * For all other implmentations we check if there are more then
       * zero entries in the acl returned.
       */
      if (acl_count_entries(acl) <= 0) {
         goto bail_out;
      }

      /*
       * Make sure this is not just a trivial ACL.
       */
#if !defined(HAVE_DARWIN_OS)
      if (acltype == BACL_TYPE_ACCESS && acl_is_trivial(acl)) {
         /*
          * The ACLs simply reflect the (already known) standard permissions
          * So we don't send an ACL stream to the SD.
          */
         goto bail_out;
      }
#endif
#if defined(HAVE_FREEBSD_OS) && defined(_PC_ACL_NFS4)
      if (acltype == BACL_TYPE_NFS4) {
         int trivial;
         if (acl_is_trivial_np(acl, &trivial) == 0) {
            if (trivial == 1) {
               /*
                * The ACLs simply reflect the (already known) standard permissions
                * So we don't send an ACL stream to the SD.
                */
               goto bail_out;
            }
         }
      }
#endif

      /*
       * Convert the internal acl representation into a text representation.
       */
      if ((acl_text = acl_to_text(acl, NULL)) != NULL) {
         jcr->acl_data->u.build->content_length =
         pm_strcpy(jcr->acl_data->u.build->content, acl_text);
         acl_free(acl);
         acl_free(acl_text);
         return bacl_exit_ok;
      }

      Mmsg2(jcr->errmsg,
            _("acl_to_text error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg2(100, "acl_to_text error file=%s ERR=%s\n",  
            jcr->last_fname, be.bstrerror());

      retval = bacl_exit_error;
      goto bail_out;
   } else {
      /*
       * Handle errors gracefully.
       */
      switch (errno) {
#if defined(BACL_ENOTSUP)
      case BACL_ENOTSUP:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_SAVE_NATIVE flag so we skip ACL saves on all other files
          * on the same filesystem. The BACL_FLAG_SAVE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
         goto bail_out;
#endif
      case ENOENT:
         goto bail_out;
      default:
         /* Some real error */
         Mmsg2(jcr->errmsg,
               _("acl_get_file error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "acl_get_file error file=%s ERR=%s\n",  
               jcr->last_fname, be.bstrerror());

         retval = bacl_exit_error;
         goto bail_out;
      }
   }

bail_out:
   if (acl) {
      acl_free(acl);
   }
   pm_strcpy(jcr->acl_data->u.build->content, "");
   jcr->acl_data->u.build->content_length = 0;
   return retval;
}

/**
 * Generic wrapper around acl_set_file call.
 */
static bacl_exit_code generic_set_acl_on_os(JCR *jcr,
                                            bacl_type acltype,
                                            char *content,
                                            uint32_t content_length)
{
   acl_t acl;
   acl_type_t ostype;
   berrno be;

   /*
    * If we get empty default ACLs, clear ACLs now
    */
   ostype = bac_to_os_acltype(acltype);
   if (ostype == ACL_TYPE_DEFAULT && strlen(content) == 0) {
      if (acl_delete_def_file(jcr->last_fname) == 0) {
         return bacl_exit_ok;
      }
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
#if defined(BACL_ENOTSUP)
      case BACL_ENOTSUP:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
          * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_RESTORE_NATIVE;
         Mmsg1(jcr->errmsg,
               _("acl_delete_def_file error on file \"%s\": filesystem doesn't support ACLs\n"),
               jcr->last_fname);
         return bacl_exit_error;
#endif
      default:
         Mmsg2(jcr->errmsg,
               _("acl_delete_def_file error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
   }

   acl = acl_from_text(content);
   if (acl == NULL) {
      Mmsg2(jcr->errmsg,
            _("acl_from_text error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "acl_from_text error acl=%s file=%s ERR=%s\n",  
            content, jcr->last_fname, be.bstrerror());
      return bacl_exit_error;
   }

#ifndef HAVE_FREEBSD_OS
   /**
    * FreeBSD always fails acl_valid() - at least on valid input...
    * As it does the right thing, given valid input, just ignore acl_valid().
    */
   if (acl_valid(acl) != 0) {
      Mmsg2(jcr->errmsg,
            _("acl_valid error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "acl_valid error acl=%s file=%s ERR=%s\n",  
            content, jcr->last_fname, be.bstrerror());
      acl_free(acl);
      return bacl_exit_error;
   }
#endif

   /**
    * Restore the ACLs, but don't complain about links which really should
    * not have attributes, and the file it is linked to may not yet be restored.
    * This is only true for the old acl streams as in the new implementation we
    * don't save acls of symlinks (which cannot have acls anyhow)
    */
   if (acl_set_file(jcr->last_fname, ostype, acl) != 0 && jcr->last_type != FT_LNK) {
      switch (errno) {
      case ENOENT:
         acl_free(acl);
         return bacl_exit_ok;
#if defined(BACL_ENOTSUP)
      case BACL_ENOTSUP:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
          * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_RESTORE_NATIVE;
         Mmsg1(jcr->errmsg,
               _("acl_set_file error on file \"%s\": filesystem doesn't support ACLs\n"),
               jcr->last_fname);
         Dmsg2(100, "acl_set_file error acl=%s file=%s filesystem doesn't support ACLs\n",
               content, jcr->last_fname);
         acl_free(acl);
         return bacl_exit_error;
#endif
      default:
         Mmsg2(jcr->errmsg,
               _("acl_set_file error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg3(100, "acl_set_file error acl=%s file=%s ERR=%s\n",
               content, jcr->last_fname, be.bstrerror());
         acl_free(acl);
         return bacl_exit_error;
      }
   }
   acl_free(acl);
   return bacl_exit_ok;
}

/**
 * OS specific functions for handling different types of acl streams.
 */
#if defined(HAVE_DARWIN_OS)
/**
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[1] = {
   STREAM_ACL_DARWIN_ACCESS_ACL
};
static int os_default_acl_streams[1] = {
   -1
};

static bacl_exit_code darwin_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
#if defined(HAVE_ACL_TYPE_EXTENDED)
   /**
    * On MacOS X, acl_get_file (name, ACL_TYPE_ACCESS)
    * and acl_get_file (name, ACL_TYPE_DEFAULT)
    * always return NULL / EINVAL.  There is no point in making
    * these two useless calls.  The real ACL is retrieved through
    * acl_get_file (name, ACL_TYPE_EXTENDED).
    *
    * Read access ACLs for files, dirs and links
    */
   if (generic_get_acl_from_os(jcr, BACL_TYPE_EXTENDED) == bacl_exit_fatal)
      return bacl_exit_fatal;
#else
   /**
    * Read access ACLs for files, dirs and links
    */
   if (generic_get_acl_from_os(jcr, BACL_TYPE_ACCESS) == bacl_exit_fatal)
      return bacl_exit_fatal;
#endif

   if (jcr->acl_data->u.build->content_length > 0) {
      return send_acl_stream(jcr, STREAM_ACL_DARWIN_ACCESS_ACL);
   }
   return bacl_exit_ok;
}

static bacl_exit_code darwin_parse_acl_streams(JCR *jcr,
                                               int stream,
                                               char *content,
                                               uint32_t content_length)
{
#if defined(HAVE_ACL_TYPE_EXTENDED)
      return generic_set_acl_on_os(jcr, BACL_TYPE_EXTENDED, content, content_length);
#else
      return generic_set_acl_on_os(jcr, BACL_TYPE_ACCESS, content, content_length);
#endif
}

/*
 * For this OS setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      darwin_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      darwin_parse_acl_streams;

#elif defined(HAVE_FREEBSD_OS)
/*
 * Define the supported ACL streams for these OSes
 */
static int os_access_acl_streams[2] = {
   STREAM_ACL_FREEBSD_ACCESS_ACL,
   STREAM_ACL_FREEBSD_NFS4_ACL
};
static int os_default_acl_streams[1] = {
   STREAM_ACL_FREEBSD_DEFAULT_ACL
};

static bacl_exit_code freebsd_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   int acl_enabled = 0;
   bacl_type acltype = BACL_TYPE_NONE;
   berrno be;

#if defined(_PC_ACL_NFS4)
   /*
    * See if filesystem supports NFS4 acls.
    */
   acl_enabled = pathconf(jcr->last_fname, _PC_ACL_NFS4);
   switch (acl_enabled) {
   case -1:
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("pathconf error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "pathconf error file=%s ERR=%s\n",
               jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
   case 0:
      break;
   default:
      acltype = BACL_TYPE_NFS4;
      break;
   }
#endif

   if (acl_enabled == 0) {
      /*
       * See if filesystem supports POSIX acls.
       */
      acl_enabled = pathconf(jcr->last_fname, _PC_ACL_EXTENDED);
      switch (acl_enabled) {
      case -1:
         switch (errno) {
         case ENOENT:
            return bacl_exit_ok;
         default:
            Mmsg2(jcr->errmsg,
                  _("pathconf error on file \"%s\": ERR=%s\n"),
                  jcr->last_fname, be.bstrerror());
            Dmsg2(100, "pathconf error file=%s ERR=%s\n",
                  jcr->last_fname, be.bstrerror());
            return bacl_exit_error;
         }
      case 0:
         break;
      default:
         acltype = BACL_TYPE_ACCESS;
         break;
      }
   }

   /*
    * If the filesystem reports it doesn't support ACLs we clear the
    * BACL_FLAG_SAVE_NATIVE flag so we skip ACL saves on all other files
    * on the same filesystem. The BACL_FLAG_SAVE_NATIVE flag gets set again
    * when we change from one filesystem to an other.
    */
   if (acl_enabled == 0) {
      jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
      pm_strcpy(jcr->acl_data->u.build->content, "");
      jcr->acl_data->u.build->content_length = 0;
      return bacl_exit_ok;
   }

   /*
    * Based on the supported ACLs retrieve and store them.
    */
   switch (acltype) {
   case BACL_TYPE_NFS4:
      /*
       * Read NFS4 ACLs for files, dirs and links
       */
      if (generic_get_acl_from_os(jcr, BACL_TYPE_NFS4) == bacl_exit_fatal)
         return bacl_exit_fatal;

      if (jcr->acl_data->u.build->content_length > 0) {
         if (send_acl_stream(jcr, STREAM_ACL_FREEBSD_NFS4_ACL) == bacl_exit_fatal)
            return bacl_exit_fatal;
      }
      break;
   case BACL_TYPE_ACCESS:
      /*
       * Read access ACLs for files, dirs and links
       */
      if (generic_get_acl_from_os(jcr, BACL_TYPE_ACCESS) == bacl_exit_fatal)
         return bacl_exit_fatal;

      if (jcr->acl_data->u.build->content_length > 0) {
         if (send_acl_stream(jcr, STREAM_ACL_FREEBSD_ACCESS_ACL) == bacl_exit_fatal)
            return bacl_exit_fatal;
      }

      /*
       * Directories can have default ACLs too
       */
      if (ff_pkt->type == FT_DIREND) {
         if (generic_get_acl_from_os(jcr, BACL_TYPE_DEFAULT) == bacl_exit_fatal)
            return bacl_exit_fatal;
         if (jcr->acl_data->u.build->content_length > 0) {
            if (send_acl_stream(jcr, STREAM_ACL_FREEBSD_DEFAULT_ACL) == bacl_exit_fatal)
               return bacl_exit_fatal;
         }
      }
      break;
   default:
      break;
   }

   return bacl_exit_ok;
}

static bacl_exit_code freebsd_parse_acl_streams(JCR *jcr,
                                                int stream,
                                                char *content,
                                                uint32_t content_length)
{
   int acl_enabled = 0;
   const char *acl_type_name;
   berrno be;

   /*
    * First make sure the filesystem supports acls.
    */
   switch (stream) {
   case STREAM_UNIX_ACCESS_ACL:
   case STREAM_ACL_FREEBSD_ACCESS_ACL:
   case STREAM_UNIX_DEFAULT_ACL:
   case STREAM_ACL_FREEBSD_DEFAULT_ACL:
      acl_enabled = pathconf(jcr->last_fname, _PC_ACL_EXTENDED);
      acl_type_name = "POSIX";
      break;
   case STREAM_ACL_FREEBSD_NFS4_ACL:
#if defined(_PC_ACL_NFS4)
      acl_enabled = pathconf(jcr->last_fname, _PC_ACL_NFS4);
#endif
      acl_type_name = "NFS4";
      break;
   default:
      acl_type_name = "unknown";
      break;
   }

   switch (acl_enabled) {
   case -1:
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("pathconf error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg3(100, "pathconf error acl=%s file=%s ERR=%s\n",
               content, jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
   case 0:
      /*
       * If the filesystem reports it doesn't support ACLs we clear the
       * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
       * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
       * when we change from one filesystem to an other.
       */
      jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
      Mmsg2(jcr->errmsg,
            _("Trying to restore acl on file \"%s\" on filesystem without %s acl support\n"),
            jcr->last_fname, acl_type_name);
      return bacl_exit_error;
   default:
      break;
   }

   /*
    * Restore the ACLs.
    */
   switch (stream) {
   case STREAM_UNIX_ACCESS_ACL:
   case STREAM_ACL_FREEBSD_ACCESS_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_ACCESS, content, content_length);
   case STREAM_UNIX_DEFAULT_ACL:
   case STREAM_ACL_FREEBSD_DEFAULT_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_DEFAULT, content, content_length);
   case STREAM_ACL_FREEBSD_NFS4_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_NFS4, content, content_length);
   default:
      break;
   }
   return bacl_exit_error;
}

/*
 * For this OSes setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      freebsd_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      freebsd_parse_acl_streams;

#elif defined(HAVE_IRIX_OS) || \
      defined(HAVE_LINUX_OS)
/*
 * Define the supported ACL streams for these OSes
 */
#if defined(HAVE_IRIX_OS)
static int os_access_acl_streams[1] = {
   STREAM_ACL_IRIX_ACCESS_ACL
};
static int os_default_acl_streams[1] = {
   STREAM_ACL_IRIX_DEFAULT_ACL
};
#elif defined(HAVE_LINUX_OS)
static int os_access_acl_streams[1] = {
   STREAM_ACL_LINUX_ACCESS_ACL
};
static int os_default_acl_streams[1] = {
   STREAM_ACL_LINUX_DEFAULT_ACL
};
#endif

static bacl_exit_code generic_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   /*
    * Read access ACLs for files, dirs and links
    */
   if (generic_get_acl_from_os(jcr, BACL_TYPE_ACCESS) == bacl_exit_fatal)
      return bacl_exit_fatal;

   if (jcr->acl_data->u.build->content_length > 0) {
      if (send_acl_stream(jcr, os_access_acl_streams[0]) == bacl_exit_fatal)
         return bacl_exit_fatal;
   }

   /*
    * Directories can have default ACLs too
    */
   if (ff_pkt->type == FT_DIREND) {
      if (generic_get_acl_from_os(jcr, BACL_TYPE_DEFAULT) == bacl_exit_fatal)
         return bacl_exit_fatal;
      if (jcr->acl_data->u.build->content_length > 0) {
         if (send_acl_stream(jcr, os_default_acl_streams[0]) == bacl_exit_fatal)
            return bacl_exit_fatal;
      }
   }
   return bacl_exit_ok;
}

static bacl_exit_code generic_parse_acl_streams(JCR *jcr,
                                                int stream,
                                                char *content,
                                                uint32_t content_length)
{
   unsigned int cnt;

   switch (stream) {
   case STREAM_UNIX_ACCESS_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_ACCESS, content, content_length);
   case STREAM_UNIX_DEFAULT_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_DEFAULT, content, content_length);
   default:
      /*
       * See what type of acl it is.
       */
      for (cnt = 0; cnt < sizeof(os_access_acl_streams) / sizeof(int); cnt++) {
         if (os_access_acl_streams[cnt] == stream) {
            return generic_set_acl_on_os(jcr, BACL_TYPE_ACCESS, content, content_length);
         }
      }
      for (cnt = 0; cnt < sizeof(os_default_acl_streams) / sizeof(int); cnt++) {
         if (os_default_acl_streams[cnt] == stream) {
            return generic_set_acl_on_os(jcr, BACL_TYPE_DEFAULT, content, content_length);
         }
      }
      break;
   }
   return bacl_exit_error;
}

/*
 * For this OSes setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      generic_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      generic_parse_acl_streams;

#elif defined(HAVE_OSF1_OS)

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[1] = {
   STREAM_ACL_TRU64_ACCESS_ACL
};
static int os_default_acl_streams[2] = {
   STREAM_ACL_TRU64_DEFAULT_ACL,
   STREAM_ACL_TRU64_DEFAULT_DIR_ACL
};

static bacl_exit_code tru64_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   /*
    * Read access ACLs for files, dirs and links
    */
   if (generic_get_acl_from_os(jcr, BACL_TYPE_ACCESS) == bacl_exit_fatal) {
      return bacl_exit_error;
   if (jcr->acl_data->u.build->content_length > 0) {
      if (!send_acl_stream(jcr, STREAM_ACL_TRU64_ACCESS_ACL))
         return bacl_exit_error;
   }
   /*
    * Directories can have default ACLs too
    */
   if (ff_pkt->type == FT_DIREND) {
      if (generic_get_acl_from_os(jcr, BACL_TYPE_DEFAULT) == bacl_exit_fatal) {
         return bacl_exit_error;
      if (jcr->acl_data->u.build->content_length > 0) {
         if (!send_acl_stream(jcr, STREAM_ACL_TRU64_DEFAULT_ACL))
            return bacl_exit_error;
      }
      /**
       * Tru64 has next to BACL_TYPE_DEFAULT also BACL_TYPE_DEFAULT_DIR acls.
       * This is an inherited acl for all subdirs.
       * See http://www.helsinki.fi/atk/unix/dec_manuals/DOC_40D/AQ0R2DTE/DOCU_018.HTM
       * Section 21.5 Default ACLs 
       */
      if (generic_get_acl_from_os(jcr, BACL_TYPE_DEFAULT_DIR) == bacl_exit_fatal) {
         return bacl_exit_error;
      if (jcr->acl_data->u.build->content_length > 0) {
         if (!send_acl_stream(jcr, STREAM_ACL_TRU64_DEFAULT_DIR_ACL))
            return bacl_exit_error;
      }
   }
   return bacl_exit_ok;
}

static bacl_exit_code tru64_parse_acl_streams(JCR *jcr,
                                              int stream,
                                              char *content,
                                              uint32_t content_length)
{
   switch (stream) {
   case STREAM_UNIX_ACCESS_ACL:
   case STREAM_ACL_TRU64_ACCESS_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_ACCESS, content, content_length);
   case STREAM_UNIX_DEFAULT_ACL:
   case STREAM_ACL_TRU64_DEFAULT_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_DEFAULT, content, content_length);
   case STREAM_ACL_TRU64_DEFAULT_DIR_ACL:
      return generic_set_acl_on_os(jcr, BACL_TYPE_DEFAULT_DIR, content, content_length);
}

/*
 * For this OS setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      tru64_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      tru64_parse_acl_streams;

#endif

#elif defined(HAVE_HPUX_OS)
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#else
#error "configure failed to detect availability of sys/acl.h"
#endif

#include <acllib.h>

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[1] = {
   STREAM_ACL_HPUX_ACL_ENTRY
};
static int os_default_acl_streams[1] = {
   -1
};

/*
 * See if an acl is a trivial one (e.g. just the stat bits encoded as acl.)
 * There is no need to store those acls as we already store the stat bits too.
 */
static bool acl_is_trivial(int count, struct acl_entry *entries, struct stat sb)
{
   int n;
   struct acl_entry ace

   for (n = 0; n < count; n++) {
      ace = entries[n];
      /*
       * See if this acl just is the stat mode in acl form.
       */
      if (!((ace.uid == sb.st_uid && ace.gid == ACL_NSGROUP) ||
            (ace.uid == ACL_NSUSER && ace.gid == sb.st_gid) ||
            (ace.uid == ACL_NSUSER && ace.gid == ACL_NSGROUP)))
         return false;
   }
   return true;
}

/*
 * OS specific functions for handling different types of acl streams.
 */
static bacl_exit_code hpux_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   int n;
   struct acl_entry acls[NACLENTRIES];
   char *acl_text;
   berrno be;

   if ((n = getacl(jcr->last_fname, 0, acls)) < 0) {
      switch (errno) {
#if defined(BACL_ENOTSUP)
      case BACL_ENOTSUP:
         /*
          * Not supported, just pretend there is nothing to see
          *
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_SAVE_NATIVE flag so we skip ACL saves on all other files
          * on the same filesystem. The BACL_FLAG_SAVE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
         pm_strcpy(jcr->acl_data->u.build->content, "");
         jcr->acl_data->u.build->content_length = 0;
         return bacl_exit_ok;
#endif
      case ENOENT:
         pm_strcpy(jcr->acl_data->u.build->content, "");
         jcr->acl_data->u.build->content_length = 0;
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("getacl error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "getacl error file=%s ERR=%s\n",  
               jcr->last_fname, be.bstrerror());

         pm_strcpy(jcr->acl_data->u.build->content, "");
         jcr->acl_data->u.build->content_length = 0;
         return bacl_exit_error;
      }
   }
   if (n == 0) {
      pm_strcpy(jcr->acl_data->u.build->content, "");
      jcr->acl_data->u.build->content_length = 0;
      return bacl_exit_ok;
   }
   if ((n = getacl(jcr->last_fname, n, acls)) > 0) {
      if (acl_is_trivial(n, acls, ff_pkt->statp)) {
         /*
          * The ACLs simply reflect the (already known) standard permissions
          * So we don't send an ACL stream to the SD.
          */
         pm_strcpy(jcr->acl_data->u.build->content, "");
         jcr->acl_data->u.build->content_length = 0;
         return bacl_exit_ok;
      }
      if ((acl_text = acltostr(n, acls, FORM_SHORT)) != NULL) {
         jcr->acl_data->u.build->content_length =
         pm_strcpy(jcr->acl_data->u.build->content, acl_text);
         actuallyfree(acl_text);

         return send_acl_stream(jcr, STREAM_ACL_HPUX_ACL_ENTRY);
      }
      Mmsg2(jcr->errmsg,
            _("acltostr error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "acltostr error acl=%s file=%s ERR=%s\n",  
            jcr->acl_data->u.build->content, jcr->last_fname, be.bstrerror());
      return bacl_exit_error;
   }
   return bacl_exit_error;
}

static bacl_exit_code hpux_parse_acl_streams(JCR *jcr,
                                             int stream,
                                             char *content,
                                             uint32_t content_length)
{
   int n, stat;
   struct acl_entry acls[NACLENTRIES];
   berrno be;

   n = strtoacl(content, 0, NACLENTRIES, acls, ACL_FILEOWNER, ACL_FILEGROUP);
   if (n <= 0) {
      Mmsg2(jcr->errmsg,
            _("strtoacl error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "strtoacl error acl=%s file=%s ERR=%s\n",  
            content, jcr->last_fname, be.bstrerror());
      return bacl_exit_error;
   }
   if (strtoacl(content, n, NACLENTRIES, acls, ACL_FILEOWNER, ACL_FILEGROUP) != n) {
      Mmsg2(jcr->errmsg,
            _("strtoacl error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "strtoacl error acl=%s file=%s ERR=%s\n",  
            content, jcr->last_fname, be.bstrerror());

      return bacl_exit_error;
   }
   /**
    * Restore the ACLs, but don't complain about links which really should
    * not have attributes, and the file it is linked to may not yet be restored.
    * This is only true for the old acl streams as in the new implementation we
    * don't save acls of symlinks (which cannot have acls anyhow)
    */
   if (setacl(jcr->last_fname, n, acls) != 0 &&
       jcr->last_type != FT_LNK) {
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
#if defined(BACL_ENOTSUP)
      case BACL_ENOTSUP:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
          * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
         Mmsg1(jcr->errmsg,
               _("setacl error on file \"%s\": filesystem doesn't support ACLs\n"),
               jcr->last_fname);
         Dmsg2(100, "setacl error acl=%s file=%s filesystem doesn't support ACLs\n",
               content, jcr->last_fname);
         return bacl_exit_error;
#endif
      default:
         Mmsg2(jcr->errmsg,
               _("setacl error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg3(100, "setacl error acl=%s file=%s ERR=%s\n",
               content, jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
   }
   return bacl_exit_ok;
}

/*
 * For this OS setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      hpux_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      hpux_parse_acl_streams;

#elif defined(HAVE_SUN_OS)
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#else
#error "configure failed to detect availability of sys/acl.h"
#endif

#if defined(HAVE_EXTENDED_ACL)
/**
 * We define some internals of the Solaris acl libs here as those
 * are not exposed yet. Probably because they want us to see the
 * acls as opague data. But as we need to support different platforms
 * and versions of Solaris we need to expose some data to be able
 * to determine the type of acl used to stuff it into the correct
 * data stream. I know this is far from portable, but maybe the
 * proper interface is exposed later on and we can get ride of
 * this kludge. Newer versions of Solaris include sys/acl_impl.h
 * which has implementation details of acls, if thats included we
 * don't have to define it ourself.
 */
#if !defined(_SYS_ACL_IMPL_H)
typedef enum acl_type {
   ACLENT_T = 0,
   ACE_T = 1
} acl_type_t;
#endif

/**
 * Two external references to functions in the libsec library function not in current include files.
 */
extern "C" {
int acl_type(acl_t *);
char *acl_strerror(int);
}

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[2] = {
   STREAM_ACL_SOLARIS_ACLENT,
   STREAM_ACL_SOLARIS_ACE
};
static int os_default_acl_streams[1] = {
   -1
};

/**
 * As the new libsec interface with acl_totext and acl_fromtext also handles
 * the old format from acltotext we can use the new functions even
 * for acls retrieved and stored in the database with older fd versions. If the
 * new interface is not defined (Solaris 9 and older we fall back to the old code)
 */
static bacl_exit_code solaris_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   int acl_enabled, flags;
   acl_t *aclp;
   char *acl_text;
   bacl_exit_code stream_status = bacl_exit_error;
   berrno be;

   /*
    * See if filesystem supports acls.
    */
   acl_enabled = pathconf(jcr->last_fname, _PC_ACL_ENABLED);
   switch (acl_enabled) {
   case 0:
      /*
       * If the filesystem reports it doesn't support ACLs we clear the
       * BACL_FLAG_SAVE_NATIVE flag so we skip ACL saves on all other files
       * on the same filesystem. The BACL_FLAG_SAVE_NATIVE flag gets set again
       * when we change from one filesystem to an other.
       */
      jcr->acl_data->flags &= ~BACL_FLAG_SAVE_NATIVE;
      pm_strcpy(jcr->acl_data->u.build->content, "");
      jcr->acl_data->u.build->content_length = 0;
      return bacl_exit_ok;
   case -1:
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("pathconf error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "pathconf error file=%s ERR=%s\n",  
               jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
   default:
      break;
   }

   /*
    * Get ACL info: don't bother allocating space if there is only a trivial ACL.
    */
   if (acl_get(jcr->last_fname, ACL_NO_TRIVIAL, &aclp) != 0) {
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("acl_get error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, acl_strerror(errno));
         Dmsg2(100, "acl_get error file=%s ERR=%s\n",  
               jcr->last_fname, acl_strerror(errno));
         return bacl_exit_error;
      }
   }

   if (!aclp) {
      /*
       * The ACLs simply reflect the (already known) standard permissions
       * So we don't send an ACL stream to the SD.
       */
      pm_strcpy(jcr->acl_data->u.build->content, "");
      jcr->acl_data->u.build->content_length = 0;
      return bacl_exit_ok;
   }

#if defined(ACL_SID_FMT)
   /*
    * New format flag added in newer Solaris versions.
    */
   flags = ACL_APPEND_ID | ACL_COMPACT_FMT | ACL_SID_FMT;
#else
   flags = ACL_APPEND_ID | ACL_COMPACT_FMT;
#endif /* ACL_SID_FMT */

   if ((acl_text = acl_totext(aclp, flags)) != NULL) {
      jcr->acl_data->u.build->content_length =
      pm_strcpy(jcr->acl_data->u.build->content, acl_text);
      actuallyfree(acl_text);

      switch (acl_type(aclp)) {
      case ACLENT_T:
         stream_status = send_acl_stream(jcr, STREAM_ACL_SOLARIS_ACLENT);
         break;
      case ACE_T:
         stream_status = send_acl_stream(jcr, STREAM_ACL_SOLARIS_ACE);
         break;
      default:
         break;
      }

      acl_free(aclp);
   }
   return stream_status;
}

static bacl_exit_code solaris_parse_acl_streams(JCR *jcr,
                                                int stream,
                                                char *content,
                                                uint32_t content_length)
{
   acl_t *aclp;
   int acl_enabled, error;
   berrno be;

   switch (stream) {
   case STREAM_UNIX_ACCESS_ACL:
   case STREAM_ACL_SOLARIS_ACLENT:
   case STREAM_ACL_SOLARIS_ACE:
      /*
       * First make sure the filesystem supports acls.
       */
      acl_enabled = pathconf(jcr->last_fname, _PC_ACL_ENABLED);
      switch (acl_enabled) {
      case 0:
         /*
          * If the filesystem reports it doesn't support ACLs we clear the
          * BACL_FLAG_RESTORE_NATIVE flag so we skip ACL restores on all other files
          * on the same filesystem. The BACL_FLAG_RESTORE_NATIVE flag gets set again
          * when we change from one filesystem to an other.
          */
         jcr->acl_data->flags &= ~BACL_FLAG_RESTORE_NATIVE;
         Mmsg1(jcr->errmsg,
               _("Trying to restore acl on file \"%s\" on filesystem without acl support\n"),
               jcr->last_fname);
         return bacl_exit_error;
      case -1:
         switch (errno) {
         case ENOENT:
            return bacl_exit_ok;
         default:
            Mmsg2(jcr->errmsg,
                  _("pathconf error on file \"%s\": ERR=%s\n"),
                  jcr->last_fname, be.bstrerror());
            Dmsg3(100, "pathconf error acl=%s file=%s ERR=%s\n",  
                  content, jcr->last_fname, be.bstrerror());
            return bacl_exit_error;
         }
      default:
         /*
          * On a filesystem with ACL support make sure this particular ACL type can be restored.
          */
         switch (stream) {
         case STREAM_ACL_SOLARIS_ACLENT:
            /*
             * An aclent can be restored on filesystems with _ACL_ACLENT_ENABLED or _ACL_ACE_ENABLED support.
             */
            if ((acl_enabled & (_ACL_ACLENT_ENABLED | _ACL_ACE_ENABLED)) == 0) {
               Mmsg1(jcr->errmsg,
                     _("Trying to restore POSIX acl on file \"%s\" on filesystem without aclent acl support\n"),
                     jcr->last_fname);
               return bacl_exit_error;
            }
            break;
         case STREAM_ACL_SOLARIS_ACE:
            /*
             * An ace can only be restored on a filesystem with _ACL_ACE_ENABLED support.
             */
            if ((acl_enabled & _ACL_ACE_ENABLED) == 0) {
               Mmsg1(jcr->errmsg,
                     _("Trying to restore NFSv4 acl on file \"%s\" on filesystem without ace acl support\n"),
                     jcr->last_fname);
               return bacl_exit_error;
            }
            break;
         default:
            /*
             * Stream id which doesn't describe the type of acl which is encoded.
             */
            break;
         }
         break;
      }

      if ((error = acl_fromtext(content, &aclp)) != 0) {
         Mmsg2(jcr->errmsg,
               _("acl_fromtext error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, acl_strerror(error));
         Dmsg3(100, "acl_fromtext error acl=%s file=%s ERR=%s\n",  
               content, jcr->last_fname, acl_strerror(error));
         return bacl_exit_error;
      }

      /*
       * Validate that the conversion gave us the correct acl type.
       */
      switch (stream) {
      case STREAM_ACL_SOLARIS_ACLENT:
         if (acl_type(aclp) != ACLENT_T) {
            Mmsg1(jcr->errmsg,
                  _("wrong encoding of acl type in acl stream on file \"%s\"\n"),
                  jcr->last_fname);
            return bacl_exit_error;
         }
         break;
      case STREAM_ACL_SOLARIS_ACE:
         if (acl_type(aclp) != ACE_T) {
            Mmsg1(jcr->errmsg,
                  _("wrong encoding of acl type in acl stream on file \"%s\"\n"),
                  jcr->last_fname);
            return bacl_exit_error;
         }
         break;
      default:
         /*
          * Stream id which doesn't describe the type of acl which is encoded.
          */
         break;
      }

      /**
       * Restore the ACLs, but don't complain about links which really should
       * not have attributes, and the file it is linked to may not yet be restored.
       * This is only true for the old acl streams as in the new implementation we
       * don't save acls of symlinks (which cannot have acls anyhow)
       */
      if ((error = acl_set(jcr->last_fname, aclp)) == -1 && jcr->last_type != FT_LNK) {
         switch (errno) {
         case ENOENT:
            acl_free(aclp);
            return bacl_exit_ok;
         default:
            Mmsg2(jcr->errmsg,
                  _("acl_set error on file \"%s\": ERR=%s\n"),
                  jcr->last_fname, acl_strerror(error));
            Dmsg3(100, "acl_set error acl=%s file=%s ERR=%s\n",  
                  content, jcr->last_fname, acl_strerror(error));
            acl_free(aclp);
            return bacl_exit_error;
         }
      }

      acl_free(aclp);
      return bacl_exit_ok;
   default:
      return bacl_exit_error;
   } /* end switch (stream) */
}

#else /* HAVE_EXTENDED_ACL */

/*
 * Define the supported ACL streams for this OS
 */
static int os_access_acl_streams[1] = {
   STREAM_ACL_SOLARIS_ACLENT
};
static int os_default_acl_streams[1] = {
   -1
};

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

/*
 * OS specific functions for handling different types of acl streams.
 */
static bacl_exit_code solaris_build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   int n;
   aclent_t *acls;
   char *acl_text;
   berrno be;

   n = acl(jcr->last_fname, GETACLCNT, 0, NULL);
   if (n < MIN_ACL_ENTRIES)
      return bacl_exit_error;

   acls = (aclent_t *)malloc(n * sizeof(aclent_t));
   if (acl(jcr->last_fname, GETACL, n, acls) == n) {
      if (acl_is_trivial(n, acls)) {
         /*
          * The ACLs simply reflect the (already known) standard permissions
          * So we don't send an ACL stream to the SD.
          */
         free(acls);
         pm_strcpy(jcr->acl_data->u.build->content, "");
         jcr->acl_data->u.build->content_length = 0;
         return bacl_exit_ok;
      }

      if ((acl_text = acltotext(acls, n)) != NULL) {
         jcr->acl_data->u.build->content_length =
         pm_strcpy(jcr->acl_data->u.build->content, acl_text);
         actuallyfree(acl_text);
         free(acls);
         return send_acl_stream(jcr, STREAM_ACL_SOLARIS_ACLENT);
      }

      Mmsg2(jcr->errmsg,
            _("acltotext error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "acltotext error acl=%s file=%s ERR=%s\n",  
            jcr->acl_data->u.build->content, jcr->last_fname, be.bstrerror());
   }

   free(acls);
   return bacl_exit_error;
}

static bacl_exit_code solaris_parse_acl_streams(JCR *jcr,
                                                int stream,
                                                char *content,
                                                uint32_t content_length)
{
   int n;
   aclent_t *acls;
   berrno be;

   acls = aclfromtext(content, &n);
   if (!acls) {
      Mmsg2(jcr->errmsg,
            _("aclfromtext error on file \"%s\": ERR=%s\n"),
            jcr->last_fname, be.bstrerror());
      Dmsg3(100, "aclfromtext error acl=%s file=%s ERR=%s\n",  
            content, jcr->last_fname, be.bstrerror());
      return bacl_exit_error;
   }

   /*
    * Restore the ACLs, but don't complain about links which really should
    * not have attributes, and the file it is linked to may not yet be restored.
    */
   if (acl(jcr->last_fname, SETACL, n, acls) == -1 && jcr->last_type != FT_LNK) {
      switch (errno) {
      case ENOENT:
         actuallyfree(acls);
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("acl(SETACL) error on file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg3(100, "acl(SETACL) error acl=%s file=%s ERR=%s\n",
               content, jcr->last_fname, be.bstrerror());
         actuallyfree(acls);
         return bacl_exit_error;
      }
   }
   actuallyfree(acls);
   return bacl_exit_ok;
}
#endif /* HAVE_EXTENDED_ACL */

/*
 * For this OS setup the build and parse function pointer to the OS specific functions.
 */
static bacl_exit_code (*os_build_acl_streams)
                      (JCR *jcr, FF_PKT *ff_pkt) =
                      solaris_build_acl_streams;
static bacl_exit_code (*os_parse_acl_streams)
                      (JCR *jcr, int stream, char *content, uint32_t content_length) =
                      solaris_parse_acl_streams;

#endif /* HAVE_SUN_OS */
#endif /* HAVE_ACL */

/*
 * Entry points when compiled with support for ACLs on a supported platform.
 */

/**
 * Read and send an ACL for the last encountered file.
 */
bacl_exit_code build_acl_streams(JCR *jcr, FF_PKT *ff_pkt)
{
   /*
    * See if we are changing from one device to an other.
    * We save the current device we are restoring to and compare
    * it with the current st_dev in the last stat performed on
    * the file we are currently storing.
    */
   if (jcr->acl_data->current_dev != ff_pkt->statp.st_dev) {
      /*
       * Reset the acl save flags.
       */
      jcr->acl_data->flags = 0;

      jcr->acl_data->flags |= BACL_FLAG_SAVE_NATIVE;

      /**
       * Save that we started scanning a new filesystem.
       */
      jcr->acl_data->current_dev = ff_pkt->statp.st_dev;
   }

#if defined(HAVE_ACL)
   /*
    * See if the BACL_FLAG_SAVE_NATIVE flag is set which lets us know if we should
    * save native ACLs.
    */
   if (jcr->acl_data->flags & BACL_FLAG_SAVE_NATIVE) {
      /*
       * Call the appropriate function.
       */
      if (os_build_acl_streams) {
         return os_build_acl_streams(jcr, ff_pkt);
      }
   } else {
      return bacl_exit_ok;
   }
#endif
   return bacl_exit_error;
}

bacl_exit_code parse_acl_streams(JCR *jcr,
                                 int stream,
                                 char *content,
                                 uint32_t content_length)
{
   int ret;
   berrno be;
   struct stat st;
   unsigned int cnt;

   /*
    * See if we are changing from one device to an other.
    * We save the current device we are restoring to and compare
    * it with the current st_dev in the last stat performed on
    * the file we are currently restoring.
    */
   ret = lstat(jcr->last_fname, &st);
   switch (ret) {
   case -1:
      switch (errno) {
      case ENOENT:
         return bacl_exit_ok;
      default:
         Mmsg2(jcr->errmsg,
               _("Unable to stat file \"%s\": ERR=%s\n"),
               jcr->last_fname, be.bstrerror());
         Dmsg2(100, "Unable to stat file \"%s\": ERR=%s\n",
               jcr->last_fname, be.bstrerror());
         return bacl_exit_error;
      }
      break;
   case 0:
      break;
   }
   if (jcr->acl_data->current_dev != st.st_dev) {
      /*
       * Reset the acl save flags.
       */
      jcr->acl_data->flags = 0;
      jcr->acl_data->flags |= BACL_FLAG_RESTORE_NATIVE;

      /*
       * Save that we started restoring to a new filesystem.
       */
      jcr->acl_data->current_dev = st.st_dev;
   }

   switch (stream) {
#if defined(HAVE_ACL)
   case STREAM_UNIX_ACCESS_ACL:
   case STREAM_UNIX_DEFAULT_ACL:
      /*
       * Handle legacy ACL streams.
       */
      if ((jcr->acl_data->flags & BACL_FLAG_RESTORE_NATIVE) && os_parse_acl_streams) {
         return os_parse_acl_streams(jcr, stream, content, content_length);
      } else {
         /*
          * Increment error count but don't log an error again for the same filesystem.
          */
         jcr->acl_data->u.parse->nr_errors++;
         return bacl_exit_ok;
      }
      break;
   default:
      if ((jcr->acl_data->flags & BACL_FLAG_RESTORE_NATIVE) && os_parse_acl_streams) {
         /*
          * Walk the os_access_acl_streams array with the supported Access ACL streams for this OS.
          */
         for (cnt = 0; cnt < sizeof(os_access_acl_streams) / sizeof(int); cnt++) {
            if (os_access_acl_streams[cnt] == stream) {
               return os_parse_acl_streams(jcr, stream, content, content_length);
            }
         }
         /*
          * Walk the os_default_acl_streams array with the supported Default ACL streams for this OS.
          */
         for (cnt = 0; cnt < sizeof(os_default_acl_streams) / sizeof(int); cnt++) {
            if (os_default_acl_streams[cnt] == stream) {
               return os_parse_acl_streams(jcr, stream, content, content_length);
            }
         }
      } else {
         /*
          * Increment error count but don't log an error again for the same filesystem.
          */
         jcr->acl_data->u.parse->nr_errors++;
         return bacl_exit_ok;
      }
      break;
#else
   default:
      break;
#endif
   }
   Qmsg2(jcr, M_WARNING, 0,
      _("Can't restore ACLs of %s - incompatible acl stream encountered - %d\n"),
      jcr->last_fname, stream);
   return bacl_exit_error;
}
#endif
