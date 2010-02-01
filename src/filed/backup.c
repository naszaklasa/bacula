/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2009 Free Software Foundation Europe e.V.

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
 *  Bacula File Daemon  backup.c  send file attributes and data
 *   to the Storage daemon.
 *
 *    Kern Sibbald, March MM
 *
 *   Version $Id$
 *
 */

#include "bacula.h"
#include "filed.h"

#if defined(HAVE_ACL)
const bool have_acl = true;
#else
const bool have_acl = false;
#endif

#if defined(HAVE_XATTR)
const bool have_xattr = true;
#else
const bool have_xattr = false;
#endif

/* Forward referenced functions */
int save_file(JCR *jcr, FF_PKT *ff_pkt, bool top_level);
static int send_data(JCR *jcr, int stream, FF_PKT *ff_pkt, DIGEST *digest, DIGEST *signature_digest);
bool encode_and_send_attributes(JCR *jcr, FF_PKT *ff_pkt, int &data_stream);
static bool crypto_session_start(JCR *jcr);
static void crypto_session_end(JCR *jcr);
static bool crypto_session_send(JCR *jcr, BSOCK *sd);

/*
 * Find all the requested files and send them
 * to the Storage daemon.
 *
 * Note, we normally carry on a one-way
 * conversation from this point on with the SD, simply blasting
 * data to him.  To properly know what is going on, we
 * also run a "heartbeat" monitor which reads the socket and
 * reacts accordingly (at the moment it has nothing to do
 * except echo the heartbeat to the Director).
 *
 */
