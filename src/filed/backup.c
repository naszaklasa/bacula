/*
 *  Bacula File Daemon  backup.c  send file attributes and data
 *   to the Storage daemon.
 *
 *    Kern Sibbald, March MM
 *
 *   Version $Id: backup.c,v 1.89.2.4 2006/03/14 21:41:40 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "filed.h"

/* Forward referenced functions */
static int save_file(FF_PKT *ff_pkt, void *pkt, bool top_level);
static int send_data(JCR *jcr, int stream, FF_PKT *ff_pkt, struct CHKSUM *chksum);
static bool encode_and_send_attributes(JCR *jcr, FF_PKT *ff_pkt, int &data_stream);
static bool read_and_send_acl(JCR *jcr, int acltype, int stream);

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

   sd = jcr->store_bsock;

   set_jcr_job_status(jcr, JS_Running);

   Dmsg1(300, "bfiled: opened data connection %d to stored\n", sd->fd);

   LockRes();
   CLIENT *client = (CLIENT *)GetNextRes(R_CLIENT, NULL);
   UnlockRes();
   uint32_t buf_size;
   if (client) {
      buf_size = client->max_network_buffer_size;
   } else {
      buf_size = 0;                   /* use default */
   }
   if (!bnet_set_buffer_size(sd, buf_size, BNET_SETBUF_WRITE)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      Jmsg(jcr, M_FATAL, 0, _("Cannot set buffer size FD->SD.\n"));
      return false;
   }

   jcr->buf_size = sd->msglen;
   /* Adjust for compression so that output buffer is
    * 12 bytes + 0.1% larger than input buffer plus 18 bytes.
    * This gives a bit extra plus room for the sparse addr if any.
    * Note, we adjust the read size to be smaller so that the
    * same output buffer can be used without growing it.
    */
   jcr->compress_buf_size = jcr->buf_size + ((jcr->buf_size+999) / 1000) + 30;
   jcr->compress_buf = get_memory(jcr->compress_buf_size);

   Dmsg1(300, "set_find_options ff=%p\n", jcr->ff);
   set_find_options((FF_PKT *)jcr->ff, jcr->incremental, jcr->mtime);
   Dmsg0(300, "start find files\n");

   start_heartbeat_monitor(jcr);

   jcr->acl_text = get_pool_memory(PM_MESSAGE);

   /* Subroutine save_file() is called for each file */
   if (!find_files(jcr, (FF_PKT *)jcr->ff, save_file, (void *)jcr)) {
      ok = false;                     /* error */
      set_jcr_job_status(jcr, JS_ErrorTerminated);
//    Jmsg(jcr, M_FATAL, 0, _("Find files error.\n"));
   }

   free_pool_memory(jcr->acl_text);

   stop_heartbeat_monitor(jcr);

   bnet_sig(sd, BNET_EOD);            /* end of sending data */

   if (jcr->big_buf) {
      free(jcr->big_buf);
      jcr->big_buf = NULL;
   }
   if (jcr->compress_buf) {
      free_pool_memory(jcr->compress_buf);
      jcr->compress_buf = NULL;
   }
   Dmsg1(100, "end blast_data ok=%d\n", ok);
   return ok;
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
static int save_file(FF_PKT *ff_pkt, void *vjcr, bool top_level)
{
   int stat, data_stream;
   struct CHKSUM chksum;
   BSOCK *sd;
   JCR *jcr = (JCR *)vjcr;

   if (job_canceled(jcr)) {
      return 0;
   }

   sd = jcr->store_bsock;
   jcr->num_files_examined++;         /* bump total file count */

   switch (ff_pkt->type) {
   case FT_LNKSAVED:                  /* Hard linked, file already saved */
      Dmsg2(130, "FT_LNKSAVED hard link: %s => %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_REGE:
      Dmsg1(130, "FT_REGE saving: %s\n", ff_pkt->fname);
      break;
   case FT_REG:
      Dmsg1(130, "FT_REG saving: %s\n", ff_pkt->fname);
      break;
   case FT_LNK:
      Dmsg2(130, "FT_LNK saving: %s -> %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_DIRBEGIN:
      return 1;                       /* not used */
   case FT_NORECURSE:
     Jmsg(jcr, M_INFO, 1, _("     Recursion turned off. Will not descend into %s\n"),
          ff_pkt->fname);
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_NOFSCHG:
      /* Suppress message for /dev filesystems */
      if (strncmp(ff_pkt->fname, "/dev/", 5) != 0) {
         Jmsg(jcr, M_INFO, 1, _("     Filesystem change prohibited. Will not descend into %s\n"),
            ff_pkt->fname);
      }
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_INVALIDFS:
      Jmsg(jcr, M_INFO, 1, _("     Disallowed filesystem. Will not descend into %s\n"),
           ff_pkt->fname);
      ff_pkt->type = FT_DIREND;       /* Backup only the directory entry */
      break;
   case FT_DIREND:
      Dmsg1(130, "FT_DIREND: %s\n", ff_pkt->link);
      break;
   case FT_SPEC:
      Dmsg1(130, "FT_SPEC saving: %s\n", ff_pkt->fname);
      break;
   case FT_RAW:
      Dmsg1(130, "FT_RAW saving: %s\n", ff_pkt->fname);
      break;
   case FT_FIFO:
      Dmsg1(130, "FT_FIFO saving: %s\n", ff_pkt->fname);
      break;
   case FT_NOACCESS: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not access %s: ERR=%s\n"), ff_pkt->fname,
         be.strerror(ff_pkt->ff_errno));
      jcr->Errors++;
      return 1;
   }
   case FT_NOFOLLOW: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not follow link %s: ERR=%s\n"), ff_pkt->fname,
         be.strerror(ff_pkt->ff_errno));
      jcr->Errors++;
      return 1;
   }
   case FT_NOSTAT: {
      berrno be;
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not stat %s: ERR=%s\n"), ff_pkt->fname,
         be.strerror(ff_pkt->ff_errno));
      jcr->Errors++;
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
      Jmsg(jcr, M_NOTSAVED, 0, _("     Could not open directory %s: ERR=%s\n"), ff_pkt->fname,
         be.strerror(ff_pkt->ff_errno));
      jcr->Errors++;
      return 1;
   }
   default:
      Jmsg(jcr, M_NOTSAVED, 0,  _("     Unknown file type %d; not saved: %s\n"), ff_pkt->type, ff_pkt->fname);
      jcr->Errors++;
      return 1;
   }

   Dmsg1(130, "bfiled: sending %s to stored\n", ff_pkt->fname);


   /*
    * Setup for signature handling.
    * Then initialise the file descriptor we use for data and other streams.
    */
   chksum_init(&chksum, ff_pkt->flags);

   binit(&ff_pkt->bfd);
   if (ff_pkt->flags & FO_PORTABLE) {
      set_portable_backup(&ff_pkt->bfd); /* disable Win32 BackupRead() */
   }
   if (ff_pkt->reader) {
      if (!set_prog(&ff_pkt->bfd, ff_pkt->reader, jcr)) {
         Jmsg(jcr, M_FATAL, 0, _("Python reader program \"%s\" not found.\n"), 
            ff_pkt->reader);
         return 0;
      }
   }

   if (!encode_and_send_attributes(jcr, ff_pkt, data_stream)) {
      return 0;
   }

   /*
    * Open any file with data that we intend to save, then save it.
    *
    * Note, if is_win32_backup, we must open the Directory so that
    * the BackupRead will save its permissions and ownership streams.
    */
   if (ff_pkt->type != FT_LNKSAVED && (S_ISREG(ff_pkt->statp.st_mode) &&
         ff_pkt->statp.st_size > 0) ||
         ff_pkt->type == FT_RAW || ff_pkt->type == FT_FIFO ||
         (!is_portable_backup(&ff_pkt->bfd) && ff_pkt->type == FT_DIREND)) {
      btimer_t *tid;
      if (ff_pkt->type == FT_FIFO) {
         tid = start_thread_timer(pthread_self(), 60);
      } else {
         tid = NULL;
      }
      if (bopen(&ff_pkt->bfd, ff_pkt->fname, O_RDONLY | O_BINARY, 0) < 0) {
         ff_pkt->ff_errno = errno;
         berrno be;
         Jmsg(jcr, M_NOTSAVED, 0, _("     Cannot open %s: ERR=%s.\n"), ff_pkt->fname,
              be.strerror());
         jcr->Errors++;
         if (tid) {
            stop_thread_timer(tid);
            tid = NULL;
         }
         return 1;
      }
      if (tid) {
         stop_thread_timer(tid);
         tid = NULL;
      }
      stat = send_data(jcr, data_stream, ff_pkt, &chksum);
      bclose(&ff_pkt->bfd);
      if (!stat) {
         return 0;
      }
   }

