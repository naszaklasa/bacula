/*
 * Append code for Storage daemon
 *  Kern Sibbald, May MM
 *
 *  Version $Id: append.c,v 1.61.2.2 2005/12/10 13:18:06 kerns Exp $
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
#include "stored.h"


/* Responses sent to the File daemon */
static char OK_data[]    = "3000 OK data\n";

/* Forward referenced functions */

/*
 *  Append Data sent from File daemon
 *
 */
bool do_append_data(JCR *jcr)
{
   int32_t n;
   int32_t file_index, stream, last_file_index;
   BSOCK *ds;
   BSOCK *fd_sock = jcr->file_bsock;
   bool ok = true;
   DEV_RECORD rec;
   char buf1[100], buf2[100];
   DCR *dcr = jcr->dcr;
   DEVICE *dev;


   if (!dcr) { 
      Jmsg0(jcr, M_FATAL, 0, _("DCR is NULL!!!\n"));
      return false;
   }                                              
   dev = dcr->dev;
   if (!dev) { 
      Jmsg0(jcr, M_FATAL, 0, _("DEVICE is NULL!!!\n"));
      return false;
   }                                              

   Dmsg1(100, "Start append data. res=%d\n", dev->reserved_device);

   memset(&rec, 0, sizeof(rec));

   ds = fd_sock;

   if (!bnet_set_buffer_size(ds, dcr->device->max_network_buffer_size, BNET_SETBUF_WRITE)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      Jmsg0(jcr, M_FATAL, 0, _("Unable to set network buffer size.\n"));
      return false;
   }

   if (!acquire_device_for_append(dcr)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return false;
   }

   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);

   if (dev->VolCatInfo.VolCatName[0] == 0) {
      Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
   }
   Dmsg1(20, "Begin append device=%s\n", dev->print_name());

   begin_data_spool(dcr);
   begin_attribute_spool(jcr);

   Dmsg0(100, "Just after acquire_device_for_append\n");
   if (dev->VolCatInfo.VolCatName[0] == 0) {
      Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
   }
   /*
    * Write Begin Session Record
    */
   if (!write_session_label(dcr, SOS_LABEL)) {
      Jmsg1(jcr, M_FATAL, 0, _("Write session label failed. ERR=%s\n"),
         strerror_dev(dev));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      ok = false;
   }
   if (dev->VolCatInfo.VolCatName[0] == 0) {
      Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
   }

   /* Tell File daemon to send data */
   if (!bnet_fsend(fd_sock, OK_data)) {
      berrno be;
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to FD. ERR=%s\n"),
            be.strerror(fd_sock->b_errno));
      ok = false;
   }

   /*
    * Get Data from File daemon, write to device.  To clarify what is
    *   going on here.  We expect:
    *     - A stream header
    *     - Multiple records of data
    *     - EOD record
    *
    *    The Stream header is just used to sychronize things, and
    *    none of the stream header is written to tape.
    *    The Multiple records of data, contain first the Attributes,
    *    then after another stream header, the file data, then
    *    after another stream header, the MD5 data if any.
    *
    *   So we get the (stream header, data, EOD) three time for each
    *   file. 1. for the Attributes, 2. for the file data if any,
    *   and 3. for the MD5 if any.
    */
   dcr->VolFirstIndex = dcr->VolLastIndex = 0;
   jcr->run_time = time(NULL);              /* start counting time for rates */
   for (last_file_index = 0; ok && !job_canceled(jcr); ) {

      /* Read Stream header from the File daemon.
       *  The stream header consists of the following:
       *    file_index (sequential Bacula file index, base 1)
       *    stream     (Bacula number to distinguish parts of data)
       *    info       (Info for Storage daemon -- compressed, encryped, ...)
       *       info is not currently used, so is read, but ignored!
       */
     if ((n=bget_msg(ds)) <= 0) {
         if (n == BNET_SIGNAL && ds->msglen == BNET_EOD) {
            break;                    /* end of data */
         }
         Jmsg1(jcr, M_FATAL, 0, _("Error reading data header from FD. ERR=%s\n"),
               bnet_strerror(ds));
         ok = false;
         break;
      }

      /*
       * This hand scanning is a bit more complicated than a simple
       *   sscanf, but it allows us to handle any size integer up to
       *   int64_t without worrying about whether %d, %ld, %lld, or %q
       *   is the correct format for each different architecture.
       * It is a real pity that sscanf() is not portable.
       */
      char *p = ds->msg;
      while (B_ISSPACE(*p)) {
         p++;
      }
      file_index = (int32_t)str_to_int64(p);
      while (B_ISDIGIT(*p)) {
         p++;
      }
      if (!B_ISSPACE(*p) || !B_ISDIGIT(*(p+1))) {
         Jmsg1(jcr, M_FATAL, 0, _("Malformed data header from FD: %s\n"), ds->msg);
         ok = false;
         break;
      }
      stream = (int32_t)str_to_int64(p);

      Dmsg2(890, "<filed: Header FilInx=%d stream=%d\n", file_index, stream);

      if (!(file_index > 0 && (file_index == last_file_index ||
          file_index == last_file_index + 1))) {
         Jmsg0(jcr, M_FATAL, 0, _("File index from FD not positive or sequential\n"));
         ok = false;
         break;
      }
      if (file_index != last_file_index) {
         jcr->JobFiles = file_index;
         last_file_index = file_index;
      }

      /* Read data stream from the File daemon.
       *  The data stream is just raw bytes
       */
      while ((n=bget_msg(ds)) > 0 && !job_canceled(jcr)) {
         rec.VolSessionId = jcr->VolSessionId;
         rec.VolSessionTime = jcr->VolSessionTime;
         rec.FileIndex = file_index;
         rec.Stream = stream;
         rec.data_len = ds->msglen;
         rec.data = ds->msg;            /* use message buffer */

         Dmsg4(850, "before writ_rec FI=%d SessId=%d Strm=%s len=%d\n",
            rec.FileIndex, rec.VolSessionId, 
            stream_to_ascii(buf1, rec.Stream,rec.FileIndex),
            rec.data_len);

         while (!write_record_to_block(dcr->block, &rec)) {
            Dmsg2(850, "!write_record_to_block data_len=%d rem=%d\n", rec.data_len,
                       rec.remainder);
            if (!write_block_to_device(dcr)) {
               Dmsg2(90, "Got write_block_to_dev error on device %s. %s\n",
                  dev->print_name(), strerror_dev(dev));
               Jmsg2(jcr, M_FATAL, 0, _("Fatal append error on device %s: ERR=%s\n"),
                     dev->print_name(), strerror_dev(dev));
               ok = false;
               break;
            }
         }
         if (!ok) {
            Dmsg0(400, "Not OK\n");
            break;
         }
         jcr->JobBytes += rec.data_len;   /* increment bytes this job */
         Dmsg4(850, "write_record FI=%s SessId=%d Strm=%s len=%d\n",
            FI_to_ascii(buf1, rec.FileIndex), rec.VolSessionId,
            stream_to_ascii(buf2, rec.Stream, rec.FileIndex), rec.data_len);

         /* Send attributes and digest to Director for Catalog */
         if (stream == STREAM_UNIX_ATTRIBUTES    || stream == STREAM_MD5_SIGNATURE ||
             stream == STREAM_UNIX_ATTRIBUTES_EX || stream == STREAM_SHA1_SIGNATURE) {
            if (!jcr->no_attributes) {
               if (are_attributes_spooled(jcr)) {
                  jcr->dir_bsock->spool = true;
               }
               Dmsg0(850, "Send attributes to dir.\n");
               if (!dir_update_file_attributes(dcr, &rec)) {
                  jcr->dir_bsock->spool = false;
                  Jmsg(jcr, M_FATAL, 0, _("Error updating file attributes. ERR=%s\n"),
                     bnet_strerror(jcr->dir_bsock));
                  ok = false;
                  break;
               }
               jcr->dir_bsock->spool = false;
            }
         }
         Dmsg0(650, "Enter bnet_get\n");
      }
      Dmsg1(650, "End read loop with FD. Stat=%d\n", n);
      if (is_bnet_error(ds)) {
         Dmsg1(350, "Network read error from FD. ERR=%s\n", bnet_strerror(ds));
         Jmsg1(jcr, M_FATAL, 0, _("Network error on data channel. ERR=%s\n"),
               bnet_strerror(ds));
         ok = false;
         break;
      }
   }

   /* Create Job status for end of session label */
   set_jcr_job_status(jcr, ok?JS_Terminated:JS_ErrorTerminated);

   Dmsg1(200, "Write session label JobStatus=%d\n", jcr->JobStatus);
   if ((!ok || job_canceled(jcr)) && dev->VolCatInfo.VolCatName[0] == 0) {
      Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
   }

   /*
    * If !OK, check if we can still write. This may not be the case
    *  if we are at the end of the tape or we got a fatal I/O error.
    */
   if (ok || dev->can_write()) {
      if (!write_session_label(dcr, EOS_LABEL)) {
         Jmsg1(jcr, M_FATAL, 0, _("Error writting end session label. ERR=%s\n"),
               strerror_dev(dev));
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         ok = false;
      }
      if (dev->VolCatInfo.VolCatName[0] == 0) {
         Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
      }
      Dmsg0(90, "back from write_end_session_label()\n");
      /* Flush out final partial block of this session */
      if (!write_block_to_device(dcr)) {
         Jmsg2(jcr, M_FATAL, 0, _("Fatal append error on device %s: ERR=%s\n"),
               dev->print_name(), strerror_dev(dev));
         Dmsg0(100, _("Set ok=FALSE after write_block_to_device.\n"));
         ok = false;
      }
   }
   if (dev->VolCatInfo.VolCatName[0] == 0) {
      Pmsg0(000, _("NULL Volume name. This shouldn't happen!!!\n"));
   }

   if (!ok) {
      discard_data_spool(dcr);
   } else {
      commit_data_spool(dcr);
   }

   if (ok) {
      ok = dvd_close_job(dcr);  /* do DVD cleanup if any */
   }
   
   /* Release the device -- and send final Vol info to DIR */
   release_device(dcr);

   if (!ok || job_canceled(jcr)) {
      discard_attribute_spool(jcr);
   } else {
      commit_attribute_spool(jcr);
   }

   dir_send_job_status(jcr);          /* update director */

   Dmsg1(100, "return from do_append_data() ok=%d\n", ok);
   return ok;
}
