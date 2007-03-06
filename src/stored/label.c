/*
 *
 *  label.c  Bacula routines to handle labels 
 *
 *   Kern Sibbald, MM
 *			      
 *
 *   Version $Id: label.c,v 1.54 2004/09/27 07:01:37 kerns Exp $
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

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Forward referenced functions */
static void create_volume_label_record(DCR *dcr, DEV_RECORD *rec);

extern char my_name[];
extern int debug_level;

/*
 * Read the volume label
 *
 *  If dcr->VolumeName == NULL, we accept any Bacula Volume
 *  If dcr->VolumeName[0] == 0, we accept any Bacula Volume
 *  otherwise dcr->VolumeName must match the Volume.
 *
 *  If VolName given, ensure that it matches   
 *
 *  Returns VOL_  code as defined in record.h
 *    VOL_NOT_READ
 *    VOL_OK
 *    VOL_NO_LABEL
 *    VOL_IO_ERROR
 *    VOL_NAME_ERROR
 *    VOL_CREATE_ERROR
 *    VOL_VERSION_ERROR
 *    VOL_LABEL_ERROR
 *    VOL_NO_MEDIA
 */  
int read_dev_volume_label(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   char *VolName = dcr->VolumeName;
   DEV_RECORD *record;
   bool ok = false;
   DEV_BLOCK *block = dcr->block;

   Dmsg3(100, "Enter read_volume_label device=%s vol=%s dev_Vol=%s\n", 
      dev_name(dev), VolName, dev->VolHdr.VolName);

   if (dev_state(dev, ST_LABEL)) {	 /* did we already read label? */
      /* Compare Volume Names allow special wild card */
      if (VolName && *VolName && *VolName != '*' && strcmp(dev->VolHdr.VolName, VolName) != 0) {
         Mmsg(jcr->errmsg, _("Wrong Volume mounted on device %s: Wanted %s have %s\n"),
	    dev_name(dev), VolName, dev->VolHdr.VolName);
	 /*
	  * Cancel Job if too many label errors
	  *  => we are in a loop
	  */
	 if (!dev->poll && jcr->label_errors++ > 100) {
            Jmsg(jcr, M_FATAL, 0, "Too many tries: %s", jcr->errmsg);
	 }
	 return VOL_NAME_ERROR;
      }
      Dmsg0(30, "Leave read_volume_label() VOL_OK\n");
      return VOL_OK;	   /* label already read */
   }

   dev->state &= ~(ST_LABEL|ST_APPEND|ST_READ);  /* set no label, no append */

   if (!rewind_dev(dev)) {
      Mmsg(jcr->errmsg, _("Couldn't rewind device %s ERR=%s\n"), dev_name(dev),
	 strerror_dev(dev));
      Dmsg1(30, "%s", jcr->errmsg);
      return VOL_NO_MEDIA;
   }
   bstrncpy(dev->VolHdr.Id, "**error**", sizeof(dev->VolHdr.Id));

   /* Read the Volume label block */
   record = new_record();
   empty_block(block);
   Dmsg0(90, "Big if statement in read_volume_label\n");
   if (!read_block_from_dev(dcr, NO_BLOCK_NUMBER_CHECK)) { 
      Mmsg(jcr->errmsg, _("Requested Volume \"%s\" on %s is not a Bacula "
           "labeled Volume, because: ERR=%s"), NPRT(VolName), dev_name(dev), 
	   strerror_dev(dev));
      Dmsg1(30, "%s", jcr->errmsg);
   } else if (!read_record_from_block(block, record)) {
      Mmsg(jcr->errmsg, _("Could not read Volume label from block.\n"));
      Dmsg1(30, "%s", jcr->errmsg);
   } else if (!unser_volume_label(dev, record)) {
      Mmsg(jcr->errmsg, _("Could not unserialize Volume label: ERR=%s\n"),
	 strerror_dev(dev));
      Dmsg1(30, "%s", jcr->errmsg);
   } else if (strcmp(dev->VolHdr.Id, BaculaId) != 0 && 
	      strcmp(dev->VolHdr.Id, OldBaculaId) != 0) {
      Mmsg(jcr->errmsg, _("Volume Header Id bad: %s\n"), dev->VolHdr.Id);
      Dmsg1(30, "%s", jcr->errmsg);
   } else {
      ok = true;
   }
   if (!ok) {
      free_record(record);
      if (forge_on || jcr->ignore_label_errors) {
	 dev->state |= ST_LABEL;      /* set has Bacula label */
         Jmsg(jcr, M_ERROR, 0, "%s", jcr->errmsg);
	 return VOL_OK;
      }
      empty_block(block);
      rewind_dev(dev);
      return VOL_NO_LABEL;
   }

   free_record(record);
   /* If we are a streaming device, we only get one chance to read */
   if (!dev_cap(dev, CAP_STREAM)) {
      empty_block(block);
      rewind_dev(dev);
   }

   if (dev->VolHdr.VerNum != BaculaTapeVersion && 
       dev->VolHdr.VerNum != OldCompatibleBaculaTapeVersion1 &&  
       dev->VolHdr.VerNum != OldCompatibleBaculaTapeVersion2) {
      Mmsg(jcr->errmsg, _("Volume on %s has wrong Bacula version. Wanted %d got %d\n"),
	 dev_name(dev), BaculaTapeVersion, dev->VolHdr.VerNum);
      Dmsg1(30, "%s", jcr->errmsg);
      return VOL_VERSION_ERROR;
   }

   /* We are looking for either an unused Bacula tape (PRE_LABEL) or
    * a Bacula volume label (VOL_LABEL)
    */
   if (dev->VolHdr.LabelType != PRE_LABEL && dev->VolHdr.LabelType != VOL_LABEL) {
      Mmsg(jcr->errmsg, _("Volume on %s has bad Bacula label type: %x\n"), 
	  dev_name(dev), dev->VolHdr.LabelType);
      Dmsg1(30, "%s", jcr->errmsg);
      return VOL_LABEL_ERROR;
   }

   dev->state |= ST_LABEL;	      /* set has Bacula label */

   /* Compare Volume Names */
   Dmsg2(30, "Compare Vol names: VolName=%s hdr=%s\n", VolName?VolName:"*", dev->VolHdr.VolName);
   if (VolName && *VolName && *VolName != '*' && strcmp(dev->VolHdr.VolName, VolName) != 0) {
      Mmsg(jcr->errmsg, _("Wrong Volume mounted on device %s: Wanted %s have %s\n"),
	   dev_name(dev), VolName, dev->VolHdr.VolName);
      Dmsg1(30, "%s", jcr->errmsg);
      /*
       * Cancel Job if too many label errors
       *  => we are in a loop
       */
      if (!dev->poll && jcr->label_errors++ > 100) {
         Jmsg(jcr, M_FATAL, 0, "Too many tries: %s", jcr->errmsg);
      }
      return VOL_NAME_ERROR;
   }
   Dmsg1(30, "Copy vol_name=%s\n", dev->VolHdr.VolName);

   if (debug_level >= 10) {
      dump_volume_label(dev);
   }
   Dmsg0(30, "Leave read_volume_label() VOL_OK\n");
   return VOL_OK;
}

/*  unser_volume_label 
 *  
 * Unserialize the Volume label into the device Volume_Label
 * structure.
 *
 * Assumes that the record is already read.
 *
 * Returns: false on error
 *	    true  on success
*/

bool unser_volume_label(DEVICE *dev, DEV_RECORD *rec)
{
   ser_declare;

   if (rec->FileIndex != VOL_LABEL && rec->FileIndex != PRE_LABEL) {
      Mmsg3(dev->errmsg, _("Expecting Volume Label, got FI=%s Stream=%s len=%d\n"), 
	      FI_to_ascii(rec->FileIndex), 
	      stream_to_ascii(rec->Stream, rec->FileIndex),
	      rec->data_len);
      if (!forge_on) {
	 return false;
      }
   }

   dev->VolHdr.LabelType = rec->FileIndex;
   dev->VolHdr.LabelSize = rec->data_len;


   /* Unserialize the record into the Volume Header */
   rec->data = check_pool_memory_size(rec->data, SER_LENGTH_Volume_Label);
   ser_begin(rec->data, SER_LENGTH_Volume_Label);
   unser_string(dev->VolHdr.Id);
   unser_uint32(dev->VolHdr.VerNum);

   if (dev->VolHdr.VerNum >= 11) {
      unser_btime(dev->VolHdr.label_btime);
      unser_btime(dev->VolHdr.write_btime);
   } else { /* old way */ 
      unser_float64(dev->VolHdr.label_date);
      unser_float64(dev->VolHdr.label_time);
   }
   unser_float64(dev->VolHdr.write_date);    /* Unused with VerNum >= 11 */
   unser_float64(dev->VolHdr.write_time);    /* Unused with VerNum >= 11 */

   unser_string(dev->VolHdr.VolName);
   unser_string(dev->VolHdr.PrevVolName);
   unser_string(dev->VolHdr.PoolName);
   unser_string(dev->VolHdr.PoolType);
   unser_string(dev->VolHdr.MediaType);

   unser_string(dev->VolHdr.HostName);
   unser_string(dev->VolHdr.LabelProg);
   unser_string(dev->VolHdr.ProgVersion);
   unser_string(dev->VolHdr.ProgDate);

   ser_end(rec->data, SER_LENGTH_Volume_Label);
   Dmsg0(90, "ser_read_vol\n");
   if (debug_level >= 90) {
      dump_volume_label(dev);	   
   }
   return true;
}

/*
 * Put a volume label into the block
 *
 *  Returns: false on failure
 *	     true  on success
 */
bool write_volume_label_to_block(DCR *dcr)
{
   DEV_RECORD rec;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   DEV_BLOCK *block = dcr->block;

   Dmsg0(20, "write Label in write_volume_label_to_block()\n");
   memset(&rec, 0, sizeof(rec));
   rec.data = get_memory(SER_LENGTH_Volume_Label);
   empty_block(block);		      /* Volume label always at beginning */

   create_volume_label_record(dcr, &rec);

   block->BlockNumber = 0;
   if (!write_record_to_block(block, &rec)) {
      free_pool_memory(rec.data);
      Jmsg1(jcr, M_FATAL, 0, _("Cannot write Volume label to block for device %s\n"),
	 dev_name(dev));
      return false;
   } else {
      Dmsg1(90, "Wrote label of %d bytes to block\n", rec.data_len);
   }
   free_pool_memory(rec.data);
   return true;
}

/* 
 *  create_volume_label_record
 *   Serialize label (from dev->VolHdr structure) into device record.
 *   Assumes that the dev->VolHdr structure is properly 
 *   initialized.
*/
static void create_volume_label_record(DCR *dcr, DEV_RECORD *rec)
{
   ser_declare;
   struct date_time dt;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   /* Serialize the label into the device record. */

   rec->data = check_pool_memory_size(rec->data, SER_LENGTH_Volume_Label);
   ser_begin(rec->data, SER_LENGTH_Volume_Label);
   ser_string(dev->VolHdr.Id);

   ser_uint32(dev->VolHdr.VerNum);

   if (dev->VolHdr.VerNum >= 11) {
      ser_btime(dev->VolHdr.label_btime);
      dev->VolHdr.write_btime = get_current_btime();
      ser_btime(dev->VolHdr.write_btime);
      dev->VolHdr.write_date = 0;
      dev->VolHdr.write_time = 0;
   } else {
      /* OLD WAY DEPRECATED */
      ser_float64(dev->VolHdr.label_date);
      ser_float64(dev->VolHdr.label_time);
      get_current_time(&dt);
      dev->VolHdr.write_date = dt.julian_day_number;
      dev->VolHdr.write_time = dt.julian_day_fraction;
   }
   ser_float64(dev->VolHdr.write_date);   /* 0 if VerNum >= 11 */
   ser_float64(dev->VolHdr.write_time);   /* 0	if VerNum >= 11 */

   ser_string(dev->VolHdr.VolName);
   ser_string(dev->VolHdr.PrevVolName);
   ser_string(dev->VolHdr.PoolName);
   ser_string(dev->VolHdr.PoolType);
   ser_string(dev->VolHdr.MediaType);

   ser_string(dev->VolHdr.HostName);
   ser_string(dev->VolHdr.LabelProg);
   ser_string(dev->VolHdr.ProgVersion);
   ser_string(dev->VolHdr.ProgDate);

   ser_end(rec->data, SER_LENGTH_Volume_Label);
   rec->data_len = ser_length(rec->data);
   rec->FileIndex = dev->VolHdr.LabelType;
   rec->VolSessionId = jcr->VolSessionId;
   rec->VolSessionTime = jcr->VolSessionTime;
   rec->Stream = jcr->NumVolumes;
   Dmsg2(100, "Created Vol label rec: FI=%s len=%d\n", FI_to_ascii(rec->FileIndex),
      rec->data_len);
}     


