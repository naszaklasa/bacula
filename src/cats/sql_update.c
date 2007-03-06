/*
 * Bacula Catalog Database Update record interface routines
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id: sql_update.c,v 1.63.2.6 2006/04/11 08:05:18 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#if    HAVE_SQLITE3 || HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL

/* -----------------------------------------------------------------------
 *
 *   Generic Routines (or almost generic)
 *
 * -----------------------------------------------------------------------
 */

/* Imported subroutines */
extern void print_result(B_DB *mdb);
extern int UpdateDB(const char *file, int line, JCR *jcr, B_DB *db, char *update_cmd);

/* -----------------------------------------------------------------------
 *
 *   Generic Routines (or almost generic)
 *
 * -----------------------------------------------------------------------
 */
/* Update the attributes record by adding the file digest */
int
db_add_SIG_to_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, char *digest,
                          int type)
{
   int stat;
   char ed1[50];

   db_lock(mdb);
   Mmsg(mdb->cmd, "UPDATE File SET MD5='%s' WHERE FileId=%s", digest, 
      edit_int64(FileId, ed1));
   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}

/* Mark the file record as being visited during database
 * verify compare. Stuff JobId into MarkId field
 */
int db_mark_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, JobId_t JobId)
{
   int stat;
   char ed1[50], ed2[50];

   db_lock(mdb);
   Mmsg(mdb->cmd, "UPDATE File SET MarkId=%s WHERE FileId=%s", 
      edit_int64(JobId, ed1), edit_int64(FileId, ed2));
   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}

/*
 * Update the Job record at start of Job
 *
 *  Returns: false on failure
 *           true  on success
 */
bool
db_update_job_start_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   char dt[MAX_TIME_LENGTH];
   time_t stime;
   struct tm tm;
   btime_t JobTDate;
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50];

   stime = jr->StartTime;
   localtime_r(&stime, &tm);
   strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
   JobTDate = (btime_t)stime;

   db_lock(mdb);
   Mmsg(mdb->cmd, "UPDATE Job SET JobStatus='%c',Level='%c',StartTime='%s',"
"ClientId=%s,JobTDate=%s,PoolId=%s WHERE JobId=%s",
      (char)(jcr->JobStatus),
      (char)(jr->JobLevel), dt, 
      edit_int64(jr->ClientId, ed1),
      edit_uint64(JobTDate, ed2), 
      edit_int64(jr->PoolId, ed3),
      edit_int64(jr->JobId, ed4));

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   mdb->changes = 0;
   db_unlock(mdb);
   return stat;
}

/*
 * Given an incoming integer, set the string buffer to either NULL or the value
 *
 */
static void edit_num_or_null(char *s, size_t n, uint64_t id) {
   char ed1[50];
   bsnprintf(s, n, id ? "%s" : "NULL", edit_int64(id, ed1));
}


/*
 * Update the Job record at end of Job
 *
 *  Returns: 0 on failure
 *           1 on success
 */
int
db_update_job_end_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   char dt[MAX_TIME_LENGTH];
   time_t ttime;
   struct tm tm;
   int stat;
   char ed1[30], ed2[30], ed3[50];
   btime_t JobTDate;
   char PoolId    [50];
   char FileSetId [50];
   char ClientId  [50];


   /* some values are set to zero, which translates to NULL in SQL */
   edit_num_or_null(PoolId,    sizeof(PoolId),    jr->PoolId);
   edit_num_or_null(FileSetId, sizeof(FileSetId), jr->FileSetId);
   edit_num_or_null(ClientId,  sizeof(ClientId),  jr->ClientId);

   ttime = jr->EndTime;
   localtime_r(&ttime, &tm);
   strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
   JobTDate = ttime;

   db_lock(mdb);
   Mmsg(mdb->cmd,
      "UPDATE Job SET JobStatus='%c', EndTime='%s', "
"ClientId=%s, JobBytes=%s, JobFiles=%u, JobErrors=%u, VolSessionId=%u, "
"VolSessionTime=%u, PoolId=%s, FileSetId=%s, JobTDate=%s WHERE JobId=%s",
      (char)(jr->JobStatus), dt, ClientId, edit_uint64(jr->JobBytes, ed1),
      jr->JobFiles, jr->JobErrors, jr->VolSessionId, jr->VolSessionTime,
      PoolId, FileSetId, edit_uint64(JobTDate, ed2), 
      edit_int64(jr->JobId, ed3));

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}


/*
 * Update Client record
 *   Returns: 0 on failure
 *            1 on success
 */
int
db_update_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   int stat;
   char ed1[50], ed2[50];
   CLIENT_DBR tcr;

   db_lock(mdb);
   memcpy(&tcr, cr, sizeof(tcr));
   if (!db_create_client_record(jcr, mdb, &tcr)) {
      db_unlock(mdb);
      return 0;
   }

   Mmsg(mdb->cmd,
"UPDATE Client SET AutoPrune=%d,FileRetention=%s,JobRetention=%s,"
"Uname='%s' WHERE Name='%s'",
      cr->AutoPrune,
      edit_uint64(cr->FileRetention, ed1),
      edit_uint64(cr->JobRetention, ed2),
      cr->Uname, cr->Name);

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}


/*
 * Update Counters record
 *   Returns: 0 on failure
 *            1 on success
 */
int db_update_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{
   db_lock(mdb);

   Mmsg(mdb->cmd,
"UPDATE Counters SET MinValue=%d,MaxValue=%d,CurrentValue=%d,"
"WrapCounter='%s' WHERE Counter='%s'",
      cr->MinValue, cr->MaxValue, cr->CurrentValue,
      cr->WrapCounter, cr->Counter);

   int stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}


int
db_update_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50];

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT count(*) from Media WHERE PoolId=%s",
      edit_int64(pr->PoolId, ed4));
   pr->NumVols = get_sql_record_max(jcr, mdb);
   Dmsg1(400, "NumVols=%d\n", pr->NumVols);

   Mmsg(mdb->cmd,
"UPDATE Pool SET NumVols=%u,MaxVols=%u,UseOnce=%d,UseCatalog=%d,"
"AcceptAnyVolume=%d,VolRetention='%s',VolUseDuration='%s',"
"MaxVolJobs=%u,MaxVolFiles=%u,MaxVolBytes=%s,Recycle=%d,"
"AutoPrune=%d,LabelType=%d,LabelFormat='%s' WHERE PoolId=%s",
      pr->NumVols, pr->MaxVols, pr->UseOnce, pr->UseCatalog,
      pr->AcceptAnyVolume, edit_uint64(pr->VolRetention, ed1),
      edit_uint64(pr->VolUseDuration, ed2),
      pr->MaxVolJobs, pr->MaxVolFiles,
      edit_uint64(pr->MaxVolBytes, ed3),
      pr->Recycle, pr->AutoPrune, pr->LabelType,
      pr->LabelFormat, 
      ed4);

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}

bool
db_update_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *sr)
{
   int stat;
   char ed1[50];

   db_lock(mdb);
   Mmsg(mdb->cmd, "UPDATE Storage SET AutoChanger=%d WHERE StorageId=%s", 
      sr->AutoChanger, edit_int64(sr->StorageId, ed1));

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);
   db_unlock(mdb);
   return stat;
}


/*
 * Update the Media Record at end of Session
 *
 * Returns: 0 on failure
 *          numrows on success
 */
int
db_update_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   char dt[MAX_TIME_LENGTH];
   time_t ttime;
   struct tm tm;
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50]; 
   char ed5[50], ed6[50], ed7[50], ed8[50];


   Dmsg1(100, "update_media: FirstWritten=%d\n", mr->FirstWritten);
   db_lock(mdb);
   if (mr->set_first_written) {
      Dmsg1(400, "Set FirstWritten Vol=%s\n", mr->VolumeName);
      ttime = mr->FirstWritten;
      localtime_r(&ttime, &tm);
      strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
      Mmsg(mdb->cmd, "UPDATE Media SET FirstWritten='%s'"
           " WHERE VolumeName='%s'", dt, mr->VolumeName);
      stat = UPDATE_DB(jcr, mdb, mdb->cmd);
      Dmsg1(400, "Firstwritten=%d\n", mr->FirstWritten);
   }

   /* Label just done? */
   if (mr->set_label_date) {
      ttime = mr->LabelDate;
      if (ttime == 0) {
         ttime = time(NULL);
      }
      localtime_r(&ttime, &tm);
      strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
      Mmsg(mdb->cmd, "UPDATE Media SET LabelDate='%s' "
           "WHERE VolumeName='%s'", dt, mr->VolumeName);
      UPDATE_DB(jcr, mdb, mdb->cmd);
   }

   if (mr->LastWritten != 0) {
      ttime = mr->LastWritten;
      localtime_r(&ttime, &tm);
      strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
      Mmsg(mdb->cmd, "UPDATE Media Set LastWritten='%s' "
           "WHERE VolumeName='%s'", dt, mr->VolumeName);
      UPDATE_DB(jcr, mdb, mdb->cmd);
   }

   Mmsg(mdb->cmd, "UPDATE Media SET VolJobs=%u,"
        "VolFiles=%u,VolBlocks=%u,VolBytes=%s,VolMounts=%u,VolErrors=%u,"
        "VolWrites=%u,MaxVolBytes=%s,VolStatus='%s',"
        "Slot=%d,InChanger=%d,VolReadTime=%s,VolWriteTime=%s,VolParts=%d,"
        "LabelType=%d,StorageId=%s,PoolId=%s,VolRetention=%s,VolUseDuration=%s,"
        "MaxVolJobs=%d,MaxVolFiles=%d"
        " WHERE VolumeName='%s'",
        mr->VolJobs, mr->VolFiles, mr->VolBlocks, edit_uint64(mr->VolBytes, ed1),
        mr->VolMounts, mr->VolErrors, mr->VolWrites,
        edit_uint64(mr->MaxVolBytes, ed2),
        mr->VolStatus, mr->Slot, mr->InChanger,
        edit_uint64(mr->VolReadTime, ed3),
        edit_uint64(mr->VolWriteTime, ed4),
        mr->VolParts,
        mr->LabelType,
        edit_int64(mr->StorageId, ed5),
        edit_int64(mr->PoolId, ed6),
        edit_uint64(mr->VolRetention, ed7),
        edit_uint64(mr->VolUseDuration, ed8),
        mr->MaxVolJobs, mr->MaxVolFiles,
        mr->VolumeName);

   Dmsg1(400, "%s\n", mdb->cmd);

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);

   /* Make sure InChanger is 0 for any record having the same Slot */
   db_make_inchanger_unique(jcr, mdb, mr);

   db_unlock(mdb);
   return stat;
}

/*
 * Update the Media Record Default values from Pool
 *
 * Returns: 0 on failure
 *          numrows on success
 */
int
db_update_media_defaults(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50];


   db_lock(mdb);
   if (mr->VolumeName[0]) {
      Mmsg(mdb->cmd, "UPDATE Media SET "
           "Recycle=%d,VolRetention=%s,VolUseDuration=%s,"
           "MaxVolJobs=%u,MaxVolFiles=%u,MaxVolBytes=%s"
           " WHERE VolumeName='%s'",
           mr->Recycle,edit_uint64(mr->VolRetention, ed1),
           edit_uint64(mr->VolUseDuration, ed2),
           mr->MaxVolJobs, mr->MaxVolFiles,
           edit_uint64(mr->MaxVolBytes, ed3),
           mr->VolumeName);
   } else {
      Mmsg(mdb->cmd, "UPDATE Media SET "
           "Recycle=%d,VolRetention=%s,VolUseDuration=%s,"
           "MaxVolJobs=%u,MaxVolFiles=%u,MaxVolBytes=%s"
           " WHERE PoolId=%s",
           mr->Recycle,edit_uint64(mr->VolRetention, ed1),
           edit_uint64(mr->VolUseDuration, ed2),
           mr->MaxVolJobs, mr->MaxVolFiles,
           edit_uint64(mr->MaxVolBytes, ed3),
           edit_int64(mr->PoolId, ed4));
   }

   Dmsg1(400, "%s\n", mdb->cmd);

   stat = UPDATE_DB(jcr, mdb, mdb->cmd);

   db_unlock(mdb);
   return stat;
}


/*
 * If we have a non-zero InChanger, ensure that no other Media
 *  record has InChanger set on the same Slot.
 *
 * This routine assumes the database is already locked.
 */
void
db_make_inchanger_unique(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   char ed1[50], ed2[50];
   if (mr->InChanger != 0 && mr->Slot != 0) {
      Mmsg(mdb->cmd, "UPDATE Media SET InChanger=0 WHERE "
           "Slot=%d AND StorageId=%s AND MediaId!=%s",
            mr->Slot, 
            edit_int64(mr->StorageId, ed1), edit_int64(mr->MediaId, ed2));
      Dmsg1(400, "%s\n", mdb->cmd);
      UPDATE_DB(jcr, mdb, mdb->cmd);
   }
}

#else

void
db_make_inchanger_unique(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
  /* DUMMY func for Bacula_DB */
  return;
}

#endif /* HAVE_SQLITE3 || HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL*/
