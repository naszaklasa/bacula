/*
 * Bacula Catalog Database Get record interface routines
 *  Note, these routines generally get a record by id or
 *        by name.  If more logic is involved, the routine
 *        should be in find.c
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
 *    Version $Id: bdb_get.c 5142 2007-07-11 16:38:14Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

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


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"
#include "bdb.h"

#ifdef HAVE_BACULA_DB

/* Forward referenced functions */


/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


/*
 * Get Job record for given JobId
 * Returns: 0 on failure
 *          1 on success
 */

bool db_get_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   JOB_DBR ojr;
   faddr_t rec_addr;
   int found = 0;
   int stat = 0;
   int len;

   db_lock(mdb);
   if (jr->JobId == 0 && jr->Name[0] == 0) { /* he wants # of Job records */
      jr->JobId = mdb->control.JobId;
      db_unlock(mdb);
      return 1;
   }
   Dmsg0(200, "Open Jobs\n");
   if (!bdb_open_jobs_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->jobfd, 0L, SEEK_SET);   /* rewind file */
   rec_addr = 0;
   /* Linear search through Job records
    */
   len = sizeof(ojr);
   while (fread(&ojr, len, 1, mdb->jobfd) > 0) {
      /* If id not zero, search by Id */
      if (jr->JobId != 0) {
         if (jr->JobId == ojr.JobId) {
           found = 1;
         }
      /* Search by Job */
      } else if (strcmp(jr->Job, ojr.Job) == 0) {
         found = 1;
         Dmsg1(200, "Found Job: %s\n", ojr.Job);
      }
      if (!found) {
         rec_addr = ftell(mdb->jobfd); /* save start next record */
         continue;
      }
      /* Found desired record, now return it */
      memcpy(jr, &ojr, len);
      jr->rec_addr = rec_addr;
      stat = ojr.JobId;
      Dmsg2(200, "Found job record: JobId=%d Job=%s",
         ojr.JobId, ojr.Job);
      break;
   }
   if (!found) {
      strcpy(mdb->errmsg, "Job record not found.\n");
   }
   db_unlock(mdb);
   Dmsg1(200, "Return job stat=%d\n", stat);
   return stat;
}


/*
 * Get the number of pool records
 *
 * Returns: -1 on failure
 *          number on success
 */
int db_get_num_pool_records(JCR *jcr, B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   stat = mdb->control.PoolId;
   db_unlock(mdb);
   return stat;
}

/*
 * This function returns a list of all the Pool record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *          1: on success
 */
int db_get_pool_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   int i = 0;
   uint32_t *id;
   POOL_DBR opr;
   int len;

   db_lock(mdb);
   *ids = NULL;
   if (!bdb_open_pools_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->poolfd, 0L, SEEK_SET);   /* rewind file */
   /* Linear search through Pool records
    */
   len = sizeof(opr);
   *num_ids = mdb->control.PoolId;
   id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
   while (fread(&opr, len, 1, mdb->poolfd) > 0) {
      id[i++] = opr.PoolId;
   }
   *ids = id;
   db_unlock(mdb);
   return 1;
}


/*
 * Get Pool Record
 * If the PoolId is non-zero, we get its record,
 *  otherwise, we search on the PoolName
 *
 * Returns: false on failure
 *          true on success
 */
bool db_get_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   POOL_DBR opr;
   faddr_t rec_addr;
   bool found = false;
   int len;

   db_lock(mdb);
   Dmsg0(200, "Open pools\n");
   if (!bdb_open_pools_file(mdb)) {
      db_unlock(mdb);
      return false;
   }
   fseek(mdb->poolfd, 0L, SEEK_SET);   /* rewind file */
   rec_addr = 0;
   /* Linear search through Pool records
    */
   len = sizeof(opr);
   while (fread(&opr, len, 1, mdb->poolfd) > 0) {
      /* If id not zero, search by Id */
      if (pr->PoolId != 0) {
         if (pr->PoolId == opr.PoolId) {
           found = true;
         }
      /* Search by Name */
      } else if (strcmp(pr->Name, opr.Name) == 0) {
         found = true;
         Dmsg1(200, "Found pool: %s\n", opr.Name);
      }
      if (!found) {
         rec_addr = ftell(mdb->poolfd); /* save start next record */
         continue;
      }
      /* Found desired record, now return it */
      memcpy(pr, &opr, len);
      pr->rec_addr = rec_addr;
      Dmsg3(200, "Found pool record: PoolId=%d Name=%s PoolType=%s\n",
         opr.PoolId, opr.Name, opr.PoolType);
      break;
   }
   if (!found) {
      strcpy(mdb->errmsg, "Pool record not found.\n");
   }
   db_unlock(mdb);
   return found;
}

/*
 * Get the number of Media records
 *
 * Returns: -1 on failure
 *          number on success
 */
int db_get_num_media_records(JCR *jcr, B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   stat = mdb->control.MediaId;
   db_unlock(mdb);
   return stat;
}

/*
 * This function returns a list of all the Media record ids
 *  for a specified PoolId
 *  The caller must free ids if non-NULL.
 *
 *  Returns false: on failure
 *          true:  on success
 */
bool db_get_media_ids(JCR *jcr, B_DB *mdb, uint32_t PoolId, int *num_ids, uint32_t *ids[])
{
   int i = 0;
   uint32_t *id;
   MEDIA_DBR omr;
   int len;

   db_lock(mdb);
   *ids = NULL;
   if (!bdb_open_media_file(mdb)) {
      db_unlock(mdb);
      return false;
   }
   fseek(mdb->mediafd, 0L, SEEK_SET);   /* rewind file */
   /* Linear search through Pool records
    */
   len = sizeof(omr);
   if (mdb->control.MediaId == 0) {
      db_unlock(mdb);
      return false;
   }
   *num_ids = mdb->control.MediaId;
   id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
   while (fread(&omr, len, 1, mdb->mediafd) > 0) {
      if (PoolId == omr.MediaId) {
         id[i++] = omr.MediaId;
      }
   }
   *ids = id;
   db_unlock(mdb);
   return true;
}

/*
 * Get Media Record
 * If the MediaId is non-zero, we get its record,
 *  otherwise, we search on the MediaName
 *
 * Returns: false on failure
 *          true on success
 */
bool db_get_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   faddr_t rec_addr;
   bool found = false;
   int len;
   MEDIA_DBR omr;

   db_lock(mdb);
   if (!bdb_open_media_file(mdb)) {
      goto get_out;
   }
   fseek(mdb->mediafd, 0L, SEEK_SET);   /* rewind file */
   rec_addr = 0;
   /* Linear search through Media records
    */
   len = sizeof(omr);
   while (fread(&omr, len, 1, mdb->mediafd) > 0) {
      if (omr.MediaId == 0) {
         continue;                    /* deleted record */
      }
      Dmsg1(200, "VolName=%s\n", omr.VolumeName);
      /* If id not zero, search by Id */
      if (mr->MediaId != 0) {
         Dmsg1(200, "MediaId=%d\n", mr->MediaId);
         if (mr->MediaId == omr.MediaId) {
           found = true;
         }
      /* Search by Name */
      } else if (strcmp(mr->VolumeName, omr.VolumeName) == 0) {
         found = true;
      }
      if (!found) {
         rec_addr = ftell(mdb->mediafd); /* save start next record */
         continue;
      }
      /* Found desired record, now return it */
      memcpy(mr, &omr, len);
      mr->rec_addr = rec_addr;
      Dmsg3(200, "Found media record: MediaId=%d Name=%s MediaType=%s\n",
         omr.MediaId, omr.VolumeName, mr->MediaType);
      break;
   }
   if (!found) {
      strcpy(mdb->errmsg, "Could not find requested Media record.\n");
   }
get_out:
   db_unlock(mdb);
   return found;
}

/*
 * Find VolumeNames for a give JobId
 *  Returns: 0 on error or no Volumes found
 *           number of volumes on success
 *              Volumes are concatenated in VolumeNames
 *              separated by a vertical bar (|).
 */
int db_get_job_volume_names(JCR *jcr, B_DB *mdb, uint32_t JobId, POOLMEM **VolumeNames)
{
   int found = 0;
   JOBMEDIA_DBR jm;
   MEDIA_DBR mr;
   int jmlen, mrlen;

   db_lock(mdb);
   if (!bdb_open_jobmedia_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   if (!bdb_open_media_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   jmlen = sizeof(jm);
   mrlen = sizeof(mr);
   *VolumeNames[0] = 0;
   fseek(mdb->jobmediafd, 0L, SEEK_SET); /* rewind the file */
   while (fread(&jm, jmlen, 1, mdb->jobmediafd) > 0) {
      if (jm.JobId == JobId) {
         /* Now look up VolumeName in Media file given MediaId */
         fseek(mdb->mediafd, 0L, SEEK_SET);
         while (fread(&mr, mrlen, 1, mdb->mediafd) > 0) {
            if (jm.MediaId == mr.MediaId) {
               if (*VolumeNames[0] != 0) {      /* if not first name, */
                  pm_strcat(VolumeNames, "|");  /* add separator */
               }
               pm_strcat(VolumeNames, mr.VolumeName); /* add Volume Name */
               found++;
            }
         }
      }
   }
   if (!found) {
      strcpy(mdb->errmsg, "No Volumes found.\n");
   }
   db_unlock(mdb);
   return found;
}

/*
 * Get Client Record
 * If the ClientId is non-zero, we get its record,
 *  otherwise, we search on the Name
 *
 * Returns: 0 on failure
 *          id on success
 */
int db_get_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   CLIENT_DBR lcr;
   int len;
   int stat = 0;

   db_lock(mdb);
   if (!bdb_open_client_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->clientfd, 0L, SEEK_SET);   /* rewind file */
   /*
    * Linear search through Client records
    */
   len = sizeof(lcr);
   while (fread(&lcr, len, 1, mdb->clientfd)) {
      /* If id not zero, search by Id */
      if (cr->ClientId != 0) {
         if (cr->ClientId != lcr.ClientId) {
            continue;
         }
      /* Search by Name */
      } else if (strcmp(cr->Name, lcr.Name) != 0) {
         continue;                 /* not found */
      }
      memcpy(cr, &lcr, len);
      stat = lcr.ClientId;
      Dmsg2(200, "Found Client record: ClientId=%d Name=%s\n",
            lcr.ClientId, lcr.Name);
      break;
   }
   if (!stat) {
      strcpy(mdb->errmsg, "Client record not found.\n");
   }
   db_unlock(mdb);
   return stat;
}

/*
 * Get FileSet Record   (We read the FILESET_DBR structure)
 * If the FileSetId is non-zero, we get its record,
 *  otherwise, we search on the FileSet (its name).
 *
 * Returns: 0 on failure
 *          id on success
 */
int db_get_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   FILESET_DBR lfsr;
   int stat = 0;

   db_lock(mdb);
   if (!bdb_open_fileset_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->filesetfd, 0L, SEEK_SET);   /* rewind file */
   /*
    * Linear search through FileSet records
    */
   while (fread(&lfsr, sizeof(lfsr), 1, mdb->filesetfd) > 0) {
      /* If id not zero, search by Id */
      if (fsr->FileSetId != 0) {
         if (fsr->FileSetId != lfsr.FileSetId) {
            continue;
         }
      /* Search by Name & MD5 */
      } else if (strcmp(fsr->FileSet, lfsr.FileSet) != 0 ||
                 strcmp(fsr->MD5, lfsr.MD5) != 0) {
         continue;                 /* not found */
      }
      /* Found desired record, now return it */
      memcpy(fsr, &lfsr, sizeof(lfsr));
      stat = fsr->FileSetId;
      Dmsg2(200, "Found FileSet record: FileSetId=%d FileSet=%s\n",
            lfsr.FileSetId, lfsr.FileSet);
      break;
   }
   if (!stat) {
      strcpy(mdb->errmsg, "FileSet record not found.\n");
   }
   db_unlock(mdb);
   return stat;
}

bool db_get_query_dbids(JCR *jcr, B_DB *mdb, POOL_MEM &query, dbid_list &ids)
{ return false; }

int db_get_file_attributes_record(JCR *jcr, B_DB *mdb, char *fname, JOB_DBR *jr, FILE_DBR *fdbr)
{ return 0; }

int db_get_job_volume_parameters(JCR *jcr, B_DB *mdb, uint32_t JobId, VOL_PARAMS **VolParams)
{ return 0; }

int db_get_client_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{ return 0; }

int db_get_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{ return 0; }


#endif /* HAVE_BACULA_DB */
