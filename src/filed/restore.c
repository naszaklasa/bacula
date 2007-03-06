/*
 *  Bacula File Daemon  restore.c Restorefiles.
 *
 *    Kern Sibbald, November MM
 *
 *   Version $Id: restore.c,v 1.65.2.5 2006/06/08 15:52:04 kerns Exp $
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

#ifdef HAVE_DARWIN_OS
#include <sys/attr.h>
#endif

/* Data received from Storage Daemon */
static char rec_header[] = "rechdr %ld %ld %ld %ld %ld";

/* Forward referenced functions */
#ifdef HAVE_LIBZ
static const char *zlib_strerror(int stat);
#endif
int32_t extract_data(JCR *jcr, BFILE *bfd, POOLMEM *buf, int32_t buflen,
      uint64_t *addr, int flags);

#define RETRY 10                      /* retry wait time */

/*
 * Close a bfd check that we are at the expected file offset.
 * Makes some code in set_attributes().
 */
int bclose_chksize(JCR *jcr, BFILE *bfd, off_t osize)
{
   char ec1[50], ec2[50];
   off_t fsize;

   fsize = blseek(bfd, 0, SEEK_CUR);
   bclose(bfd);                              /* first close file */
   if (fsize > 0 && fsize != osize) {
      Qmsg3(jcr, M_ERROR, 0, _("Size of data or stream of %s not correct. Original %s, restored %s.\n"),
            jcr->last_fname, edit_uint64(osize, ec1),
            edit_uint64(fsize, ec2));
      return -1;
   }
   return 0;
}

/*
 * Restore the requested files.
 *
 */
void do_restore(JCR *jcr)
{
   BSOCK *sd;
   int32_t stream = 0;
   int32_t prev_stream;
   uint32_t VolSessionId, VolSessionTime;
   bool extract = false;
   int32_t file_index;
   char ec1[50];                      /* Buffer printing huge values */

   BFILE bfd;                         /* File content */
   uint64_t fileAddr = 0;             /* file write address */
   uint32_t size;                     /* Size of file */
   BFILE altbfd;                      /* Alternative data stream */
   uint64_t alt_addr = 0;             /* Write address for alternative stream */
   intmax_t alt_size = 0;             /* Size of alternate stream */
   int flags;                         /* Options for extract_data() */
   int stat;
   ATTR *attr;

   /* The following variables keep track of "known unknowns" */
   int non_support_data = 0;
   int non_support_attr = 0;
   int non_support_rsrc = 0;
   int non_support_finfo = 0;
   int non_support_acl = 0;
   int non_support_progname = 0;

   /* Finally, set up for special configurations */
#ifdef HAVE_DARWIN_OS
   intmax_t rsrc_len = 0;             /* Original length of resource fork */
   struct attrlist attrList;

   memset(&attrList, 0, sizeof(attrList));
   attrList.bitmapcount = ATTR_BIT_MAP_COUNT;
   attrList.commonattr = ATTR_CMN_FNDRINFO;
#endif

   sd = jcr->store_bsock;
   set_jcr_job_status(jcr, JS_Running);

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
      return;
   }
   jcr->buf_size = sd->msglen;

#ifdef HAVE_LIBZ
   uint32_t compress_buf_size = jcr->buf_size + 12 + ((jcr->buf_size+999) / 1000) + 100;
   jcr->compress_buf = (char *)bmalloc(compress_buf_size);
   jcr->compress_buf_size = compress_buf_size;
