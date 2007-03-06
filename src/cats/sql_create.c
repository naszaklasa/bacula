/*
 * Bacula Catalog Database Create record interface routines
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id: sql_create.c,v 1.80 2005/09/15 16:28:21 kerns Exp $
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

/* Forward referenced subroutines */
static int db_create_file_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);
static int db_create_filename_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);
static int db_create_path_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);


/* Imported subroutines */
extern void print_dashes(B_DB *mdb);
extern void print_result(B_DB *mdb);
extern int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);
extern int InsertDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);
extern int UpdateDB(const char *file, int line, JCR *jcr, B_DB *db, char *update_cmd);
extern void split_path_and_file(JCR *jcr, B_DB *mdb, const char *fname);


/* Create a new record for the Job
 * Returns: 0 on failure
 *          1 on success
 */
int
db_create_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   char dt[MAX_TIME_LENGTH];
   time_t stime;
   struct tm tm;
   int stat;
   utime_t JobTDate;
   char ed1[30];

   db_lock(mdb);

   stime = jr->SchedTime;
   ASSERT(stime != 0);

   localtime_r(&stime, &tm);
   strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
   JobTDate = (utime_t)stime;

   /* Must create it */
   Mmsg(mdb->cmd,
"INSERT INTO Job (Job,Name,Type,Level,JobStatus,SchedTime,JobTDate) VALUES "
"('%s','%s','%c','%c','%c','%s',%s)",
           jr->Job, jr->Name, (char)(jr->JobType), (char)(jr->JobLevel),
           (char)(jr->JobStatus), dt, edit_uint64(JobTDate, ed1));

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB Job record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      jr->JobId = 0;
      stat = 0;
   } else {
      jr->JobId = sql_insert_id(mdb, N_("Job"));
      stat = 1;
   }
   db_unlock(mdb);
   return stat;
}

/* Create a JobMedia record for medium used this job
 * Returns: false on failure
 *          true  on success
 */
bool
db_create_jobmedia_record(JCR *jcr, B_DB *mdb, JOBMEDIA_DBR *jm)
{
   bool ok = true;;
   int count;
   char ed1[50], ed2[50];

   db_lock(mdb);

   /* Now get count for VolIndex */
   Mmsg(mdb->cmd, "SELECT count(*) from JobMedia WHERE JobId=%s",
        edit_int64(jm->JobId, ed1));
   count = get_sql_record_max(jcr, mdb);
   if (count < 0) {
      count = 0;
   }
   count++;

   Mmsg(mdb->cmd,
        "INSERT INTO JobMedia (JobId,MediaId,FirstIndex,LastIndex,"
        "StartFile,EndFile,StartBlock,EndBlock,VolIndex,Copy,Stripe) "
        "VALUES (%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u)",
        edit_int64(jm->JobId, ed1),
        edit_int64(jm->MediaId, ed2),
        jm->FirstIndex, jm->LastIndex,
        jm->StartFile, jm->EndFile, jm->StartBlock, jm->EndBlock,count,
        jm->Copy, jm->Stripe);

   Dmsg0(300, mdb->cmd);
   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create JobMedia record %s failed: ERR=%s\n"), mdb->cmd,
         sql_strerror(mdb));
      ok = false;
   } else {
      /* Worked, now update the Media record with the EndFile and EndBlock */
      Mmsg(mdb->cmd,
           "UPDATE Media SET EndFile=%u, EndBlock=%u WHERE MediaId=%u",
           jm->EndFile, jm->EndBlock, jm->MediaId);
      if (!UPDATE_DB(jcr, mdb, mdb->cmd)) {
         Mmsg2(&mdb->errmsg, _("Update Media record %s failed: ERR=%s\n"), mdb->cmd,
              sql_strerror(mdb));
         ok = false;
      }
   }
   db_unlock(mdb);
   Dmsg0(300, "Return from JobMedia\n");
   return ok;
}



/* Create Unique Pool record
 * Returns: false on failure
 *          true  on success
 */
bool
db_create_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   bool stat;        
   char ed1[30], ed2[30], ed3[50];

   Dmsg0(200, "In create pool\n");
   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT PoolId,Name FROM Pool WHERE Name='%s'", pr->Name);
   Dmsg1(200, "selectpool: %s\n", mdb->cmd);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 0) {
         Mmsg1(&mdb->errmsg, _("pool record %s already exists\n"), pr->Name);
         sql_free_result(mdb);
         db_unlock(mdb);
         return false;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd,
"INSERT INTO Pool (Name,NumVols,MaxVols,UseOnce,UseCatalog,"
"AcceptAnyVolume,AutoPrune,Recycle,VolRetention,VolUseDuration,"
"MaxVolJobs,MaxVolFiles,MaxVolBytes,PoolType,LabelType,LabelFormat) "
"VALUES ('%s',%u,%u,%d,%d,%d,%d,%d,%s,%s,%u,%u,%s,'%s',%d,'%s')",
                  pr->Name,
                  pr->NumVols, pr->MaxVols,
                  pr->UseOnce, pr->UseCatalog,
                  pr->AcceptAnyVolume,
                  pr->AutoPrune, pr->Recycle,
                  edit_uint64(pr->VolRetention, ed1),
                  edit_uint64(pr->VolUseDuration, ed2),
                  pr->MaxVolJobs, pr->MaxVolFiles,
                  edit_uint64(pr->MaxVolBytes, ed3),
                  pr->PoolType, pr->LabelType, pr->LabelFormat);
   Dmsg1(200, "Create Pool: %s\n", mdb->cmd);
   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db Pool record %s failed: ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      pr->PoolId = 0;
      stat = false;
   } else {
      pr->PoolId = sql_insert_id(mdb, N_("Pool"));
      stat = true;
   }
   db_unlock(mdb);
   return stat;
}

/*
 * Create Unique Device record
 * Returns: false on failure
 *          true  on success
 */
bool
db_create_device_record(JCR *jcr, B_DB *mdb, DEVICE_DBR *dr)
{
   bool ok;
   char ed1[30], ed2[30];

   Dmsg0(200, "In create Device\n");
   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT DeviceId,Name FROM Device WHERE Name='%s'", dr->Name);
   Dmsg1(200, "selectdevice: %s\n", mdb->cmd);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 0) {
         Mmsg1(&mdb->errmsg, _("Device record %s already exists\n"), dr->Name);
         sql_free_result(mdb);
         db_unlock(mdb);
         return false;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd,
"INSERT INTO Device (Name,MediaTypeId,StorageId) VALUES ('%s',%s,%s)",
                  dr->Name,
                  edit_uint64(dr->MediaTypeId, ed1),
                  edit_int64(dr->StorageId, ed2));
   Dmsg1(200, "Create Device: %s\n", mdb->cmd);
   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db Device record %s failed: ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      dr->DeviceId = 0;
      ok = false;
   } else {
      dr->DeviceId = sql_insert_id(mdb, N_("Device"));
      ok = true;
   }
   db_unlock(mdb);
   return ok;
}



/*
 * Create a Unique record for Storage -- no duplicates
 * Returns: false on failure
 *          true  on success with id in sr->StorageId
 */
bool db_create_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *sr)
{
   SQL_ROW row;
   bool ok;

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT StorageId,AutoChanger FROM Storage WHERE Name='%s'", sr->Name);

   sr->StorageId = 0;
   sr->created = false;
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      /* If more than one, report error, but return first row */
      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Storage record!: %d\n"), (int)(mdb->num_rows));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      }
      if (mdb->num_rows >= 1) {
         if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching Storage row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
            sql_free_result(mdb);
            db_unlock(mdb);
            return false;
         }
         sr->StorageId = str_to_int64(row[0]);
         sr->AutoChanger = atoi(row[1]);   /* bool */
         sql_free_result(mdb);
         db_unlock(mdb);
         return true;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd, "INSERT INTO Storage (Name,AutoChanger)"
        " VALUES ('%s',%d)", sr->Name, sr->AutoChanger);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB Storage record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      ok = false;
   } else {
      sr->StorageId = sql_insert_id(mdb, N_("Storage"));
      sr->created = true;
      ok = true;
   }
   db_unlock(mdb);
   return ok;
}


/*
 * Create Unique MediaType record
 * Returns: false on failure
 *          true  on success
 */
