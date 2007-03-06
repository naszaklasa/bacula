/*
 * Bacula Catalog Database Update record interface routines
 *
 * Bacula Catalog Database routines written specifically
 *  for Bacula.  Note, these routines are VERY dumb and
 *  do not provide all the functionality of an SQL database.
 *  The purpose of these routines is to ensure that Bacula
 *  can limp along if no real database is loaded on the
 *  system.
 *
 *    Kern Sibbald, January MMI
 *
 *
 *    Version $Id: bdb_update.c,v 1.22 2006/11/27 10:02:59 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

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


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"
#include "bdb.h"

#ifdef HAVE_BACULA_DB

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


/*
 * This is called at Job start time to add the
 * most current start fields to the job record.
 * It is assumed that you did a db_create_job_record() already.
 */
bool db_update_job_start_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   int len, stat = 1;
   JOB_DBR ojr;

   db_lock(mdb);

   Dmsg0(200, "In db_update_job_start_record\n");
   len = sizeof(ojr);
   memcpy(&ojr, jr, len);

   if (!db_get_job_record(jcr, mdb, &ojr)) {
      db_unlock(mdb);
      return 0;
   }


   fseek(mdb->jobfd, ojr.rec_addr, SEEK_SET);
   if (fwrite(jr, len, 1, mdb->jobfd) != 1) {
      Mmsg1(mdb->errmsg, _("Error updating DB Job file. ERR=%s\n"), strerror(errno));
      stat = 0;
   }
   fflush(mdb->jobfd);

   db_unlock(mdb);
   return stat;
}

/*
 * This is called at Job termination time to add all the
 * other fields to the job record.
 */
int db_update_job_end_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   int len, stat = 1;
   JOB_DBR ojr;

   db_lock(mdb);

   Dmsg0(200, "In db_update_job_start_record\n");
   len = sizeof(ojr);
   memcpy(&ojr, jr, len);

   if (!db_get_job_record(jcr, mdb, &ojr)) {
      db_unlock(mdb);
      return 0;
   }

   fseek(mdb->jobfd, ojr.rec_addr, SEEK_SET);
   if (fwrite(jr, len, 1, mdb->jobfd) != 1) {
      Mmsg1(&mdb->errmsg, _("Error updating DB Job file. ERR=%s\n"), strerror(errno));
      stat = 0;
   }
   fflush(mdb->jobfd);

   db_unlock(mdb);
   return stat;
}


int db_update_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   int stat = 1;
   MEDIA_DBR omr;
   int len;

   db_lock(mdb);
   Dmsg0(200, "In db_update_media_record\n");
   mr->MediaId = 0;
   len = sizeof(omr);
   memcpy(&omr, mr, len);

   if (!db_get_media_record(jcr, mdb, &omr)) {
      db_unlock(mdb);
      return 0;
   }


   /* Don't allow some fields to change by copying from master record */
   strcpy(mr->VolumeName, omr.VolumeName);
   strcpy(mr->MediaType, omr.MediaType);
   mr->MediaId = omr.MediaId;
   mr->PoolId = omr.PoolId;
   mr->MaxVolBytes = omr.MaxVolBytes;
   mr->VolCapacityBytes = omr.VolCapacityBytes;
   mr->Recycle = omr.Recycle;

   fseek(mdb->mediafd, omr.rec_addr, SEEK_SET);
   if (fwrite(mr, len, 1, mdb->mediafd) != 1) {
      Mmsg1(mdb->errmsg, _("Error updating DB Media file. ERR=%s\n"), strerror(errno));
      stat = 0;
   }
   fflush(mdb->mediafd);

   db_unlock(mdb);
   return stat;
}

int db_update_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   int stat = 1;
   POOL_DBR opr;
   int len;

   db_lock(mdb);
   Dmsg0(200, "In db_update_pool_record\n");
   len = sizeof(opr);
   memcpy(&opr, pr, len);

   if (!db_get_pool_record(jcr, mdb, &opr)) {
      db_unlock(mdb);
      return 0;
   }


   /* Update specific fields */
   opr.NumVols = pr->NumVols;
   opr.MaxVols = pr->MaxVols;
   opr.UseOnce = pr->UseOnce;
   opr.UseCatalog = pr->UseCatalog;
   opr.AcceptAnyVolume = pr->AcceptAnyVolume;
   strcpy(opr.LabelFormat, pr->LabelFormat);

   fseek(mdb->poolfd, opr.rec_addr, SEEK_SET);
   if (fwrite(&opr, len, 1, mdb->poolfd) != 1) {
      Mmsg1(mdb->errmsg, _("Error updating DB Media file. ERR=%s\n"), strerror(errno));
      stat = 0;
   } else {
      memcpy(pr, &opr, len);          /* return record written */
   }
   fflush(mdb->poolfd);

   db_unlock(mdb);
   return stat;
}

int db_add_digest_to_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, char *digest, int type)
{
   return 1;
}

int db_mark_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, JobId_t JobId)
{
   return 1;
}

int db_update_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   return 1;
}

int db_update_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{
   return 0;
}

int db_update_media_defaults(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   return 1;
}

void db_make_inchanger_unique(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
  return;
}

#endif /* HAVE_BACULA_DB */
