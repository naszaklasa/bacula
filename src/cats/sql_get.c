/*
 * Bacula Catalog Database Get record interface routines
 *  Note, these routines generally get a record by id or
 *	  by name.  If more logic is involved, the routine
 *	  should be in find.c 
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id: sql_get.c,v 1.70 2004/09/22 19:51:05 kerns Exp $
 */

/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

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

/* Forward referenced functions */
static int db_get_file_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr, FILE_DBR *fdbr);
static int db_get_filename_record(JCR *jcr, B_DB *mdb);
static int db_get_path_record(JCR *jcr, B_DB *mdb);


/* Imported subroutines */
extern void print_result(B_DB *mdb);
extern int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);
extern void split_path_and_file(JCR *jcr, B_DB *mdb, const char *fname);



/*
 * Given a full filename (with path), look up the File record
 * (with attributes) in the database.
 *
 *  Returns: 0 on failure
 *	     1 on success with the File record in FILE_DBR
 */
int db_get_file_attributes_record(JCR *jcr, B_DB *mdb, char *fname, JOB_DBR *jr, FILE_DBR *fdbr)
{
   int stat;
   Dmsg1(100, "db_get_file_att_record fname=%s \n", fname);

   db_lock(mdb);
   split_path_and_file(jcr, mdb, fname);

   fdbr->FilenameId = db_get_filename_record(jcr, mdb);

   fdbr->PathId = db_get_path_record(jcr, mdb);

   stat = db_get_file_record(jcr, mdb, jr, fdbr);

   db_unlock(mdb);

   return stat;
}

 
/*
 * Get a File record   
 * Returns: 0 on failure
 *	    1 on success
 *
 *  DO NOT use Jmsg in this routine.
 *
 *  Note in this routine, we do not use Jmsg because it may be
 *    called to get attributes of a non-existent file, which is
 *    "normal" if a new file is found during Verify.
 */
static
int db_get_file_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr, FILE_DBR *fdbr)
{
   SQL_ROW row;
   int stat = 0;

   if (jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG) {
   Mmsg(mdb->cmd, 
"SELECT FileId, LStat, MD5 FROM File,Job WHERE "
"File.JobId=Job.JobId AND File.PathId=%u AND "
"File.FilenameId=%u AND Job.Type='B' AND Job.JobSTATUS='T' AND "
"ClientId=%u ORDER BY StartTime DESC LIMIT 1",
      fdbr->PathId, fdbr->FilenameId, jr->ClientId);
   } else {
      Mmsg(mdb->cmd, 
"SELECT FileId, LStat, MD5 FROM File WHERE File.JobId=%u AND File.PathId=%u AND "
"File.FilenameId=%u", fdbr->JobId, fdbr->PathId, fdbr->FilenameId);
   }
   Dmsg3(050, "Get_file_record JobId=%u FilenameId=%u PathId=%u\n",
      fdbr->JobId, fdbr->FilenameId, fdbr->PathId);
      
   Dmsg1(100, "Query=%s\n", mdb->cmd);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(050, "get_file_record num_rows=%d\n", (int)mdb->num_rows);
      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("get_file_record want 1 got rows=%d\n"),
	    mdb->num_rows);
      }
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("Error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    fdbr->FileId = (FileId_t)str_to_int64(row[0]);
	    bstrncpy(fdbr->LStat, row[1], sizeof(fdbr->LStat));
	    bstrncpy(fdbr->SIG, row[2], sizeof(fdbr->SIG));
	    stat = 1;
	 }
      } else {
         Mmsg2(&mdb->errmsg, _("File record for PathId=%u FilenameId=%u not found.\n"),
	    fdbr->PathId, fdbr->FilenameId);
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("File record not found in Catalog.\n"));
   }
   return stat;

}

/* Get Filename record	 
 * Returns: 0 on failure
 *	    FilenameId on success
 *
 *   DO NOT use Jmsg in this routine (see notes for get_file_record)
 */
