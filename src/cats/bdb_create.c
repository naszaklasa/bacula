/*
 * Bacula Catalog Database Create record routines
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
 *    Version $Id: bdb_create.c,v 1.10 2005/01/29 22:38:57 kerns Exp $
 */

/*
   Copyright (C) 2001-2003 Kern Sibbald and John Walker

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


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C 		      /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"
#include "bdb.h"

#ifdef HAVE_BACULA_DB

/* Forward referenced functions */
bool db_create_pool_record(B_DB *mdb, POOL_DBR *pr);

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

int db_create_file_attributes_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   /* *****FIXME***** implement this */
   return 1;
}

int db_create_file_item(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   /****FIXME***** not implemented */
   return 1;
}


/*
 * Create a new record for the Job
 *   This record is created at the start of the Job,
 *   it is updated in bdb_update.c when the Job terminates.
 *
 * Returns: 0 on failure
 *	    1 on success
 */
int db_create_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   int len;

   db_lock(mdb);
   if (!bdb_open_jobs_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   mdb->control.JobId++;
   bdb_write_control_file(mdb);

   len = sizeof(JOB_DBR);
   jr->JobId = mdb->control.JobId;
   fseek(mdb->jobfd, 0L, SEEK_END);
   if (fwrite(jr, len, 1, mdb->jobfd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB Jobs file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->jobfd);
   db_unlock(mdb);
   return 1;
}

/* Create a JobMedia record for Volume used this job
 * Returns: 0 on failure
 *	    record-id on success
 */
bool db_create_jobmedia_record(JCR *jcr, B_DB *mdb, JOBMEDIA_DBR *jm)
{
   int len;

   db_lock(mdb);
   if (!bdb_open_jobmedia_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   mdb->control.JobMediaId++;
   jm->JobMediaId = mdb->control.JobMediaId;
   bdb_write_control_file(mdb);

   len = sizeof(JOBMEDIA_DBR);

   fseek(mdb->jobmediafd, 0L, SEEK_END);
   if (fwrite(jm, len, 1, mdb->jobmediafd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB JobMedia file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->jobmediafd);
   db_unlock(mdb);
   return jm->JobMediaId;
}


/*
 *  Create a unique Pool record
 * Returns: 0 on failure
 *	    1 on success
 */
bool db_create_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   int len;
   POOL_DBR mpr;

   memset(&mpr, 0, sizeof(mpr));
   strcpy(mpr.Name, pr->Name);
   if (db_get_pool_record(jcr, mdb, &mpr)) {
      Mmsg1(&mdb->errmsg, "Pool record %s already exists\n", mpr.Name);
      return 0;
   }

   db_lock(mdb);
   if (!bdb_open_pools_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }

   mdb->control.PoolId++;
   pr->PoolId = mdb->control.PoolId;
   bdb_write_control_file(mdb);

   len = sizeof(mpr);
   fseek(mdb->poolfd, 0L, SEEK_END);
   if (fwrite(pr, len, 1, mdb->poolfd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB Pools file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->poolfd);
   db_unlock(mdb);
   return 1;
}

bool db_create_device_record(JCR *jcr, B_DB *mdb, DEVICE_DBR *dr)
{ return false; }

bool db_create_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *dr)
{ return false; }

bool db_create_mediatype_record(JCR *jcr, B_DB *mdb, MEDIATYPE_DBR *dr)
{ return false; }


/*
 * Create Unique Media record.	This record
 *   contains all the data pertaining to a specific
 *   Volume.
 *
 * Returns: 0 on failure
 *	    1 on success
 */
int db_create_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   int len;
   MEDIA_DBR mmr;

   db_lock(mdb);
   memset(&mmr, 0, sizeof(mmr));
   strcpy(mmr.VolumeName, mr->VolumeName);
   if (db_get_media_record(jcr, mdb, &mmr)) {
      Mmsg1(&mdb->errmsg, "Media record %s already exists\n", mmr.VolumeName);
      db_unlock(mdb);
      return 0;
   }


   mdb->control.MediaId++;
   mr->MediaId = mdb->control.MediaId;
   bdb_write_control_file(mdb);

   len = sizeof(mmr);
   fseek(mdb->mediafd, 0L, SEEK_END);
   if (fwrite(mr, len, 1, mdb->mediafd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB Media file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->mediafd);
   db_unlock(mdb);
   return 1;
}


/*
 *  Create a unique Client record or return existing record
 * Returns: 0 on failure
 *	    1 on success
 */
int db_create_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   int len;
   CLIENT_DBR lcr;

   db_lock(mdb);
   cr->ClientId = 0;
   if (db_get_client_record(jcr, mdb, cr)) {
      Mmsg1(&mdb->errmsg, "Client record %s already exists\n", cr->Name);
      db_unlock(mdb);
      return 1;
   }

   if (!bdb_open_client_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }

   mdb->control.ClientId++;
   cr->ClientId = mdb->control.ClientId;
   bdb_write_control_file(mdb);

   fseek(mdb->clientfd, 0L, SEEK_END);
   len = sizeof(lcr);
   if (fwrite(cr, len, 1, mdb->clientfd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB Client file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->clientfd);
   db_unlock(mdb);
   return 1;
}

/*
 *  Create a unique FileSet record or return existing record
 *
 *   Note, here we write the FILESET_DBR structure
 *
 * Returns: 0 on failure
 *	    1 on success
 */
bool db_create_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   int len;
   FILESET_DBR lfsr;

   db_lock(mdb);
   fsr->FileSetId = 0;
   if (db_get_fileset_record(jcr, mdb, fsr)) {
      Mmsg1(&mdb->errmsg, "FileSet record %s already exists\n", fsr->FileSet);
      db_unlock(mdb);
      return 1;
   }

   if (!bdb_open_fileset_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }

   mdb->control.FileSetId++;
   fsr->FileSetId = mdb->control.FileSetId;
   bdb_write_control_file(mdb);

   fseek(mdb->clientfd, 0L, SEEK_END);
   len = sizeof(lfsr);
   if (fwrite(fsr,  len, 1, mdb->filesetfd) != 1) {
      Mmsg1(&mdb->errmsg, "Error writing DB FileSet file. ERR=%s\n", strerror(errno));
      db_unlock(mdb);
      return 0;
   }
   fflush(mdb->filesetfd);
   db_unlock(mdb);
   return 1;
}

int db_create_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{ return 0; }


#endif /* HAVE_BACULA_DB */
