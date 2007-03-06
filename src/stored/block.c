/*
 *
 *   block.c -- tape block handling functions
 *
 *		Kern Sibbald, March MMI
 *		   added BB02 format October MMII
 *
 *   Version $Id: block.c,v 1.90 2004/10/02 10:19:09 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#include "bacula.h"
#include "stored.h"

extern int debug_level;
static bool terminate_writing_volume(DCR *dcr);

/*
 * Dump the block header, then walk through
 * the block printing out the record headers.
 */
void dump_block(DEV_BLOCK *b, const char *msg)
{
   ser_declare;
   char *p;
   char Id[BLKHDR_ID_LENGTH+1];
   uint32_t CheckSum, BlockCheckSum;
   uint32_t block_len;
   uint32_t BlockNumber;
   uint32_t VolSessionId, VolSessionTime, data_len;
   int32_t  FileIndex;
   int32_t  Stream;
   int bhl, rhl;

   unser_begin(b->buf, BLKHDR1_LENGTH);
   unser_uint32(CheckSum);
   unser_uint32(block_len);
   unser_uint32(BlockNumber);
   unser_bytes(Id, BLKHDR_ID_LENGTH);
   ASSERT(unser_length(b->buf) == BLKHDR1_LENGTH);
   Id[BLKHDR_ID_LENGTH] = 0;
   if (Id[3] == '2') {
      unser_uint32(VolSessionId);
      unser_uint32(VolSessionTime);
      bhl = BLKHDR2_LENGTH;
      rhl = RECHDR2_LENGTH;
   } else {
      VolSessionId = VolSessionTime = 0;
      bhl = BLKHDR1_LENGTH;
      rhl = RECHDR1_LENGTH;
   }

   if (block_len > 100000) {
      Dmsg3(20, "Dump block %s 0x%x blocksize too big %u\n", msg, b, block_len);
      return;
   }

   BlockCheckSum = bcrc32((uint8_t *)b->buf+BLKHDR_CS_LENGTH,
			 block_len-BLKHDR_CS_LENGTH);
   Pmsg6(000, "Dump block %s %x: size=%d BlkNum=%d\n\
               Hdrcksum=%x cksum=%x\n",
      msg, b, block_len, BlockNumber, CheckSum, BlockCheckSum);
   p = b->buf + bhl;
   while (p < (b->buf + block_len+WRITE_RECHDR_LENGTH)) { 
      unser_begin(p, WRITE_RECHDR_LENGTH);
      if (rhl == RECHDR1_LENGTH) {
	 unser_uint32(VolSessionId);
	 unser_uint32(VolSessionTime);
      }
      unser_int32(FileIndex);
      unser_int32(Stream);
      unser_uint32(data_len);
      Pmsg6(000, "   Rec: VId=%u VT=%u FI=%s Strm=%s len=%d p=%x\n",
	   VolSessionId, VolSessionTime, FI_to_ascii(FileIndex), 
	   stream_to_ascii(Stream, FileIndex), data_len, p);
      p += data_len + rhl;
  }
}
    
/*
 * Create a new block structure.			   
 * We pass device so that the block can inherit the
 * min and max block sizes.
 */
DEV_BLOCK *new_block(DEVICE *dev)
{
   DEV_BLOCK *block = (DEV_BLOCK *)get_memory(sizeof(DEV_BLOCK));

   memset(block, 0, sizeof(DEV_BLOCK));

   /* If the user has specified a max_block_size, use it as the default */
   if (dev->max_block_size == 0) {
      block->buf_len = DEFAULT_BLOCK_SIZE;
   } else {
      block->buf_len = dev->max_block_size;
   }
   block->dev = dev;
   block->block_len = block->buf_len;  /* default block size */
   block->buf = get_memory(block->buf_len); 
   empty_block(block);
   block->BlockVer = BLOCK_VER;       /* default write version */
   Dmsg1(350, "Returning new block=%x\n", block);
   return block;
}


/*
 * Duplicate an existing block (eblock)
 */
DEV_BLOCK *dup_block(DEV_BLOCK *eblock)
{
   DEV_BLOCK *block = (DEV_BLOCK *)get_memory(sizeof(DEV_BLOCK));
   int buf_len = sizeof_pool_memory(eblock->buf);

   memcpy(block, eblock, sizeof(DEV_BLOCK));
   block->buf = get_memory(buf_len);
   memcpy(block->buf, eblock->buf, buf_len);
   return block;
}