bool
db_create_mediatype_record(JCR *jcr, B_DB *mdb, MEDIATYPE_DBR *mr)
{
   bool stat;        

   Dmsg0(200, "In create mediatype\n");
   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT MediaTypeId,MediaType FROM MediaType WHERE MediaType='%s'", mr->MediaType);
   Dmsg1(200, "selectmediatype: %s\n", mdb->cmd);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 0) {
         Mmsg1(&mdb->errmsg, _("mediatype record %s already exists\n"), mr->MediaType);
         sql_free_result(mdb);
         db_unlock(mdb);
         return false;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd,
"INSERT INTO MediaType (MediaType,ReadOnly) "
"VALUES ('%s',%d)",
                  mr->MediaType,
                  mr->ReadOnly);
   Dmsg1(200, "Create mediatype: %s\n", mdb->cmd);
   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db mediatype record %s failed: ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      mr->MediaTypeId = 0;
      stat = false;
   } else {
      mr->MediaTypeId = sql_insert_id(mdb, N_("MediaType"));
      stat = true;
   }
   db_unlock(mdb);
   return stat;
}


/*
 * Create Media record. VolumeName and non-zero Slot must be unique
 *
 * Returns: 0 on failure
 *          1 on success
 */
int
db_create_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   int stat;
   char ed1[50], ed2[50], ed3[50], ed4[50], ed5[50], ed6[50], ed7[50], ed8[50];
   struct tm tm;

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT MediaId FROM Media WHERE VolumeName='%s'",
           mr->VolumeName);
   Dmsg1(500, "selectpool: %s\n", mdb->cmd);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 0) {
         Mmsg1(&mdb->errmsg, _("Volume \"%s\" already exists.\n"), mr->VolumeName);
         sql_free_result(mdb);
         db_unlock(mdb);
         return 0;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd,
"INSERT INTO Media (VolumeName,MediaType,PoolId,MaxVolBytes,VolCapacityBytes,"
"Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,"
"VolStatus,Slot,VolBytes,InChanger,VolReadTime,VolWriteTime,VolParts,"
"EndFile,EndBlock,LabelType,StorageId) "
"VALUES ('%s','%s',%u,%s,%s,%d,%s,%s,%u,%u,'%s',%d,%s,%d,%s,%s,%d,0,0,%d,%s)",
          mr->VolumeName,
          mr->MediaType, mr->PoolId,
          edit_uint64(mr->MaxVolBytes,ed1),
          edit_uint64(mr->VolCapacityBytes, ed2),
          mr->Recycle,
          edit_uint64(mr->VolRetention, ed3),
          edit_uint64(mr->VolUseDuration, ed4),
          mr->MaxVolJobs,
          mr->MaxVolFiles,
          mr->VolStatus,
          mr->Slot,
          edit_uint64(mr->VolBytes, ed5),
          mr->InChanger,
          edit_uint64(mr->VolReadTime, ed6),
          edit_uint64(mr->VolWriteTime, ed7),
          mr->VolParts,
          mr->LabelType,
          edit_int64(mr->StorageId, ed8) 
          );


   Dmsg1(500, "Create Volume: %s\n", mdb->cmd);
   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB Media record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      stat = 0;
   } else {
      mr->MediaId = sql_insert_id(mdb, N_("Media"));
      stat = 1;
      if (mr->set_label_date) {
         char dt[MAX_TIME_LENGTH];
         if (mr->LabelDate == 0) {
            mr->LabelDate = time(NULL);
         }
         localtime_r(&mr->LabelDate, &tm);
         strftime(dt, sizeof(dt), "%Y-%m-%d %T", &tm);
         Mmsg(mdb->cmd, "UPDATE Media SET LabelDate='%s' "
              "WHERE MediaId=%d", dt, mr->MediaId);
         stat = UPDATE_DB(jcr, mdb, mdb->cmd);
      }
   }

   /*
    * Make sure that if InChanger is non-zero any other identical slot
    *   has InChanger zero.
    */
   db_make_inchanger_unique(jcr, mdb, mr);

   db_unlock(mdb);
   return stat;
}