static int db_get_filename_record(JCR *jcr, B_DB *mdb)
{
   SQL_ROW row;
   int FilenameId = 0;

   mdb->esc_name = check_pool_memory_size(mdb->esc_name, 2*mdb->fnl+2);
   db_escape_string(mdb->esc_name, mdb->fname, mdb->fnl);
   
   Mmsg(mdb->cmd, "SELECT FilenameId FROM Filename WHERE Name='%s'", mdb->esc_name);
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      char ed1[30];
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
         Mmsg2(&mdb->errmsg, _("More than one Filename!: %s for file: %s\n"),
	    edit_uint64(mdb->num_rows, ed1), mdb->fname);
         Jmsg(jcr, M_WARNING, 0, "%s", mdb->errmsg);
      } 
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    FilenameId = atoi(row[0]);
	    if (FilenameId <= 0) {
               Mmsg2(&mdb->errmsg, _("Get DB Filename record %s found bad record: %d\n"),
		  mdb->cmd, FilenameId); 
	       FilenameId = 0;
	    }
	 }
      } else {
         Mmsg1(&mdb->errmsg, _("Filename record: %s not found.\n"), mdb->fname);
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Filename record: %s not found in Catalog.\n"), mdb->fname);
   }
   return FilenameId;
}

/* Get path record   
 * Returns: 0 on failure
 *	    PathId on success
 *
 *   DO NOT use Jmsg in this routine (see notes for get_file_record)
 */
static int db_get_path_record(JCR *jcr, B_DB *mdb)
{
   SQL_ROW row;
   uint32_t PathId = 0;

   mdb->esc_name = check_pool_memory_size(mdb->esc_name, 2*mdb->pnl+2);
   db_escape_string(mdb->esc_name, mdb->path, mdb->pnl);

   if (mdb->cached_path_id != 0 && mdb->cached_path_len == mdb->pnl &&
       strcmp(mdb->cached_path, mdb->path) == 0) {
      return mdb->cached_path_id;
   }	      

   Mmsg(mdb->cmd, "SELECT PathId FROM Path WHERE Path='%s'", mdb->esc_name);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      char ed1[30];
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
         Mmsg2(&mdb->errmsg, _("More than one Path!: %s for path: %s\n"),
	    edit_uint64(mdb->num_rows, ed1), mdb->path);
         Jmsg(jcr, M_WARNING, 0, "%s", mdb->errmsg);
      } 
      /* Even if there are multiple paths, take the first one */
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	 } else {
	    PathId = atoi(row[0]);
	    if (PathId <= 0) {
               Mmsg2(&mdb->errmsg, _("Get DB path record %s found bad record: %u\n"),
		  mdb->cmd, PathId); 
	       PathId = 0;
	    } else {
	       /* Cache path */
	       if (PathId != mdb->cached_path_id) {
		  mdb->cached_path_id = PathId;
		  mdb->cached_path_len = mdb->pnl;
		  pm_strcpy(&mdb->cached_path, mdb->path);
	       }
	    }
	 }
      } else {	
         Mmsg1(&mdb->errmsg, _("Path record: %s not found.\n"), mdb->path);
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Path record: %s not found in Catalog.\n"), mdb->path);
   }
   return PathId;
}


/* 
 * Get Job record for given JobId or Job name
 * Returns: 0 on failure
 *	    1 on success
 */
int db_get_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   SQL_ROW row;

   db_lock(mdb);
   if (jr->JobId == 0) {
      Mmsg(mdb->cmd, "SELECT VolSessionId,VolSessionTime,"
"PoolId,StartTime,EndTime,JobFiles,JobBytes,JobTDate,Job,JobStatus,"
"Type,Level,ClientId "
"FROM Job WHERE Job='%s'", jr->Job);
    } else {
      Mmsg(mdb->cmd, "SELECT VolSessionId,VolSessionTime,"
"PoolId,StartTime,EndTime,JobFiles,JobBytes,JobTDate,Job,JobStatus,"
"Type,Level,ClientId "
"FROM Job WHERE JobId=%u", jr->JobId);
    }

   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      db_unlock(mdb);
      return 0; 		      /* failed */
   }
   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(&mdb->errmsg, _("No Job found for JobId %u\n"), jr->JobId);
      sql_free_result(mdb);
      db_unlock(mdb);
      return 0; 		      /* failed */
   }

   jr->VolSessionId = str_to_uint64(row[0]);
   jr->VolSessionTime = str_to_uint64(row[1]);
   jr->PoolId = str_to_uint64(row[2]);
   bstrncpy(jr->cStartTime, row[3]!=NULL?row[3]:"", sizeof(jr->cStartTime));
   bstrncpy(jr->cEndTime, row[4]!=NULL?row[4]:"", sizeof(jr->cEndTime));
   jr->JobFiles = atol(row[5]);
   jr->JobBytes = str_to_int64(row[6]);
   jr->JobTDate = str_to_int64(row[7]);
   bstrncpy(jr->Job, row[8]!=NULL?row[8]:"", sizeof(jr->Job));
   jr->JobStatus = (int)*row[9];
   jr->JobType = (int)*row[10];
   jr->JobLevel = (int)*row[11];
   jr->ClientId = str_to_uint64(row[12]!=NULL?row[12]:(char *)"");
   sql_free_result(mdb);

   db_unlock(mdb);
   return 1;
}

