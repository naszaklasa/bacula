/*
 *
 *   Bacula Director -- User Agent Database prune Command
 *	Applies retention periods
 *
 *     Kern Sibbald, February MMII
 *
 *   Version $Id: ua_prune.c,v 1.36 2004/11/21 13:10:15 kerns Exp $
 */

/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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

#include "bacula.h"
#include "dird.h"

/* Imported functions */
int mark_media_purged(UAContext *ua, MEDIA_DBR *mr);

/* Forward referenced functions */


#define MAX_DEL_LIST_LEN 1000000

/* Imported variables */
extern char *select_job;
extern char *drop_deltabs[];
extern char *create_deltabs[];
extern char *insert_delcand;
extern char *select_backup_del;
extern char *select_verify_del;
extern char *select_restore_del;
extern char *select_admin_del;
extern char *cnt_File;
extern char *del_File;
extern char *upd_Purged;
extern char *cnt_DelCand;
extern char *del_Job;
extern char *del_JobMedia;
extern char *cnt_JobMedia;
extern char *sel_JobMedia;


/* In memory list of JobIds */
struct s_file_del_ctx {
   JobId_t *JobId;
   int num_ids; 		      /* ids stored */
   int max_ids; 		      /* size of array */
   int num_del; 		      /* number deleted */
   int tot_ids; 		      /* total to process */
};

struct s_job_del_ctx {
   JobId_t *JobId;		      /* array of JobIds */
   char *PurgedFiles;		      /* Array of PurgedFile flags */
   int num_ids; 		      /* ids stored */
   int max_ids; 		      /* size of array */
   int num_del; 		      /* number deleted */
   int tot_ids; 		      /* total to process */
};

struct s_count_ctx {
   int count;
};


/*
 * Called here to count entries to be deleted 
 */
static int count_handler(void *ctx, int num_fields, char **row)
{
   struct s_count_ctx *cnt = (struct s_count_ctx *)ctx;

   if (row[0]) {
      cnt->count = atoi(row[0]);
   } else {
      cnt->count = 0;
   }
   return 0;
}


/*
 * Called here to count the number of Jobs to be pruned
 */
static int file_count_handler(void *ctx, int num_fields, char **row)
{
   struct s_file_del_ctx *del = (struct s_file_del_ctx *)ctx;
   del->tot_ids++;
   return 0;
}


/*
 * Called here to make in memory list of JobIds to be
 *  deleted and the associated PurgedFiles flag.
 *  The in memory list will then be transversed
 *  to issue the SQL DELETE commands.  Note, the list
 *  is allowed to get to MAX_DEL_LIST_LEN to limit the
 *  maximum malloc'ed memory.
 */
static int job_delete_handler(void *ctx, int num_fields, char **row)
{
   struct s_job_del_ctx *del = (struct s_job_del_ctx *)ctx;

   if (del->num_ids == MAX_DEL_LIST_LEN) {  
      return 1;
   }
   if (del->num_ids == del->max_ids) {
      del->max_ids = (del->max_ids * 3) / 2;
      del->JobId = (JobId_t *)brealloc(del->JobId, sizeof(JobId_t) * del->max_ids);
      del->PurgedFiles = (char *)brealloc(del->PurgedFiles, del->max_ids);
   }
   del->JobId[del->num_ids] = (JobId_t)str_to_int64(row[0]);
   del->PurgedFiles[del->num_ids++] = (char)str_to_int64(row[0]);
   return 0;
}

static int file_delete_handler(void *ctx, int num_fields, char **row)
{
   struct s_file_del_ctx *del = (struct s_file_del_ctx *)ctx;

   if (del->num_ids == MAX_DEL_LIST_LEN) {  
      return 1;
   }
   if (del->num_ids == del->max_ids) {
      del->max_ids = (del->max_ids * 3) / 2;
      del->JobId = (JobId_t *)brealloc(del->JobId, sizeof(JobId_t) *
	 del->max_ids);
   }
   del->JobId[del->num_ids++] = (JobId_t)str_to_int64(row[0]);
   return 0;
}

/*
 *   Prune records from database
 *
 *    prune files (from) client=xxx
 *    prune jobs (from) client=xxx
 *    prune volume=xxx	
 */
int prunecmd(UAContext *ua, const char *cmd)
{
   CLIENT *client;
   POOL_DBR pr;
   MEDIA_DBR mr;
   int kw;

   static const char *keywords[] = {
      N_("Files"),
      N_("Jobs"),
      N_("Volume"),
      NULL};

   if (!open_db(ua)) {
      return 01;
   }

   /* First search args */
   kw = find_arg_keyword(ua, keywords);  
   if (kw < 0 || kw > 2) {
      /* no args, so ask user */
      kw = do_keyword_prompt(ua, _("Choose item to prune"), keywords);  
   }	   
    
   switch (kw) {
   case 0:  /* prune files */
      client = get_client_resource(ua);
      if (!client || !confirm_retention(ua, &client->FileRetention, "File")) {
	 return 0;
      }
      prune_files(ua, client);
      return 1;
   case 1:  /* prune jobs */
      client = get_client_resource(ua);
      if (!client || !confirm_retention(ua, &client->JobRetention, "Job")) {
	 return 0;
      }
      /* ****FIXME**** allow user to select JobType */
      prune_jobs(ua, client, JT_BACKUP);
      return 1;
   case 2:  /* prune volume */
      if (!select_pool_and_media_dbr(ua, &pr, &mr)) {
	 return 0;
      }
      if (!confirm_retention(ua, &mr.VolRetention, "Volume")) {
	 return 0;
      }
      prune_volume(ua, &mr);
      return 1;
   default:
      break;
   }

   return 1;
}

/*
 * Prune File records from the database. For any Job which
 * is older than the retention period, we unconditionally delete
 * all File records for that Job.  This is simple enough that no
 * temporary tables are needed. We simply make an in memory list of
 * the JobIds meeting the prune conditions, then delete all File records
 * pointing to each of those JobIds.
 *
 * This routine assumes you want the pruning to be done. All checking
 *  must be done before calling this routine.
 */
int prune_files(UAContext *ua, CLIENT *client)
{
   struct s_file_del_ctx del;
   POOLMEM *query = get_pool_memory(PM_MESSAGE);
   int i;
   utime_t now, period;
   CLIENT_DBR cr;
   char ed1[50], ed2[50];

   db_lock(ua->db);
   memset(&cr, 0, sizeof(cr));
   memset(&del, 0, sizeof(del));
   bstrncpy(cr.Name, client->hdr.name, sizeof(cr.Name));
   if (!db_create_client_record(ua->jcr, ua->db, &cr)) {
      db_unlock(ua->db);
      return 0;
   }

   period = client->FileRetention;
   now = (utime_t)time(NULL);
       
   /* Select Jobs -- for counting */
   Mmsg(query, select_job, edit_uint64(now - period, ed1), cr.ClientId);
   Dmsg1(050, "select sql=%s\n", query);
   if (!db_sql_query(ua->db, query, file_count_handler, (void *)&del)) {
      if (ua->verbose) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }
      
   if (del.tot_ids == 0) {
      if (ua->verbose) {
         bsendmsg(ua, _("No Files found to prune.\n"));
      }
      goto bail_out;
   }

   if (del.tot_ids < MAX_DEL_LIST_LEN) {
      del.max_ids = del.tot_ids + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN; 
   }
   del.tot_ids = 0;

   del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);

   /* Now process same set but making a delete list */
   db_sql_query(ua->db, query, file_delete_handler, (void *)&del);

   for (i=0; i < del.num_ids; i++) {
      struct s_count_ctx cnt;
      Dmsg1(050, "Delete JobId=%d\n", del.JobId[i]);
      Mmsg(query, cnt_File, del.JobId[i]);
      cnt.count = 0;
      db_sql_query(ua->db, query, count_handler, (void *)&cnt);
      del.tot_ids += cnt.count;
      Mmsg(query, del_File, del.JobId[i]);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      /* 
       * Now mark Job as having files purged. This is necessary to
       * avoid having too many Jobs to process in future prunings. If
       * we don't do this, the number of JobId's in our in memory list
       * could grow very large.
       */
      Mmsg(query, upd_Purged, del.JobId[i]);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);
   }
   edit_uint64_with_commas(del.tot_ids, ed1);
   edit_uint64_with_commas(del.num_ids, ed2);
   bsendmsg(ua, _("Pruned %s Files from %s Jobs for client %s from catalog.\n"), 
      ed1, ed2, client->hdr.name);
   
bail_out:
   db_unlock(ua->db);
   if (del.JobId) {
      free(del.JobId);
   }
   free_pool_memory(query);
   return 1;
}


static void drop_temp_tables(UAContext *ua) 
{
   int i;
   for (i=0; drop_deltabs[i]; i++) {
      db_sql_query(ua->db, drop_deltabs[i], NULL, (void *)NULL);
   }
}

static int create_temp_tables(UAContext *ua) 
{
   int i;
   /* Create temp tables and indicies */
   for (i=0; create_deltabs[i]; i++) {
      if (!db_sql_query(ua->db, create_deltabs[i], NULL, (void *)NULL)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
         Dmsg0(050, "create DelTables table failed\n");
	 return 0;
      }
   }
   return 1;
}



/*
 * Purging Jobs is a bit more complicated than purging Files
 * because we delete Job records only if there is a more current
 * backup of the FileSet. Otherwise, we keep the Job record.
 * In other words, we never delete the only Job record that
 * contains a current backup of a FileSet. This prevents the
 * Volume from being recycled and destroying a current backup.
 *
 * For Verify Jobs, we do not delete the last InitCatalog.
 *
 * For Restore Jobs there are no restrictions.
 */
int prune_jobs(UAContext *ua, CLIENT *client, int JobType)
{
   struct s_job_del_ctx del;
   struct s_count_ctx cnt;
   POOLMEM *query = (char *)get_pool_memory(PM_MESSAGE);
   int i;
   utime_t now, period;
   CLIENT_DBR cr;
   char ed1[50];

   db_lock(ua->db);
   memset(&cr, 0, sizeof(cr));
   memset(&del, 0, sizeof(del));
   bstrncpy(cr.Name, client->hdr.name, sizeof(cr.Name));
   if (!db_create_client_record(ua->jcr, ua->db, &cr)) {
      db_unlock(ua->db);
      return 0;
   }

   period = client->JobRetention;
   now = (utime_t)time(NULL);

   /* Drop any previous temporary tables still there */
   drop_temp_tables(ua);

   /* Create temp tables and indicies */
   if (!create_temp_tables(ua)) {
      goto bail_out;
   }

   /* 
    * Select all files that are older than the JobRetention period
    *  and stuff them into the "DeletionCandidates" table.
    */
   edit_uint64(now - period, ed1);
   Mmsg(query, insert_delcand, (char)JobType, ed1, cr.ClientId);
   if (!db_sql_query(ua->db, query, NULL, (void *)NULL)) {
      if (ua->verbose) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      Dmsg0(050, "insert delcand failed\n");
      goto bail_out;
   }

   /* Count Files to be deleted */
   pm_strcpy(query, cnt_DelCand);
   Dmsg1(100, "select sql=%s\n", query);
   cnt.count = 0;
   if (!db_sql_query(ua->db, query, count_handler, (void *)&cnt)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }
      
   if (cnt.count == 0) {
      if (ua->verbose) {
         bsendmsg(ua, _("No Jobs found to prune.\n"));
      }
      goto bail_out;
   }

   if (cnt.count < MAX_DEL_LIST_LEN) {
      del.max_ids = cnt.count + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN; 
   }
   del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);
   del.PurgedFiles = (char *)malloc(del.max_ids);

   switch (JobType) {
   case JT_BACKUP:
      Mmsg(query, select_backup_del, ed1, ed1, cr.ClientId);
      break;
   case JT_RESTORE:
      Mmsg(query, select_restore_del, ed1, ed1, cr.ClientId);
      break;
   case JT_VERIFY:
      Mmsg(query, select_verify_del, ed1, ed1, cr.ClientId);
      break;
   case JT_ADMIN:
      Mmsg(query, select_admin_del, ed1, ed1, cr.ClientId);
      break;
   }
   if (!db_sql_query(ua->db, query, job_delete_handler, (void *)&del)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   }

   /* 
    * OK, now we have the list of JobId's to be pruned, first check
    * if the Files have been purged, if not, purge (delete) them.
    * Then delete the Job entry, and finally and JobMedia records.
    */
   for (i=0; i < del.num_ids; i++) {
      Dmsg1(050, "Delete JobId=%d\n", del.JobId[i]);
      if (!del.PurgedFiles[i]) {
	 Mmsg(query, del_File, del.JobId[i]);
	 if (!db_sql_query(ua->db, query, NULL, (void *)NULL)) {
            bsendmsg(ua, "%s", db_strerror(ua->db));
	 }
         Dmsg1(050, "Del sql=%s\n", query);
      }

      Mmsg(query, del_Job, del.JobId[i]);
      if (!db_sql_query(ua->db, query, NULL, (void *)NULL)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      Dmsg1(050, "Del sql=%s\n", query);

      Mmsg(query, del_JobMedia, del.JobId[i]);
      if (!db_sql_query(ua->db, query, NULL, (void *)NULL)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      Dmsg1(050, "Del sql=%s\n", query);
   }
   bsendmsg(ua, _("Pruned %d %s for client %s from catalog.\n"), del.num_ids,
      del.num_ids==1?_("Job"):_("Jobs"), client->hdr.name);
   
bail_out:
   drop_temp_tables(ua);
   db_unlock(ua->db);
   if (del.JobId) {
      free(del.JobId);
   }
   if (del.PurgedFiles) {
      free(del.PurgedFiles);
   }
   free_pool_memory(query);
   return 1;
}

/*
 * Prune a given Volume
 */
int prune_volume(UAContext *ua, MEDIA_DBR *mr)
{
   POOLMEM *query = (char *)get_pool_memory(PM_MESSAGE);
   struct s_count_ctx cnt;
   struct s_file_del_ctx del;
   int i, stat = 0;
   JOB_DBR jr;
   utime_t now, period;

   db_lock(ua->db);
   memset(&jr, 0, sizeof(jr));
   memset(&del, 0, sizeof(del));

   /*
    * Find out how many Jobs remain on this Volume by 
    *  counting the JobMedia records.
    */
   cnt.count = 0;
   Mmsg(query, cnt_JobMedia, mr->MediaId);
   if (!db_sql_query(ua->db, query, count_handler, (void *)&cnt)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }
      
   if (cnt.count == 0) {
      if (strcmp(mr->VolStatus, "Purged") != 0 && verbose) {
         bsendmsg(ua, "There are no Jobs associated with Volume \"%s\". Marking it purged.\n",
	    mr->VolumeName);
      }
      stat = mark_media_purged(ua, mr);
      goto bail_out;
   }

   if (cnt.count < MAX_DEL_LIST_LEN) {
      del.max_ids = cnt.count + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN; 
   }

   /* 
    * Now get a list of JobIds for Jobs written to this Volume
    *	Could optimize here by adding JobTDate > (now - period).
    */
   del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);
   Mmsg(query, sel_JobMedia, mr->MediaId);
   if (!db_sql_query(ua->db, query, file_delete_handler, (void *)&del)) {
      if (ua->verbose) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }

   /* Use Volume Retention to prune Jobs and their Files */
   period = mr->VolRetention;
   now = (utime_t)time(NULL);

   Dmsg3(200, "Now=%d period=%d now-period=%d\n", (int)now, (int)period,
      (int)(now-period));

   for (i=0; i < del.num_ids; i++) {
      jr.JobId = del.JobId[i];
      if (!db_get_job_record(ua->jcr, ua->db, &jr)) {
	 continue;
      }
      Dmsg2(200, "Looking at %s JobTdate=%d\n", jr.Job, (int)jr.JobTDate);
      if (jr.JobTDate >= (now - period)) {
	 continue;
      }
      Dmsg2(200, "Delete JobId=%d Job=%s\n", del.JobId[i], jr.Job);
      Mmsg(query, del_File, del.JobId[i]);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Mmsg(query, del_Job, del.JobId[i]);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Mmsg(query, del_JobMedia, del.JobId[i]);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);
      del.num_del++;
   }
   if (del.JobId) {
      free(del.JobId);
   }
   if (ua->verbose && del.num_del != 0) {
      bsendmsg(ua, _("Pruned %d %s on Volume \"%s\" from catalog.\n"), del.num_del,
         del.num_del == 1 ? "Job" : "Jobs", mr->VolumeName);
   }

   /* If purged, mark it so */
   if (del.num_ids == del.num_del) {
      Dmsg0(200, "Volume is purged.\n");
      stat = mark_media_purged(ua, mr);
   }

bail_out:
   db_unlock(ua->db);
   free_pool_memory(query);
   return stat;
}