#ifdef HAVE_DARWIN_OS
   /* Regular files can have resource forks and Finder Info */
   if (ff_pkt->type != FT_LNKSAVED && (S_ISREG(ff_pkt->statp.st_mode) &&
            ff_pkt->flags & FO_HFSPLUS)) {
      if (ff_pkt->hfsinfo.rsrclength > 0) {
         int flags;
         if (!bopen_rsrc(&ff_pkt->bfd, ff_pkt->fname, O_RDONLY | O_BINARY, 0) < 0) {
            ff_pkt->ff_errno = errno;
            berrno be;
            Jmsg(jcr, M_NOTSAVED, -1, _("     Cannot open resource fork for %s: ERR=%s.\n"), ff_pkt->fname,
                  be.strerror());
            jcr->Errors++;
            if (is_bopen(&ff_pkt->bfd)) {
               bclose(&ff_pkt->bfd);
            }
            return 1;
         }
         flags = ff_pkt->flags;
         ff_pkt->flags &= ~(FO_GZIP|FO_SPARSE);
         stat = send_data(jcr, STREAM_MACOS_FORK_DATA, ff_pkt, &chksum);
         ff_pkt->flags = flags;
         bclose(&ff_pkt->bfd);
         if (!stat) {
            return 0;
         }
      }

      Dmsg1(300, "Saving Finder Info for \"%s\"\n", ff_pkt->fname);
      bnet_fsend(sd, "%ld %d 0", jcr->JobFiles, STREAM_HFSPLUS_ATTRIBUTES);
      Dmsg1(300, "bfiled>stored:header %s\n", sd->msg);
      memcpy(sd->msg, ff_pkt->hfsinfo.fndrinfo, 32);
      sd->msglen = 32;
      chksum_update(&chksum, (unsigned char *)sd->msg, sd->msglen);
      bnet_send(sd);
      bnet_sig(sd, BNET_EOD);
   }
