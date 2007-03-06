/*
 * Bacula Catalog Database List records interface routines
 * 
 *    Kern Sibbald, March 2000
 *
 *    Version $Id: sql_list.c,v 1.28 2004/09/22 19:51:05 kerns Exp $
 */

/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

#if    HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL

/* -----------------------------------------------------------------------
 *
 *   Generic Routines (or almost generic)
 *
 * -----------------------------------------------------------------------
 */

/* Imported subroutines */
extern void list_result(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);
extern int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);


/* 
 * Submit general SQL query
 */
int db_list_sql_query(JCR *jcr, B_DB *mdb, char *query, DB_LIST_HANDLER *sendit, 
		      void *ctx, int verbose, e_list_type type)
{
   db_lock(mdb);
   if (sql_query(mdb, query) != 0) {
      Mmsg(mdb->errmsg, _("Query failed: %s\n"), sql_strerror(mdb));
      if (verbose) {
	 sendit(ctx, mdb->errmsg);
      }
      db_unlock(mdb);
      return 0;
   }

   mdb->result = sql_store_result(mdb);

   if (mdb->result) {
      list_result(jcr, mdb, sendit, ctx, type);
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return 1;
}

void
db_list_pool_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type) 
{
   db_lock(mdb);
   if (type == VERT_LIST) {
      Mmsg(mdb->cmd, "SELECT PoolId,Name,NumVols,MaxVols,UseOnce,UseCatalog,"
         "AcceptAnyVolume,VolRetention,VolUseDuration,MaxVolJobs,MaxVolBytes,"
         "AutoPrune,Recycle,PoolType,LabelFormat,Enabled,ScratchPoolId,"
         "RecyclePoolId "
          "FROM Pool ORDER BY PoolId");
   } else {
      Mmsg(mdb->cmd, "SELECT PoolId,Name,NumVols,MaxVols,PoolType,LabelFormat "
        "FROM Pool ORDER BY PoolId");
   }

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, type);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}

void
db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{
   db_lock(mdb);
   if (type == VERT_LIST) {
      Mmsg(mdb->cmd, "SELECT ClientId,Name,Uname,AutoPrune,FileRetention,"
         "FileRetention,JobRetention "
         "FROM Client ORDER BY ClientId");
   } else {
      Mmsg(mdb->cmd, "SELECT ClientId,Name,FileRetention,JobRetention "
         "FROM Client ORDER BY ClientId");
   }

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, type);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}


/*
 * If VolumeName is non-zero, list the record for that Volume
 *   otherwise, list the Volumes in the Pool specified by PoolId
 */
void
db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr, 
		      DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{
   db_lock(mdb);
   if (type == VERT_LIST) {
      if (mdbr->VolumeName[0] != 0) {
         Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,Slot,PoolId,"
            "MediaType,FirstWritten,LastWritten,LabelDate,VolJobs,"
            "VolFiles,VolBlocks,VolMounts,VolBytes,VolErrors,VolWrites,"
            "VolCapacityBytes,VolStatus,Recycle,VolRetention,"
            "VolUseDuration,MaxVolJobs,MaxVolFiles,MaxVolBytes,InChanger,"
            "EndFile,EndBlock "
            "FROM Media WHERE Media.VolumeName='%s'", mdbr->VolumeName);
      } else {
         Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,Slot,PoolId,"
            "MediaType,FirstWritten,LastWritten,LabelDate,VolJobs,"
            "VolFiles,VolBlocks,VolMounts,VolBytes,VolErrors,VolWrites,"
            "VolCapacityBytes,VolStatus,Recycle,VolRetention,"
            "VolUseDuration,MaxVolJobs,MaxVolFiles,MaxVolBytes,InChanger,"
            "EndFile,EndBlock "
            "FROM Media WHERE Media.PoolId=%u ORDER BY MediaId", mdbr->PoolId);
      }
   } else {
      if (mdbr->VolumeName[0] != 0) {
         Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolStatus,"
            "VolBytes,VolFiles,VolRetention,Recycle,Slot,InChanger,MediaType,LastWritten "
            "FROM Media WHERE Media.VolumeName='%s'", mdbr->VolumeName);
      } else {
         Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolStatus,"
            "VolBytes,VolFiles,VolRetention,Recycle,Slot,InChanger,MediaType,LastWritten "
            "FROM Media WHERE Media.PoolId=%u ORDER BY MediaId", mdbr->PoolId);
      }
   }

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, type);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}

void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, uint32_t JobId, 
			      DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{
   db_lock(mdb);
   if (type == VERT_LIST) {
      if (JobId > 0) {			 /* do by JobId */
         Mmsg(mdb->cmd, "SELECT JobMediaId,JobId,MediaId,Media.VolumeName,"
            "FirstIndex,LastIndex,StartFile,JobMedia.EndFile,StartBlock,"
            "JobMedia.EndBlock "
            "FROM JobMedia,Media WHERE Media.MediaId=JobMedia.MediaId "
            "AND JobMedia.JobId=%u", JobId);
      } else {
         Mmsg(mdb->cmd, "SELECT JobMediaId,JobId,MediaId,Media.VolumeName,"
            "FirstIndex,LastIndex,StartFile,JobMedia.EndFile,StartBlock,"
            "JobMedia.EndBlock "
            "FROM JobMedia,Media WHERE Media.MediaId=JobMedia.MediaId");
      }

   } else {
      if (JobId > 0) {			 /* do by JobId */
         Mmsg(mdb->cmd, "SELECT JobId,Media.VolumeName,FirstIndex,LastIndex "
            "FROM JobMedia,Media WHERE Media.MediaId=JobMedia.MediaId "
            "AND JobMedia.JobId=%u", JobId);
      } else {
         Mmsg(mdb->cmd, "SELECT JobId,Media.VolumeName,FirstIndex,LastIndex "
            "FROM JobMedia,Media WHERE Media.MediaId=JobMedia.MediaId");
      }
   }
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, type);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}



/*
 * List Job record(s) that match JOB_DBR
 *
 *  Currently, we return all jobs or if jr->JobId is set,
 *  only the job with the specified id.
 */
void
db_list_job_records(JCR *jcr, B_DB *mdb, JOB_DBR *jr, DB_LIST_HANDLER *sendit, 
		    void *ctx, e_list_type type)
{
   db_lock(mdb);
   if (type == VERT_LIST) {
      if (jr->JobId == 0 && jr->Job[0] == 0) {
	 Mmsg(mdb->cmd, 
            "SELECT JobId,Job,Job.Name,PurgedFiles,Type,Level,"
            "Job.ClientId,Client.Name,JobStatus,SchedTime,"
            "StartTime,EndTime,JobTDate,"
            "VolSessionId,VolSessionTime,JobFiles,JobErrors,"
            "JobMissingFiles,Job.PoolId,Pool.Name,Job.FileSetId,FileSet.FileSet "
            "FROM Job,Client,Pool,FileSet WHERE "
            "Client.ClientId=Job.ClientId AND Pool.PoolId=Job.PoolId "
            "AND FileSet.FileSetId=Job.FileSetId  ORDER BY StartTime");
      } else {				 /* single record */
	 Mmsg(mdb->cmd, 
            "SELECT JobId,Job,Job.Name,PurgedFiles,Type,Level,"
            "Job.ClientId,Client.Name,JobStatus,SchedTime,"
            "StartTime,EndTime,JobTDate,"
            "VolSessionId,VolSessionTime,JobFiles,JobErrors,"
            "JobMissingFiles,Job.PoolId,Pool.Name,Job.FileSetId,FileSet.FileSet "
            "FROM Job,Client,Pool,FileSet WHERE Job.JobId=%u AND "
            "Client.ClientId=Job.ClientId AND Pool.PoolId=Job.PoolId "
            "AND FileSet.FileSetId=Job.FileSetId", jr->JobId);
      }
   } else {
      if (jr->JobId == 0 && jr->Job[0] == 0) {
	 Mmsg(mdb->cmd, 
           "SELECT JobId,Name,StartTime,Type,Level,JobFiles,JobBytes,JobStatus "
           "FROM Job ORDER BY StartTime");
      } else {				 /* single record */
         Mmsg(mdb->cmd, "SELECT JobId,Name,StartTime,Type,Level,"
            "JobFiles,JobBytes,JobStatus FROM Job WHERE JobId=%u", jr->JobId);
      }
   }
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }
   list_result(jcr, mdb, sendit, ctx, type);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}

/*
 * List Job totals
 *
 */
void
db_list_job_totals(JCR *jcr, B_DB *mdb, JOB_DBR *jr, DB_LIST_HANDLER *sendit, void *ctx)
{
   db_lock(mdb);

   /* List by Job */
   Mmsg(mdb->cmd, "SELECT  count(*) AS Jobs,sum(JobFiles) "
      "AS Files,sum(JobBytes) AS Bytes,Name AS Job FROM Job GROUP BY Name");

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, HORZ_LIST);
   
   sql_free_result(mdb);

   /* Do Grand Total */
   Mmsg(mdb->cmd, "SELECT count(*) AS Jobs,sum(JobFiles) \
AS Files,sum(JobBytes) As Bytes FROM Job");

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, HORZ_LIST);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}

/* 
 * Stupid MySQL is NON-STANDARD !  
 */
#ifdef HAVE_MYSQL
#define FN "CONCAT(Path.Path,Filename.Name)"
#else
#define FN "Path.Path||Filename.Name"
#endif

void
db_list_files_for_job(JCR *jcr, B_DB *mdb, uint32_t jobid, DB_LIST_HANDLER *sendit, void *ctx)
{
   db_lock(mdb);

   Mmsg(mdb->cmd, "SELECT " FN " AS Filename FROM File,"
"Filename,Path WHERE File.JobId=%u AND Filename.FilenameId=File.FilenameId "
"AND Path.PathId=File.PathId",
      jobid);

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return;
   }

   list_result(jcr, mdb, sendit, ctx, HORZ_LIST);
   
   sql_free_result(mdb);
   db_unlock(mdb);
}


#endif /* HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL */
