/*
 *
 *   Bacula Director -- User Agent Database Purge Command
 *
 *      Purges Files from specific JobIds
 * or
 *      Purges Jobs from Volumes
 *
 *     Kern Sibbald, February MMII
 *
 *   Version $Id: ua_purge.c,v 1.28 2005/08/15 07:25:10 kerns Exp $
 */
/*
   Copyright (C) 2002-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "dird.h"

/* Forward referenced functions */
static int purge_files_from_client(UAContext *ua, CLIENT *client);
static int purge_jobs_from_client(UAContext *ua, CLIENT *client);

#define MAX_DEL_LIST_LEN 1000000

static const char *select_jobsfiles_from_client =
   "SELECT JobId FROM Job "
   "WHERE ClientId=%s "
   "AND PurgedFiles=0";

static const char *select_jobs_from_client =
   "SELECT JobId, PurgedFiles FROM Job "
   "WHERE ClientId=%s";


/* In memory list of JobIds */
struct s_file_del_ctx {
   JobId_t *JobId;
   int num_ids;                       /* ids stored */
   int max_ids;                       /* size of array */
   int num_del;                       /* number deleted */
   int tot_ids;                       /* total to process */
};

struct s_job_del_ctx {
   JobId_t *JobId;                    /* array of JobIds */
   char *PurgedFiles;                 /* Array of PurgedFile flags */
   int num_ids;                       /* ids stored */
   int max_ids;                       /* size of array */
   int num_del;                       /* number deleted */
   int tot_ids;                       /* total to process */
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
      cnt->count = str_to_int64(row[0]);
   } else {
      cnt->count = 0;
   }
   return 0;
}

/*
 * Called here to count entries to be deleted
 */
static int file_count_handler(void *ctx, int num_fields, char **row)
{
   struct s_file_del_ctx *del = (struct s_file_del_ctx *)ctx;
   del->tot_ids++;
   return 0;
}


static int job_count_handler(void *ctx, int num_fields, char **row)
{
   struct s_job_del_ctx *del = (struct s_job_del_ctx *)ctx;
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
 *   Purge records from database
 *
 *     Purge Files (from) [Job|JobId|Client|Volume]
 *     Purge Jobs  (from) [Client|Volume]
 *
 *  N.B. Not all above is implemented yet.
 */
int purgecmd(UAContext *ua, const char *cmd)
{
   int i;
   CLIENT *client;
   MEDIA_DBR mr;
   JOB_DBR  jr;
   static const char *keywords[] = {
      N_("files"),
      N_("jobs"),
      N_("volume"),
      NULL};

   static const char *files_keywords[] = {
      N_("Job"),
      N_("JobId"),
      N_("Client"),
      N_("Volume"),
      NULL};

   static const char *jobs_keywords[] = {
      N_("Client"),
      N_("Volume"),
      NULL};

   bsendmsg(ua, _(
      "\nThis command is can be DANGEROUS!!!\n\n"
      "It purges (deletes) all Files from a Job,\n"
      "JobId, Client or Volume; or it purges (deletes)\n"
      "all Jobs from a Client or Volume without regard\n"
      "for retention periods. Normally you should use the\n"
      "PRUNE command, which respects retention periods.\n"));

   if (!open_db(ua)) {
      return 1;
   }
   switch (find_arg_keyword(ua, keywords)) {
   /* Files */
   case 0:
      switch(find_arg_keyword(ua, files_keywords)) {
      case 0:                         /* Job */
      case 1:                         /* JobId */
         if (get_job_dbr(ua, &jr)) {
            purge_files_from_job(ua, &jr);
         }
         return 1;
      case 2:                         /* client */
         client = get_client_resource(ua);
         if (client) {
            purge_files_from_client(ua, client);
         }
         return 1;
      case 3:                         /* Volume */
         if (select_media_dbr(ua, &mr)) {
            purge_files_from_volume(ua, &mr);
         }
         return 1;
      }
   /* Jobs */
   case 1:
      switch(find_arg_keyword(ua, jobs_keywords)) {
      case 0:                         /* client */
         client = get_client_resource(ua);
         if (client) {
            purge_jobs_from_client(ua, client);
         }
         return 1;
      case 1:                         /* Volume */
         if (select_media_dbr(ua, &mr)) {
            purge_jobs_from_volume(ua, &mr);
         }
         return 1;
      }
   /* Volume */
   case 2:
      while ((i=find_arg(ua, N_("volume"))) >= 0) {
         if (select_media_dbr(ua, &mr)) {
            purge_jobs_from_volume(ua, &mr);
         }
         *ua->argk[i] = 0;            /* zap keyword already seen */
         bsendmsg(ua, "\n");
      }
      return 1;
   default:
      break;
   }
   switch (do_keyword_prompt(ua, _("Choose item to purge"), keywords)) {
   case 0:                            /* files */
      client = get_client_resource(ua);
      if (client) {
         purge_files_from_client(ua, client);
      }
      break;
   case 1:                            /* jobs */
      client = get_client_resource(ua);
      if (client) {
         purge_jobs_from_client(ua, client);
      }
      break;
   case 2:                            /* Volume */
      if (select_media_dbr(ua, &mr)) {
         purge_jobs_from_volume(ua, &mr);
      }
      break;
   }
   return 1;
}

/*
 * Purge File records from the database. For any Job which
 * is older than the retention period, we unconditionally delete
 * all File records for that Job.  This is simple enough that no
 * temporary tables are needed. We simply make an in memory list of
 * the JobIds meeting the prune conditions, then delete all File records
 * pointing to each of those JobIds.
 */
static int purge_files_from_client(UAContext *ua, CLIENT *client)
{
   struct s_file_del_ctx del;
   char *query = (char *)get_pool_memory(PM_MESSAGE);
   int i;
   CLIENT_DBR cr;
   char ed1[50];

   memset(&cr, 0, sizeof(cr));
   memset(&del, 0, sizeof(del));

   bstrncpy(cr.Name, client->hdr.name, sizeof(cr.Name));
   if (!db_create_client_record(ua->jcr, ua->db, &cr)) {
      return 0;
   }
   bsendmsg(ua, _("Begin purging files for Client \"%s\"\n"), cr.Name);
   Mmsg(query, select_jobsfiles_from_client, edit_int64(cr.ClientId, ed1));

   Dmsg1(050, "select sql=%s\n", query);

   if (!db_sql_query(ua->db, query, file_count_handler, (void *)&del)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }

   if (del.tot_ids == 0) {
      bsendmsg(ua, _("No Files found for client %s to purge from %s catalog.\n"),
         client->hdr.name, client->catalog->hdr.name);
      goto bail_out;
   }

   if (del.tot_ids < MAX_DEL_LIST_LEN) {
      del.max_ids = del.tot_ids + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN;
   }
   del.tot_ids = 0;

   del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);

   db_sql_query(ua->db, query, file_delete_handler, (void *)&del);

   for (i=0; i < del.num_ids; i++) {
      edit_int64(del.JobId[i], ed1);
      Dmsg1(050, "Delete JobId=%s\n", ed1);
      Mmsg(query, "DELETE FROM File WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      /*
       * Now mark Job as having files purged. This is necessary to
       * avoid having too many Jobs to process in future prunings. If
       * we don't do this, the number of JobId's in our in memory list
       * will grow very large.
       */
      Mmsg(query, "UPDATE Job Set PurgedFiles=1 WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);
   }
   bsendmsg(ua, _("%d Files for client \"%s\" purged from %s catalog.\n"), del.num_ids,
      client->hdr.name, client->catalog->hdr.name);

bail_out:
   if (del.JobId) {
      free(del.JobId);
   }
   free_pool_memory(query);
   return 1;
}



/*
 * Purge Job records from the database. For any Job which
 * is older than the retention period, we unconditionally delete
 * it and all File records for that Job.  This is simple enough that no
 * temporary tables are needed. We simply make an in memory list of
 * the JobIds meeting the prune conditions, then delete the Job,
 * Files, and JobMedia records in that list.
 */
static int purge_jobs_from_client(UAContext *ua, CLIENT *client)
{
   struct s_job_del_ctx del;
   char *query = (char *)get_pool_memory(PM_MESSAGE);
   int i;
   CLIENT_DBR cr;
   char ed1[50];

   memset(&cr, 0, sizeof(cr));
   memset(&del, 0, sizeof(del));

   bstrncpy(cr.Name, client->hdr.name, sizeof(cr.Name));
   if (!db_create_client_record(ua->jcr, ua->db, &cr)) {
      return 0;
   }

   bsendmsg(ua, _("Begin purging jobs from Client \"%s\"\n"), cr.Name);
   Mmsg(query, select_jobs_from_client, edit_int64(cr.ClientId, ed1));

   Dmsg1(050, "select sql=%s\n", query);

   if (!db_sql_query(ua->db, query, job_count_handler, (void *)&del)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }
   if (del.tot_ids == 0) {
      bsendmsg(ua, _("No Jobs found for client %s to purge from %s catalog.\n"),
         client->hdr.name, client->catalog->hdr.name);
      goto bail_out;
   }

   if (del.tot_ids < MAX_DEL_LIST_LEN) {
      del.max_ids = del.tot_ids + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN;
   }

   del.tot_ids = 0;

   del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);
   del.PurgedFiles = (char *)malloc(del.max_ids);

   db_sql_query(ua->db, query, job_delete_handler, (void *)&del);

   /*
    * OK, now we have the list of JobId's to be purged, first check
    * if the Files have been purged, if not, purge (delete) them.
    * Then delete the Job entry, and finally and JobMedia records.
    */
   for (i=0; i < del.num_ids; i++) {
      edit_int64(del.JobId[i], ed1);
      Dmsg1(050, "Delete JobId=%s\n", ed1); 
      if (!del.PurgedFiles[i]) {
         Mmsg(query, "DELETE FROM File WHERE JobId=%s", ed1);
         db_sql_query(ua->db, query, NULL, (void *)NULL);
         Dmsg1(050, "Del sql=%s\n", query);
      }

      Mmsg(query, "DELETE FROM Job WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);

      Mmsg(query, "DELETE FROM JobMedia WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);
   }
   bsendmsg(ua, _("%d Jobs for client %s purged from %s catalog.\n"), del.num_ids,
      client->hdr.name, client->catalog->hdr.name);

bail_out:
   if (del.JobId) {
      free(del.JobId);
   }
   if (del.PurgedFiles) {
      free(del.PurgedFiles);
   }
   free_pool_memory(query);
   return 1;
}

void purge_files_from_job(UAContext *ua, JOB_DBR *jr)
{
   char *query = (char *)get_pool_memory(PM_MESSAGE);
   char ed1[50];

   edit_int64(jr->JobId,ed1);
   Mmsg(query, "DELETE FROM File WHERE JobId=%s", ed1);
   db_sql_query(ua->db, query, NULL, (void *)NULL);

   Mmsg(query, "UPDATE Job Set PurgedFiles=1 WHERE JobId=%s", ed1);
   db_sql_query(ua->db, query, NULL, (void *)NULL);

   free_pool_memory(query);
}

void purge_files_from_volume(UAContext *ua, MEDIA_DBR *mr )
{} /* ***FIXME*** implement */

/*
 * Returns: 1 if Volume purged
 *          0 if Volume not purged
 */
int purge_jobs_from_volume(UAContext *ua, MEDIA_DBR *mr)
{
   char *query = (char *)get_pool_memory(PM_MESSAGE);
   struct s_count_ctx cnt;
   struct s_file_del_ctx del;
   int i, stat = 0;
   JOB_DBR jr;
   char ed1[50];

   stat = strcmp(mr->VolStatus, "Append") == 0 ||
          strcmp(mr->VolStatus, "Full")   == 0 ||
          strcmp(mr->VolStatus, "Used")   == 0 ||
          strcmp(mr->VolStatus, "Error")  == 0;
   if (!stat) {
      bsendmsg(ua, "\n");
      bsendmsg(ua, _("Volume \"%s\" has VolStatus \"%s\" and cannot be purged.\n"
                     "The VolStatus must be: Append, Full, Used, or Error to be purged.\n"),
                     mr->VolumeName, mr->VolStatus);
      goto bail_out;
   }

   memset(&jr, 0, sizeof(jr));
   memset(&del, 0, sizeof(del));
   cnt.count = 0;
   Mmsg(query, "SELECT count(*) FROM JobMedia WHERE MediaId=%s", 
        edit_int64(mr->MediaId, ed1));
   if (!db_sql_query(ua->db, query, count_handler, (void *)&cnt)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }

   if (cnt.count == 0) {
      bsendmsg(ua, _("There are no Jobs associated with Volume \"%s\". Marking it purged.\n"),
         mr->VolumeName);
      if (!mark_media_purged(ua, mr)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
         goto bail_out;
      }
      goto bail_out;
   }

   if (cnt.count < MAX_DEL_LIST_LEN) {
      del.max_ids = cnt.count + 1;
   } else {
      del.max_ids = MAX_DEL_LIST_LEN;
   }

   /*
    * Check if he wants to purge a single jobid
    */
   i = find_arg_with_value(ua, "jobid");
   if (i >= 0) {
      del.JobId = (JobId_t *)malloc(sizeof(JobId_t));
      del.num_ids = 1;
      del.JobId[0] = str_to_int64(ua->argv[i]);
   } else {
      /*
       * Purge ALL JobIds
       */
      del.JobId = (JobId_t *)malloc(sizeof(JobId_t) * del.max_ids);

      Mmsg(query, "SELECT JobId FROM JobMedia WHERE MediaId=%s", 
           edit_int64(mr->MediaId, ed1));
      if (!db_sql_query(ua->db, query, file_delete_handler, (void *)&del)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
         Dmsg0(050, "Count failed\n");
         goto bail_out;
      }
   }

   for (i=0; i < del.num_ids; i++) {
      edit_int64(del.JobId[i], ed1);
      Dmsg1(050, "Delete JobId=%s\n", ed1);
      Mmsg(query, "DELETE FROM File WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Mmsg(query, "DELETE FROM Job WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Mmsg(query, "DELETE FROM JobMedia WHERE JobId=%s", ed1);
      db_sql_query(ua->db, query, NULL, (void *)NULL);
      Dmsg1(050, "Del sql=%s\n", query);
      del.num_del++;
   }
   if (del.JobId) {
      free(del.JobId);
   }
   bsendmsg(ua, _("%d File%s on Volume \"%s\" purged from catalog.\n"), del.num_del,
      del.num_del==1?"":"s", mr->VolumeName);

   /* If purged, mark it so */
   cnt.count = 0;
   Mmsg(query, "SELECT count(*) FROM JobMedia WHERE MediaId=%s", 
        edit_int64(mr->MediaId, ed1));
   if (!db_sql_query(ua->db, query, count_handler, (void *)&cnt)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      Dmsg0(050, "Count failed\n");
      goto bail_out;
   }

   if (cnt.count == 0) {
      bsendmsg(ua, _("There are no more Jobs associated with Volume \"%s\". Marking it purged.\n"),
         mr->VolumeName);
      if (!(stat = mark_media_purged(ua, mr))) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
         goto bail_out;
      }
   }

bail_out:
   free_pool_memory(query);
   return stat;
}

/*
 * IF volume status is Append, Full, Used, or Error, mark it Purged
 *   Purged volumes can then be recycled (if enabled).
 */
bool mark_media_purged(UAContext *ua, MEDIA_DBR *mr)
{
   JCR *jcr = ua->jcr;
   if (strcmp(mr->VolStatus, "Append") == 0 ||
       strcmp(mr->VolStatus, "Full")   == 0 ||
       strcmp(mr->VolStatus, "Used")   == 0 ||
       strcmp(mr->VolStatus, "Error")  == 0) {
      bstrncpy(mr->VolStatus, "Purged", sizeof(mr->VolStatus));
      if (!db_update_media_record(jcr, ua->db, mr)) {
         return false;
      }
      pm_strcpy(jcr->VolumeName, mr->VolumeName);
      generate_job_event(jcr, "VolumePurged");
      return true;
   } else {
      bsendmsg(ua, _("Cannot purge Volume with VolStatus=%s\n"), mr->VolStatus);
   }
   return strcpy(mr->VolStatus, "Purged") == 0;
}