#endif

   if (ff_pkt->flags & FO_ACL) {
      /* Read access ACLs for files, dirs and links */
      if (!read_and_send_acl(jcr, BACL_TYPE_ACCESS, STREAM_UNIX_ATTRIBUTES_ACCESS_ACL)) {
         return 0;
      }
      /* Directories can have default ACLs too */
      if (ff_pkt->type == FT_DIREND && (BACL_CAP & BACL_CAP_DEFAULTS_DIR)) {
         if (!read_and_send_acl(jcr, BACL_TYPE_DEFAULT, STREAM_UNIX_ATTRIBUTES_DEFAULT_ACL)) {
            return 0;
         }
      }
   }

   /* Terminate any signature and send it to Storage daemon and the Director */
   if (chksum.updated) {
      int stream = 0;
      chksum_final(&chksum);
      if (chksum.type == CHKSUM_MD5) {
         stream = STREAM_MD5_SIGNATURE;
      } else if (chksum.type == CHKSUM_SHA1) {
         stream = STREAM_SHA1_SIGNATURE;
      } else {
         Jmsg1(jcr, M_WARNING, 0, _("Unknown signature type %i.\n"), chksum.type);
      }
      if (stream != 0) {
         bnet_fsend(sd, "%ld %d 0", jcr->JobFiles, stream);
         Dmsg1(300, "bfiled>stored:header %s\n", sd->msg);
         memcpy(sd->msg, chksum.signature, chksum.length);
         sd->msglen = chksum.length;
         bnet_send(sd);
         bnet_sig(sd, BNET_EOD);              /* end of checksum */
      }
   }

   return 1;
}

/*
 * Send data read from an already open file descriptor.
 *
 * We return 1 on sucess and 0 on errors.
 *
 * ***FIXME***
 * We use ff_pkt->statp.st_size when FO_SPARSE.
 * Currently this is not a problem as the only other stream, resource forks,
 * are not handled as sparse files.
 */
