/*
 * Read code for Storage daemon
 *
 *     Kern Sibbald, November MM
 *
 *   Version $Id: read.c,v 1.48.2.1 2005/12/10 13:18:07 kerns Exp $
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

/* Forward referenced subroutines */
static bool record_cb(DCR *dcr, DEV_RECORD *rec);


/* Responses sent to the File daemon */
static char OK_data[]    = "3000 OK data\n";
static char FD_error[]   = "3000 error\n";
static char rec_header[] = "rechdr %ld %ld %ld %ld %ld";

/*
 *  Read Data and send to File Daemon
 *   Returns: false on failure
 *            true  on success
 */
bool do_read_data(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   bool ok = true;
   DCR *dcr = jcr->read_dcr;

   Dmsg0(20, "Start read data.\n");

   if (!bnet_set_buffer_size(fd, dcr->device->max_network_buffer_size, BNET_SETBUF_WRITE)) {
      return false;
   }


   create_restore_volume_list(jcr);
   if (jcr->NumVolumes == 0) {
      Jmsg(jcr, M_FATAL, 0, _("No Volume names found for restore.\n"));
      free_restore_volume_list(jcr);
      bnet_fsend(fd, FD_error);
      return false;
   }

   Dmsg2(200, "Found %d volumes names to restore. First=%s\n", jcr->NumVolumes,
      jcr->VolList->VolumeName);

   /* Ready device for reading */
   if (!acquire_device_for_read(dcr)) {
      free_restore_volume_list(jcr);
      bnet_fsend(fd, FD_error);
      return false;
   }

   /* Tell File daemon we will send data */
   bnet_fsend(fd, OK_data);
   ok = read_records(dcr, record_cb, mount_next_read_volume);

   /* Send end of data to FD */
   bnet_sig(fd, BNET_EOD);

   if (!release_device(dcr)) {
      ok = false;
   }

   free_restore_volume_list(jcr);
   Dmsg0(30, "Done reading.\n");
   return ok;
}

/*
 * Called here for each record from read_records()
 *  Returns: true if OK
 *           false if error
 */
static bool record_cb(DCR *dcr, DEV_RECORD *rec)
{
   JCR *jcr = dcr->jcr;
   BSOCK *fd = jcr->file_bsock;
   bool ok = true;
   POOLMEM *save_msg;

   if (rec->FileIndex < 0) {
      return true;
   }
   Dmsg5(400, "Send to FD: SessId=%u SessTim=%u FI=%d Strm=%d, len=%d\n",
      rec->VolSessionId, rec->VolSessionTime, rec->FileIndex, rec->Stream,
      rec->data_len);

   /* Send record header to File daemon */
   if (!bnet_fsend(fd, rec_header, rec->VolSessionId, rec->VolSessionTime,
          rec->FileIndex, rec->Stream, rec->data_len)) {
      Pmsg1(000, _(">filed: Error Hdr=%s\n"), fd->msg);
      Jmsg1(jcr, M_FATAL, 0, _("Error sending to File daemon. ERR=%s\n"),
         bnet_strerror(fd));
      return false;
   } else {
      Dmsg1(400, ">filed: Hdr=%s\n", fd->msg);
   }


   /* Send data record to File daemon */
   save_msg = fd->msg;          /* save fd message pointer */
   fd->msg = rec->data;         /* pass data directly to bnet_send */
   fd->msglen = rec->data_len;
   Dmsg1(400, ">filed: send %d bytes data.\n", fd->msglen);
   if (!bnet_send(fd)) {
      Pmsg1(000, _("Error sending to FD. ERR=%s\n"), bnet_strerror(fd));
      Jmsg1(jcr, M_FATAL, 0, _("Error sending to File daemon. ERR=%s\n"),
         bnet_strerror(fd));

      ok = false;
   }
   fd->msg = save_msg;                /* restore fd message pointer */
   return ok;
}