/*
 * Create a Unique record for the client -- no duplicates
 * Returns: 0 on failure
 *          1 on success with id in cr->ClientId
 */
int db_create_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   SQL_ROW row;
   int stat;
   char ed1[50], ed2[50];

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT ClientId,Uname FROM Client WHERE Name='%s'", cr->Name);

   cr->ClientId = 0;
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      /* If more than one, report error, but return first row */
      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Client!: %d\n"), (int)(mdb->num_rows));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      }
      if (mdb->num_rows >= 1) {
         if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching Client row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
            sql_free_result(mdb);
            db_unlock(mdb);
            return 0;
         }
         cr->ClientId = str_to_int64(row[0]);
         if (row[1]) {
            bstrncpy(cr->Uname, row[1], sizeof(cr->Uname));
         } else {
            cr->Uname[0] = 0;         /* no name */
         }
         sql_free_result(mdb);
         db_unlock(mdb);
         return 1;
      }
      sql_free_result(mdb);
   }

   /* Must create it */
   Mmsg(mdb->cmd, "INSERT INTO Client (Name,Uname,AutoPrune,"
"FileRetention,JobRetention) VALUES "
"('%s','%s',%d,%s,%s)", cr->Name, cr->Uname, cr->AutoPrune,
      edit_uint64(cr->FileRetention, ed1),
      edit_uint64(cr->JobRetention, ed2));

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB Client record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      cr->ClientId = 0;
      stat = 0;
   } else {
      cr->ClientId = sql_insert_id(mdb, N_("Client"));
      stat = 1;
   }
   db_unlock(mdb);
   return stat;
}





/*
 * Create a Unique record for the counter -- no duplicates
 * Returns: 0 on failure
 *          1 on success with counter filled in
 */
int db_create_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{
   COUNTER_DBR mcr;
   int stat;

   db_lock(mdb);
   memset(&mcr, 0, sizeof(mcr));
   bstrncpy(mcr.Counter, cr->Counter, sizeof(mcr.Counter));
   if (db_get_counter_record(jcr, mdb, &mcr)) {
      memcpy(cr, &mcr, sizeof(COUNTER_DBR));
      db_unlock(mdb);
      return 1;
   }

   /* Must create it */
   Mmsg(mdb->cmd, "INSERT INTO Counters (Counter,MinValue,MaxValue,CurrentValue,"
      "WrapCounter) VALUES ('%s','%d','%d','%d','%s')",
      cr->Counter, cr->MinValue, cr->MaxValue, cr->CurrentValue,
      cr->WrapCounter);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB Counters record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   } else {
      stat = 1;
   }
   db_unlock(mdb);
   return stat;
}


/*
 * Create a FileSet record. This record is unique in the
 *  name and the MD5 signature of the include/exclude sets.
 *  Returns: 0 on failure
 *           1 on success with FileSetId in record
 */
bool db_create_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   SQL_ROW row;
   bool stat;
   struct tm tm;

   db_lock(mdb);
   fsr->created = false;
   Mmsg(mdb->cmd, "SELECT FileSetId,CreateTime FROM FileSet WHERE "
"FileSet='%s' AND MD5='%s'", fsr->FileSet, fsr->MD5);

   fsr->FileSetId = 0;
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one FileSet!: %d\n"), (int)(mdb->num_rows));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      }
      if (mdb->num_rows >= 1) {
         if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching FileSet row: ERR=%s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
            sql_free_result(mdb);
            db_unlock(mdb);
            return false;
         }
         fsr->FileSetId = str_to_int64(row[0]);
         if (row[1] == NULL) {
            fsr->cCreateTime[0] = 0;
         } else {
            bstrncpy(fsr->cCreateTime, row[1], sizeof(fsr->cCreateTime));
         }
         sql_free_result(mdb);
         db_unlock(mdb);
         return true;
      }
      sql_free_result(mdb);
   }

   if (fsr->CreateTime == 0 && fsr->cCreateTime[0] == 0) {
      fsr->CreateTime = time(NULL);
   }
   localtime_r(&fsr->CreateTime, &tm);
   strftime(fsr->cCreateTime, sizeof(fsr->cCreateTime), "%Y-%m-%d %T", &tm);

   /* Must create it */
      Mmsg(mdb->cmd, "INSERT INTO FileSet (FileSet,MD5,CreateTime) "
"VALUES ('%s','%s','%s')", fsr->FileSet, fsr->MD5, fsr->cCreateTime);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create DB FileSet record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      fsr->FileSetId = 0;
      stat = false;
   } else {
      fsr->FileSetId = sql_insert_id(mdb, N_("FileSet"));
      fsr->created = true;
      stat = true;
   }

   db_unlock(mdb);
   return stat;
}


