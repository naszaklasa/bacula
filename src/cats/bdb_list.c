/*
 * Bacula Catalog Database List records interface routines
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
 *    Version $Id: bdb_list.c,v 1.12 2004/12/21 16:18:30 kerns Exp $
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

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

/*
 * Submit general SQL query
 */
int db_list_sql_query(JCR *jcr, B_DB *mdb, char *query, DB_LIST_HANDLER *sendit,
		      void *ctx, int verbose)
{
   sendit(ctx, "SQL Queries not implemented with internal database.\n");
   return 0;
}


/*
 * List all the pool records
 */
void db_list_pool_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx)
{
   int len;
   POOL_DBR pr;

   Dmsg0(90, "Enter list_pool_records\n");
   db_lock(mdb);
   if (!bdb_open_pools_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   sendit(ctx, "  PoolId NumVols MaxVols  Type       PoolName\n");
   sendit(ctx, "===================================================\n");
   fseek(mdb->poolfd, 0L, SEEK_SET);   /* rewind file */
   len = sizeof(pr);
   while (fread(&pr, len, 1, mdb->poolfd) > 0) {
	 Mmsg(mdb->cmd, " %7d  %6d  %6d  %-10s %s\n",
	    pr.PoolId, pr.NumVols, pr.MaxVols, pr.PoolType, pr.Name);
	 sendit(ctx, mdb->cmd);
   }
   sendit(ctx, "===================================================\n");
   db_unlock(mdb);
   Dmsg0(90, "Leave list_pool_records\n");
   return;
}


/*
 * List Media records
 */
void db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr,
			   DB_LIST_HANDLER *sendit, void *ctx)
{
   char ewc[30];
   int len;
   MEDIA_DBR mr;

   db_lock(mdb);
   if (!bdb_open_media_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   sendit(ctx, "  Status           VolBytes  MediaType        VolumeName\n");
   sendit(ctx, "=============================================================\n");
   fseek(mdb->mediafd, 0L, SEEK_SET);	/* rewind file */
   len = sizeof(mr);
   while (fread(&mr, len, 1, mdb->mediafd) > 0) {
	 Mmsg(mdb->cmd, " %-10s %17s %-15s  %s\n",
	    mr.VolStatus, edit_uint64_with_commas(mr.VolBytes, ewc),
	    mr.MediaType, mr.VolumeName);
	 sendit(ctx, mdb->cmd);
   }
   sendit(ctx, "====================================================================\n");
   db_unlock(mdb);
   return;
}

void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, uint32_t JobId,
			      DB_LIST_HANDLER *sendit, void *ctx)
{
   JOBMEDIA_DBR jm;
   MEDIA_DBR mr;
   int jmlen, mrlen;

   db_lock(mdb);
   if (!bdb_open_jobmedia_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   if (!bdb_open_media_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   sendit(ctx, "    JobId VolumeName    FirstIndex LastIndex\n");
   sendit(ctx, "============================================\n");
   jmlen = sizeof(jm);
   mrlen = sizeof(mr);
   fseek(mdb->jobmediafd, 0L, SEEK_SET); /* rewind the file */
   while (fread(&jm, jmlen, 1, mdb->jobmediafd) > 0) {
      /* List by JobId */
      if (JobId != 0) {
	 if (jm.JobId == JobId) {
	    /* Now find VolumeName in corresponding Media record */
	    fseek(mdb->mediafd, 0L, SEEK_SET);
	    while (fread(&mr, mrlen, 1, mdb->mediafd) > 0) {
	       if (mr.MediaId == jm.MediaId) {
		  Mmsg(mdb->cmd, " %7d  %-10s %10d %10d\n",
		       jm.JobId, mr.VolumeName, jm.FirstIndex, jm.LastIndex);
		  sendit(ctx, mdb->cmd);
		  break;
	       }
	    }
	 }
      } else {
	 /* List all records */
	 fseek(mdb->mediafd, 0L, SEEK_SET);
	 while (fread(&mr, mrlen, 1, mdb->mediafd) > 0) {
	    if (mr.MediaId == jm.MediaId) {
	       Mmsg(mdb->cmd, " %7d  %-10s %10d %10d\n",
		    jm.JobId, mr.VolumeName, jm.FirstIndex, jm.LastIndex);
	       sendit(ctx, mdb->cmd);
	       break;
	    }
	 }
      }
   }

   sendit(ctx, "============================================\n");
   db_unlock(mdb);
   return;
}


/*
 * List Job records
 */
void db_list_job_records(JCR *jcr, B_DB *mdb, JOB_DBR *jr,
			 DB_LIST_HANDLER *sendit, void *ctx)
{
   int jrlen;
   JOB_DBR ojr;
   int done = 0;
   char ewc1[30], ewc2[30];
   char dt[MAX_TIME_LENGTH];
   struct tm tm;

   db_lock(mdb);
   if (!bdb_open_jobs_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   fseek(mdb->jobfd, 0L, SEEK_SET);   /* rewind file */
   /*
    * Linear search through Job records
    */
   sendit(ctx, "   JobId   StartTime   Type Level         Bytes      Files Stat JobName\n");
   sendit(ctx, "==========================================================================\n");
   jrlen = sizeof(ojr);
   while (!done && fread(&ojr, jrlen, 1, mdb->jobfd) > 0) {
      if (jr->JobId != 0) {
	 if (jr->JobId == ojr.JobId) {
	    done = 1;
	 } else {
	    continue;
	 }
      }
      localtime_r(&ojr.StartTime, &tm);
      strftime(dt, sizeof(dt), "%m-%d %H:%M", &tm);
      Mmsg(mdb->cmd, " %7d  %-10s   %c    %c   %14s %10s  %c  %s\n",
		ojr.JobId, dt, (char)ojr.JobType, (char)ojr.JobLevel,
		edit_uint64_with_commas(ojr.JobBytes, ewc1),
		edit_uint64_with_commas(ojr.JobFiles, ewc2),
		(char)ojr.JobStatus, ojr.Name);
      sendit(ctx, mdb->cmd);
   }
   sendit(ctx, "============================================================================\n");
   db_unlock(mdb);
   return;
}


/*
 * List Job Totals
 */
void db_list_job_totals(JCR *jcr, B_DB *mdb, JOB_DBR *jr,
			DB_LIST_HANDLER *sendit, void *ctx)
{
   char ewc1[30], ewc2[30], ewc3[30];
   int jrlen;
   JOB_DBR ojr;
   uint64_t total_bytes = 0;
   uint64_t total_files = 0;
   uint32_t total_jobs = 0;

   db_lock(mdb);
   if (!bdb_open_jobs_file(mdb)) {
      db_unlock(mdb);
      return;
   }
   fseek(mdb->jobfd, 0L, SEEK_SET);   /* rewind file */
   /*
    * Linear search through JobStart records
    */
   sendit(ctx, "   NumJobs   NumFiles          NumBytes\n");
   sendit(ctx, "=======================================\n");
   jrlen = sizeof(ojr);
   while (fread(&ojr, jrlen, 1, mdb->jobfd) > 0) {
      total_files += ojr.JobFiles;
      total_bytes += ojr.JobBytes;
      total_jobs++;
   }
   Mmsg(mdb->cmd, " %7s  %10s   %15s\n",
	     edit_uint64_with_commas(total_jobs, ewc1),
	     edit_uint64_with_commas(total_files, ewc2),
	     edit_uint64_with_commas(total_bytes, ewc3));
   sendit(ctx, mdb->cmd);
   sendit(ctx, "=======================================\n");
   db_unlock(mdb);
   return;
}



void db_list_files_for_job(JCR *jcr, B_DB *mdb, uint32_t jobid, DB_LIST_HANDLER *sendit, void *ctx)
{ }

void db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx)
{ }

int db_list_sql_query(JCR *jcr, B_DB *mdb, char *query, DB_LIST_HANDLER *sendit,
		      void *ctx, int verbose, e_list_type type)
{
   return 0;
}

void
db_list_pool_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void
db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr,
		      DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, uint32_t JobId,
			      DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void
db_list_job_records(JCR *jcr, B_DB *mdb, JOB_DBR *jr, DB_LIST_HANDLER *sendit,
		    void *ctx, e_list_type type)
{ }

void
db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }




#endif /* HAVE_BACULA_DB */