/*
 * Find VolumeNames for a given JobId
 *  Returns: 0 on error or no Volumes found
 *	     number of volumes on success
 *		Volumes are concatenated in VolumeNames
 *		separated by a vertical bar (|) in the order
 *		that they were written.
 *
 *  Returns: number of volumes on success
 */
int db_get_job_volume_names(JCR *jcr, B_DB *mdb, uint32_t JobId, POOLMEM **VolumeNames)
{
   SQL_ROW row;
   int stat = 0;
   int i;

   db_lock(mdb);
   /* Get one entry per VolumeName, but "sort" by VolIndex */
   Mmsg(mdb->cmd, 
        "SELECT VolumeName,MAX(VolIndex) FROM JobMedia,Media WHERE "
        "JobMedia.JobId=%u AND JobMedia.MediaId=Media.MediaId "
        "GROUP BY VolumeName "
        "ORDER BY 2 ASC", JobId);

   Dmsg1(130, "VolNam=%s\n", mdb->cmd);
   *VolumeNames[0] = 0;
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(130, "Num rows=%d\n", mdb->num_rows);
      if (mdb->num_rows <= 0) {
         Mmsg1(&mdb->errmsg, _("No volumes found for JobId=%d\n"), JobId);
	 stat = 0;
      } else {
	 stat = mdb->num_rows;
	 for (i=0; i < stat; i++) {
	    if ((row = sql_fetch_row(mdb)) == NULL) {
               Mmsg2(&mdb->errmsg, _("Error fetching row %d: ERR=%s\n"), i, sql_strerror(mdb));
               Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	       stat = 0;
	       break;
	    } else {
	       if (*VolumeNames[0] != 0) {
                  pm_strcat(VolumeNames, "|");
	       }
	       pm_strcat(VolumeNames, row[0]);
	    }
	 }
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("No Volume for JobId %d found in Catalog.\n"), JobId);
   }
   db_unlock(mdb);
   return stat;
}

/*
 * Find Volume parameters for a give JobId
 *  Returns: 0 on error or no Volumes found
 *	     number of volumes on success
 *	     List of Volumes and start/end file/blocks (malloced structure!)
 *
 *  Returns: number of volumes on success
 */
int db_get_job_volume_parameters(JCR *jcr, B_DB *mdb, uint32_t JobId, VOL_PARAMS **VolParams)
{
   SQL_ROW row;
   int stat = 0;
   int i;
   VOL_PARAMS *Vols = NULL;

   db_lock(mdb);
   Mmsg(mdb->cmd, 
"SELECT VolumeName,FirstIndex,LastIndex,StartFile,JobMedia.EndFile,"
"StartBlock,JobMedia.EndBlock"
" FROM JobMedia,Media WHERE JobMedia.JobId=%u"
" AND JobMedia.MediaId=Media.MediaId ORDER BY VolIndex,JobMediaId", JobId);

   Dmsg1(130, "VolNam=%s\n", mdb->cmd);
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(130, "Num rows=%d\n", mdb->num_rows);
      if (mdb->num_rows <= 0) {
         Mmsg1(&mdb->errmsg, _("No volumes found for JobId=%d\n"), JobId);
	 stat = 0;
      } else {
	 stat = mdb->num_rows;
	 if (stat > 0) {
	    *VolParams = Vols = (VOL_PARAMS *)malloc(stat * sizeof(VOL_PARAMS));
	 }
	 for (i=0; i < stat; i++) {
	    if ((row = sql_fetch_row(mdb)) == NULL) {
               Mmsg2(&mdb->errmsg, _("Error fetching row %d: ERR=%s\n"), i, sql_strerror(mdb));
               Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	       stat = 0;
	       break;
	    } else {
	       bstrncpy(Vols[i].VolumeName, row[0], MAX_NAME_LENGTH);
	       Vols[i].FirstIndex = str_to_uint64(row[1]);
	       Vols[i].LastIndex = str_to_uint64(row[2]);
	       Vols[i].StartFile = str_to_uint64(row[3]);
	       Vols[i].EndFile = str_to_uint64(row[4]);
	       Vols[i].StartBlock = str_to_uint64(row[5]);
	       Vols[i].EndBlock = str_to_uint64(row[6]);
	    }
	 }
      }
      sql_free_result(mdb);
   }
   db_unlock(mdb);
   return stat;
}



/* 
 * Get the number of pool records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_pool_records(JCR *jcr, B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT count(*) from Pool");
   stat = get_sql_record_max(jcr, mdb);
   db_unlock(mdb);
   return stat;
}

/*
 * This function returns a list of all the Pool record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_pool_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   db_lock(mdb);
   *ids = NULL;
   Mmsg(mdb->cmd, "SELECT PoolId FROM Pool");
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = str_to_uint64(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(mdb->errmsg, _("Pool id select failed: ERR=%s\n"), sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   }
   db_unlock(mdb);
   return stat;
}

/*
 * This function returns a list of all the Client record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_client_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   db_lock(mdb);
   *ids = NULL;
   Mmsg(mdb->cmd, "SELECT ClientId FROM Client");
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = str_to_uint64(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(mdb->errmsg, _("Client id select failed: ERR=%s\n"), sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   }
   db_unlock(mdb);
   return stat;
}



/* Get Pool Record   
 * If the PoolId is non-zero, we get its record,
 *  otherwise, we search on the PoolName
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pdbr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (pdbr->PoolId != 0) {		  /* find by id */
      Mmsg(mdb->cmd, 
"SELECT PoolId,Name,NumVols,MaxVols,UseOnce,UseCatalog,AcceptAnyVolume,\
AutoPrune,Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
MaxVolBytes,PoolType,LabelFormat FROM Pool WHERE Pool.PoolId=%u", pdbr->PoolId);
   } else {			      /* find by name */
      Mmsg(mdb->cmd, 
"SELECT PoolId,Name,NumVols,MaxVols,UseOnce,UseCatalog,AcceptAnyVolume,\
AutoPrune,Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,\
MaxVolBytes,PoolType,LabelFormat FROM Pool WHERE Pool.Name='%s'", pdbr->Name);
   }  

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Pool!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	 } else {
	    pdbr->PoolId = str_to_int64(row[0]);
            bstrncpy(pdbr->Name, row[1]!=NULL?row[1]:"", sizeof(pdbr->Name));
	    pdbr->NumVols = str_to_int64(row[2]);
	    pdbr->MaxVols = str_to_int64(row[3]);
	    pdbr->UseOnce = str_to_int64(row[4]);
	    pdbr->UseCatalog = str_to_int64(row[5]);
	    pdbr->AcceptAnyVolume = str_to_int64(row[6]);
	    pdbr->AutoPrune = str_to_int64(row[7]);
	    pdbr->Recycle = str_to_int64(row[8]);
	    pdbr->VolRetention = str_to_int64(row[9]);
	    pdbr->VolUseDuration = str_to_int64(row[10]);
	    pdbr->MaxVolJobs = str_to_int64(row[11]);
	    pdbr->MaxVolFiles = str_to_int64(row[12]);
	    pdbr->MaxVolBytes = str_to_uint64(row[13]);
            bstrncpy(pdbr->PoolType, row[13]!=NULL?row[14]:"", sizeof(pdbr->PoolType));
            bstrncpy(pdbr->LabelFormat, row[14]!=NULL?row[15]:"", sizeof(pdbr->LabelFormat));
	    stat = pdbr->PoolId;
	 }
      } else {
         Mmsg(mdb->errmsg, _("Pool record not found in Catalog.\n"));
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Pool record not found in Catalog.\n"));
   }
   db_unlock(mdb);
   return stat;
}

/* Get Client Record   
 * If the ClientId is non-zero, we get its record,
 *  otherwise, we search on the Client Name
 *
 * Returns: 0 on failure
 *	    1 on success 
 */