/*
 * Create a volume label in memory
 */
void create_volume_label(DEVICE *dev, const char *VolName, const char *PoolName)
{
   DEVRES *device = (DEVRES *)dev->device;

   Dmsg0(90, "Start create_volume_label()\n");

   ASSERT(dev != NULL);

   memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));

   bstrncpy(dev->VolHdr.Id, BaculaId, sizeof(dev->VolHdr.Id));
   dev->VolHdr.VerNum = BaculaTapeVersion;
   dev->VolHdr.LabelType = PRE_LABEL;  /* Mark tape as unused */
   bstrncpy(dev->VolHdr.VolName, VolName, sizeof(dev->VolHdr.VolName));
   bstrncpy(dev->VolHdr.PoolName, PoolName, sizeof(dev->VolHdr.PoolName));
   bstrncpy(dev->VolHdr.MediaType, device->media_type, sizeof(dev->VolHdr.MediaType));

   bstrncpy(dev->VolHdr.PoolType, "Backup", sizeof(dev->VolHdr.PoolType));

   dev->VolHdr.label_btime = get_current_btime();
   dev->VolHdr.label_date = 0;
   dev->VolHdr.label_time = 0;

   if (gethostname(dev->VolHdr.HostName, sizeof(dev->VolHdr.HostName)) != 0) {
      dev->VolHdr.HostName[0] = 0;
   }
   bstrncpy(dev->VolHdr.LabelProg, my_name, sizeof(dev->VolHdr.LabelProg));
   sprintf(dev->VolHdr.ProgVersion, "Ver. %s %s", VERSION, BDATE);
   sprintf(dev->VolHdr.ProgDate, "Build %s %s", __DATE__, __TIME__);
   dev->state |= ST_LABEL;	      /* set has Bacula label */
   if (debug_level >= 90) {
      dump_volume_label(dev);
   }
}

/*
 * Write a Volume Label
 *  !!! Note, this is ONLY used for writing
 *	      a fresh volume label.  Any data
 *	      after the label will be destroyed,
 *	      in fact, we write the label 5 times !!!!
 * 
 *  This routine expects that open_device() was previously called.
 *
 *  This routine should be used only when labeling a blank tape.
 */
bool write_new_volume_label_to_dev(DCR *dcr, const char *VolName, const char *PoolName)
{
   bool ok = false;
   DEVICE *dev = dcr->dev;


   Dmsg0(99, "write_volume_label()\n");
   empty_block(dcr->block);
   /* Create PRE_LABEL */	
   create_volume_label(dev, VolName, PoolName);

   if (!rewind_dev(dev)) {
      memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));
      Dmsg2(30, "Bad status on %s from rewind. ERR=%s\n", dev_name(dev), strerror_dev(dev));
      if (!forge_on) {
	 return false;
      }
   }

   create_volume_label_record(dcr, dcr->rec);
   dcr->rec->Stream = 0;

   /* Temporarily mark in append state to enable writing */
   dev->state |= ST_APPEND;
   if (!write_record_to_block(dcr->block, dcr->rec)) {
      Dmsg2(30, "Bad Label write on %s. ERR=%s\n", dev_name(dev), strerror_dev(dev));
      memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));
      goto bail_out;
   } else {
      Dmsg2(30, "Wrote label of %d bytes to %s\n", dcr->rec->data_len, dev_name(dev));
   }
      
   Dmsg0(99, "Call write_block_to_dev()\n");
   if (!write_block_to_dev(dcr)) {
      memset(&dev->VolHdr, 0, sizeof(dev->VolHdr));
      Dmsg2(30, "Bad Label write on %s. ERR=%s\n", dev_name(dev), strerror_dev(dev));
      goto bail_out;
   }
   Dmsg0(99, " Wrote block to device\n");
     
   if (weof_dev(dev, 1) == 0) {
      dev->state |= ST_LABEL;
      ok = true;
   }

   if (debug_level >= 20)  {
      dump_volume_label(dev);
   }

bail_out:
   dev->state &= ~ST_APPEND;	      /* remove append since this is PRE_LABEL */
   return ok;
}     


/*
 * Create session label
 *  The pool memory must be released by the calling program
 */
void create_session_label(DCR *dcr, DEV_RECORD *rec, int label)
{
   JCR *jcr = dcr->jcr;
   ser_declare;

   rec->VolSessionId   = jcr->VolSessionId;
   rec->VolSessionTime = jcr->VolSessionTime;
   rec->Stream	       = jcr->JobId;

   rec->data = check_pool_memory_size(rec->data, SER_LENGTH_Session_Label);
   ser_begin(rec->data, SER_LENGTH_Session_Label);
   ser_string(BaculaId);
   ser_uint32(BaculaTapeVersion);

   ser_uint32(jcr->JobId);

   /* Changed in VerNum 11 */
   ser_btime(get_current_btime());
   ser_float64(0);

   ser_string(dcr->pool_name);
   ser_string(dcr->pool_type);
   ser_string(jcr->job_name);	      /* base Job name */
   ser_string(jcr->client_name);

   /* Added in VerNum 10 */
   ser_string(jcr->Job);	      /* Unique name of this Job */
   ser_string(jcr->fileset_name);
   ser_uint32(jcr->JobType);
   ser_uint32(jcr->JobLevel);
   /* Added in VerNum 11 */
   ser_string(jcr->fileset_md5);

   if (label == EOS_LABEL) {
      ser_uint32(jcr->JobFiles);
      ser_uint64(jcr->JobBytes);
      ser_uint32(dcr->StartBlock);
      ser_uint32(dcr->EndBlock);
      ser_uint32(dcr->StartFile);
      ser_uint32(dcr->EndFile);
      ser_uint32(jcr->JobErrors);

      /* Added in VerNum 11 */
      ser_uint32(jcr->JobStatus);
   }
   ser_end(rec->data, SER_LENGTH_Session_Label);
   rec->data_len = ser_length(rec->data);
}

/* Write session label
 *  Returns: false on failure
 *	     true  on success 
 */
bool write_session_label(DCR *dcr, int label)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   DEV_RECORD *rec;
   DEV_BLOCK *block = dcr->block;

   rec = new_record();
   Dmsg1(90, "session_label record=%x\n", rec);
   switch (label) {
   case SOS_LABEL:
      if (dev->state & ST_TAPE) {
	 dcr->StartBlock = dev->block_num;
	 dcr->StartFile  = dev->file;
      } else {
	 dcr->StartBlock = (uint32_t)dev->file_addr;
	 dcr->StartFile = (uint32_t)(dev->file_addr >> 32);
      }
      break;
   case EOS_LABEL:
      if (dev->state & ST_TAPE) {
	 dcr->EndBlock = dev->EndBlock;
	 dcr->EndFile  = dev->EndFile;
      } else {
	 dcr->EndBlock = (uint32_t)dev->file_addr;
	 dcr->EndFile = (uint32_t)(dev->file_addr >> 32);
      }
      break;
   default:
      Jmsg1(jcr, M_ABORT, 0, _("Bad session label = %d\n"), label);
      break;
   }
   create_session_label(dcr, rec, label);
   rec->FileIndex = label;

   /* 
    * We guarantee that the session record can totally fit
    *  into a block. If not, write the block, and put it in
    *  the next block. Having the sesssion record totally in
    *  one block makes reading them much easier (no need to
    *  read the next block).
    */
   if (!can_write_record_to_block(block, rec)) {
      Dmsg0(100, "Cannot write session label to block.\n");
      if (!write_block_to_device(dcr)) {
         Dmsg0(90, "Got session label write_block_to_dev error.\n");
	 /* ****FIXME***** errno is not set here */
         Jmsg(jcr, M_FATAL, 0, _("Error writing Session label to %s: %s\n"), 
			   dev_vol_name(dev), strerror(errno));
	 free_record(rec);
	 return false;
      }
   }
   if (!write_record_to_block(block, rec)) {
      Jmsg(jcr, M_FATAL, 0, _("Error writing Session label to %s: %s\n"), 
			dev_vol_name(dev), strerror(errno));
      free_record(rec);
      return false;
   }

   Dmsg6(20, "Write sesson_label record JobId=%d FI=%s SessId=%d Strm=%s len=%d " 
             "remainder=%d\n", jcr->JobId,
      FI_to_ascii(rec->FileIndex), rec->VolSessionId, 
      stream_to_ascii(rec->Stream, rec->FileIndex), rec->data_len,
      rec->remainder);

   free_record(rec);
   Dmsg2(20, "Leave write_session_label Block=%d File=%d\n", 
      dev->block_num, dev->file);
   return true;
}

void dump_volume_label(DEVICE *dev)
{
   int dbl = debug_level;
   uint32_t File;
   const char *LabelType;
   char buf[30];
   struct tm tm;
   struct date_time dt;

   debug_level = 1;
   File = dev->file;
   switch (dev->VolHdr.LabelType) {
      case PRE_LABEL:
         LabelType = "PRE_LABEL";
	 break;
      case VOL_LABEL:
         LabelType = "VOL_LABEL";
	 break;
      case EOM_LABEL:
         LabelType = "EOM_LABEL";
	 break;
      case SOS_LABEL:
         LabelType = "SOS_LABEL";
	 break;
      case EOS_LABEL:
         LabelType = "EOS_LABEL";
	 break;
      case EOT_LABEL:
	 goto bail_out;
      default:
	 LabelType = buf;
         sprintf(buf, "Unknown %d", dev->VolHdr.LabelType);
	 break;
   }
	      
   
   Pmsg11(-1, "\nVolume Label:\n\
Id                : %s\
VerNo             : %d\n\
VolName           : %s\n\
PrevVolName       : %s\n\
VolFile           : %d\n\
LabelType         : %s\n\
LabelSize         : %d\n\
PoolName          : %s\n\
MediaType         : %s\n\
PoolType          : %s\n\
HostName          : %s\n\
",
	     dev->VolHdr.Id, dev->VolHdr.VerNum,
	     dev->VolHdr.VolName, dev->VolHdr.PrevVolName,
	     File, LabelType, dev->VolHdr.LabelSize, 
	     dev->VolHdr.PoolName, dev->VolHdr.MediaType, 
	     dev->VolHdr.PoolType, dev->VolHdr.HostName);

   if (dev->VolHdr.VerNum >= 11) {
      char dt[50];
      bstrftime(dt, sizeof(dt), btime_to_unix(dev->VolHdr.label_btime));
      Pmsg1(-1, "Date label written: %s\n", dt);
   } else {
   dt.julian_day_number   = dev->VolHdr.label_date;
   dt.julian_day_fraction = dev->VolHdr.label_time;
   tm_decode(&dt, &tm);
   Pmsg5(-1, "\
Date label written: %04d-%02d-%02d at %02d:%02d\n", 
      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);
   }

bail_out:
   debug_level = dbl;
}

bool unser_session_label(SESSION_LABEL *label, DEV_RECORD *rec) 
{
   ser_declare;

   rec->data = check_pool_memory_size(rec->data, SER_LENGTH_Session_Label);
   unser_begin(rec->data, SER_LENGTH_Session_Label);
   unser_string(label->Id);
   unser_uint32(label->VerNum);
   unser_uint32(label->JobId);
   if (label->VerNum >= 11) {
      unser_btime(label->write_btime);
   } else {
      unser_float64(label->write_date);
   }
   unser_float64(label->write_time);
   unser_string(label->PoolName);
   unser_string(label->PoolType);
   unser_string(label->JobName);
   unser_string(label->ClientName);
   if (label->VerNum >= 10) {
      unser_string(label->Job); 	 /* Unique name of this Job */
      unser_string(label->FileSetName);
      unser_uint32(label->JobType);
      unser_uint32(label->JobLevel);
   }
   if (label->VerNum >= 11) {
      unser_string(label->FileSetMD5);
   } else {
      label->FileSetMD5[0] = 0;
   }
   if (rec->FileIndex == EOS_LABEL) {
      unser_uint32(label->JobFiles);
      unser_uint64(label->JobBytes);
      unser_uint32(label->StartBlock);
      unser_uint32(label->EndBlock);
      unser_uint32(label->StartFile);
      unser_uint32(label->EndFile);
      unser_uint32(label->JobErrors);
      if (label->VerNum >= 11) {
	 unser_uint32(label->JobStatus);
      } else {
	 label->JobStatus = JS_Terminated; /* kludge */
      }
   }	  
   return true;
}


static void dump_session_label(DEV_RECORD *rec, const char *type)
{
   int dbl;
   struct date_time dt;
   struct tm tm;
   SESSION_LABEL label;
   char ec1[30], ec2[30], ec3[30], ec4[30], ec5[30], ec6[30], ec7[30];

   unser_session_label(&label, rec);
   dbl = debug_level;
   debug_level = 1;
   Pmsg7(-1, "\n%s Record:\n\
JobId             : %d\n\
VerNum            : %d\n\
PoolName          : %s\n\
PoolType          : %s\n\
JobName           : %s\n\
ClientName        : %s\n\
",    type, label.JobId, label.VerNum,
      label.PoolName, label.PoolType,
      label.JobName, label.ClientName);

   if (label.VerNum >= 10) {
      Pmsg4(-1, "\
Job (unique name) : %s\n\
FileSet           : %s\n\
JobType           : %c\n\
JobLevel          : %c\n\
", label.Job, label.FileSetName, label.JobType, label.JobLevel);
   }

   if (rec->FileIndex == EOS_LABEL) {
      Pmsg8(-1, "\
JobFiles          : %s\n\
JobBytes          : %s\n\
StartBlock        : %s\n\
EndBlock          : %s\n\
StartFile         : %s\n\
EndFile           : %s\n\
JobErrors         : %s\n\
JobStatus         : %c\n\
",
	 edit_uint64_with_commas(label.JobFiles, ec1),
	 edit_uint64_with_commas(label.JobBytes, ec2),
	 edit_uint64_with_commas(label.StartBlock, ec3),
	 edit_uint64_with_commas(label.EndBlock, ec4),
	 edit_uint64_with_commas(label.StartFile, ec5),
	 edit_uint64_with_commas(label.EndFile, ec6),
	 edit_uint64_with_commas(label.JobErrors, ec7), 
	 label.JobStatus);
   }
   if (label.VerNum >= 11) {
      char dt[50];
      bstrftime(dt, sizeof(dt), btime_to_unix(label.write_btime));
      Pmsg1(-1, _("Date written      : %s\n"), dt);
   } else {
      dt.julian_day_number   = label.write_date;
      dt.julian_day_fraction = label.write_time;
      tm_decode(&dt, &tm);
      Pmsg5(-1, _("\
Date written      : %04d-%02d-%02d at %02d:%02d\n"),
      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);
   }

   debug_level = dbl;
}

void dump_label_record(DEVICE *dev, DEV_RECORD *rec, int verbose)
{
   const char *type;
   int dbl;

   dbl = debug_level;
   debug_level = 1;
   switch (rec->FileIndex) {
   case PRE_LABEL:
      type = _("Fresh Volume");   
      break;
   case VOL_LABEL:
      type = _("Volume");
      break;
   case SOS_LABEL:
      type = _("Begin Job Session");
      break;
   case EOS_LABEL:
      type = _("End Job Session");
      break;
   case EOM_LABEL:
      type = _("End of Media");
      break;
   case EOT_LABEL:
      type = ("End of Tape");
      break;
   default:
      type = _("Unknown");
      break;
   }
   if (verbose) {
      switch (rec->FileIndex) {
      case PRE_LABEL:
      case VOL_LABEL:
	 unser_volume_label(dev, rec);
	 dump_volume_label(dev);
	 break;
      case SOS_LABEL:
	 dump_session_label(rec, type);
	 break;
      case EOS_LABEL:
	 dump_session_label(rec, type);
	 break;
      case EOM_LABEL:
         Pmsg5(-1, "%s Record: SessId=%d SessTime=%d JobId=%d DataLen=%d\n",
	    type, rec->VolSessionId, rec->VolSessionTime, rec->Stream, rec->data_len);
	 break;
      case EOT_LABEL:
         Pmsg0(-1, _("End of physical tape.\n"));
	 break;
      default:
         Pmsg5(-1, "%s Record: SessId=%d SessTime=%d JobId=%d DataLen=%d\n",
	    type, rec->VolSessionId, rec->VolSessionTime, rec->Stream, rec->data_len);
	 break;
      }
   } else {
      SESSION_LABEL label;
      switch (rec->FileIndex) {
      case SOS_LABEL:
	 unser_session_label(&label, rec);
         Pmsg6(-1, "%s Record: SessId=%d SessTime=%d JobId=%d Level=%c Type=%c\n",
	    type, rec->VolSessionId, rec->VolSessionTime, rec->Stream, 
	    label.JobLevel, label.JobType);
	 break;
      case EOS_LABEL:
	 char ed1[30], ed2[30];
	 unser_session_label(&label, rec);
         Pmsg6(-1, "%s Record: SessId=%d SessTime=%d JobId=%d Level=%c Type=%c\n",
	    type, rec->VolSessionId, rec->VolSessionTime, rec->Stream, 
	    label.JobLevel, label.JobType);
         Pmsg4(-1, "   Files=%s Bytes=%s Errors=%d Status=%c\n",
	    edit_uint64_with_commas(label.JobFiles, ed1),
	    edit_uint64_with_commas(label.JobBytes, ed2),
	    label.JobErrors, (char)label.JobStatus);
	 break;
      case EOM_LABEL:
      case PRE_LABEL:
      case VOL_LABEL:
      default:
         Pmsg5(-1, "%s Record: SessId=%d SessTime=%d JobId=%d DataLen=%d\n",
      type, rec->VolSessionId, rec->VolSessionTime, rec->Stream, rec->data_len);
	 break;
      case EOT_LABEL:
	 break;
      }
   }
   debug_level = dbl;
}
