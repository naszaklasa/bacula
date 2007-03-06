/*
 *
 *  This routine provides a routine that will handle all
 *    the gory little details of reading a record from a Bacula
 *    archive. It uses a callback to pass you each record in turn,
 *    as well as a callback for mounting the next tape.  It takes
 *    care of reading blocks, applying the bsr, ...
 *    Note, this routine is really the heart of the restore routines,
 *    and we are *really* bit pushing here so be careful about making
 *    any modifications.
 *
 *    Kern E. Sibbald, August MMII
 *
 *   Version $Id: read_record.c,v 1.58.2.2 2006/03/14 21:41:45 kerns Exp $
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

/* Forward referenced functions */
static void handle_session_record(DEVICE *dev, DEV_RECORD *rec, SESSION_LABEL *sessrec);
static BSR *position_to_first_file(JCR *jcr, DEVICE *dev);
static bool try_repositioning(JCR *jcr, DEV_RECORD *rec, DEVICE *dev);
#ifdef DEBUG
static char *rec_state_to_str(DEV_RECORD *rec);
#endif

bool read_records(DCR *dcr,
       bool record_cb(DCR *dcr, DEV_RECORD *rec),
       bool mount_cb(DCR *dcr))
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   DEV_BLOCK *block = dcr->block;
   DEV_RECORD *rec = NULL;
   uint32_t record;
   bool ok = true;
   bool done = false;
   SESSION_LABEL sessrec;
   dlist *recs;                         /* linked list of rec packets open */

   recs = New(dlist(rec, &rec->link));
   position_to_first_file(jcr, dev);
   jcr->mount_next_volume = false;

   for ( ; ok && !done; ) {
      if (job_canceled(jcr)) {
         ok = false;
         break;
      }
      if (!read_block_from_device(dcr, CHECK_BLOCK_NUMBERS)) {
         if (dev->at_eot()) {
            DEV_RECORD *trec = new_record();
            Jmsg(jcr, M_INFO, 0, _("End of Volume at file %u on device %s, Volume \"%s\"\n"),
                 dev->file, dev->print_name(), dcr->VolumeName);
            if (!mount_cb(dcr)) {
               Jmsg(jcr, M_INFO, 0, _("End of all volumes.\n"));
               ok = false;            /* Stop everything */
               /*
                * Create EOT Label so that Media record may
                *  be properly updated because this is the last
                *  tape.
                */
               trec->FileIndex = EOT_LABEL;
               trec->File = dev->file;
               ok = record_cb(dcr, trec);
               free_record(trec);
               if (jcr->mount_next_volume) {
                  jcr->mount_next_volume = false;
                  dev->clear_eot();
               }
               break;
            }
            jcr->mount_next_volume = false;
            /*
             * We just have a new tape up, now read the label (first record)
             *  and pass it off to the callback routine, then continue
             *  most likely reading the previous record.
             */
            read_block_from_device(dcr, NO_BLOCK_NUMBER_CHECK);
            read_record_from_block(block, trec);
            handle_session_record(dev, trec, &sessrec);
            ok = record_cb(dcr, trec);
            free_record(trec);
            position_to_first_file(jcr, dev);
            /* After reading label, we must read first data block */
            continue;

         } else if (dev->at_eof()) {
            if (verbose) {
               Jmsg(jcr, M_INFO, 0, _("End of file %u  on device %s, Volume \"%s\"\n"),
                  dev->file, dev->print_name(), dcr->VolumeName);
            }
            Dmsg3(200, "End of file %u  on device %s, Volume \"%s\"\n",
                  dev->file, dev->print_name(), dcr->VolumeName);
            continue;
         } else if (dev->is_short_block()) {
            Jmsg1(jcr, M_ERROR, 0, "%s", dev->errmsg);
            continue;
         } else {
            /* I/O error or strange end of tape */
            display_tape_error_status(jcr, dev);
            if (forge_on || jcr->ignore_label_errors) {
               dev->fsr(1);       /* try skipping bad record */
               Pmsg0(000, _("Did fsr\n"));
               continue;              /* try to continue */
            }
            ok = false;               /* stop everything */
            break;
         }
      }
      Dmsg2(300, "New block at position=(file:block) %u:%u\n", dev->file, dev->block_num);
#ifdef if_and_when_FAST_BLOCK_REJECTION_is_working
      /* this does not stop when file/block are too big */
      if (!match_bsr_block(jcr->bsr, block)) {
         if (try_repositioning(jcr, rec, dev)) {
            break;                    /* get next volume */
         }
         continue;                    /* skip this record */
      }
#endif

      /*
       * Get a new record for each Job as defined by
       *   VolSessionId and VolSessionTime
       */
      bool found = false;
      foreach_dlist(rec, recs) {
         if (rec->VolSessionId == block->VolSessionId &&
             rec->VolSessionTime == block->VolSessionTime) {
            found = true;
            break;
          }
      }
      if (!found) {
         rec = new_record();
         recs->prepend(rec);
         Dmsg2(300, "New record for SI=%d ST=%d\n",
             block->VolSessionId, block->VolSessionTime);
      }
      Dmsg3(300, "After mount next vol. stat=%s blk=%d rem=%d\n", rec_state_to_str(rec),
            block->BlockNumber, rec->remainder);
      record = 0;
      rec->state = 0;
      Dmsg1(300, "Block empty %d\n", is_block_empty(rec));
      for (rec->state=0; ok && !is_block_empty(rec); ) {
         if (!read_record_from_block(block, rec)) {
            Dmsg3(400, "!read-break. state=%s blk=%d rem=%d\n", rec_state_to_str(rec),
                  block->BlockNumber, rec->remainder);
            break;
         }
         Dmsg5(300, "read-OK. state=%s blk=%d rem=%d file:block=%u:%u\n",
                 rec_state_to_str(rec), block->BlockNumber, rec->remainder,
                 dev->file, dev->block_num);
         /*
          * At this point, we have at least a record header.
          *  Now decide if we want this record or not, but remember
          *  before accessing the record, we may need to read again to
          *  get all the data.
          */
         record++;
         Dmsg6(300, "recno=%d state=%s blk=%d SI=%d ST=%d FI=%d\n", record,
            rec_state_to_str(rec), block->BlockNumber,
            rec->VolSessionId, rec->VolSessionTime, rec->FileIndex);

         if (rec->FileIndex == EOM_LABEL) { /* end of tape? */
            Dmsg0(40, "Get EOM LABEL\n");
            break;                         /* yes, get out */
         }

         /* Some sort of label? */
         if (rec->FileIndex < 0) {
            handle_session_record(dev, rec, &sessrec);
            ok = record_cb(dcr, rec);
            if (rec->FileIndex == EOS_LABEL) {
               Dmsg2(300, "Remove rec. SI=%d ST=%d\n", rec->VolSessionId,
                  rec->VolSessionTime);
               recs->remove(rec);
               free_record(rec);
            }
            continue;
         } /* end if label record */

         /*
          * Apply BSR filter
          */
         if (jcr->bsr) {
            int stat = match_bsr(jcr->bsr, rec, &dev->VolHdr, &sessrec);
            if (stat == -1) { /* no more possible matches */
               done = true;   /* all items found, stop */
               Dmsg2(300, "All done=(file:block) %u:%u\n", dev->file, dev->block_num);
               break;
            } else if (stat == 0) {  /* no match */
               Dmsg4(300, "Clear rem=%d FI=%d before set_eof pos %u:%u\n",
                  rec->remainder, rec->FileIndex, dev->file, dev->block_num);
               rec->remainder = 0;
               rec->state &= ~REC_PARTIAL_RECORD;
               if (try_repositioning(jcr, rec, dev)) {
                  break;
               }
               continue;              /* we don't want record, read next one */
            }
         }
         dcr->VolLastIndex = rec->FileIndex;  /* let caller know where we are */
         if (is_partial_record(rec)) {
            Dmsg6(300, "Partial, break. recno=%d state=%s blk=%d SI=%d ST=%d FI=%d\n", record,
               rec_state_to_str(rec), block->BlockNumber,
               rec->VolSessionId, rec->VolSessionTime, rec->FileIndex);
            break;                    /* read second part of record */
         }
         ok = record_cb(dcr, rec);
         if (rec->Stream == STREAM_MD5_SIGNATURE || rec->Stream == STREAM_SHA1_SIGNATURE) {
            Dmsg3(300, "Done FI=%u before set_eof pos %u:%u\n", rec->FileIndex,
                  dev->file, dev->block_num);
            if (match_set_eof(jcr->bsr, rec) && try_repositioning(jcr, rec, dev)) {
               Dmsg2(300, "Break after match_set_eof pos %u:%u\n",
                     dev->file, dev->block_num);
               break;
            }
            Dmsg2(300, "After set_eof pos %u:%u\n", dev->file, dev->block_num);
         }
      } /* end for loop over records */
      Dmsg2(300, "After end records position=(file:block) %u:%u\n", dev->file, dev->block_num);
   } /* end for loop over blocks */
// Dmsg2(300, "Position=(file:block) %u:%u\n", dev->file, dev->block_num);

   /* Walk down list and free all remaining allocated recs */
   while (!recs->empty()) {
      rec = (DEV_RECORD *)recs->first();
      recs->remove(rec);
      free_record(rec);
   }
   delete recs;
   print_block_read_errors(jcr, block);
   return ok;
}

/*
 * See if we can reposition.
 *   Returns:  true  if at end of volume
 *             false otherwise
 */
static bool try_repositioning(JCR *jcr, DEV_RECORD *rec, DEVICE *dev)
{
   BSR *bsr;
   bsr = find_next_bsr(jcr->bsr, dev);
   if (bsr == NULL && jcr->bsr->mount_next_volume) {
      Dmsg0(300, "Would mount next volume here\n");
      Dmsg2(300, "Current postion (file:block) %u:%u\n",
         dev->file, dev->block_num);
      jcr->bsr->mount_next_volume = false;
      if (!dev->at_eot()) {
         /* Set EOT flag to force mount of next Volume */
         jcr->mount_next_volume = true;
         dev->set_eot();
      }
      rec->Block = 0;
      return true;
   }
   if (bsr) {
      if (verbose) {
         Jmsg(jcr, M_INFO, 0, _("Reposition from (file:block) %u:%u to %u:%u\n"),
            dev->file, dev->block_num, bsr->volfile->sfile,
            bsr->volblock->sblock);
      }
      Dmsg4(300, "Try_Reposition from (file:block) %u:%u to %u:%u\n",
            dev->file, dev->block_num, bsr->volfile->sfile,
            bsr->volblock->sblock);
      reposition_dev(dev, bsr->volfile->sfile, bsr->volblock->sblock);
      rec->Block = 0;
   }
   return false;
}

