/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2009 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *  Database routines that are exported by the cats library for
 *    use elsewhere in Bacula (mainly the Director).
 *
 *    Version $Id$
 */

#ifndef __SQL_PROTOS_H
#define __SQL_PROTOS_H

#include "cats.h"

/* Database prototypes */

/* sql.c */
B_DB *db_init(JCR *jcr, const char *db_driver, const char *db_name, const char *db_user, 
              const char *db_password, const char *db_address, int db_port, 
              const char *db_socket, int mult_db_connections);
B_DB *db_init_database(JCR *jcr, const char *db_name, const char *db_user, const char *db_password,
                       const char *db_address, int db_port, const char *db_socket,
                       int mult_db_connections);
int  db_open_database(JCR *jcr, B_DB *db);
void db_close_database(JCR *jcr, B_DB *db);
bool db_open_batch_connexion(JCR *jcr, B_DB *mdb);
void db_escape_string(JCR *jcr, B_DB *db, char *snew, char *old, int len);
char *db_strerror(B_DB *mdb);
int  db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index);
bool db_sql_query(B_DB *mdb, const char *cmd, DB_RESULT_HANDLER *result_handler, void *ctx);
void db_start_transaction(JCR *jcr, B_DB *mdb);
void db_end_transaction(JCR *jcr, B_DB *mdb);
int db_int64_handler(void *ctx, int num_fields, char **row);
int db_list_handler(void *ctx, int num_fields, char **row);
void db_thread_cleanup();
void _dbg_print_db(JCR *jcr, FILE *fp);

/* sql_create.c */
bool db_create_file_attributes_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);
bool db_create_job_record(JCR *jcr, B_DB *db, JOB_DBR *jr);
int db_create_media_record(JCR *jcr, B_DB *db, MEDIA_DBR *media_dbr);
int db_create_client_record(JCR *jcr, B_DB *db, CLIENT_DBR *cr);
bool db_create_fileset_record(JCR *jcr, B_DB *db, FILESET_DBR *fsr);
bool db_create_pool_record(JCR *jcr, B_DB *db, POOL_DBR *pool_dbr);
bool db_create_jobmedia_record(JCR *jcr, B_DB *mdb, JOBMEDIA_DBR *jr);
int db_create_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr);
bool db_create_device_record(JCR *jcr, B_DB *mdb, DEVICE_DBR *dr);
bool db_create_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *sr);
bool db_create_mediatype_record(JCR *jcr, B_DB *mdb, MEDIATYPE_DBR *mr);
bool db_write_batch_file_records(JCR *jcr);
bool my_batch_start(JCR *jcr, B_DB *mdb);
bool my_batch_end(JCR *jcr, B_DB *mdb, const char *error);
bool my_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);

/* sql_delete.c */
int db_delete_pool_record(JCR *jcr, B_DB *db, POOL_DBR *pool_dbr);
int db_delete_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr);

/* sql_find.c */
bool db_find_last_job_start_time(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM **stime, int JobLevel);
bool db_find_job_start_time(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM **stime);
bool db_find_last_jobid(JCR *jcr, B_DB *mdb, const char *Name, JOB_DBR *jr);
int db_find_next_volume(JCR *jcr, B_DB *mdb, int index, bool InChanger, MEDIA_DBR *mr);
bool db_find_failed_job_since(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM *stime, int &JobLevel);

/* sql_get.c */
bool db_get_pool_record(JCR *jcr, B_DB *db, POOL_DBR *pdbr);
int db_get_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr);
bool db_get_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr);
int db_get_job_volume_names(JCR *jcr, B_DB *mdb, JobId_t JobId, POOLMEM **VolumeNames);
int db_get_file_attributes_record(JCR *jcr, B_DB *mdb, char *fname, JOB_DBR *jr, FILE_DBR *fdbr);
int db_get_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr);
bool db_get_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr);
int db_get_num_media_records(JCR *jcr, B_DB *mdb);
int db_get_num_pool_records(JCR *jcr, B_DB *mdb);
int db_get_pool_ids(JCR *jcr, B_DB *mdb, int *num_ids, DBId_t **ids);
int db_get_client_ids(JCR *jcr, B_DB *mdb, int *num_ids, DBId_t **ids);
bool db_get_media_ids(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr, int *num_ids, uint32_t **ids);
int db_get_job_volume_parameters(JCR *jcr, B_DB *mdb, JobId_t JobId, VOL_PARAMS **VolParams);
int db_get_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cdbr);
int db_get_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr);
bool db_get_query_dbids(JCR *jcr, B_DB *mdb, POOL_MEM &query, dbid_list &ids);
bool db_get_file_list(JCR *jcr, B_DB *mdb, char *jobids, DB_RESULT_HANDLER *result_handler, void *ctx);
bool db_accurate_get_jobids(JCR *jcr, B_DB *mdb, JOB_DBR *jr, db_list_ctx *jobids);
int db_get_int_handler(void *list, int num_fields, char **row);


/* sql_list.c */
enum e_list_type {
   HORZ_LIST,
   VERT_LIST
};
void db_list_pool_records(JCR *jcr, B_DB *db, POOL_DBR *pr, DB_LIST_HANDLER sendit, void *ctx, e_list_type type);
void db_list_job_records(JCR *jcr, B_DB *db, JOB_DBR *jr, DB_LIST_HANDLER sendit, void *ctx, e_list_type type);
void db_list_job_totals(JCR *jcr, B_DB *db, JOB_DBR *jr, DB_LIST_HANDLER sendit, void *ctx);
void db_list_files_for_job(JCR *jcr, B_DB *db, uint32_t jobid, DB_LIST_HANDLER sendit, void *ctx);
void db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);
void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, JobId_t JobId, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);
void db_list_joblog_records(JCR *jcr, B_DB *mdb, JobId_t JobId, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);
int  db_list_sql_query(JCR *jcr, B_DB *mdb, const char *query, DB_LIST_HANDLER *sendit, void *ctx, int verbose, e_list_type type);
void db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);
void db_list_copies_records(JCR *jcr, B_DB *mdb, uint32_t limit, char *jobids, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type);

/* sql_update.c */
bool db_update_job_start_record(JCR *jcr, B_DB *db, JOB_DBR *jr);
int  db_update_job_end_record(JCR *jcr, B_DB *db, JOB_DBR *jr);
int  db_update_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr);
int  db_update_pool_record(JCR *jcr, B_DB *db, POOL_DBR *pr);
bool db_update_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *sr);
int  db_update_media_record(JCR *jcr, B_DB *db, MEDIA_DBR *mr);
int  db_update_media_defaults(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr);
int  db_update_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr);
int  db_add_digest_to_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, char *digest, int type);
int  db_mark_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, JobId_t JobId);
void db_make_inchanger_unique(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr);
int db_update_stats(JCR *jcr, B_DB *mdb, utime_t age);

#endif /* __SQL_PROTOS_H */