bool blast_data_to_storage_daemon(JCR *jcr, char *addr)
{
   BSOCK *sd;
   bool ok = true;
   // TODO landonf: Allow user to specify encryption algorithm

   sd = jcr->store_bsock;

   set_jcr_job_status(jcr, JS_Running);

   Dmsg1(300, "bfiled: opened data connection %d to stored\n", sd->m_fd);

   LockRes();
   CLIENT *client = (CLIENT *)GetNextRes(R_CLIENT, NULL);
   UnlockRes();
   uint32_t buf_size;
   if (client) {
      buf_size = client->max_network_buffer_size;
   } else {
      buf_size = 0;                   /* use default */
   }
   if (!sd->set_buffer_size(buf_size, BNET_SETBUF_WRITE)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      Jmsg(jcr, M_FATAL, 0, _("Cannot set buffer size FD->SD.\n"));
      return false;
   }

   jcr->buf_size = sd->msglen;
   /* Adjust for compression so that output buffer is
    *  12 bytes + 0.1% larger than input buffer plus 18 bytes.
    *  This gives a bit extra plus room for the sparse addr if any.
    *  Note, we adjust the read size to be smaller so that the
    *  same output buffer can be used without growing it.
    *
    * The zlib compression workset is initialized here to minimize
    *  the "per file" load. The jcr member is only set, if the init 
    *  was successful.
    */
   jcr->compress_buf_size = jcr->buf_size + ((jcr->buf_size+999) / 1000) + 30;
   jcr->compress_buf = get_memory(jcr->compress_buf_size);
   
#ifdef HAVE_LIBZ
   z_stream *pZlibStream = (z_stream*)malloc(sizeof(z_stream));  
   if (pZlibStream) {
      pZlibStream->zalloc = Z_NULL;      
      pZlibStream->zfree = Z_NULL;
      pZlibStream->opaque = Z_NULL;
      pZlibStream->state = Z_NULL;

      if (deflateInit(pZlibStream, Z_DEFAULT_COMPRESSION) == Z_OK) {
         jcr->pZLIB_compress_workset = pZlibStream;
      } else {
         free (pZlibStream);
      }
   }
#endif

   if (!crypto_session_start(jcr)) {
      return false;
   }

   set_find_options((FF_PKT *)jcr->ff, jcr->incremental, jcr->mtime);

   /* in accurate mode, we overwrite the find_one check function */
   if (jcr->accurate) {
      set_find_changed_function((FF_PKT *)jcr->ff, accurate_check_file);
   } 
   
   start_heartbeat_monitor(jcr);

   if (have_acl) {
      jcr->acl_data = get_pool_memory(PM_MESSAGE);
   }
   if (have_xattr) {
      jcr->xattr_data = get_pool_memory(PM_MESSAGE);
   }

   /* Subroutine save_file() is called for each file */
   if (!find_files(jcr, (FF_PKT *)jcr->ff, save_file, plugin_save)) {
      ok = false;                     /* error */
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   accurate_send_deleted_list(jcr);              /* send deleted list to SD  */

   stop_heartbeat_monitor(jcr);

   sd->signal(BNET_EOD);            /* end of sending data */

   if (have_acl && jcr->acl_data) {
      free_pool_memory(jcr->acl_data);
      jcr->acl_data = NULL;
   }
   if (have_xattr && jcr->xattr_data) {
      free_pool_memory(jcr->xattr_data);
      jcr->xattr_data = NULL;
   }
   if (jcr->big_buf) {
      free(jcr->big_buf);
      jcr->big_buf = NULL;
   }
   if (jcr->compress_buf) {
      free_pool_memory(jcr->compress_buf);
      jcr->compress_buf = NULL;
   }
   if (jcr->pZLIB_compress_workset) {
      /* Free the zlib stream */
#ifdef HAVE_LIBZ
      deflateEnd((z_stream *)jcr->pZLIB_compress_workset);
#endif
      free (jcr->pZLIB_compress_workset);
      jcr->pZLIB_compress_workset = NULL;
   }
   crypto_session_end(jcr);


   Dmsg1(100, "end blast_data ok=%d\n", ok);
   return ok;
}

static bool crypto_session_start(JCR *jcr)
{
   crypto_cipher_t cipher = CRYPTO_CIPHER_AES_128_CBC;

   /*
    * Create encryption session data and a cached, DER-encoded session data
    * structure. We use a single session key for each backup, so we'll encode
    * the session data only once.
    */
   if (jcr->crypto.pki_encrypt) {
      uint32_t size = 0;

      /* Create per-job session encryption context */
      jcr->crypto.pki_session = crypto_session_new(cipher, jcr->crypto.pki_recipients);

      /* Get the session data size */
      if (!crypto_session_encode(jcr->crypto.pki_session, (uint8_t *)0, &size)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred while encrypting the stream.\n"));
         return false;
      }

      /* Allocate buffer */
      jcr->crypto.pki_session_encoded = get_memory(size);

      /* Encode session data */
      if (!crypto_session_encode(jcr->crypto.pki_session, (uint8_t *)jcr->crypto.pki_session_encoded, &size)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred while encrypting the stream.\n"));
         return false;
      }

      /* ... and store the encoded size */
      jcr->crypto.pki_session_encoded_size = size;

      /* Allocate the encryption/decryption buffer */
      jcr->crypto.crypto_buf = get_memory(CRYPTO_CIPHER_MAX_BLOCK_SIZE);
   }
   return true;
}

static void crypto_session_end(JCR *jcr)
{
   if (jcr->crypto.crypto_buf) {
      free_pool_memory(jcr->crypto.crypto_buf);
      jcr->crypto.crypto_buf = NULL;
   }
   if (jcr->crypto.pki_session) {
      crypto_session_free(jcr->crypto.pki_session);
   }
   if (jcr->crypto.pki_session_encoded) {
      free_pool_memory(jcr->crypto.pki_session_encoded);
      jcr->crypto.pki_session_encoded = NULL;
   }
}

static bool crypto_session_send(JCR *jcr, BSOCK *sd)
{
   POOLMEM *msgsave;

   /* Send our header */
   Dmsg2(100, "Send hdr fi=%ld stream=%d\n", jcr->JobFiles, STREAM_ENCRYPTED_SESSION_DATA);
   sd->fsend("%ld %d 0", jcr->JobFiles, STREAM_ENCRYPTED_SESSION_DATA);

   msgsave = sd->msg;
   sd->msg = jcr->crypto.pki_session_encoded;
   sd->msglen = jcr->crypto.pki_session_encoded_size;
   jcr->JobBytes += sd->msglen;

   Dmsg1(100, "Send data len=%d\n", sd->msglen);
   sd->send();
   sd->msg = msgsave;
   sd->signal(BNET_EOD);
   return true;
}


/*
 * Called here by find() for each file included.
 *   This is a callback. The original is find_files() above.
 *
 *  Send the file and its data to the Storage daemon.
 *
 *  Returns: 1 if OK
 *           0 if error
 *          -1 to ignore file/directory (not used here)
 */
int save_file(JCR *jcr, FF_PKT *ff_pkt, bool top_level)
{
   bool do_read = false;
   int stat, data_stream; 
   int rtnstat = 0;
   DIGEST *digest = NULL;
   DIGEST *signing_digest = NULL;
   int digest_stream = STREAM_NONE;
   SIGNATURE *sig = NULL;
   bool has_file_data = false;
   // TODO landonf: Allow the user to specify the digest algorithm
#ifdef HAVE_SHA2
   crypto_digest_t signing_algorithm = CRYPTO_DIGEST_SHA256;
#else
   crypto_digest_t signing_algorithm = CRYPTO_DIGEST_SHA1;
#endif
   BSOCK *sd = jcr->store_bsock;

   if (job_canceled(jcr)) {
      return 0;
   }

   jcr->num_files_examined++;         /* bump total file count */

   switch (ff_pkt->type) {
   case FT_LNKSAVED:                  /* Hard linked, file already saved */
      Dmsg2(130, "FT_LNKSAVED hard link: %s => %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_REGE:
      Dmsg1(130, "FT_REGE saving: %s\n", ff_pkt->fname);
      has_file_data = true;
      break;
   case FT_REG:
      Dmsg1(130, "FT_REG saving: %s\n", ff_pkt->fname);
      has_file_data = true;
      break;
   case FT_LNK:
      Dmsg2(130, "FT_LNK saving: %s -> %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_DIRBEGIN:
      jcr->num_files_examined--;      /* correct file count */
      return 1;                       /* not used */
   case FT_NORECURSE:
      Jmsg(jcr, M_INFO, 1, _("     Recursion turned off. Will not descend from %s into %s\n"),
           ff_pkt->top_fname, ff_pkt->fname);
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_NOFSCHG:
      /* Suppress message for /dev filesystems */
      if (!is_in_fileset(ff_pkt)) {
         Jmsg(jcr, M_INFO, 1, _("     %s is a different filesystem. Will not descend from %s into %s\n"),
              ff_pkt->fname, ff_pkt->top_fname, ff_pkt->fname);
      }
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_INVALIDFS:
      Jmsg(jcr, M_INFO, 1, _("     Disallowed filesystem. Will not descend from %s into %s\n"),
           ff_pkt->top_fname, ff_pkt->fname);
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_INVALIDDT:
      Jmsg(jcr, M_INFO, 1, _("     Disallowed drive type. Will not descend into %s\n"),
           ff_pkt->fname);
      break;
   case FT_REPARSE:
   case FT_DIREND:
      Dmsg1(130, "FT_DIREND: %s\n", ff_pkt->link);
      break;
   case FT_SPEC:
      Dmsg1(130, "FT_SPEC saving: %s\n", ff_pkt->fname);
      if (S_ISSOCK(ff_pkt->statp.st_mode)) {
        Jmsg(jcr, M_SKIPPED, 1, _("     Socket file skipped: %s\n"), ff_pkt->fname);
        return 1;
      }
      break;
   case FT_RAW:
      Dmsg1(130, "FT_RAW saving: %s\n", ff_pkt->fname);
      has_file_data = true;
      break;
   case FT_FIFO:
      Dmsg1(130, "FT_FIFO saving: %s\n", ff_pkt->fname);
      break;
   case FT_NOACCESS: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not access \"%s\": ERR=%s\n"), ff_pkt->fname,
         be.bstrerror(ff_pkt->ff_errno));
      jcr->JobErrors++;
      return 1;
   }
   case FT_NOFOLLOW: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not follow link \"%s\": ERR=%s\n"), 
           ff_pkt->fname, be.bstrerror(ff_pkt->ff_errno));
      jcr->JobErrors++;
      return 1;
   }
   case FT_NOSTAT: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not stat \"%s\": ERR=%s\n"), ff_pkt->fname,
         be.bstrerror(ff_pkt->ff_errno));
      jcr->JobErrors++;
      return 1;
   }
   case FT_DIRNOCHG:
   case FT_NOCHG:
      Jmsg(jcr, M_SKIPPED, 1, _("     Unchanged file skipped: %s\n"), ff_pkt->fname);
      return 1;
   case FT_ISARCH:
      Jmsg(jcr, M_NOTSAVED, 0, _("     Archive file not saved: %s\n"), ff_pkt->fname);
      return 1;
   case FT_NOOPEN: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not open directory \"%s\": ERR=%s\n"), 
           ff_pkt->fname, be.bstrerror(ff_pkt->ff_errno));
      jcr->JobErrors++;
      return 1;
   }
   default:
      Jmsg(jcr, M_NOTSAVED, 0,  _("     Unknown file type %d; not saved: %s\n"), 
           ff_pkt->type, ff_pkt->fname);
      jcr->JobErrors++;
      return 1;
   }

   Dmsg1(130, "bfiled: sending %s to stored\n", ff_pkt->fname);

   /* Digests and encryption are only useful if there's file data */
   if (has_file_data) {
      /*
       * Setup for digest handling. If this fails, the digest will be set to NULL
       * and not used. Note, the digest (file hash) can be any one of the four
       * algorithms below.
       *
       * The signing digest is a single algorithm depending on
       * whether or not we have SHA2.              
       *   ****FIXME****  the signing algoritm should really be
       *   determined a different way!!!!!!  What happens if
       *   sha2 was available during backup but not restore?
       */
      if (ff_pkt->flags & FO_MD5) {
         digest = crypto_digest_new(jcr, CRYPTO_DIGEST_MD5);
         digest_stream = STREAM_MD5_DIGEST;

      } else if (ff_pkt->flags & FO_SHA1) {
         digest = crypto_digest_new(jcr, CRYPTO_DIGEST_SHA1);
         digest_stream = STREAM_SHA1_DIGEST;

      } else if (ff_pkt->flags & FO_SHA256) {
         digest = crypto_digest_new(jcr, CRYPTO_DIGEST_SHA256);
         digest_stream = STREAM_SHA256_DIGEST;

      } else if (ff_pkt->flags & FO_SHA512) {
         digest = crypto_digest_new(jcr, CRYPTO_DIGEST_SHA512);
         digest_stream = STREAM_SHA512_DIGEST;
      }

      /* Did digest initialization fail? */
      if (digest_stream != STREAM_NONE && digest == NULL) {
         Jmsg(jcr, M_WARNING, 0, _("%s digest initialization failed\n"),
            stream_to_ascii(digest_stream));
      }

      /*
       * Set up signature digest handling. If this fails, the signature digest will be set to
       * NULL and not used.
       */
      // TODO landonf: We should really only calculate the digest once, for both verification and signing.
      if (jcr->crypto.pki_sign) {
         signing_digest = crypto_digest_new(jcr, signing_algorithm);

         /* Full-stop if a failure occurred initializing the signature digest */
         if (signing_digest == NULL) {
            Jmsg(jcr, M_NOTSAVED, 0, _("%s signature digest initialization failed\n"),
               stream_to_ascii(signing_algorithm));
            jcr->JobErrors++;
            goto good_rtn;
         }
      }

      /* Enable encryption */
      if (jcr->crypto.pki_encrypt) {
         ff_pkt->flags |= FO_ENCRYPT;
      }
   }

   /* Initialize the file descriptor we use for data and other streams. */
   binit(&ff_pkt->bfd);
   if (ff_pkt->flags & FO_PORTABLE) {
      set_portable_backup(&ff_pkt->bfd); /* disable Win32 BackupRead() */
   }
   if (ff_pkt->cmd_plugin) {
      if (!set_cmd_plugin(&ff_pkt->bfd, jcr)) {
         goto bail_out;
      }
      send_plugin_name(jcr, sd, true);      /* signal start of plugin data */
   }

   /* Send attributes -- must be done after binit() */
   if (!encode_and_send_attributes(jcr, ff_pkt, data_stream)) {
      goto bail_out;
   }

   /* Set up the encryption context and send the session data to the SD */
   if (has_file_data && jcr->crypto.pki_encrypt) {
      if (!crypto_session_send(jcr, sd)) {
         goto bail_out;
      }
   }

   /*
    * Open any file with data that we intend to save, then save it.
    *
    * Note, if is_win32_backup, we must open the Directory so that
    * the BackupRead will save its permissions and ownership streams.
    */
   if (ff_pkt->type != FT_LNKSAVED && S_ISREG(ff_pkt->statp.st_mode)) {
#ifdef HAVE_WIN32
      do_read = !is_portable_backup(&ff_pkt->bfd) || ff_pkt->statp.st_size > 0;
#else
      do_read = ff_pkt->statp.st_size > 0;  
#endif
   } else if (ff_pkt->type == FT_RAW || ff_pkt->type == FT_FIFO ||
              ff_pkt->type == FT_REPARSE ||
         (!is_portable_backup(&ff_pkt->bfd) && ff_pkt->type == FT_DIREND)) {
      do_read = true;
   }
   if (ff_pkt->cmd_plugin) {
      do_read = true;
   }

   Dmsg1(400, "do_read=%d\n", do_read);
   if (do_read) {
      btimer_t *tid;

      if (ff_pkt->type == FT_FIFO) {
         tid = start_thread_timer(jcr, pthread_self(), 60);
      } else {
         tid = NULL;
      }
      int noatime = ff_pkt->flags & FO_NOATIME ? O_NOATIME : 0;
      ff_pkt->bfd.reparse_point = ff_pkt->type == FT_REPARSE;
      if (bopen(&ff_pkt->bfd, ff_pkt->fname, O_RDONLY | O_BINARY | noatime, 0) < 0) {
         ff_pkt->ff_errno = errno;
         berrno be;
         Jmsg(jcr, M_NOTSAVED, 0, _("     Cannot open \"%s\": ERR=%s.\n"), ff_pkt->fname,
              be.bstrerror());
         jcr->JobErrors++;
         if (tid) {
            stop_thread_timer(tid);
            tid = NULL;
         }
         goto good_rtn;
      }
      if (tid) {
         stop_thread_timer(tid);
         tid = NULL;
      }

      stat = send_data(jcr, data_stream, ff_pkt, digest, signing_digest);

      if (ff_pkt->flags & FO_CHKCHANGES) {
         has_file_changed(jcr, ff_pkt);
      }

      bclose(&ff_pkt->bfd);
      
      if (!stat) {
         goto bail_out;
      }
   }

#ifdef HAVE_DARWIN_OS
   /* Regular files can have resource forks and Finder Info */
   if (ff_pkt->type != FT_LNKSAVED && (S_ISREG(ff_pkt->statp.st_mode) &&
            ff_pkt->flags & FO_HFSPLUS)) {
      if (ff_pkt->hfsinfo.rsrclength > 0) {
         int flags;
         int rsrc_stream;
         if (!bopen_rsrc(&ff_pkt->bfd, ff_pkt->fname, O_RDONLY | O_BINARY, 0) < 0) {
            ff_pkt->ff_errno = errno;
            berrno be;
            Jmsg(jcr, M_NOTSAVED, -1, _("     Cannot open resource fork for \"%s\": ERR=%s.\n"), 
                 ff_pkt->fname, be.bstrerror());
            jcr->JobErrors++;
            if (is_bopen(&ff_pkt->bfd)) {
               bclose(&ff_pkt->bfd);
            }
            goto good_rtn;
         }
         flags = ff_pkt->flags;
         ff_pkt->flags &= ~(FO_GZIP|FO_SPARSE);
         if (flags & FO_ENCRYPT) {
            rsrc_stream = STREAM_ENCRYPTED_MACOS_FORK_DATA;
         } else {
            rsrc_stream = STREAM_MACOS_FORK_DATA;
         }
         stat = send_data(jcr, rsrc_stream, ff_pkt, digest, signing_digest);
         ff_pkt->flags = flags;
         bclose(&ff_pkt->bfd);
         if (!stat) {
            goto bail_out;
         }
      }

      Dmsg1(300, "Saving Finder Info for \"%s\"\n", ff_pkt->fname);
      sd->fsend("%ld %d 0", jcr->JobFiles, STREAM_HFSPLUS_ATTRIBUTES);
      Dmsg1(300, "bfiled>stored:header %s\n", sd->msg);
      pm_memcpy(sd->msg, ff_pkt->hfsinfo.fndrinfo, 32);
      sd->msglen = 32;
      if (digest) {
         crypto_digest_update(digest, (uint8_t *)sd->msg, sd->msglen);
      }
      if (signing_digest) {
         crypto_digest_update(signing_digest, (uint8_t *)sd->msg, sd->msglen);
      }
      sd->send();
      sd->signal(BNET_EOD);
   }
#endif

   /*
    * Save ACLs when requested and available for anything not being a symlink and not being a plugin.
    */
   if (have_acl) {
      if (ff_pkt->flags & FO_ACL && ff_pkt->type != FT_LNK && !ff_pkt->cmd_plugin) {
         if (!build_acl_streams(jcr, ff_pkt))
            goto bail_out;
      }
   }

   /*
    * Save Extended Attributes when requested and available for all files not being a plugin.
    */
   if (have_xattr) {
      if (ff_pkt->flags & FO_XATTR && !ff_pkt->cmd_plugin) {
         if (!build_xattr_streams(jcr, ff_pkt))
            goto bail_out;
      }
   }

   /* Terminate the signing digest and send it to the Storage daemon */
   if (signing_digest) {
      uint32_t size = 0;

      if ((sig = crypto_sign_new(jcr)) == NULL) {
         Jmsg(jcr, M_FATAL, 0, _("Failed to allocate memory for crypto signature.\n"));
         goto bail_out;
      }

      if (!crypto_sign_add_signer(sig, signing_digest, jcr->crypto.pki_keypair)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred while signing the stream.\n"));
         goto bail_out;
      }

      /* Get signature size */
      if (!crypto_sign_encode(sig, NULL, &size)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred while signing the stream.\n"));
         goto bail_out;
      }

      /* Grow the bsock buffer to fit our message if necessary */
      if (sizeof_pool_memory(sd->msg) < (int32_t)size) {
         sd->msg = realloc_pool_memory(sd->msg, size);
      }

      /* Send our header */
      sd->fsend("%ld %ld 0", jcr->JobFiles, STREAM_SIGNED_DIGEST);
      Dmsg1(300, "bfiled>stored:header %s\n", sd->msg);

      /* Encode signature data */
      if (!crypto_sign_encode(sig, (uint8_t *)sd->msg, &size)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred while signing the stream.\n"));
         goto bail_out;
      }

      sd->msglen = size;
      sd->send();
      sd->signal(BNET_EOD);              /* end of checksum */
   }

   /* Terminate any digest and send it to Storage daemon */
   if (digest) {
      uint32_t size;

      sd->fsend("%ld %d 0", jcr->JobFiles, digest_stream);
      Dmsg1(300, "bfiled>stored:header %s\n", sd->msg);

      size = CRYPTO_DIGEST_MAX_SIZE;

      /* Grow the bsock buffer to fit our message if necessary */
      if (sizeof_pool_memory(sd->msg) < (int32_t)size) {
         sd->msg = realloc_pool_memory(sd->msg, size);
      }

      if (!crypto_digest_finalize(digest, (uint8_t *)sd->msg, &size)) {
         Jmsg(jcr, M_FATAL, 0, _("An error occurred finalizing signing the stream.\n"));
         goto bail_out;
      }

      sd->msglen = size;
      sd->send();
      sd->signal(BNET_EOD);              /* end of checksum */
   }
   if (ff_pkt->cmd_plugin) {
      send_plugin_name(jcr, sd, false); /* signal end of plugin data */
   }