/*
 * Position to the first file on this volume
 */
static BSR *position_to_first_file(JCR *jcr, DEVICE *dev)
{
   BSR *bsr = NULL;
   /*
    * Now find and position to first file and block
    *   on this tape.
    */
   if (jcr->bsr) {
      jcr->bsr->reposition = true;    /* force repositioning */
      bsr = find_next_bsr(jcr->bsr, dev);
      if (bsr && (bsr->volfile->sfile != 0 || bsr->volblock->sblock != 0)) {
         Jmsg(jcr, M_INFO, 0, _("Forward spacing to file:block %u:%u.\n"),
            bsr->volfile->sfile, bsr->volblock->sblock);
         Dmsg2(300, "Forward spacing to file:block %u:%u.\n",
            bsr->volfile->sfile, bsr->volblock->sblock);
         reposition_dev(dev, bsr->volfile->sfile, bsr->volblock->sblock);
      }
   }
   return bsr;
}


static void handle_session_record(DEVICE *dev, DEV_RECORD *rec, SESSION_LABEL *sessrec)
{
   const char *rtype;
   char buf[100];

   memset(sessrec, 0, sizeof(sessrec));
   switch (rec->FileIndex) {
   case PRE_LABEL:
      rtype = _("Fresh Volume Label");
      break;
   case VOL_LABEL:
      rtype = _("Volume Label");
      unser_volume_label(dev, rec);
      break;
   case SOS_LABEL:
      rtype = _("Begin Session");
      unser_session_label(sessrec, rec);
      break;
   case EOS_LABEL:
      rtype = _("End Session");
      break;
   case EOM_LABEL:
      rtype = _("End of Media");
      break;
   default:
      bsnprintf(buf, sizeof(buf), _("Unknown code %d\n"), rec->FileIndex);
      rtype = buf;
      break;
   }
   Dmsg5(300, _("%s Record: VolSessionId=%d VolSessionTime=%d JobId=%d DataLen=%d\n"),
         rtype, rec->VolSessionId, rec->VolSessionTime, rec->Stream, rec->data_len);
}

#ifdef DEBUG
static char *rec_state_to_str(DEV_RECORD *rec)
{
   static char buf[200];
   buf[0] = 0;
   if (rec->state & REC_NO_HEADER) {
      bstrncat(buf, "Nohdr,", sizeof(buf));
   }
   if (is_partial_record(rec)) {
      bstrncat(buf, "partial,", sizeof(buf));
   }
   if (rec->state & REC_BLOCK_EMPTY) {
      bstrncat(buf, "empty,", sizeof(buf));
   }
   if (rec->state & REC_NO_MATCH) {
      bstrncat(buf, "Nomatch,", sizeof(buf));
   }
   if (rec->state & REC_CONTINUATION) {
      bstrncat(buf, "cont,", sizeof(buf));
   }
   if (buf[0]) {
      buf[strlen(buf)-1] = 0;
   }
   return buf;
}
#endif