/* 
 * Only the first block checksum error was reported.
 *   If there are more, report it now.
 */
void print_block_read_errors(JCR *jcr, DEV_BLOCK *block)
{
   if (block->read_errors > 1) {
      Jmsg(jcr, M_ERROR, 0, _("%d block read errors not printed.\n"),
	 block->read_errors);
   }
}

/*
 * Free block 
 */
void free_block(DEV_BLOCK *block)
{
   Dmsg1(199, "free_block buffer %x\n", block->buf);
   free_memory(block->buf);
   Dmsg1(199, "free_block block %x\n", block);
   free_memory((POOLMEM *)block);
}

/* Empty the block -- for writing */
void empty_block(DEV_BLOCK *block)
{
   block->binbuf = WRITE_BLKHDR_LENGTH;
   block->bufp = block->buf + block->binbuf;
   block->read_len = 0;
   block->write_failed = false;
   block->block_read = false;
   block->FirstIndex = block->LastIndex = 0;
}

/*
 * Create block header just before write. The space
 * in the buffer should have already been reserved by
 * init_block.
 */
void ser_block_header(DEV_BLOCK *block)
{
   ser_declare;
   uint32_t CheckSum = 0;
   uint32_t block_len = block->binbuf;
   
   Dmsg1(390, "ser_block_header: block_len=%d\n", block_len);
   ser_begin(block->buf, BLKHDR2_LENGTH);
   ser_uint32(CheckSum);
   ser_uint32(block_len);
   ser_uint32(block->BlockNumber);
   ser_bytes(WRITE_BLKHDR_ID, BLKHDR_ID_LENGTH);
   if (BLOCK_VER >= 2) {
      ser_uint32(block->VolSessionId);
      ser_uint32(block->VolSessionTime);
   }

   /* Checksum whole block except for the checksum */
   CheckSum = bcrc32((uint8_t *)block->buf+BLKHDR_CS_LENGTH, 
		 block_len-BLKHDR_CS_LENGTH);
   Dmsg1(390, "ser_bloc_header: checksum=%x\n", CheckSum);
   ser_begin(block->buf, BLKHDR2_LENGTH);
   ser_uint32(CheckSum);	      /* now add checksum to block header */
}

/*
 * Unserialize the block header for reading block.
 *  This includes setting all the buffer pointers correctly.
 *
 *  Returns: false on failure (not a block)
 *	     true  on success
 */
static bool unser_block_header(JCR *jcr, DEVICE *dev, DEV_BLOCK *block)
{
   ser_declare;
   char Id[BLKHDR_ID_LENGTH+1];
   uint32_t CheckSum, BlockCheckSum;
   uint32_t block_len;
   uint32_t block_end;
   uint32_t BlockNumber;
   int bhl;

   unser_begin(block->buf, BLKHDR_LENGTH);
   unser_uint32(CheckSum);
   unser_uint32(block_len);
   unser_uint32(BlockNumber);
   unser_bytes(Id, BLKHDR_ID_LENGTH);
   ASSERT(unser_length(block->buf) == BLKHDR1_LENGTH);

   Id[BLKHDR_ID_LENGTH] = 0;
   if (Id[3] == '1') {
      bhl = BLKHDR1_LENGTH;
      block->BlockVer = 1;
      block->bufp = block->buf + bhl;
      if (strncmp(Id, BLKHDR1_ID, BLKHDR_ID_LENGTH) != 0) {
	 dev->dev_errno = EIO;
         Mmsg4(dev->errmsg, _("Volume data error at %u:%u! Wanted ID: \"%s\", got \"%s\". Buffer discarded.\n"),
	    dev->file, dev->block_num, BLKHDR1_ID, Id);
	 if (block->read_errors == 0 || verbose >= 2) {
            Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
	 }
	 block->read_errors++;
	 return false;
      }
   } else if (Id[3] == '2') {
      unser_uint32(block->VolSessionId);
      unser_uint32(block->VolSessionTime);
      bhl = BLKHDR2_LENGTH;
      block->BlockVer = 2;
      block->bufp = block->buf + bhl;
      if (strncmp(Id, BLKHDR2_ID, BLKHDR_ID_LENGTH) != 0) {
	 dev->dev_errno = EIO;
         Mmsg4(dev->errmsg, _("Volume data error at %u:%u! Wanted ID: \"%s\", got \"%s\". Buffer discarded.\n"),
	    dev->file, dev->block_num, BLKHDR2_ID, Id);
	 if (block->read_errors == 0 || verbose >= 2) {
            Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
	 }
	 block->read_errors++;
	 return false;
      }
   } else {
      dev->dev_errno = EIO;
      Mmsg4(dev->errmsg, _("Volume data error at %u:%u! Wanted ID: \"%s\", got \"%s\". Buffer discarded.\n"),
	  dev->file, dev->block_num, BLKHDR2_ID, Id);
      if (block->read_errors == 0 || verbose >= 2) {
         Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      }
      block->read_errors++;
      unser_uint32(block->VolSessionId);
      unser_uint32(block->VolSessionTime);
      return false;
   }

   /* Sanity check */
   if (block_len > MAX_BLOCK_LENGTH) {
      dev->dev_errno = EIO;
      Mmsg3(dev->errmsg,  _("Volume data error at %u:%u! Block length %u is insane (too large), probably due to a bad archive.\n"),
	 dev->file, dev->block_num, block_len);
      if (block->read_errors == 0 || verbose >= 2) {
         Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      }
      block->read_errors++;
      return false;
   }

   Dmsg1(390, "unser_block_header block_len=%d\n", block_len);
   /* Find end of block or end of buffer whichever is smaller */
   if (block_len > block->read_len) {
      block_end = block->read_len;
   } else {
      block_end = block_len;
   }
   block->binbuf = block_end - bhl;
   block->block_len = block_len;
   block->BlockNumber = BlockNumber;
   Dmsg3(390, "Read binbuf = %d %d block_len=%d\n", block->binbuf,
      bhl, block_len);
   if (block_len <= block->read_len) {
      BlockCheckSum = bcrc32((uint8_t *)block->buf+BLKHDR_CS_LENGTH,
			 block_len-BLKHDR_CS_LENGTH);
      if (BlockCheckSum != CheckSum) {
	 dev->dev_errno = EIO;
         Mmsg5(dev->errmsg, _("Volume data error at %u:%u! Block checksum mismatch in block %u: calc=%x blk=%x\n"), 
	    dev->file, dev->block_num, (unsigned)BlockNumber, BlockCheckSum, CheckSum);
	 if (block->read_errors == 0 || verbose >= 2) {
            Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
	 }
	 block->read_errors++;
	 if (!forge_on) {
	    return false;
	 }
      }
   }
   return true;
}

/*  
 * Write a block to the device, with locking and unlocking
 *
 * Returns: true  on success
 *	  : false on failure
 *
 */
bool write_block_to_device(DCR *dcr)
{
   bool stat = true;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   if (dcr->spooling) {
      stat = write_block_to_spool_file(dcr);
      return stat;
   }

   if (!dcr->dev_locked) {
      lock_device(dev);
   }

   /*
    * If a new volume has been mounted since our last write
    *	Create a JobMedia record for the previous volume written,
    *	and set new parameters to write this volume   
    * The same applies for if we are in a new file.
    */
   if (dcr->NewVol || dcr->NewFile) {
      if (job_canceled(jcr)) {
	 stat = false;
	 goto bail_out;
      }
      /* Create a jobmedia record for this job */
      if (!dir_create_jobmedia_record(dcr)) {
	 dev->dev_errno = EIO;
         Jmsg(jcr, M_FATAL, 0, _("Could not create JobMedia record for Volume=\"%s\" Job=%s\n"),
	    dcr->VolCatInfo.VolCatName, jcr->Job);
	 set_new_volume_parameters(dcr);
	 stat = false;
	 goto bail_out;
      }
      if (dcr->NewVol) {
	 /* Note, setting a new volume also handles any pending new file */
	 set_new_volume_parameters(dcr);
	 dcr->NewFile = false;	      /* this handled for new file too */
      } else {
	 set_new_file_parameters(dcr);
      }
   }

   if (!write_block_to_dev(dcr)) {
       if (job_canceled(jcr) || jcr->JobType == JT_SYSTEM) {
	  stat = false;
       } else {
	  stat = fixup_device_block_write_error(dcr);
       }
   }

bail_out:
   if (!dcr->dev_locked) {
      unlock_device(dev);
   }
   return stat;
}

/*
 * Write a block to the device 
 *
 *  Returns: true  on success or EOT
 *	     false on hard error
 */
bool write_block_to_dev(DCR *dcr)
{
   ssize_t stat = 0;
   uint32_t wlen;		      /* length to write */
   int hit_max1, hit_max2;
   bool ok = true;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   DEV_BLOCK *block = dcr->block;

#ifdef NO_TAPE_WRITE_TEST
   empty_block(block);
   return true;
#endif
   ASSERT(block->binbuf == ((uint32_t) (block->bufp - block->buf)));

   /* dump_block(block, "before write"); */
   if (dev->state & ST_WEOT) {
      Dmsg0(100, "return write_block_to_dev with ST_WEOT\n");
      dev->dev_errno = ENOSPC;
      Jmsg(jcr, M_FATAL, 0,  _("Cannot write block. Device at EOM.\n"));
      return false;
   }
   if (!(dev->state & ST_APPEND)) {
      dev->dev_errno = EIO;
      Jmsg(jcr, M_FATAL, 0, _("Attempt to write on read-only Volume.\n"));
      return false;
   }
   wlen = block->binbuf;
   if (wlen <= WRITE_BLKHDR_LENGTH) {  /* Does block have data in it? */
      Dmsg0(100, "return write_block_to_dev no data to write\n");
      return true;
   }
   /* 
    * Clear to the end of the buffer if it is not full,
    *  and on tape devices, apply min and fixed blocking.
    */
   if (wlen != block->buf_len) {
      uint32_t blen;		      /* current buffer length */

      Dmsg2(200, "binbuf=%d buf_len=%d\n", block->binbuf, block->buf_len);
      blen = wlen;

      /* Adjust write size to min/max for tapes only */
      if (dev->state & ST_TAPE) {
	 /* check for fixed block size */
	 if (dev->min_block_size == dev->max_block_size) {
	    wlen = block->buf_len;    /* fixed block size already rounded */
	 /* Check for min block size */
	 } else if (wlen < dev->min_block_size) {
	    wlen =  ((dev->min_block_size + TAPE_BSIZE - 1) / TAPE_BSIZE) * TAPE_BSIZE;
	 /* Ensure size is rounded */
	 } else {
	    wlen = ((wlen + TAPE_BSIZE - 1) / TAPE_BSIZE) * TAPE_BSIZE;
	 }
      }
      if (wlen-blen > 0) {
	 memset(block->bufp, 0, wlen-blen); /* clear garbage */
      }
   }  

   ser_block_header(block);

   /* Limit maximum Volume size to value specified by user */
   hit_max1 = (dev->max_volume_size > 0) &&
       ((dev->VolCatInfo.VolCatBytes + block->binbuf)) >= dev->max_volume_size;
   hit_max2 = (dev->VolCatInfo.VolCatMaxBytes > 0) &&
       ((dev->VolCatInfo.VolCatBytes + block->binbuf)) >= dev->VolCatInfo.VolCatMaxBytes;
   if (hit_max1 || hit_max2) {	 
      char ed1[50];
      uint64_t max_cap;
      Dmsg0(10, "==== Output bytes Triggered medium max capacity.\n");
      if (hit_max1) {
	 max_cap = dev->max_volume_size;
      } else {
	 max_cap = dev->VolCatInfo.VolCatMaxBytes;
      }
      Jmsg(jcr, M_INFO, 0, _("User defined maximum volume capacity %s exceeded on device %s.\n"),
	    edit_uint64_with_commas(max_cap, ed1),  dev->dev_name);
      terminate_writing_volume(dcr);
      dev->dev_errno = ENOSPC;
      return false;
   }

   /* Limit maximum File size on volume to user specified value */
   if ((dev->max_file_size > 0) && 
       (dev->file_size+block->binbuf) >= dev->max_file_size) {
      dev->file_size = 0;	      /* reset file size */

      if (dev_state(dev, ST_TAPE) && weof_dev(dev, 1) != 0) {		 /* write eof */
         Dmsg0(190, "WEOF error in max file size.\n");
	 terminate_writing_volume(dcr);
	 dev->dev_errno = ENOSPC;
	 return false;
      }

      /* Create a JobMedia record so restore can seek */
      if (!dir_create_jobmedia_record(dcr)) {
         Dmsg0(190, "Error from create_job_media.\n");
	 dev->dev_errno = EIO;
          Jmsg(jcr, M_FATAL, 0, _("Could not create JobMedia record for Volume=\"%s\" Job=%s\n"),
	       dcr->VolCatInfo.VolCatName, jcr->Job);
	  terminate_writing_volume(dcr);
	  dev->dev_errno = EIO;
	  return false;
      }
      dev->VolCatInfo.VolCatFiles = dev->file;
      if (!dir_update_volume_info(dcr, false)) {
         Dmsg0(190, "Error from update_vol_info.\n");
	 terminate_writing_volume(dcr);
	 dev->dev_errno = EIO;
	 return false;
      }
      Dmsg0(100, "dir_update_volume_info max file size -- OK\n");


      /*
       * Walk through all attached dcrs setting flag to call
       * set_new_file_parameters() when that dcr is next used.
       */
      DCR *mdcr;
      foreach_dlist(mdcr, dev->attached_dcrs) {
	 if (mdcr->jcr->JobId == 0) {
	    continue;
	 }
	 mdcr->NewFile = true;	      /* set reminder to do set_new_file_params */
      }
      /* Set new file/block parameters for current dcr */
      set_new_file_parameters(dcr);
   }

   dev->VolCatInfo.VolCatWrites++;
   Dmsg1(300, "Write block of %u bytes\n", wlen);      
#ifdef DEBUG_BLOCK_ZEROING
   uint32_t *bp = (uint32_t *)block->buf;
   if (bp[0] == 0 && bp[1] == 0 && bp[2] == 0 && block->buf[12] == 0) {
      Jmsg0(jcr, M_ABORT, 0, "Write block header zeroed.\n");
   }
#endif

   stat = write(dev->fd, block->buf, (size_t)wlen);

#ifdef DEBUG_BLOCK_ZEROING
   if (bp[0] == 0 && bp[1] == 0 && bp[2] == 0 && block->buf[12] == 0) {
      Jmsg0(jcr, M_ABORT, 0, "Write block header zeroed.\n");
   }
#endif

   if (stat != (ssize_t)wlen) {
      /* Some devices simply report EIO when the volume is full.
       * With a little more thought we may be able to check
       * capacity and distinguish real errors and EOT
       * conditions.  In any case, we probably want to
       * simulate an End of Medium.
       */
      if (stat == -1) {
	 berrno be;
	 clrerror_dev(dev, -1);
	 if (dev->dev_errno == 0) {
	    dev->dev_errno = ENOSPC;	    /* out of space */
	 }
	 if (dev->dev_errno != ENOSPC) {
            Jmsg4(jcr, M_ERROR, 0, _("Write error at %u:%u on device %s. ERR=%s.\n"), 
	       dev->file, dev->block_num, dev->dev_name, be.strerror());
	 }
      } else {
	dev->dev_errno = ENOSPC;	    /* out of space */
      }  
      if (dev->dev_errno == ENOSPC) {
         Jmsg(jcr, M_INFO, 0, _("End of Volume \"%s\" at %u:%u on device %s. Write of %u bytes got %d.\n"), 
	    dev->VolCatInfo.VolCatName,
	    dev->file, dev->block_num, dev->dev_name, wlen, stat);
      }
      Dmsg6(100, "=== Write error. size=%u rtn=%d dev_blk=%d blk_blk=%d errno=%d: ERR=%s\n", 
	 wlen, stat, dev->block_num, block->BlockNumber, dev->dev_errno, strerror(dev->dev_errno));

      ok = terminate_writing_volume(dcr);
      if (!ok && !forge_on) {
	 return false;
      }

#define CHECK_LAST_BLOCK
#ifdef	CHECK_LAST_BLOCK
      /* 
       * If the device is a tape and it supports backspace record,
       *   we backspace over one or two eof marks depending on 
       *   how many we just wrote, then over the last record,
       *   then re-read it and verify that the block number is
       *   correct.
       */
      if (ok && (dev->state & ST_TAPE) && dev_cap(dev, CAP_BSR)) {
	 /* Now back up over what we wrote and read the last block */
	 if (!bsf_dev(dev, 1)) {
	    ok = false;
            Jmsg(jcr, M_ERROR, 0, _("Backspace file at EOT failed. ERR=%s\n"), strerror(dev->dev_errno));
	 }
	 if (ok && dev_cap(dev, CAP_TWOEOF) && !bsf_dev(dev, 1)) {
	    ok = false;
            Jmsg(jcr, M_ERROR, 0, _("Backspace file at EOT failed. ERR=%s\n"), strerror(dev->dev_errno));
	 }
	 /* Backspace over record */
	 if (ok && !bsr_dev(dev, 1)) {
	    ok = false;
            Jmsg(jcr, M_ERROR, 0, _("Backspace record at EOT failed. ERR=%s\n"), strerror(dev->dev_errno));
	    /*
	     *	On FreeBSD systems, if the user got here, it is likely that his/her
             *    tape drive is "frozen".  The correct thing to do is a 
	     *	  rewind(), but if we do that, higher levels in cleaning up, will
	     *	  most likely write the EOS record over the beginning of the
	     *	  tape.  The rewind *is* done later in mount.c when another
	     *	  tape is requested. Note, the clrerror_dev() call in bsr_dev()
	     *	  calls ioctl(MTCERRSTAT), which *should* fix the problem.
	     */
	 }
	 if (ok) {
	    DEV_BLOCK *lblock = new_block(dev);
	    /* Note, this can destroy dev->errmsg */
	    dcr->block = lblock;
	    if (!read_block_from_dev(dcr, NO_BLOCK_NUMBER_CHECK)) {
               Jmsg(jcr, M_ERROR, 0, _("Re-read last block at EOT failed. ERR=%s"), dev->errmsg);
	    } else {
	       if (lblock->BlockNumber+1 == block->BlockNumber) {
                  Jmsg(jcr, M_INFO, 0, _("Re-read of last block succeeded.\n"));
	       } else {
		  Jmsg(jcr, M_ERROR, 0, _(
"Re-read of last block failed. Last block=%u Current block=%u.\n"),
		       lblock->BlockNumber, block->BlockNumber);
	       }
	    }
	    free_block(lblock);
	    dcr->block = block;
	 }
      }
#endif
      return false;
   }

   /* We successfully wrote the block, now do housekeeping */

   dev->VolCatInfo.VolCatBytes += block->binbuf;
   dev->VolCatInfo.VolCatBlocks++;   
   dev->EndBlock = dev->block_num;
   dev->EndFile  = dev->file;
   dev->block_num++;
   block->BlockNumber++;

   /* Update dcr values */
   if (dev_state(dev, ST_TAPE)) {
      dcr->EndBlock = dev->EndBlock;
      dcr->EndFile  = dev->EndFile;
   } else {
      /* Save address of start of block just written */
      dcr->EndBlock = (uint32_t)dev->file_addr;
      dcr->EndFile = (uint32_t)(dev->file_addr >> 32);
   }
   if (dcr->VolFirstIndex == 0 && block->FirstIndex > 0) {
      dcr->VolFirstIndex = block->FirstIndex;
   }
   if (block->LastIndex > 0) {
      dcr->VolLastIndex = block->LastIndex;
   }
   dcr->WroteVol = true;
   dev->file_addr += wlen;	      /* update file address */
   dev->file_size += wlen;

   Dmsg2(300, "write_block: wrote block %d bytes=%d\n", dev->block_num, wlen);
   empty_block(block);
   return true;
}

static bool terminate_writing_volume(DCR *dcr)
{
   DEVICE *dev = dcr->dev;
   bool ok = true;

   /* Create a JobMedia record to indicated end of tape */
   dev->VolCatInfo.VolCatFiles = dev->file;
   if (!dir_create_jobmedia_record(dcr)) {
      Dmsg0(190, "Error from create JobMedia\n");
      dev->dev_errno = EIO;
       Jmsg(dcr->jcr, M_FATAL, 0, _("Could not create JobMedia record for Volume=\"%s\" Job=%s\n"),
	    dcr->VolCatInfo.VolCatName, dcr->jcr->Job);
       ok = false;
       goto bail_out;
   }
   dcr->block->write_failed = true;
   if (weof_dev(dev, 1) != 0) { 	/* end the tape */
      dev->VolCatInfo.VolCatErrors++;
      Jmsg(dcr->jcr, M_ERROR, 0, "Error writing final EOF to tape. This tape may not be readable.\n"
           "%s", dev->errmsg);
      ok = false;
      Dmsg0(100, "WEOF error.\n");
   }
   dev->VolCatInfo.VolCatFiles = dev->file;
   if (!dir_update_volume_info(dcr, false)) {
      ok = false;
   }
   Dmsg1(100, "dir_update_volume_info terminate writing -- %s\n", ok?"OK":"ERROR");


   /*
    * Walk through all attached dcrs setting flag to call
    * set_new_file_parameters() when that dcr is next used.
    */
   DCR *mdcr;
   foreach_dlist(mdcr, dev->attached_dcrs) {
      if (mdcr->jcr->JobId == 0) {
	 continue;
      }
      mdcr->NewFile = true;	   /* set reminder to do set_new_file_params */
   }
   /* Set new file/block parameters for current dcr */
   set_new_file_parameters(dcr);

   if (ok && dev_cap(dev, CAP_TWOEOF) && weof_dev(dev, 1) != 0) {  /* end the tape */
      dev->VolCatInfo.VolCatErrors++;
      /* This may not be fatal since we already wrote an EOF */
      Jmsg(dcr->jcr, M_ERROR, 0, "%s", dev->errmsg);
   }
bail_out:
   dev->state |= (ST_EOF|ST_EOT|ST_WEOT);
   dev->state &= ~ST_APPEND;	      /* make tape read-only */
   Dmsg1(100, "Leave terminate_writing_volume -- %s\n", ok?"OK":"ERROR");
   return ok;
}


/*  
 * Read block with locking
 *
 */
bool read_block_from_device(DCR *dcr, bool check_block_numbers)
{
   bool stat;
   DEVICE *dev = dcr->dev;
   Dmsg0(200, "Enter read_block_from_device\n");
   lock_device(dev);
   stat = read_block_from_dev(dcr, check_block_numbers);
   unlock_device(dev);
   Dmsg0(200, "Leave read_block_from_device\n");
   return stat;
}

/*
 * Read the next block into the block structure and unserialize
 *  the block header.  For a file, the block may be partially
 *  or completely in the current buffer.
 */
bool read_block_from_dev(DCR *dcr, bool check_block_numbers)
{
   ssize_t stat;
   int looping;
   uint32_t BlockNumber;
   int retry;
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   DEV_BLOCK *block = dcr->block;

   if (dev_state(dev, ST_EOT)) {
      return false;
   }
   looping = 0;
   Dmsg1(200, "Full read() in read_block_from_device() len=%d\n",
	 block->buf_len);
reread:
   if (looping > 1) {
      dev->dev_errno = EIO;
      Mmsg1(dev->errmsg, _("Block buffer size looping problem on device %s\n"),
	 dev->dev_name);
      Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      block->read_len = 0;
      return false;
   }
   retry = 0;
   do {
//    uint32_t *bp = (uint32_t *)block->buf;
//    Dmsg3(000, "Read %p %u at %llu\n", block->buf, block->buf_len, lseek(dev->fd, 0, SEEK_CUR));

      stat = read(dev->fd, block->buf, (size_t)block->buf_len);

//    Dmsg8(000, "stat=%d Csum=%u blen=%u bnum=%u %c%c%c%c\n",stat, bp[0],bp[1],bp[2],
//	block->buf[12],block->buf[13],block->buf[14],block->buf[15]);

      if (retry == 1) {
	 dev->VolCatInfo.VolCatErrors++;   
      }
   } while (stat == -1 && (errno == EINTR || errno == EIO) && retry++ < 11);
   if (stat < 0) {
      berrno be;
      clrerror_dev(dev, -1);
      Dmsg1(200, "Read device got: ERR=%s\n", be.strerror());
      block->read_len = 0;
      Mmsg4(dev->errmsg, _("Read error at file:blk %u:%u on device %s. ERR=%s.\n"), 
	 dev->file, dev->block_num, dev->dev_name, be.strerror());
      Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      if (dev->state & ST_EOF) {  /* EOF just seen? */
	 dev->state |= ST_EOT;	  /* yes, error => EOT */
      }
      return false;
   }
   Dmsg1(200, "Read device got %d bytes\n", stat);
   if (stat == 0) {		/* Got EOF ! */
      dev->block_num = block->read_len = 0;
      Mmsg3(dev->errmsg, _("Read zero bytes at %u:%u on device %s.\n"), 
	 dev->file, dev->block_num, dev->dev_name);
      if (dev->state & ST_EOF) { /* EOF already read? */
	 dev->state |= ST_EOT;	/* yes, 2 EOFs => EOT */
	 block->read_len = 0;
	 return 0;
      }
      dev->file++;		/* increment file */
      dev->state |= ST_EOF;	/* set EOF read */
      block->read_len = 0;
      return false;		/* return eof */
   }
   /* Continue here for successful read */
   block->read_len = stat;	/* save length read */
   if (block->read_len < BLKHDR2_LENGTH) {
      dev->dev_errno = EIO;
      Mmsg4(dev->errmsg, _("Volume data error at %u:%u! Very short block of %d bytes on device %s discarded.\n"), 
	 dev->file, dev->block_num, block->read_len, dev->dev_name);
      Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      dev->state |= ST_SHORT;	/* set short block */
      block->read_len = block->binbuf = 0;
      return false;		/* return error */
   }  

   BlockNumber = block->BlockNumber + 1;
   if (!unser_block_header(jcr, dev, block)) {
      if (forge_on) {
	 dev->file_addr += block->read_len;
	 dev->file_size += block->read_len;
	 goto reread;
      }
      return false;
   }

   /*
    * If the block is bigger than the buffer, we reposition for
    *  re-reading the block, allocate a buffer of the correct size,
    *  and go re-read.
    */
   if (block->block_len > block->buf_len) {
      dev->dev_errno = EIO;
      Mmsg2(dev->errmsg,  _("Block length %u is greater than buffer %u. Attempting recovery.\n"),
	 block->block_len, block->buf_len);
      Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      Pmsg1(000, "%s", dev->errmsg);
      /* Attempt to reposition to re-read the block */
      if (dev->state & ST_TAPE) {
         Dmsg0(200, "BSR for reread; block too big for buffer.\n");
	 if (!bsr_dev(dev, 1)) {
            Jmsg(jcr, M_ERROR, 0, "%s", strerror_dev(dev));
	    block->read_len = 0;
	    return false;
	 }
      } else {
         Dmsg0(200, "Seek to beginning of block for reread.\n");
	 off_t pos = lseek(dev->fd, (off_t)0, SEEK_CUR); /* get curr pos */
	 pos -= block->read_len;
	 lseek(dev->fd, pos, SEEK_SET);   
	 dev->file_addr = pos;
      }
      Mmsg1(dev->errmsg, _("Setting block buffer size to %u bytes.\n"), block->block_len);
      Jmsg(jcr, M_INFO, 0, "%s", dev->errmsg);
      Pmsg1(000, "%s", dev->errmsg);
      /* Set new block length */
      dev->max_block_size = block->block_len;
      block->buf_len = block->block_len;
      free_memory(block->buf);
      block->buf = get_memory(block->buf_len);
      empty_block(block);
      looping++;
      goto reread;		      /* re-read block with correct block size */
   }

   if (block->block_len > block->read_len) {
      dev->dev_errno = EIO;
      Mmsg4(dev->errmsg, _("Volume data error at %u:%u! Short block of %d bytes on device %s discarded.\n"), 
	 dev->file, dev->block_num, block->read_len, dev->dev_name);
      Jmsg(jcr, M_ERROR, 0, "%s", dev->errmsg);
      dev->state |= ST_SHORT;	/* set short block */
      block->read_len = block->binbuf = 0;
      return false;		/* return error */
   }  

   dev->state &= ~(ST_EOF|ST_SHORT); /* clear EOF and short block */
   dev->VolCatInfo.VolCatReads++;   
   dev->VolCatInfo.VolCatRBytes += block->read_len;

   dev->VolCatInfo.VolCatBytes += block->block_len;
   dev->VolCatInfo.VolCatBlocks++;   
   dev->EndBlock = dev->block_num;
   dev->EndFile  = dev->file;
   dev->block_num++;

   /* Update dcr values */
   if (dev->state & ST_TAPE) {
      dcr->EndBlock = dev->EndBlock;
      dcr->EndFile  = dev->EndFile;
   } else {
      dcr->EndBlock = (uint32_t)dev->file_addr;
      dcr->EndFile = (uint32_t)(dev->file_addr >> 32);
      dev->block_num = dcr->EndBlock;
      dev->file = dcr->EndFile;
   }
   dev->file_addr += block->block_len;
   dev->file_size += block->block_len;

   /*
    * If we read a short block on disk,
    * seek to beginning of next block. This saves us
    * from shuffling blocks around in the buffer. Take a
    * look at this from an efficiency stand point later, but
    * it should only happen once at the end of each job.
    *
    * I've been lseek()ing negative relative to SEEK_CUR for 30
    *	years now. However, it seems that with the new off_t definition,
    *	it is not possible to seek negative amounts, so we use two
    *	lseek(). One to get the position, then the second to do an
    *	absolute positioning -- so much for efficiency.  KES Sep 02.
    */
   Dmsg0(200, "At end of read block\n");
   if (block->read_len > block->block_len && !(dev->state & ST_TAPE)) {
      off_t pos = lseek(dev->fd, (off_t)0, SEEK_CUR); /* get curr pos */
      pos -= (block->read_len - block->block_len);
      lseek(dev->fd, pos, SEEK_SET);   
      Dmsg2(200, "Did lseek blk_size=%d rdlen=%d\n", block->block_len,
	    block->read_len);
      dev->file_addr = pos;
      dev->file_size = pos;
   }
   Dmsg2(200, "Exit read_block read_len=%d block_len=%d\n",
      block->read_len, block->block_len);
   block->block_read = true;
   return true;
}