#endif

   /*
    * Get a record from the Storage daemon. We are guaranteed to
    *   receive records in the following order:
    *   1. Stream record header
    *   2. Stream data
    *        a. Attributes (Unix or Win32)
    *    or  b. File data for the file
    *    or  c. Alternate data stream (e.g. Resource Fork)
    *    or  d. Finder info
    *    or  e. ACLs
    *    or  f. Possibly MD5 or SHA1 record
    *   3. Repeat step 1
    *
    * NOTE: We keep track of two bacula file descriptors:
    *   1. bfd for file data.
    *      This fd is opened for non empty files when an attribute stream is
    *      encountered and closed when we find the next attribute stream.
    *   2. alt_bfd for alternate data streams
    *      This fd is opened every time we encounter a new alternate data
    *      stream for the current file. When we find any other stream, we
    *      close it again.
    *      The expected size of the stream, alt_len, should be set when
    *      opening the fd.
    */
   binit(&bfd);
   binit(&altbfd);
   attr = new_attr();
   jcr->acl_text = get_pool_memory(PM_MESSAGE);

   while (bget_msg(sd) >= 0 && !job_canceled(jcr)) {
      /* Remember previous stream type */
      prev_stream = stream;

      /* First we expect a Stream Record Header */
      if (sscanf(sd->msg, rec_header, &VolSessionId, &VolSessionTime, &file_index,
          &stream, &size) != 5) {
         Jmsg1(jcr, M_FATAL, 0, _("Record header scan error: %s\n"), sd->msg);
         goto bail_out;
      }
      Dmsg2(30, "Got hdr: FilInx=%d Stream=%d.\n", file_index, stream);

      /* * Now we expect the Stream Data */
      if (bget_msg(sd) < 0) {
         Jmsg1(jcr, M_FATAL, 0, _("Data record error. ERR=%s\n"), bnet_strerror(sd));
         goto bail_out;
      }
      if (size != (uint32_t)sd->msglen) {
         Jmsg2(jcr, M_FATAL, 0, _("Actual data size %d not same as header %d\n"), sd->msglen, size);
         goto bail_out;
      }
      Dmsg1(30, "Got stream data, len=%d\n", sd->msglen);

      /* If we change streams, close and reset alternate data streams */
      if (prev_stream != stream) {
         if (is_bopen(&altbfd)) {
            bclose_chksize(jcr, &altbfd, alt_size);
         }
         alt_size = -1; /* Use an impossible value and set a proper one below */
         alt_addr = 0;
      }

      /* File Attributes stream */
      switch (stream) {
      case STREAM_UNIX_ATTRIBUTES:
      case STREAM_UNIX_ATTRIBUTES_EX:
         Dmsg1(30, "Stream=Unix Attributes. extract=%d\n", extract);
         /*
          * If extracting, it was from previous stream, so
          * close the output file.
          */
         if (extract) {
            if (size > 0 && !is_bopen(&bfd)) {
               Jmsg0(jcr, M_ERROR, 0, _("Logic error: output file should be open\n"));
            }
            set_attributes(jcr, attr, &bfd);
            extract = false;
            Dmsg0(30, "Stop extracting.\n");
         } else if (is_bopen(&bfd)) {
            Jmsg0(jcr, M_ERROR, 0, _("Logic error: output file should not be open\n"));
            bclose(&bfd);
         }

         /*
          * Unpack and do sanity check fo attributes.
          */
         if (!unpack_attributes_record(jcr, stream, sd->msg, attr)) {
            goto bail_out;
         }
         if (file_index != attr->file_index) {
            Jmsg(jcr, M_FATAL, 0, _("Record header file index %ld not equal record index %ld\n"),
                 file_index, attr->file_index);
            Dmsg0(100, "File index error\n");
            goto bail_out;
         }

         Dmsg3(200, "File %s\nattrib=%s\nattribsEx=%s\n", attr->fname,
               attr->attr, attr->attrEx);

         attr->data_stream = decode_stat(attr->attr, &attr->statp, &attr->LinkFI);

         if (!is_restore_stream_supported(attr->data_stream)) {
            if (!non_support_data++) {
               Jmsg(jcr, M_ERROR, 0, _("%s stream not supported on this Client.\n"),
                  stream_to_ascii(attr->data_stream));
            }
            continue;
         }

         build_attr_output_fnames(jcr, attr);

         /*
          * Now determine if we are extracting or not.
          */
         jcr->num_files_examined++;
         Dmsg1(30, "Outfile=%s\n", attr->ofname);
         extract = false;
         stat = create_file(jcr, attr, &bfd, jcr->replace);
         switch (stat) {
         case CF_ERROR:
         case CF_SKIP:
            break;
         case CF_EXTRACT:        /* File created and we expect file data */
            extract = true;
            /* FALLTHROUGH */
         case CF_CREATED:        /* File created, but there is no content */
            jcr->lock();
            pm_strcpy(jcr->last_fname, attr->ofname);
            jcr->last_type = attr->type;
            jcr->unlock();
            jcr->JobFiles++;
            fileAddr = 0;
            print_ls_output(jcr, attr);
#ifdef HAVE_DARWIN_OS
            /* Only restore the resource fork for regular files */
            from_base64(&rsrc_len, attr->attrEx);
            if (attr->type == FT_REG && rsrc_len > 0) {
               extract = true;
            }
#endif
            if (!extract) {
               /* set attributes now because file will not be extracted */
               set_attributes(jcr, attr, &bfd);
            }
            break;
         }
         break;

      /* Data stream */
      case STREAM_FILE_DATA:
      case STREAM_SPARSE_DATA:
      case STREAM_WIN32_DATA:
      case STREAM_GZIP_DATA:
      case STREAM_SPARSE_GZIP_DATA:
      case STREAM_WIN32_GZIP_DATA:
         /* Force an expected, consistent stream type here */
         if (extract && (prev_stream == stream || prev_stream == STREAM_UNIX_ATTRIBUTES
                  || prev_stream == STREAM_UNIX_ATTRIBUTES_EX)) {
            flags = 0;
            if (stream == STREAM_SPARSE_DATA || stream == STREAM_SPARSE_GZIP_DATA) {
               flags |= FO_SPARSE;
            }
            if (stream == STREAM_GZIP_DATA || stream == STREAM_SPARSE_GZIP_DATA
                  || stream == STREAM_WIN32_GZIP_DATA) {
               flags |= FO_GZIP;
            }
            if (extract_data(jcr, &bfd, sd->msg, sd->msglen, &fileAddr, flags) < 0) {
               extract = false;
               bclose(&bfd);
               continue;
            }
         }
         break;

      /* Resource fork stream - only recorded after a file to be restored */
      /* Silently ignore if we cannot write - we already reported that */
      case STREAM_MACOS_FORK_DATA:
#ifdef HAVE_DARWIN_OS
         if (extract) {
            if (prev_stream != stream) {
               if (bopen_rsrc(&altbfd, jcr->last_fname, O_WRONLY | O_TRUNC | O_BINARY, 0) < 0) {
                  Jmsg(jcr, M_ERROR, 0, _("     Cannot open resource fork for %s.\n"), jcr->last_fname);
                  extract = false;
                  continue;
               }
               alt_size = rsrc_len;
               Dmsg0(30, "Restoring resource fork\n");
            }
            flags = 0;
            if (extract_data(jcr, &altbfd, sd->msg, sd->msglen, &alt_addr, flags) < 0) {
               extract = false;
               bclose(&altbfd);
               continue;
            }
         }
#else
         non_support_rsrc++;
#endif
         break;

      case STREAM_HFSPLUS_ATTRIBUTES:
#ifdef HAVE_DARWIN_OS
         Dmsg0(30, "Restoring Finder Info\n");
         if (sd->msglen != 32) {
            Jmsg(jcr, M_ERROR, 0, _("     Invalid length of Finder Info (got %d, not 32)\n"), sd->msglen);
            continue;
         }
         if (setattrlist(jcr->last_fname, &attrList, sd->msg, sd->msglen, 0) != 0) {
            Jmsg(jcr, M_ERROR, 0, _("     Could not set Finder Info on %s\n"), jcr->last_fname);
            continue;
         }
#else
         non_support_finfo++;
#endif

      case STREAM_UNIX_ATTRIBUTES_ACCESS_ACL:
#ifdef HAVE_ACL
         pm_strcpy(jcr->acl_text, sd->msg);
         Dmsg2(400, "Restoring ACL type 0x%2x <%s>\n", BACL_TYPE_ACCESS, jcr->acl_text);
         if (bacl_set(jcr, BACL_TYPE_ACCESS) != 0) {
               Jmsg1(jcr, M_WARNING, 0, _("Can't restore ACL of %s\n"), jcr->last_fname);
         }
#else 
         non_support_acl++;
#endif
         break;

      case STREAM_UNIX_ATTRIBUTES_DEFAULT_ACL:
#ifdef HAVE_ACL
         pm_strcpy(jcr->acl_text, sd->msg);
         Dmsg2(400, "Restoring ACL type 0x%2x <%s>\n", BACL_TYPE_DEFAULT, jcr->acl_text);
         if (bacl_set(jcr, BACL_TYPE_DEFAULT) != 0) {
               Jmsg1(jcr, M_WARNING, 0, _("Can't restore default ACL of %s\n"), jcr->last_fname);
         }
#else 
         non_support_acl++;
#endif
         break;

      case STREAM_MD5_SIGNATURE:
      case STREAM_SHA1_SIGNATURE:
         break;

      case STREAM_PROGRAM_NAMES:
      case STREAM_PROGRAM_DATA:
         if (!non_support_progname) {
            Pmsg0(000, "Got Program Name or Data Stream. Ignored.\n");
            non_support_progname++;
         }
         break;

      default:
         /* If extracting, wierd stream (not 1 or 2), close output file anyway */
         if (extract) {
            Dmsg1(30, "Found wierd stream %d\n", stream);
            if (size > 0 && !is_bopen(&bfd)) {
               Jmsg0(jcr, M_ERROR, 0, _("Logic error: output file should be open\n"));
            }
            set_attributes(jcr, attr, &bfd);
            extract = false;
         } else if (is_bopen(&bfd)) {
            Jmsg0(jcr, M_ERROR, 0, _("Logic error: output file should not be open\n"));
            bclose(&bfd);
         }
         Jmsg(jcr, M_ERROR, 0, _("Unknown stream=%d ignored. This shouldn't happen!\n"), stream);
         Dmsg2(0, "None of above!!! stream=%d data=%s\n", stream,sd->msg);
         break;
      } /* end switch(stream) */

   } /* end while get_msg() */

   /* If output file is still open, it was the last one in the
    * archive since we just hit an end of file, so close the file.
    */
   if (is_bopen(&altbfd)) {
      bclose_chksize(jcr, &altbfd, alt_size);
   }
   if (extract) {
      set_attributes(jcr, attr, &bfd);
   }
   if (is_bopen(&bfd)) {
      bclose(&bfd);
   }
   set_jcr_job_status(jcr, JS_Terminated);
   goto ok_out;