good_rtn:
   rtnstat = 1;                       /* good return */

bail_out:
   if (digest) {
      crypto_digest_free(digest);
   }
   if (signing_digest) {
      crypto_digest_free(signing_digest);
   }
   if (sig) {
      crypto_sign_free(sig);        
   }
   return rtnstat;
}

/*
 * Send data read from an already open file descriptor.
 *
 * We return 1 on sucess and 0 on errors.
 *
 * ***FIXME***
 * We use ff_pkt->statp.st_size when FO_SPARSE to know when to stop
 *  reading.
 * Currently this is not a problem as the only other stream, resource forks,
 * are not handled as sparse files.
 */
static int send_data(JCR *jcr, int stream, FF_PKT *ff_pkt, DIGEST *digest, 
                     DIGEST *signing_digest)
{
   BSOCK *sd = jcr->store_bsock;
   uint64_t fileAddr = 0;             /* file address */
   char *rbuf, *wbuf;
   int32_t rsize = jcr->buf_size;      /* read buffer size */
   POOLMEM *msgsave;
   CIPHER_CONTEXT *cipher_ctx = NULL; /* Quell bogus uninitialized warnings */
   const uint8_t *cipher_input;
   uint32_t cipher_input_len;
   uint32_t cipher_block_size;
   uint32_t encrypted_len;
#ifdef FD_NO_SEND_TEST
   return 1;
#endif

   msgsave = sd->msg;
   rbuf = sd->msg;                    /* read buffer */
   wbuf = sd->msg;                    /* write buffer */
   cipher_input = (uint8_t *)rbuf;    /* encrypt uncompressed data */

   Dmsg1(300, "Saving data, type=%d\n", ff_pkt->type);

#ifdef HAVE_LIBZ
   uLong compress_len = 0;
   uLong max_compress_len = 0;
   const Bytef *cbuf = NULL;
   int zstat;

   if (ff_pkt->flags & FO_GZIP) {
      if (ff_pkt->flags & FO_SPARSE) {
         cbuf = (Bytef *)jcr->compress_buf + SPARSE_FADDR_SIZE;
         max_compress_len = jcr->compress_buf_size - SPARSE_FADDR_SIZE;
      } else {
         cbuf = (Bytef *)jcr->compress_buf;
         max_compress_len = jcr->compress_buf_size; /* set max length */
      }
      wbuf = jcr->compress_buf;    /* compressed output here */
      cipher_input = (uint8_t *)jcr->compress_buf; /* encrypt compressed data */

      /* 
       * Only change zlib parameters if there is no pending operation.
       * This should never happen as deflatereset is called after each
       * deflate.
       */

      if (((z_stream*)jcr->pZLIB_compress_workset)->total_in == 0) {
         /* set gzip compression level - must be done per file */
         if ((zstat=deflateParams((z_stream*)jcr->pZLIB_compress_workset, 
              ff_pkt->GZIP_level, Z_DEFAULT_STRATEGY)) != Z_OK) {
            Jmsg(jcr, M_FATAL, 0, _("Compression deflateParams error: %d\n"), zstat);
            set_jcr_job_status(jcr, JS_ErrorTerminated);
            goto err;
         }
      }
   }
#else
   const uint32_t max_compress_len = 0;
#endif

   if (ff_pkt->flags & FO_ENCRYPT) {
      if (ff_pkt->flags & FO_SPARSE) {
         Jmsg0(jcr, M_FATAL, 0, _("Encrypting sparse data not supported.\n"));
         goto err;
      }
      /* Allocate the cipher context */
      if ((cipher_ctx = crypto_cipher_new(jcr->crypto.pki_session, true, 
           &cipher_block_size)) == NULL) {
         /* Shouldn't happen! */
         Jmsg0(jcr, M_FATAL, 0, _("Failed to initialize encryption context.\n"));
         goto err;
      }

      /*
       * Grow the crypto buffer, if necessary.
       * crypto_cipher_update() will buffer up to (cipher_block_size - 1).
       * We grow crypto_buf to the maximum number of blocks that
       * could be returned for the given read buffer size.
       * (Using the larger of either rsize or max_compress_len)
       */
      jcr->crypto.crypto_buf = check_pool_memory_size(jcr->crypto.crypto_buf, 
           (MAX(rsize + (int)sizeof(uint32_t), (int32_t)max_compress_len) + 
            cipher_block_size - 1) / cipher_block_size * cipher_block_size);

      wbuf = jcr->crypto.crypto_buf; /* Encrypted, possibly compressed output here. */
   }

   /*
    * Send Data header to Storage daemon
    *    <file-index> <stream> <info>
    */
   if (!sd->fsend("%ld %d 0", jcr->JobFiles, stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      goto err;
   }
   Dmsg1(300, ">stored: datahdr %s\n", sd->msg);

   /*
    * Make space at beginning of buffer for fileAddr because this
    *   same buffer will be used for writing if compression is off.
    */
   if (ff_pkt->flags & FO_SPARSE) {
      rbuf += SPARSE_FADDR_SIZE;
      rsize -= SPARSE_FADDR_SIZE;
#ifdef HAVE_FREEBSD_OS
      /*
       * To read FreeBSD partitions, the read size must be
       *  a multiple of 512.
       */
      rsize = (rsize/512) * 512;
#endif
   }

   /* a RAW device read on win32 only works if the buffer is a multiple of 512 */
#ifdef HAVE_WIN32
   if (S_ISBLK(ff_pkt->statp.st_mode))
      rsize = (rsize/512) * 512;
#endif
   
   /*
    * Read the file data
    */
   while ((sd->msglen=(uint32_t)bread(&ff_pkt->bfd, rbuf, rsize)) > 0) {

      /* Check for sparse blocks */
      if (ff_pkt->flags & FO_SPARSE) {
         ser_declare;
         bool allZeros = false;
         if ((sd->msglen == rsize &&
              fileAddr+sd->msglen < (uint64_t)ff_pkt->statp.st_size) ||
             ((ff_pkt->type == FT_RAW || ff_pkt->type == FT_FIFO) &&
               (uint64_t)ff_pkt->statp.st_size == 0)) {
            allZeros = is_buf_zero(rbuf, rsize);
         }
         if (!allZeros) {
            /* Put file address as first data in buffer */
            ser_begin(wbuf, SPARSE_FADDR_SIZE);
            ser_uint64(fileAddr);     /* store fileAddr in begin of buffer */
         }
         fileAddr += sd->msglen;      /* update file address */
         /* Skip block of all zeros */
         if (allZeros) {
            continue;                 /* skip block of zeros */
         }
      }

      jcr->ReadBytes += sd->msglen;         /* count bytes read */

      /* Uncompressed cipher input length */
      cipher_input_len = sd->msglen;

      /* Update checksum if requested */
      if (digest) {
         crypto_digest_update(digest, (uint8_t *)rbuf, sd->msglen);
      }

      /* Update signing digest if requested */
      if (signing_digest) {
         crypto_digest_update(signing_digest, (uint8_t *)rbuf, sd->msglen);
      }

#ifdef HAVE_LIBZ
      /* Do compression if turned on */
      if (ff_pkt->flags & FO_GZIP && jcr->pZLIB_compress_workset) {
         Dmsg3(400, "cbuf=0x%x rbuf=0x%x len=%u\n", cbuf, rbuf, sd->msglen);
         
         ((z_stream*)jcr->pZLIB_compress_workset)->next_in   = (Bytef *)rbuf;
                ((z_stream*)jcr->pZLIB_compress_workset)->avail_in  = sd->msglen;
         ((z_stream*)jcr->pZLIB_compress_workset)->next_out  = (Bytef *)cbuf;
                ((z_stream*)jcr->pZLIB_compress_workset)->avail_out = max_compress_len;

         if ((zstat=deflate((z_stream*)jcr->pZLIB_compress_workset, Z_FINISH)) != Z_STREAM_END) {
            Jmsg(jcr, M_FATAL, 0, _("Compression deflate error: %d\n"), zstat);
            set_jcr_job_status(jcr, JS_ErrorTerminated);
            goto err;
         }
         compress_len = ((z_stream*)jcr->pZLIB_compress_workset)->total_out;
         /* reset zlib stream to be able to begin from scratch again */
         if ((zstat=deflateReset((z_stream*)jcr->pZLIB_compress_workset)) != Z_OK) {
            Jmsg(jcr, M_FATAL, 0, _("Compression deflateReset error: %d\n"), zstat);
            set_jcr_job_status(jcr, JS_ErrorTerminated);
            goto err;
         }

         Dmsg2(400, "compressed len=%d uncompressed len=%d\n", compress_len, 
               sd->msglen);

         sd->msglen = compress_len;      /* set compressed length */
         cipher_input_len = compress_len;
      }
#endif
      /* 
       * Note, here we prepend the current record length to the beginning
       *  of the encrypted data. This is because both sparse and compression
       *  restore handling want records returned to them with exactly the
       *  same number of bytes that were processed in the backup handling.
       *  That is, both are block filters rather than a stream.  When doing
       *  compression, the compression routines may buffer data, so that for
       *  any one record compressed, when it is decompressed the same size
       *  will not be obtained. Of course, the buffered data eventually comes
       *  out in subsequent crypto_cipher_update() calls or at least
       *  when crypto_cipher_finalize() is called.  Unfortunately, this
       *  "feature" of encryption enormously complicates the restore code.
       */
      if (ff_pkt->flags & FO_ENCRYPT) {
         uint32_t initial_len = 0;
         ser_declare;

         if (ff_pkt->flags & FO_SPARSE) {
            cipher_input_len += SPARSE_FADDR_SIZE;
         }

         /* Encrypt the length of the input block */
         uint8_t packet_len[sizeof(uint32_t)];

         ser_begin(packet_len, sizeof(uint32_t));
         ser_uint32(cipher_input_len);    /* store data len in begin of buffer */
         Dmsg1(20, "Encrypt len=%d\n", cipher_input_len);

         if (!crypto_cipher_update(cipher_ctx, packet_len, sizeof(packet_len),
             (uint8_t *)jcr->crypto.crypto_buf, &initial_len)) {
            /* Encryption failed. Shouldn't happen. */
            Jmsg(jcr, M_FATAL, 0, _("Encryption error\n"));
            goto err;
         }

         /* Encrypt the input block */
         if (crypto_cipher_update(cipher_ctx, cipher_input, cipher_input_len, 
             (uint8_t *)&jcr->crypto.crypto_buf[initial_len], &encrypted_len)) {
            if ((initial_len + encrypted_len) == 0) {
               /* No full block of data available, read more data */
               continue;
            }
            Dmsg2(400, "encrypted len=%d unencrypted len=%d\n", encrypted_len, 
                  sd->msglen);
            sd->msglen = initial_len + encrypted_len; /* set encrypted length */
         } else {
            /* Encryption failed. Shouldn't happen. */
            Jmsg(jcr, M_FATAL, 0, _("Encryption error\n"));
            goto err;
         }
      }

      /* Send the buffer to the Storage daemon */
      if (ff_pkt->flags & FO_SPARSE) {
         sd->msglen += SPARSE_FADDR_SIZE; /* include fileAddr in size */
      }
      sd->msg = wbuf;              /* set correct write buffer */
      if (!sd->send()) {
         Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
               sd->bstrerror());
         goto err;
      }
      Dmsg1(130, "Send data to SD len=%d\n", sd->msglen);
      /*          #endif */
      jcr->JobBytes += sd->msglen;      /* count bytes saved possibly compressed/encrypted */
      sd->msg = msgsave;                /* restore read buffer */

   } /* end while read file data */

   if (sd->msglen < 0) {                 /* error */
      berrno be;
      Jmsg(jcr, M_ERROR, 0, _("Read error on file %s. ERR=%s\n"),
         ff_pkt->fname, be.bstrerror(ff_pkt->bfd.berrno));
      if (jcr->JobErrors++ > 1000) {       /* insanity check */
         Jmsg(jcr, M_FATAL, 0, _("Too many errors.\n"));
      }
   } else if (ff_pkt->flags & FO_ENCRYPT) {
      /* 
       * For encryption, we must call finalize to push out any
       *  buffered data.
       */
      if (!crypto_cipher_finalize(cipher_ctx, (uint8_t *)jcr->crypto.crypto_buf, 
           &encrypted_len)) {
         /* Padding failed. Shouldn't happen. */
         Jmsg(jcr, M_FATAL, 0, _("Encryption padding error\n"));
         goto err;
      }

      /* Note, on SSL pre-0.9.7, there is always some output */
      if (encrypted_len > 0) {
         sd->msglen = encrypted_len;      /* set encrypted length */
         sd->msg = jcr->crypto.crypto_buf;       /* set correct write buffer */
         if (!sd->send()) {
            Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
                  sd->bstrerror());
            goto err;
         }
         Dmsg1(130, "Send data to SD len=%d\n", sd->msglen);
         jcr->JobBytes += sd->msglen;     /* count bytes saved possibly compressed/encrypted */
         sd->msg = msgsave;               /* restore bnet buffer */
      }
   }

   if (!sd->signal(BNET_EOD)) {        /* indicate end of file data */
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      goto err;
   }

   /* Free the cipher context */
   if (cipher_ctx) {
      crypto_cipher_free(cipher_ctx);
   }
   return 1;

err:
   /* Free the cipher context */
   if (cipher_ctx) {
      crypto_cipher_free(cipher_ctx);
   }

   sd->msg = msgsave; /* restore bnet buffer */
   sd->msglen = 0;
   return 0;
}

bool encode_and_send_attributes(JCR *jcr, FF_PKT *ff_pkt, int &data_stream) 
{
   BSOCK *sd = jcr->store_bsock;
   char attribs[MAXSTRING];
   char attribsEx[MAXSTRING];
   int attr_stream;
   int stat;
#ifdef FD_NO_SEND_TEST
   return true;
#endif

   Dmsg1(300, "encode_and_send_attrs fname=%s\n", ff_pkt->fname);
   /* Find what data stream we will use, then encode the attributes */
   if ((data_stream = select_data_stream(ff_pkt)) == STREAM_NONE) {
      /* This should not happen */
      Jmsg0(jcr, M_FATAL, 0, _("Invalid file flags, no supported data stream type.\n"));
      return false;
   }
   encode_stat(attribs, &ff_pkt->statp, ff_pkt->LinkFI, data_stream);

   /* Now possibly extend the attributes */
   attr_stream = encode_attribsEx(jcr, attribsEx, ff_pkt);

   Dmsg3(300, "File %s\nattribs=%s\nattribsEx=%s\n", ff_pkt->fname, attribs, attribsEx);

   jcr->lock();
   jcr->JobFiles++;                    /* increment number of files sent */
   ff_pkt->FileIndex = jcr->JobFiles;  /* return FileIndex */
   pm_strcpy(jcr->last_fname, ff_pkt->fname);
   jcr->unlock();

   /*
    * Send Attributes header to Storage daemon
    *    <file-index> <stream> <info>
    */
   if (!sd->fsend("%ld %d 0", jcr->JobFiles, attr_stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return false;
   }
   Dmsg1(300, ">stored: attrhdr %s\n", sd->msg);

   /*
    * Send file attributes to Storage daemon
    *   File_index
    *   File type
    *   Filename (full path)
    *   Encoded attributes
    *   Link name (if type==FT_LNK or FT_LNKSAVED)
    *   Encoded extended-attributes (for Win32)
    *
    * For a directory, link is the same as fname, but with trailing
    * slash. For a linked file, link is the link.
    */
   if (ff_pkt->type != FT_DELETED) { /* already stripped */
      strip_path(ff_pkt);
   }
   if (ff_pkt->type == FT_LNK || ff_pkt->type == FT_LNKSAVED) {
      Dmsg2(300, "Link %s to %s\n", ff_pkt->fname, ff_pkt->link);
      stat = sd->fsend("%ld %d %s%c%s%c%s%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->fname, 0, attribs, 0, ff_pkt->link, 0,
               attribsEx, 0);
   } else if (ff_pkt->type == FT_DIREND || ff_pkt->type == FT_REPARSE) {
      /* Here link is the canonical filename (i.e. with trailing slash) */
      stat = sd->fsend("%ld %d %s%c%s%c%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->link, 0, attribs, 0, 0, attribsEx, 0);
   } else {
      stat = sd->fsend("%ld %d %s%c%s%c%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->fname, 0, attribs, 0, 0, attribsEx, 0);
   }
   if (ff_pkt->type != FT_DELETED) {
      unstrip_path(ff_pkt);
   }

   Dmsg2(300, ">stored: attr len=%d: %s\n", sd->msglen, sd->msg);
   if (!stat) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
      return false;
   }
   sd->signal(BNET_EOD);            /* indicate end of attributes data */
   return true;
}

/* 
 * Do in place strip of path
 */
static bool do_strip(int count, char *in)
{
   char *out = in;
   int stripped;
   int numsep = 0;

   /* Copy to first path separator -- Win32 might have c: ... */
   while (*in && !IsPathSeparator(*in)) {    
      out++; in++;
   }
   out++; in++;
   numsep++;                     /* one separator seen */
   for (stripped=0; stripped<count && *in; stripped++) {
      while (*in && !IsPathSeparator(*in)) {
         in++;                   /* skip chars */
      }
      if (*in) {
         numsep++;               /* count separators seen */
         in++;                   /* skip separator */
      }
   }
   /* Copy to end */
   while (*in) {                /* copy to end */
      if (IsPathSeparator(*in)) {
         numsep++;
      }
      *out++ = *in++;
   }
   *out = 0;
   Dmsg4(500, "stripped=%d count=%d numsep=%d sep>count=%d\n", 
         stripped, count, numsep, numsep>count);
   return stripped==count && numsep>count;
}

/*
 * If requested strip leading components of the path so that we can
 *   save file as if it came from a subdirectory.  This is most useful
 *   for dealing with snapshots, by removing the snapshot directory, or
 *   in handling vendor migrations where files have been restored with
 *   a vendor product into a subdirectory.
 */
void strip_path(FF_PKT *ff_pkt)
{
   if (!(ff_pkt->flags & FO_STRIPPATH) || ff_pkt->strip_path <= 0) {
      Dmsg1(200, "No strip for %s\n", ff_pkt->fname);
      return;
   }
   if (!ff_pkt->fname_save) {
     ff_pkt->fname_save = get_pool_memory(PM_FNAME); 
     ff_pkt->link_save = get_pool_memory(PM_FNAME);
   }
   pm_strcpy(ff_pkt->fname_save, ff_pkt->fname);
   if (ff_pkt->type != FT_LNK && ff_pkt->fname != ff_pkt->link) {
      pm_strcpy(ff_pkt->link_save, ff_pkt->link);
      Dmsg2(500, "strcpy link_save=%d link=%d\n", strlen(ff_pkt->link_save),
         strlen(ff_pkt->link));
      sm_check(__FILE__, __LINE__, true);
   }

   /* 
    * Strip path.  If it doesn't succeed put it back.  If
    *  it does, and there is a different link string,
    *  attempt to strip the link. If it fails, back them
    *  both back.
    * Do not strip symlinks.
    * I.e. if either stripping fails don't strip anything.
    */
   if (!do_strip(ff_pkt->strip_path, ff_pkt->fname)) {
      unstrip_path(ff_pkt);
      goto rtn;
   } 
   /* Strip links but not symlinks */
   if (ff_pkt->type != FT_LNK && ff_pkt->fname != ff_pkt->link) {
      if (!do_strip(ff_pkt->strip_path, ff_pkt->link)) {
         unstrip_path(ff_pkt);
      }
   }

rtn:
   Dmsg3(100, "fname=%s stripped=%s link=%s\n", ff_pkt->fname_save, ff_pkt->fname, 
       ff_pkt->link);
}

void unstrip_path(FF_PKT *ff_pkt)
{
   if (!(ff_pkt->flags & FO_STRIPPATH) || ff_pkt->strip_path <= 0) {
      return;
   }
   strcpy(ff_pkt->fname, ff_pkt->fname_save);
   if (ff_pkt->type != FT_LNK && ff_pkt->fname != ff_pkt->link) {
      Dmsg2(500, "strcpy link=%s link_save=%s\n", ff_pkt->link,
          ff_pkt->link_save);
      strcpy(ff_pkt->link, ff_pkt->link_save);
      Dmsg2(500, "strcpy link=%d link_save=%d\n", strlen(ff_pkt->link),
          strlen(ff_pkt->link_save));
      sm_check(__FILE__, __LINE__, true);
   }
}