static int send_data(JCR *jcr, int stream, FF_PKT *ff_pkt, struct CHKSUM *chksum)
{
   BSOCK *sd = jcr->store_bsock;
   uint64_t fileAddr = 0;             /* file address */
   char *rbuf, *wbuf;
   int rsize = jcr->buf_size;      /* read buffer size */
   POOLMEM *msgsave;
#ifdef FD_NO_SEND_TEST
   return 1;
#endif

   msgsave = sd->msg;
   rbuf = sd->msg;                    /* read buffer */
   wbuf = sd->msg;                    /* write buffer */


   Dmsg1(300, "Saving data, type=%d\n", ff_pkt->type);


#ifdef HAVE_LIBZ
   uLong compress_len, max_compress_len = 0;
   const Bytef *cbuf = NULL;

   if (ff_pkt->flags & FO_GZIP) {
      if (ff_pkt->flags & FO_SPARSE) {
         cbuf = (Bytef *)jcr->compress_buf + SPARSE_FADDR_SIZE;
         max_compress_len = jcr->compress_buf_size - SPARSE_FADDR_SIZE;
      } else {
         cbuf = (Bytef *)jcr->compress_buf;
         max_compress_len = jcr->compress_buf_size; /* set max length */
      }
      wbuf = jcr->compress_buf;    /* compressed output here */
   }
#endif

   /*
    * Send Data header to Storage daemon
    *    <file-index> <stream> <info>
    */
   if (!bnet_fsend(sd, "%ld %d 0", jcr->JobFiles, stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return 0;
   }
   Dmsg1(300, ">stored: datahdr %s\n", sd->msg);

   /*
    * Make space at beginning of buffer for fileAddr because this
    *   same buffer will be used for writing if compression if off.
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
      int sparseBlock = 0;

      /* Check for sparse blocks */
      if (ff_pkt->flags & FO_SPARSE) {
         ser_declare;
         if (sd->msglen == rsize &&
             fileAddr+sd->msglen < (uint64_t)ff_pkt->statp.st_size ||
             ((ff_pkt->type == FT_RAW || ff_pkt->type == FT_FIFO) &&
               (uint64_t)ff_pkt->statp.st_size == 0)) {
            sparseBlock = is_buf_zero(rbuf, rsize);
         }

         ser_begin(wbuf, SPARSE_FADDR_SIZE);
         ser_uint64(fileAddr);     /* store fileAddr in begin of buffer */
      }

      jcr->ReadBytes += sd->msglen;         /* count bytes read */
      fileAddr += sd->msglen;

      /* Update checksum if requested */
      chksum_update(chksum, (unsigned char *)rbuf, sd->msglen);

#ifdef HAVE_LIBZ
      /* Do compression if turned on */
      if (!sparseBlock && ff_pkt->flags & FO_GZIP) {
         int zstat;
         compress_len = max_compress_len;
         Dmsg4(400, "cbuf=0x%x len=%u rbuf=0x%x len=%u\n", cbuf, compress_len,
            rbuf, sd->msglen);
         /* NOTE! This call modifies compress_len !!! */
         if ((zstat=compress2((Bytef *)cbuf, &compress_len,
               (const Bytef *)rbuf, (uLong)sd->msglen,
               ff_pkt->GZIP_level)) != Z_OK) {
            Jmsg(jcr, M_FATAL, 0, _("Compression error: %d\n"), zstat);
            sd->msg = msgsave;
            sd->msglen = 0;
            set_jcr_job_status(jcr, JS_ErrorTerminated);
            return 0;
         }
         Dmsg2(400, "compressed len=%d uncompressed len=%d\n",
            compress_len, sd->msglen);

         sd->msglen = compress_len;      /* set compressed length */
      }
#endif

      /* Send the buffer to the Storage daemon */
      if (!sparseBlock) {
         if (ff_pkt->flags & FO_SPARSE) {
            sd->msglen += SPARSE_FADDR_SIZE; /* include fileAddr in size */
         }
         sd->msg = wbuf;              /* set correct write buffer */
         if (!bnet_send(sd)) {
            Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
                  bnet_strerror(sd));
            sd->msg = msgsave;     /* restore bnet buffer */
            sd->msglen = 0;
            return 0;
         }
      }
      Dmsg1(130, "Send data to SD len=%d\n", sd->msglen);
      /*          #endif */
      jcr->JobBytes += sd->msglen;      /* count bytes saved possibly compressed */
      sd->msg = msgsave;                /* restore read buffer */

   } /* end while read file data */


   if (sd->msglen < 0) {
      berrno be;
      Jmsg(jcr, M_ERROR, 0, _("Read error on file %s. ERR=%s\n"),
         ff_pkt->fname, be.strerror(ff_pkt->bfd.berrno));
      if (jcr->Errors++ > 1000) {       /* insanity check */
         Jmsg(jcr, M_FATAL, 0, _("Too many errors.\n"));
      }

   }

   if (!bnet_sig(sd, BNET_EOD)) {        /* indicate end of file data */
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return 0;
   }

   return 1;
}

/*
 * Read and send an ACL for the last encountered file.
 */
static bool read_and_send_acl(JCR *jcr, int acltype, int stream)
{
#ifdef HAVE_ACL
   BSOCK *sd = jcr->store_bsock;
   POOLMEM *msgsave;
   int len;
#ifdef FD_NO_SEND_TEST
   return true;
#endif

   len = bacl_get(jcr, acltype);
   if (len < 0) {
      Jmsg1(jcr, M_WARNING, 0, _("Error reading ACL of %s\n"), jcr->last_fname);
      return true; 
   }
   if (len == 0) {
      return true;                    /* no ACL */
   }

   /* Send header */
   if (!bnet_fsend(sd, "%ld %d 0", jcr->JobFiles, stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return false;
   }

   /* Send the buffer to the storage deamon */
   Dmsg2(400, "Backing up ACL type 0x%2x <%s>\n", acltype, jcr->acl_text);
   msgsave = sd->msg;
   sd->msg = jcr->acl_text;
   sd->msglen = len + 1;
   if (!bnet_send(sd)) {
      sd->msg = msgsave;
      sd->msglen = 0;
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return false;
   }

   jcr->JobBytes += sd->msglen;
   sd->msg = msgsave;
   if (!bnet_sig(sd, BNET_EOD)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return false;
   }

   Dmsg1(200, "ACL of file: %s successfully backed up!\n", jcr->last_fname);
#endif
   return true;
}

static bool encode_and_send_attributes(JCR *jcr, FF_PKT *ff_pkt, int &data_stream) 
{
   BSOCK *sd = jcr->store_bsock;
   char attribs[MAXSTRING];
   char attribsEx[MAXSTRING];
   int attr_stream;
   int stat;
#ifdef FD_NO_SEND_TEST
   return true;
#endif

   /* Find what data stream we will use, then encode the attributes */
   data_stream = select_data_stream(ff_pkt);
   encode_stat(attribs, ff_pkt, data_stream);

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
   if (!bnet_fsend(sd, "%ld %d 0", jcr->JobFiles, attr_stream)) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
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
   if (ff_pkt->type == FT_LNK || ff_pkt->type == FT_LNKSAVED) {
      Dmsg2(300, "Link %s to %s\n", ff_pkt->fname, ff_pkt->link);
      stat = bnet_fsend(sd, "%ld %d %s%c%s%c%s%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->fname, 0, attribs, 0, ff_pkt->link, 0,
               attribsEx, 0);
   } else if (ff_pkt->type == FT_DIREND) {
      /* Here link is the canonical filename (i.e. with trailing slash) */
      stat = bnet_fsend(sd, "%ld %d %s%c%s%c%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->link, 0, attribs, 0, 0, attribsEx, 0);
   } else {
      stat = bnet_fsend(sd, "%ld %d %s%c%s%c%c%s%c", jcr->JobFiles,
               ff_pkt->type, ff_pkt->fname, 0, attribs, 0, 0, attribsEx, 0);
   }

   Dmsg2(300, ">stored: attr len=%d: %s\n", sd->msglen, sd->msg);
   if (!stat) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            bnet_strerror(sd));
      return false;
   }
   bnet_sig(sd, BNET_EOD);            /* indicate end of attributes data */
   return true;
}