/*
 *  struct stat
 *  {
 *      dev_t         st_dev;       * device *
 *      ino_t         st_ino;       * inode *
 *      mode_t        st_mode;      * protection *
 *      nlink_t       st_nlink;     * number of hard links *
 *      uid_t         st_uid;       * user ID of owner *
 *      gid_t         st_gid;       * group ID of owner *
 *      dev_t         st_rdev;      * device type (if inode device) *
 *      off_t         st_size;      * total size, in bytes *
 *      unsigned long st_blksize;   * blocksize for filesystem I/O *
 *      unsigned long st_blocks;    * number of blocks allocated *
 *      time_t        st_atime;     * time of last access *
 *      time_t        st_mtime;     * time of last modification *
 *      time_t        st_ctime;     * time of last inode change *
 *  };
 */



/*
 * Create File record in B_DB
 *
 *  In order to reduce database size, we store the File attributes,
 *  the FileName, and the Path separately.  In principle, there
 *  is a single FileName record and a single Path record, no matter
 *  how many times it occurs.  This is this subroutine, we separate
 *  the file and the path and create three database records.
 */
int db_create_file_attributes_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{

   db_lock(mdb);
   Dmsg1(300, "Fname=%s\n", ar->fname);
   Dmsg0(500, "put_file_into_catalog\n");
   /*
    * Make sure we have an acceptable attributes record.
    */
   if (!(ar->Stream == STREAM_UNIX_ATTRIBUTES ||
         ar->Stream == STREAM_UNIX_ATTRIBUTES_EX)) {
      Mmsg1(&mdb->errmsg, _("Attempt to put non-attributes into catalog. Stream=%d\n"),
         ar->Stream);
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      goto bail_out;
   }


   split_path_and_file(jcr, mdb, ar->fname);

   if (!db_create_filename_record(jcr, mdb, ar)) {
      goto bail_out;
   }
   Dmsg1(500, "db_create_filename_record: %s\n", mdb->esc_name);


   if (!db_create_path_record(jcr, mdb, ar)) {
      goto bail_out;
   }
   Dmsg1(500, "db_create_path_record: %s\n", mdb->esc_name);

   /* Now create master File record */
   if (!db_create_file_record(jcr, mdb, ar)) {
      goto bail_out;
   }
   Dmsg0(500, "db_create_file_record OK\n");

   Dmsg3(300, "CreateAttributes Path=%s File=%s FilenameId=%d\n", mdb->path, mdb->fname, ar->FilenameId);
   db_unlock(mdb);
   return 1;

bail_out:
   db_unlock(mdb);
   return 0;
}

/*
 * This is the master File entry containing the attributes.
 *  The filename and path records have already been created.
 */
static int db_create_file_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   int stat;
   static char *no_sig = "0";
   char *sig;

   ASSERT(ar->JobId);
   ASSERT(ar->PathId);
   ASSERT(ar->FilenameId);

   if (ar->Sig == NULL) {
      sig = no_sig;
   } else {
      sig = ar->Sig;
   }

   /* Must create it */
   Mmsg(mdb->cmd,
        "INSERT INTO File (FileIndex,JobId,PathId,FilenameId,"
        "LStat,MD5) VALUES (%u,%u,%u,%u,'%s','%s')",
        ar->FileIndex, ar->JobId, ar->PathId, ar->FilenameId,
        ar->attr, sig);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db File record %s failed. ERR=%s"),
         mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_FATAL, 0, "%s", mdb->errmsg);
      ar->FileId = 0;
      stat = 0;
   } else {
      ar->FileId = sql_insert_id(mdb, N_("File"));
      stat = 1;
   }
   return stat;
}

/* Create a Unique record for the Path -- no duplicates */
static int db_create_path_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   SQL_ROW row;
   int stat;

   mdb->esc_name = check_pool_memory_size(mdb->esc_name, 2*mdb->pnl+2);
   db_escape_string(mdb->esc_name, mdb->path, mdb->pnl);

   if (mdb->cached_path_id != 0 && mdb->cached_path_len == mdb->pnl &&
       strcmp(mdb->cached_path, mdb->path) == 0) {
      ar->PathId = mdb->cached_path_id;
      return 1;
   }

   Mmsg(mdb->cmd, "SELECT PathId FROM Path WHERE Path='%s'", mdb->esc_name);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
         char ed1[30];
         Mmsg2(&mdb->errmsg, _("More than one Path!: %s for path: %s\n"),
            edit_uint64(mdb->num_rows, ed1), mdb->path);
         Jmsg(jcr, M_WARNING, 0, "%s", mdb->errmsg);
      }
      /* Even if there are multiple paths, take the first one */
      if (mdb->num_rows >= 1) {
         if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
            sql_free_result(mdb);
            ar->PathId = 0;
            ASSERT(ar->PathId);
            return 0;
         }
         ar->PathId = str_to_int64(row[0]);
         sql_free_result(mdb);
         /* Cache path */
         if (ar->PathId != mdb->cached_path_id) {
            mdb->cached_path_id = ar->PathId;
            mdb->cached_path_len = mdb->pnl;
            pm_strcpy(mdb->cached_path, mdb->path);
         }
         ASSERT(ar->PathId);
         return 1;
      }
      sql_free_result(mdb);
   }

   Mmsg(mdb->cmd, "INSERT INTO Path (Path) VALUES ('%s')", mdb->esc_name);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db Path record %s failed. ERR=%s\n"),
         mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_FATAL, 0, "%s", mdb->errmsg);
      ar->PathId = 0;
      stat = 0;
   } else {
      ar->PathId = sql_insert_id(mdb, N_("Path"));
      stat = 1;
   }

   /* Cache path */
   if (stat && ar->PathId != mdb->cached_path_id) {
      mdb->cached_path_id = ar->PathId;
      mdb->cached_path_len = mdb->pnl;
      pm_strcpy(mdb->cached_path, mdb->path);
   }
   return stat;
}

/* Create a Unique record for the filename -- no duplicates */
static int db_create_filename_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   SQL_ROW row;

   mdb->esc_name = check_pool_memory_size(mdb->esc_name, 2*mdb->fnl+2);
   db_escape_string(mdb->esc_name, mdb->fname, mdb->fnl);

   Mmsg(mdb->cmd, "SELECT FilenameId FROM Filename WHERE Name='%s'", mdb->esc_name);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
         char ed1[30];
         Mmsg2(&mdb->errmsg, _("More than one Filename! %s for file: %s\n"),
            edit_uint64(mdb->num_rows, ed1), mdb->fname);
         Jmsg(jcr, M_WARNING, 0, "%s", mdb->errmsg);
      }
      if (mdb->num_rows >= 1) {
         if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg2(&mdb->errmsg, _("Error fetching row for file=%s: ERR=%s\n"),
                mdb->fname, sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
            ar->FilenameId = 0;
         } else {
            ar->FilenameId = str_to_int64(row[0]);
         }
         sql_free_result(mdb);
         return ar->FilenameId > 0;
      }
      sql_free_result(mdb);
   }

   Mmsg(mdb->cmd, "INSERT INTO Filename (Name) VALUES ('%s')", mdb->esc_name);

   if (!INSERT_DB(jcr, mdb, mdb->cmd)) {
      Mmsg2(&mdb->errmsg, _("Create db Filename record %s failed. ERR=%s\n"),
            mdb->cmd, sql_strerror(mdb));
      Jmsg(jcr, M_FATAL, 0, "%s", mdb->errmsg);
      ar->FilenameId = 0;
   } else {
      ar->FilenameId = sql_insert_id(mdb, N_("Filename"));
   }
   return ar->FilenameId > 0;
}

#endif /* HAVE_SQLITE3 || HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL */
