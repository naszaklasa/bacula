/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2007 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula Director -- catreq.c -- handles the message channel
 *    catalog request from the Storage daemon.
 *
 *     Kern Sibbald, March MMI
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *      Handle Catalog services.
 *
 *   Version $Id: catreq.c 4992 2007-06-07 14:46:43Z kerns $
 */

#include "bacula.h"
#include "dird.h"
#include "findlib/find.h"

/*
 * Handle catalog request
 *  For now, we simply return next Volume to be used
 */

/* Requests from the Storage daemon */
static char Find_media[] = "CatReq Job=%127s FindMedia=%d pool_name=%127s media_type=%127s\n";
static char Get_Vol_Info[] = "CatReq Job=%127s GetVolInfo VolName=%127s write=%d\n";

static char Update_media[] = "CatReq Job=%127s UpdateMedia VolName=%s"
   " VolJobs=%u VolFiles=%u VolBlocks=%u VolBytes=%" lld " VolMounts=%u"
   " VolErrors=%u VolWrites=%u MaxVolBytes=%" lld " EndTime=%d VolStatus=%10s"
   " Slot=%d relabel=%d InChanger=%d VolReadTime=%" lld " VolWriteTime=%" lld
   " VolFirstWritten=%" lld " VolParts=%u\n";

static char Create_job_media[] = "CatReq Job=%127s CreateJobMedia "
   " FirstIndex=%u LastIndex=%u StartFile=%u EndFile=%u "
   " StartBlock=%u EndBlock=%u Copy=%d Strip=%d MediaId=%" lld "\n";


/* Responses  sent to Storage daemon */
static char OK_media[] = "1000 OK VolName=%s VolJobs=%u VolFiles=%u"
   " VolBlocks=%u VolBytes=%s VolMounts=%u VolErrors=%u VolWrites=%u"
   " MaxVolBytes=%s VolCapacityBytes=%s VolStatus=%s Slot=%d"
   " MaxVolJobs=%u MaxVolFiles=%u InChanger=%d VolReadTime=%s"
   " VolWriteTime=%s EndFile=%u EndBlock=%u VolParts=%u LabelType=%d"
   " MediaId=%s\n";

static char OK_create[] = "1000 OK CreateJobMedia\n";


static int send_volume_info_to_storage_daemon(JCR *jcr, BSOCK *sd, MEDIA_DBR *mr)
{
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50], ed5[50], ed6[50];

   jcr->MediaId = mr->MediaId;
   pm_strcpy(jcr->VolumeName, mr->VolumeName);
   bash_spaces(mr->VolumeName);
   stat = bnet_fsend(sd, OK_media, mr->VolumeName, mr->VolJobs,
      mr->VolFiles, mr->VolBlocks, edit_uint64(mr->VolBytes, ed1),
      mr->VolMounts, mr->VolErrors, mr->VolWrites,
      edit_uint64(mr->MaxVolBytes, ed2),
      edit_uint64(mr->VolCapacityBytes, ed3),
      mr->VolStatus, mr->Slot, mr->MaxVolJobs, mr->MaxVolFiles,
      mr->InChanger,
      edit_int64(mr->VolReadTime, ed4),
      edit_int64(mr->VolWriteTime, ed5),
      mr->EndFile, mr->EndBlock,
      mr->VolParts,
      mr->LabelType,
      edit_uint64(mr->MediaId, ed6));
   unbash_spaces(mr->VolumeName);
   Dmsg2(100, "Vol Info for %s: %s", jcr->Job, sd->msg);
   return stat;
}

void catalog_request(JCR *jcr, BSOCK *bs)
{
   MEDIA_DBR mr, sdmr;
   JOBMEDIA_DBR jm;
   char Job[MAX_NAME_LENGTH];
   char pool_name[MAX_NAME_LENGTH];
   int index, ok, label, writing;
   POOLMEM *omsg;
   POOL_DBR pr;
   uint32_t Stripe;
   uint64_t MediaId;
   utime_t VolFirstWritten;

   memset(&mr, 0, sizeof(mr));
   memset(&sdmr, 0, sizeof(sdmr));
   memset(&jm, 0, sizeof(jm));
   Dsm_check(1);      

   /*
    * Request to find next appendable Volume for this Job
    */
   Dmsg1(100, "catreq %s", bs->msg);
   if (!jcr->db) {
      omsg = get_memory(bs->msglen+1);
      pm_strcpy(omsg, bs->msg);
      bnet_fsend(bs, _("1990 Invalid Catalog Request: %s"), omsg);    
      Jmsg1(jcr, M_FATAL, 0, _("Invalid Catalog request; DB not open: %s"), omsg);
      free_memory(omsg);
      return;
   }
   /*
    * Find next appendable medium for SD
    */
   if (sscanf(bs->msg, Find_media, &Job, &index, &pool_name, &mr.MediaType) == 4) {
      memset(&pr, 0, sizeof(pr));
      bstrncpy(pr.Name, pool_name, sizeof(pr.Name));
      unbash_spaces(pr.Name);
      ok = db_get_pool_record(jcr, jcr->db, &pr);
      if (ok) {
         mr.PoolId = pr.PoolId;
         mr.StorageId = jcr->wstore->StorageId;
         ok = find_next_volume_for_append(jcr, &mr, index, fnv_create_vol, fnv_prune);
         Dmsg3(050, "find_media ok=%d idx=%d vol=%s\n", ok, index, mr.VolumeName);
      }
      /*
       * Send Find Media response to Storage daemon
       */
      if (ok) {
         send_volume_info_to_storage_daemon(jcr, bs, &mr);
      } else {
         bnet_fsend(bs, _("1901 No Media.\n"));
         Dmsg0(500, "1901 No Media.\n");
      }

   /*
    * Request to find specific Volume information
    */
   } else if (sscanf(bs->msg, Get_Vol_Info, &Job, &mr.VolumeName, &writing) == 3) {
      Dmsg1(100, "CatReq GetVolInfo Vol=%s\n", mr.VolumeName);
      /*
       * Find the Volume
       */
      unbash_spaces(mr.VolumeName);
      if (db_get_media_record(jcr, jcr->db, &mr)) {
         const char *reason = NULL;           /* detailed reason for rejection */
         /*
          * If we are reading, accept any volume (reason == NULL)
          * If we are writing, check if the Volume is valid
          *   for this job, and do a recycle if necessary
          */
         if (writing) {
            /*
             * SD wants to write this Volume, so make
             *   sure it is suitable for this job, i.e.
             *   Pool matches, and it is either Append or Recycle
             *   and Media Type matches and Pool allows any volume.
             */
            if (mr.PoolId != jcr->jr.PoolId) {
               reason = _("not in Pool");
            } else if (strcmp(mr.MediaType, jcr->wstore->media_type) != 0) {
               reason = _("not correct MediaType");
            } else {
               /*
                * Now try recycling if necessary
                *   reason set non-NULL if we cannot use it
                */
               check_if_volume_valid_or_recyclable(jcr, &mr, &reason);
            }
         }
         if (!reason && mr.Enabled != 1) {
            reason = _("is not Enabled");
         }
         if (reason == NULL) {
            /*
             * Send Find Media response to Storage daemon
             */
            send_volume_info_to_storage_daemon(jcr, bs, &mr);
         } else {
            /* Not suitable volume */
            bnet_fsend(bs, _("1998 Volume \"%s\" status is %s, %s.\n"), mr.VolumeName,
               mr.VolStatus, reason);
         }

      } else {
         bnet_fsend(bs, _("1997 Volume \"%s\" not in catalog.\n"), mr.VolumeName);
         Dmsg1(100, "1997 Volume \"%s\" not in catalog.\n", mr.VolumeName);
      }

   /*
    * Request to update Media record. Comes typically at the end
    *  of a Storage daemon Job Session, when labeling/relabeling a
    *  Volume, or when an EOF mark is written.
    */
   } else if (sscanf(bs->msg, Update_media, &Job, &sdmr.VolumeName,
      &sdmr.VolJobs, &sdmr.VolFiles, &sdmr.VolBlocks, &sdmr.VolBytes,
      &sdmr.VolMounts, &sdmr.VolErrors, &sdmr.VolWrites, &sdmr.MaxVolBytes,
      &sdmr.LastWritten, &sdmr.VolStatus, &sdmr.Slot, &label, &sdmr.InChanger,
      &sdmr.VolReadTime, &sdmr.VolWriteTime, &VolFirstWritten,
      &sdmr.VolParts) == 19) {

      db_lock(jcr->db);
      Dmsg3(400, "Update media %s oldStat=%s newStat=%s\n", sdmr.VolumeName,
         mr.VolStatus, sdmr.VolStatus);
      bstrncpy(mr.VolumeName, sdmr.VolumeName, sizeof(mr.VolumeName)); /* copy Volume name */
      unbash_spaces(mr.VolumeName);
      if (!db_get_media_record(jcr, jcr->db, &mr)) {
         Jmsg(jcr, M_ERROR, 0, _("Unable to get Media record for Volume %s: ERR=%s\n"),
              mr.VolumeName, db_strerror(jcr->db));
         bnet_fsend(bs, _("1991 Catalog Request for vol=%s failed: %s"),
            mr.VolumeName, db_strerror(jcr->db));
         db_unlock(jcr->db);
         return;
      }
      /* Set first written time if this is first job */
      if (mr.FirstWritten == 0) {
         if (VolFirstWritten == 0) {
            mr.FirstWritten = jcr->start_time;   /* use Job start time as first write */
         } else {
            mr.FirstWritten = VolFirstWritten;
         }
         mr.set_first_written = true;
      }
      /* If we just labeled the tape set time */
      if (label || mr.LabelDate == 0) {
         mr.LabelDate = jcr->start_time;
         mr.set_label_date = true;
         if (mr.InitialWrite == 0) {
            mr.InitialWrite = jcr->start_time;
         }
         Dmsg2(400, "label=%d labeldate=%d\n", label, mr.LabelDate);
      } else {
         /*
          * Insanity check for VolFiles get set to a smaller value
          */
         if (sdmr.VolFiles < mr.VolFiles) {
            Jmsg(jcr, M_FATAL, 0, _("Volume Files at %u being set to %u"
                 " for Volume \"%s\". This is incorrect.\n"),
               mr.VolFiles, sdmr.VolFiles, mr.VolumeName);
            bnet_fsend(bs, _("1992 Update Media error. VolFiles=%u, CatFiles=%u\n"),
               sdmr.VolFiles, mr.VolFiles);
            db_unlock(jcr->db);
            return;
         }
      }
      Dmsg2(400, "Update media: BefVolJobs=%u After=%u\n", mr.VolJobs, sdmr.VolJobs);
      /* Copy updated values to original media record */
      mr.VolJobs      = sdmr.VolJobs;
      mr.VolFiles     = sdmr.VolFiles;
      mr.VolBlocks    = sdmr.VolBlocks;
      mr.VolBytes     = sdmr.VolBytes;
      mr.VolMounts    = sdmr.VolMounts;
      mr.VolErrors    = sdmr.VolErrors;
      mr.VolWrites    = sdmr.VolWrites;
      mr.LastWritten  = sdmr.LastWritten;
      mr.Slot         = sdmr.Slot;
      mr.InChanger    = sdmr.InChanger;
      mr.VolReadTime  = sdmr.VolReadTime;
      mr.VolWriteTime = sdmr.VolWriteTime;
      mr.VolParts     = sdmr.VolParts;
      bstrncpy(mr.VolStatus, sdmr.VolStatus, sizeof(mr.VolStatus));
      if (jcr->wstore && jcr->wstore->StorageId) {
         mr.StorageId = jcr->wstore->StorageId;
      }

      Dmsg2(400, "db_update_media_record. Stat=%s Vol=%s\n", mr.VolStatus, mr.VolumeName);
      /*
       * Update the database, then before sending the response to the
       *  SD, check if the Volume has expired.
       */
      if (!db_update_media_record(jcr, jcr->db, &mr)) {
         Jmsg(jcr, M_FATAL, 0, _("Catalog error updating Media record. %s"),
            db_strerror(jcr->db));
         bnet_fsend(bs, _("1993 Update Media error\n"));
         Dmsg0(400, "send error\n");
      } else {
         (void)has_volume_expired(jcr, &mr);
         send_volume_info_to_storage_daemon(jcr, bs, &mr);
      }
      db_unlock(jcr->db);

   /*
    * Request to create a JobMedia record
    */
   } else if (sscanf(bs->msg, Create_job_media, &Job,
      &jm.FirstIndex, &jm.LastIndex, &jm.StartFile, &jm.EndFile,
      &jm.StartBlock, &jm.EndBlock, &jm.Copy, &Stripe, &MediaId) == 10) {

      if (jcr->mig_jcr) {
         jm.JobId = jcr->mig_jcr->JobId;
      } else {
         jm.JobId = jcr->JobId;
      }
      jm.MediaId = MediaId;
      Dmsg6(400, "create_jobmedia JobId=%d MediaId=%d SF=%d EF=%d FI=%d LI=%d\n",
         jm.JobId, jm.MediaId, jm.StartFile, jm.EndFile, jm.FirstIndex, jm.LastIndex);
      if (!db_create_jobmedia_record(jcr, jcr->db, &jm)) {
         Jmsg(jcr, M_FATAL, 0, _("Catalog error creating JobMedia record. %s"),
            db_strerror(jcr->db));
         bnet_fsend(bs, _("1991 Update JobMedia error\n"));
      } else {
         Dmsg0(400, "JobMedia record created\n");
         bnet_fsend(bs, OK_create);
      }

   } else {
      omsg = get_memory(bs->msglen+1);
      pm_strcpy(omsg, bs->msg);
      bnet_fsend(bs, _("1990 Invalid Catalog Request: %s"), omsg);
      Jmsg1(jcr, M_FATAL, 0, _("Invalid Catalog request: %s"), omsg);
      free_memory(omsg);
   }
   Dmsg1(400, ">CatReq response: %s", bs->msg);
   Dmsg1(400, "Leave catreq jcr 0x%x\n", jcr);
   return;
}

/*
 * Update File Attributes in the catalog with data
 *  sent by the Storage daemon.  Note, we receive the whole
 *  attribute record, but we select out only the stat packet,
 *  VolSessionId, VolSessionTime, FileIndex, and file name
 *  to store in the catalog.
 */
void catalog_update(JCR *jcr, BSOCK *bs)
{
   unser_declare;
   uint32_t VolSessionId, VolSessionTime;
   int32_t Stream;
   uint32_t FileIndex;
   uint32_t data_len;
   char *p;
   int len;
   char *fname, *attr;
   ATTR_DBR *ar = NULL;
   POOLMEM *omsg;

   Dsm_check(1);
   if (!jcr->pool->catalog_files) {
      return;                         /* user disabled cataloging */
   }
   if (!jcr->db) {
      omsg = get_memory(bs->msglen+1);
      pm_strcpy(omsg, bs->msg);
      bnet_fsend(bs, _("1991 Invalid Catalog Update: %s"), omsg);    
      Jmsg1(jcr, M_FATAL, 0, _("Invalid Catalog Update; DB not open: %s"), omsg);
      free_memory(omsg);
      return;
   }

   /* Start transaction allocates jcr->attr and jcr->ar if needed */
   db_start_transaction(jcr, jcr->db);     /* start transaction if not already open */
   ar = jcr->ar;      

   /* Start by scanning directly in the message buffer to get Stream   
    *  there may be a cached attr so we cannot yet write into
    *  jcr->attr or jcr->ar  
    */
   p = bs->msg;
   skip_nonspaces(&p);                /* UpdCat */
   skip_spaces(&p);
   skip_nonspaces(&p);                /* Job=nnn */
   skip_spaces(&p);
   skip_nonspaces(&p);                /* FileAttributes */
   p += 1;
   unser_begin(p, 0);
   unser_uint32(VolSessionId);
   unser_uint32(VolSessionTime);
   unser_int32(FileIndex);
   unser_int32(Stream);
   unser_uint32(data_len);
   p += unser_length(p);

   Dmsg1(400, "UpdCat msg=%s\n", bs->msg);
   Dmsg5(400, "UpdCat VolSessId=%d VolSessT=%d FI=%d Strm=%d data_len=%d\n",
      VolSessionId, VolSessionTime, FileIndex, Stream, data_len);

   if (Stream == STREAM_UNIX_ATTRIBUTES || Stream == STREAM_UNIX_ATTRIBUTES_EX) {
      if (jcr->cached_attribute) {
         Dmsg2(400, "Cached attr. Stream=%d fname=%s\n", ar->Stream, ar->fname);
         if (!db_create_file_attributes_record(jcr, jcr->db, ar)) {
            Jmsg1(jcr, M_FATAL, 0, _("Attribute create error. %s"), db_strerror(jcr->db));
         }
      }
      /* Any cached attr is flushed so we can reuse jcr->attr and jcr->ar */
      jcr->attr = check_pool_memory_size(jcr->attr, bs->msglen);
      memcpy(jcr->attr, bs->msg, bs->msglen);
      p = jcr->attr - bs->msg + p;    /* point p into jcr->attr */
      skip_nonspaces(&p);             /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip FileType */
      skip_spaces(&p);
      fname = p;
      len = strlen(fname);        /* length before attributes */
      attr = &fname[len+1];

      Dmsg2(400, "dird<stored: stream=%d %s\n", Stream, fname);
      Dmsg1(400, "dird<stored: attr=%s\n", attr);
      ar->attr = attr;
      ar->fname = fname;
      ar->FileIndex = FileIndex;
      ar->Stream = Stream;
      ar->link = NULL;
      if (jcr->mig_jcr) {
         ar->JobId = jcr->mig_jcr->JobId;
      } else {
         ar->JobId = jcr->JobId;
      }
      ar->Digest = NULL;
      ar->DigestType = CRYPTO_DIGEST_NONE;
      jcr->cached_attribute = true;

      Dmsg2(400, "dird<filed: stream=%d %s\n", Stream, fname);
      Dmsg1(400, "dird<filed: attr=%s\n", attr);

   } else if (crypto_digest_stream_type(Stream) != CRYPTO_DIGEST_NONE) {
      fname = p;
      if (ar->FileIndex != FileIndex) {
         Jmsg(jcr, M_WARNING, 0, _("Got %s but not same File as attributes\n"), stream_to_ascii(Stream));
      } else {
         /* Update digest in catalog */
         char digestbuf[BASE64_SIZE(CRYPTO_DIGEST_MAX_SIZE)];
         int len = 0;
         int type = CRYPTO_DIGEST_NONE;

         switch(Stream) {
         case STREAM_MD5_DIGEST:
            len = CRYPTO_DIGEST_MD5_SIZE;
            type = CRYPTO_DIGEST_MD5;
            break;
         case STREAM_SHA1_DIGEST:
            len = CRYPTO_DIGEST_SHA1_SIZE;
            type = CRYPTO_DIGEST_SHA1;
            break;
         case STREAM_SHA256_DIGEST:
            len = CRYPTO_DIGEST_SHA256_SIZE;
            type = CRYPTO_DIGEST_SHA256;
            break;
         case STREAM_SHA512_DIGEST:
            len = CRYPTO_DIGEST_SHA512_SIZE;
            type = CRYPTO_DIGEST_SHA512;
            break;
         default:
            /* Never reached ... */
            Jmsg(jcr, M_ERROR, 0, _("Catalog error updating file digest. Unsupported digest stream type: %d"),
                 Stream);
         }

         bin_to_base64(digestbuf, sizeof(digestbuf), fname, len, true);
         Dmsg3(400, "DigestLen=%d Digest=%s type=%d\n", strlen(digestbuf), digestbuf, Stream);
         if (jcr->cached_attribute) {
            ar->Digest = digestbuf;
            ar->DigestType = type;
            Dmsg2(400, "Cached attr with digest. Stream=%d fname=%s\n", ar->Stream, ar->fname);
            if (!db_create_file_attributes_record(jcr, jcr->db, ar)) {
               Jmsg1(jcr, M_FATAL, 0, _("Attribute create error. %s"), db_strerror(jcr->db));
            }
            jcr->cached_attribute = false; 
         } else {
            if (!db_add_digest_to_file_record(jcr, jcr->db, ar->FileId, digestbuf, type)) {
               Jmsg(jcr, M_ERROR, 0, _("Catalog error updating file digest. %s"),
                  db_strerror(jcr->db));
            }
         }
      }
   }
}