int db_get_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cdbr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (cdbr->ClientId != 0) {		    /* find by id */
      Mmsg(mdb->cmd, 
"SELECT ClientId,Name,Uname,AutoPrune,FileRetention,JobRetention "
"FROM Client WHERE Client.ClientId=%u", cdbr->ClientId);
   } else {			      /* find by name */
      Mmsg(mdb->cmd, 
"SELECT ClientId,Name,Uname,AutoPrune,FileRetention,JobRetention "
"FROM Client WHERE Client.Name='%s'", cdbr->Name);
   }  

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Client!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	 } else {
	    cdbr->ClientId = str_to_int64(row[0]);
            bstrncpy(cdbr->Name, row[1]!=NULL?row[1]:"", sizeof(cdbr->Name));
            bstrncpy(cdbr->Uname, row[2]!=NULL?row[1]:"", sizeof(cdbr->Uname));
	    cdbr->AutoPrune = str_to_int64(row[3]);
	    cdbr->FileRetention = str_to_int64(row[4]);
	    cdbr->JobRetention = str_to_int64(row[5]);
	    stat = 1;
	 }
      } else {
         Mmsg(mdb->errmsg, _("Client record not found in Catalog.\n"));
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Client record not found in Catalog.\n"));
   }
   db_unlock(mdb);
   return stat;
}

/*
 * Get Counter Record
 *
 * Returns: 0 on failure
 *	    1 on success
 */
int db_get_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{
   SQL_ROW row;

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT MinValue,MaxValue,CurrentValue,WrapCounter "
      "FROM Counters WHERE Counter='%s'", cr->Counter);

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      
      /* If more than one, report error, but return first row */
      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Counter!: %d\n"), (int)(mdb->num_rows));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      }
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching Counter row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	    sql_free_result(mdb);
	    db_unlock(mdb);
	    return 0;
	 }
	 cr->MinValue = atoi(row[0]);
	 cr->MaxValue = atoi(row[1]);
	 cr->CurrentValue = atoi(row[2]);
	 if (row[3]) {
	    bstrncpy(cr->WrapCounter, row[3], sizeof(cr->WrapCounter));
	 } else {
	    cr->WrapCounter[0] = 0;
	 }
	 sql_free_result(mdb);
	 db_unlock(mdb);
	 return 1;
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Counter record: %s not found in Catalog.\n"), cr->Counter);
   }  
   db_unlock(mdb);
   return 0;
}


/* Get FileSet Record	
 * If the FileSetId is non-zero, we get its record,
 *  otherwise, we search on the name
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (fsr->FileSetId != 0) {		    /* find by id */
      Mmsg(mdb->cmd, 
           "SELECT FileSetId,FileSet,MD5,CreateTime FROM FileSet "
           "WHERE FileSetId=%u", fsr->FileSetId);
   } else {			      /* find by name */
      Mmsg(mdb->cmd, 
           "SELECT FileSetId,FileSet,CreateTime,MD5 FROM FileSet "
           "WHERE FileSet='%s' ORDER BY CreateTime DESC LIMIT 1", fsr->FileSet);
   }  

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("Error got %s FileSets but expected only one!\n"), 
	    edit_uint64(mdb->num_rows, ed1));
	 sql_data_seek(mdb, mdb->num_rows-1);
      }
      if ((row = sql_fetch_row(mdb)) == NULL) {
         Mmsg1(&mdb->errmsg, _("FileSet record \"%s\" not found.\n"), fsr->FileSet);
      } else {
	 fsr->FileSetId = atoi(row[0]);
         bstrncpy(fsr->FileSet, row[1]!=NULL?row[1]:"", sizeof(fsr->FileSet));
         bstrncpy(fsr->MD5, row[2]!=NULL?row[2]:"", sizeof(fsr->MD5));
         bstrncpy(fsr->cCreateTime, row[3]!=NULL?row[3]:"", sizeof(fsr->cCreateTime));
	 stat = fsr->FileSetId;
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("FileSet record not found in Catalog.\n"));
   }
   db_unlock(mdb);
   return stat;
}


/* 
 * Get the number of Media records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_media_records(JCR *jcr, B_DB *mdb)
{
   int stat = 0;

   db_lock(mdb);
   Mmsg(mdb->cmd, "SELECT count(*) from Media");
   stat = get_sql_record_max(jcr, mdb);
   db_unlock(mdb);
   return stat;
}


/*
 * This function returns a list of all the Media record ids for
 *     the current Pool.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_media_ids(JCR *jcr, B_DB *mdb, uint32_t PoolId, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   db_lock(mdb);
   *ids = NULL;
   Mmsg(mdb->cmd, "SELECT MediaId FROM Media WHERE PoolId=%u", PoolId);
   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = (uint32_t)atoi(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(mdb->errmsg, _("Media id select failed: ERR=%s\n"), sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      stat = 0;
   }
   db_unlock(mdb);
   return stat;
}


/* Get Media Record   
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   SQL_ROW row;
   int stat = 0;

   db_lock(mdb);
   if (mr->MediaId == 0 && mr->VolumeName[0] == 0) {
      Mmsg(mdb->cmd, "SELECT count(*) from Media");
      mr->MediaId = get_sql_record_max(jcr, mdb);
      db_unlock(mdb);
      return 1;
   }
   if (mr->MediaId != 0) {		 /* find by id */
      Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,"
         "VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,"
         "MediaType,VolStatus,PoolId,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,"
         "Recycle,Slot,FirstWritten,LastWritten,InChanger,EndFile,EndBlock "
         "FROM Media WHERE MediaId=%d", mr->MediaId);
   } else {			      /* find by name */
      Mmsg(mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,"
         "VolBytes,VolMounts,VolErrors,VolWrites,MaxVolBytes,VolCapacityBytes,"
         "MediaType,VolStatus,PoolId,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,"
         "Recycle,Slot,FirstWritten,LastWritten,InChanger,EndFile,EndBlock "
         "FROM Media WHERE VolumeName='%s'", mr->VolumeName);
   }  

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Volume!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
         Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
            Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
	 } else {
	    /* return values */
	    mr->MediaId = str_to_int64(row[0]);
            bstrncpy(mr->VolumeName, row[1]!=NULL?row[1]:"", sizeof(mr->VolumeName));
	    mr->VolJobs = str_to_int64(row[2]);
	    mr->VolFiles = str_to_int64(row[3]);
	    mr->VolBlocks = str_to_int64(row[4]);
	    mr->VolBytes = str_to_uint64(row[5]);
	    mr->VolMounts = str_to_int64(row[6]);
	    mr->VolErrors = str_to_int64(row[7]);
	    mr->VolWrites = str_to_int64(row[8]);
	    mr->MaxVolBytes = str_to_uint64(row[9]);
	    mr->VolCapacityBytes = str_to_uint64(row[10]);
            bstrncpy(mr->MediaType, row[11]!=NULL?row[11]:"", sizeof(mr->MediaType));
            bstrncpy(mr->VolStatus, row[12]!=NULL?row[12]:"", sizeof(mr->VolStatus));
	    mr->PoolId = str_to_int64(row[13]);
	    mr->VolRetention = str_to_uint64(row[14]);
	    mr->VolUseDuration = str_to_uint64(row[15]);
	    mr->MaxVolJobs = str_to_int64(row[16]);
	    mr->MaxVolFiles = str_to_int64(row[17]);
	    mr->Recycle = str_to_int64(row[18]);
	    mr->Slot = str_to_int64(row[19]);
            bstrncpy(mr->cFirstWritten, row[20]!=NULL?row[20]:"", sizeof(mr->cFirstWritten));
	    mr->FirstWritten = (time_t)str_to_utime(mr->cFirstWritten);
            bstrncpy(mr->cLastWritten, row[21]!=NULL?row[21]:"", sizeof(mr->cLastWritten));
	    mr->LastWritten = (time_t)str_to_utime(mr->cLastWritten);
	    mr->InChanger = str_to_uint64(row[22]);
	    mr->EndFile = str_to_uint64(row[23]);
	    mr->EndBlock = str_to_uint64(row[24]);
	    stat = mr->MediaId;
	 }
      } else {
	 if (mr->MediaId != 0) {
            Mmsg1(&mdb->errmsg, _("Media record MediaId=%u not found.\n"), mr->MediaId);
	 } else {
            Mmsg1(&mdb->errmsg, _("Media record for Volume \"%s\" not found.\n"), 
		  mr->VolumeName);
	 }
      }
      sql_free_result(mdb);
   } else {
      Mmsg(mdb->errmsg, _("Media record not found in Catalog.\n"));
   }
   db_unlock(mdb);
   return stat;
}


#endif /* HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL */
