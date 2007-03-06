/*
 * Bacula Catalog Database Find record interface routines
 *
 *  Note, generally, these routines are more complicated
 *	  that a simple search by name or id. Such simple
 *	  request are in get.c
 * 
 *    Kern Sibbald, December 2000
 *
 *    Version $Id: sql_find.c,v 1.48 2004/10/07 16:23:25 kerns Exp $
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
extern void print_result(B_DB *mdb);
extern int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);

/*
 * Find job start time if JobId specified, otherwise
 * find last full save for Incremental and Differential saves.
 *
 *  StartTime is returned in stime
 *	
 * Returns: 0 on failure
 *	    1 on success, jr is unchanged, but stime is set
 */
int
db_find_job_start_time(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM **stime)
{
   SQL_ROW row;

   db_lock(mdb);

   pm_strcpy(stime, "0000-00-00 00:00:00");   /* default */
   /* If no Id given, we must find corresponding job */
   if (jr->JobId == 0) {
      /* Differential is since last Full backup */
	 Mmsg(mdb->cmd, 
"SELECT StartTime FROM Job WHERE JobStatus='T' AND Type='%c' AND "
"Level='%c' AND Name='%s' AND ClientId=%u AND FileSetId=%u "
"ORDER BY StartTime DESC LIMIT 1",
	   jr->JobType, L_FULL, jr->Name, jr->ClientId, jr->FileSetId);

      if (jr->JobLevel == L_DIFFERENTIAL) {
	 /* SQL cmd for Differential backup already edited above */

      /* Incremental is since last Full, Incremental, or Differential */
      } else if (jr->JobLevel == L_INCREMENTAL) {
	 /* 
	  * For an Incremental job, we must first ensure
	  *  that a Full backup was done (cmd edited above)
	  *  then we do a second look to find the most recent
	  *  backup
	  */
	 if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
            Mmsg2(&mdb->errmsg, _("Query error for start time request: ERR=%s\nCMD=%s\n"), 
	       sql_strerror(mdb), mdb->cmd);
	    db_unlock(mdb);
	    return 0;
	 }
	 if ((row = sql_fetch_row(mdb)) == NULL) {
	    sql_free_result(mdb);
            Mmsg(mdb->errmsg, _("No prior Full backup Job record found.\n"));
	    db_unlock(mdb);
	    return 0;
	 }
	 sql_free_result(mdb);
	 /* Now edit SQL command for Incremental Job */
	 Mmsg(mdb->cmd, 
"SELECT StartTime FROM Job WHERE JobStatus='T' AND Type='%c' AND "
"Level IN ('%c','%c','%c') AND Name='%s' AND ClientId=%u "
"AND FileSetId=%u ORDER BY StartTime DESC LIMIT 1",
	    jr->JobType, L_INCREMENTAL, L_DIFFERENTIAL, L_FULL, jr->Name,
	    jr->ClientId, jr->FileSetId);
      } else {
         Mmsg1(&mdb->errmsg, _("Unknown level=%d\n"), jr->JobLevel);
	 db_unlock(mdb);
	 return 0;
      }
   } else {
      Dmsg1(100, "Submitting: %s\n", mdb->cmd);
      Mmsg(mdb->cmd, "SELECT StartTime FROM Job WHERE Job.JobId=%u", jr->JobId);
   }

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      pm_strcpy(stime, "");                   /* set EOS */
      Mmsg2(&mdb->errmsg, _("Query error for start time request: ERR=%s\nCMD=%s\n"),
	 sql_strerror(mdb),  mdb->cmd);
      db_unlock(mdb);
      return 0;
   }

   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg2(&mdb->errmsg, _("No Job record found: ERR=%s\nCMD=%s\n"),
	 sql_strerror(mdb),  mdb->cmd);
      sql_free_result(mdb);
      db_unlock(mdb);
      return 0;
   }
   Dmsg1(100, "Got start time: %s\n", row[0]);
   pm_strcpy(stime, row[0]);

   sql_free_result(mdb);

   db_unlock(mdb);
   return 1;
}

/*
 * Find last failed job since given start-time 
 *   it must be either Full or Diff.
 *
 * Returns: false on failure
 *	    true  on success, jr is unchanged and stime unchanged
 *		  level returned in JobLevel
 */
bool
db_find_failed_job_since(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM *stime, int &JobLevel)
{
   SQL_ROW row;

   db_lock(mdb);
   /* Differential is since last Full backup */
   Mmsg(mdb->cmd, 
"SELECT Level FROM Job WHERE JobStatus!='T' AND Type='%c' AND "
"Level IN ('%c','%c') AND Name='%s' AND ClientId=%u "
"AND FileSetId=%u AND StartTime>'%s' "
"ORDER BY StartTime DESC LIMIT 1",
	 jr->JobType, L_FULL, L_DIFFERENTIAL, jr->Name,
	 jr->ClientId, jr->FileSetId, stime);

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return false;
   }

   if ((row = sql_fetch_row(mdb)) == NULL) {
      sql_free_result(mdb);
      db_unlock(mdb);
      return false;
   }
   JobLevel = (int)*row[0];
   sql_free_result(mdb);

   db_unlock(mdb);
   return true;
}


/* 
 * Find JobId of last job that ran.  E.g. for
 *   VERIFY_CATALOG we want the JobId of the last INIT.
 *   For VERIFY_VOLUME_TO_CATALOG, we want the JobId of the last Job.
 *
 * Returns: 1 on success
 *	    0 on failure
 */
int
db_find_last_jobid(JCR *jcr, B_DB *mdb, const char *Name, JOB_DBR *jr)
{
   SQL_ROW row;

   /* Find last full */
   db_lock(mdb);
   if (jr->JobLevel == L_VERIFY_CATALOG) {
      Mmsg(mdb->cmd, 
"SELECT JobId FROM Job WHERE Type='V' AND Level='%c' AND "
" JobStatus='T' AND Name='%s' AND "
"ClientId=%u ORDER BY StartTime DESC LIMIT 1",
	   L_VERIFY_INIT, jr->Name, jr->ClientId);
   } else if (jr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG ||
	      jr->JobLevel == L_VERIFY_DISK_TO_CATALOG) {
      if (Name) {
	 Mmsg(mdb->cmd,
"SELECT JobId FROM Job WHERE Type='B' AND JobStatus='T' AND "
"Name='%s' ORDER BY StartTime DESC LIMIT 1", Name);
      } else {
	 Mmsg(mdb->cmd, 
"SELECT JobId FROM Job WHERE Type='B' AND JobStatus='T' AND "
"ClientId=%u ORDER BY StartTime DESC LIMIT 1", jr->ClientId);
      }
   } else {
      Mmsg1(&mdb->errmsg, _("Unknown Job level=%c\n"), jr->JobLevel);
      db_unlock(mdb);
      return 0;
   }
   Dmsg1(100, "Query: %s\n", mdb->cmd);
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return 0;
   }
   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(&mdb->errmsg, _("No Job found for: %s.\n"), mdb->cmd);
      sql_free_result(mdb);
      db_unlock(mdb);
      return 0;
   }

   jr->JobId = atoi(row[0]);
   sql_free_result(mdb);

   Dmsg1(100, "db_get_last_jobid: got JobId=%d\n", jr->JobId);
   if (jr->JobId <= 0) {
      Mmsg1(&mdb->errmsg, _("No Job found for: %s\n"), mdb->cmd);
      db_unlock(mdb);
      return 0;
   }

   db_unlock(mdb);
   return 1;
}

/* 
 * Find Available Media (Volume) for Pool
 *
 * Find a Volume for a given PoolId, MediaType, and Status.
 *
 * Returns: 0 on failure
 *	    numrows on success
 */
int
db_find_next_volume(JCR *jcr, B_DB *mdb, int item, bool InChanger, MEDIA_DBR *mr) 
{
   SQL_ROW row;
   int numrows;
   const char *changer, *order;

   db_lock(mdb);
   if (item == -1) {	   /* find oldest volume */
      /* Find oldest volume */
      Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,"
          "VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,"
          "VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,Recycle,Slot,"
          "FirstWritten,LastWritten,VolStatus,InChanger "
          "FROM Media WHERE PoolId=%u AND MediaType='%s' AND VolStatus IN ('Full',"
          "'Recycle','Purged','Used','Append') "
          "ORDER BY LastWritten LIMIT 1", mr->PoolId, mr->MediaType); 
     item = 1;
   } else {
      /* Find next available volume */
      if (InChanger) {
         changer = "AND InChanger=1";
      } else {
         changer = "";
      }
      if (strcmp(mr->VolStatus, "Recycled") == 0 ||
          strcmp(mr->VolStatus, "Purged") == 0) {
         order = "ORDER BY LastWritten ASC,MediaId";  /* take oldest */
      } else {
         order = "ORDER BY LastWritten IS NULL,LastWritten DESC,MediaId";   /* take most recently written */
      }  
      Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,"
          "VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,"
          "VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,Recycle,Slot,"
          "FirstWritten,LastWritten,VolStatus,InChanger "
          "FROM Media WHERE PoolId=%u AND MediaType='%s' AND VolStatus='%s' "
          "%s " 
          "%s LIMIT %d",
	  mr->PoolId, mr->MediaType, mr->VolStatus, changer, order, item);
   }
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return 0;
   }

   numrows = sql_num_rows(mdb);
   if (item > numrows) {
      Mmsg2(&mdb->errmsg, _("Request for Volume item %d greater than max %d\n"),
	 item, numrows);
      db_unlock(mdb);
      return 0;
   }
   
   /* Seek to desired item 
    * Note, we use base 1; SQL uses base 0
    */
   sql_data_seek(mdb, item-1);

   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(&mdb->errmsg, _("No Volume record found for item %d.\n"), item);
      sql_free_result(mdb);
      db_unlock(mdb);
      return 0;
   }

   /* Return fields in Media Record */
   mr->MediaId = str_to_int64(row[0]);
   bstrncpy(mr->VolumeName, row[1], sizeof(mr->VolumeName));
   mr->VolJobs = str_to_int64(row[2]);
   mr->VolFiles = str_to_int64(row[3]);
   mr->VolBlocks = str_to_int64(row[4]);
   mr->VolBytes = str_to_uint64(row[5]);
   mr->VolMounts = str_to_int64(row[6]);
   mr->VolErrors = str_to_int64(row[7]);
   mr->VolWrites = str_to_int64(row[8]);
   mr->MaxVolBytes = str_to_uint64(row[9]);
   mr->VolCapacityBytes = str_to_uint64(row[10]);
   mr->VolRetention = str_to_uint64(row[11]);
   mr->VolUseDuration = str_to_uint64(row[12]);
   mr->MaxVolJobs = str_to_int64(row[13]);
   mr->MaxVolFiles = str_to_int64(row[14]);
   mr->Recycle = str_to_int64(row[15]);
   mr->Slot = str_to_int64(row[16]);
   bstrncpy(mr->cFirstWritten, row[17]!=NULL?row[17]:"", sizeof(mr->cFirstWritten));
   mr->FirstWritten = (time_t)str_to_utime(mr->cFirstWritten);
   bstrncpy(mr->cLastWritten, row[18]!=NULL?row[18]:"", sizeof(mr->cLastWritten));
   mr->LastWritten = (time_t)str_to_utime(mr->cLastWritten);
   bstrncpy(mr->VolStatus, row[19], sizeof(mr->VolStatus));
   mr->InChanger = str_to_int64(row[20]);
   sql_free_result(mdb);

   db_unlock(mdb);
   return numrows;
}


#endif /* HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL */
