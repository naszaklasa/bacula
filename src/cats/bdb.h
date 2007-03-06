/*
 * Definitions common to the Bacula Database Routines (bdb).
 */

/* bdb.c */
extern int bdb_open_jobs_file(B_DB *mdb);
extern int bdb_write_control_file(B_DB *mdb);
extern int bdb_open_jobmedia_file(B_DB *mdb);
extern int bdb_open_pools_file(B_DB *mdb);
extern int bdb_open_media_file(B_DB *mdb);
extern int bdb_open_client_file(B_DB *mdb);
extern int bdb_open_fileset_file(B_DB *mdb);
extern int db_get_media_record(B_DB *mdb, MEDIA_DBR *mr);