bail_out:
   set_jcr_job_status(jcr, JS_ErrorTerminated);
ok_out:
   if (jcr->compress_buf) {
      free(jcr->compress_buf);
      jcr->compress_buf = NULL;
      jcr->compress_buf_size = 0;
   }
   bclose(&altbfd);
   bclose(&bfd);
   free_attr(attr);
   free_pool_memory(jcr->acl_text);
   Dmsg2(10, "End Do Restore. Files=%d Bytes=%s\n", jcr->JobFiles,
      edit_uint64(jcr->JobBytes, ec1));
   if (non_support_data > 1 || non_support_attr > 1) {
      Jmsg(jcr, M_ERROR, 0, _("%d non-supported data streams and %d non-supported attrib streams ignored.\n"),
         non_support_data, non_support_attr);
   }
   if (non_support_rsrc) {
      Jmsg(jcr, M_INFO, 0, _("%d non-supported resource fork streams ignored.\n"), non_support_rsrc);
   }
   if (non_support_finfo) {
      Jmsg(jcr, M_INFO, 0, _("%d non-supported Finder Info streams ignored.\n"), non_support_rsrc);
   }
   if (non_support_acl) {
      Jmsg(jcr, M_INFO, 0, _("%d non-supported acl streams ignored.\n"), non_support_acl);
   }

}

#ifdef HAVE_LIBZ
/*
 * Convert ZLIB error code into an ASCII message
 */
static const char *zlib_strerror(int stat)
{
   if (stat >= 0) {
      return _("None");
   }
   switch (stat) {
   case Z_ERRNO:
      return _("Zlib errno");
   case Z_STREAM_ERROR:
      return _("Zlib stream error");
   case Z_DATA_ERROR:
      return _("Zlib data error");
   case Z_MEM_ERROR:
      return _("Zlib memory error");
   case Z_BUF_ERROR:
      return _("Zlib buffer error");
   case Z_VERSION_ERROR:
      return _("Zlib version error");
   default:
      return _("*none*");
   }
}
#endif

/*
 * In the context of jcr, write data to bfd.
 * We write buflen bytes in buf at addr. addr is updated in place.
 * The flags specify whether to use sparse files or compression.
 * Return value is the number of bytes written, or -1 on errors.
 */
int32_t extract_data(JCR *jcr, BFILE *bfd, POOLMEM *buf, int32_t buflen,
      uint64_t *addr, int flags)
{
   int stat;
   char *wbuf;                        /* write buffer */
   uint32_t wsize;                    /* write size */
   uint32_t rsize;                    /* read size */
   char ec1[50];                      /* Buffer printing huge values */

   if (flags & FO_SPARSE) {
      ser_declare;
      uint64_t faddr;
      char ec1[50];
      wbuf = buf + SPARSE_FADDR_SIZE;
      rsize = buflen - SPARSE_FADDR_SIZE;
      ser_begin(buf, SPARSE_FADDR_SIZE);
      unser_uint64(faddr);
      if (*addr != faddr) {
         *addr = faddr;
         if (blseek(bfd, (off_t)*addr, SEEK_SET) < 0) {
            berrno be;
            Jmsg3(jcr, M_ERROR, 0, _("Seek to %s error on %s: ERR=%s\n"),
                  edit_uint64(*addr, ec1), jcr->last_fname, 
                  be.strerror(bfd->berrno));
            return -1;
         }
      }
   } else {
      wbuf = buf;
      rsize = buflen;
   }
   wsize = rsize;

   if (flags & FO_GZIP) {
#ifdef HAVE_LIBZ
      uLong compress_len;
      /* 
       * NOTE! We only use uLong and Byte because they are
       *  needed by the zlib routines, they should not otherwise
       *  be used in Bacula.
       */
      compress_len = jcr->compress_buf_size;
      Dmsg2(100, "Comp_len=%d msglen=%d\n", compress_len, wsize);
      if ((stat=uncompress((Byte *)jcr->compress_buf, &compress_len,
                  (const Byte *)wbuf, (uLong)rsize)) != Z_OK) {
         Jmsg(jcr, M_ERROR, 0, _("Uncompression error on file %s. ERR=%s\n"),
               jcr->last_fname, zlib_strerror(stat));
         return -1;
      }
      wbuf = jcr->compress_buf;
      wsize = compress_len;
      Dmsg2(100, "Write uncompressed %d bytes, total before write=%s\n", compress_len, edit_uint64(jcr->JobBytes, ec1));
#else
      Jmsg(jcr, M_ERROR, 0, _("GZIP data stream found, but GZIP not configured!\n"));
      return -1;
#endif
   } else {
      Dmsg2(30, "Write %u bytes, total before write=%s\n", wsize, edit_uint64(jcr->JobBytes, ec1));
   }

   if (bwrite(bfd, wbuf, wsize) != (ssize_t)wsize) {
      berrno be;
      Jmsg2(jcr, M_ERROR, 0, _("Write error on %s: %s\n"), 
            jcr->last_fname, be.strerror(bfd->berrno));
      return -1;
   }

   jcr->JobBytes += wsize;
   jcr->ReadBytes += rsize;
   *addr += wsize;

   return wsize;
}
