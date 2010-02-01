/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2008 Free Software Foundation Europe e.V.

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
 * Bacula Catalog Database interface routines
 *
 *     Almost generic set of SQL database interface routines
 *      (with a little more work)
 *     SQL engine specific routines are in mysql.c, postgresql.c,
 *       sqlite.c, ...
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id$
 */

/* The following is necessary so that we do not include
 * the dummy external definition of B_DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#if    HAVE_SQLITE3 || HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL || HAVE_DBI

uint32_t bacula_db_version = 0;

int db_type = -1;        /* SQL engine type index */

/* Forward referenced subroutines */
void print_dashes(B_DB *mdb);
void print_result(B_DB *mdb);

B_DB *db_init(JCR *jcr, const char *db_driver, const char *db_name, const char *db_user,
              const char *db_password, const char *db_address, int db_port,
              const char *db_socket, int mult_db_connections)
{
#ifdef HAVE_DBI
   char *p;
   if (!db_driver) {
      Jmsg0(jcr, M_ABORT, 0, _("Driver type not specified in Catalog resource.\n"));
   }
   if (strlen(db_driver) < 5 || db_driver[3] != ':' || strncasecmp(db_driver, "dbi", 3) != 0) {
      Jmsg0(jcr, M_ABORT, 0, _("Invalid driver type, must be \"dbi:<type>\"\n"));
   }
   p = (char *)(db_driver + 4);
   if (strcasecmp(p, "mysql") == 0) {
      db_type = SQL_TYPE_MYSQL;
   } else if (strcasecmp(p, "postgresql") == 0) {
      db_type = SQL_TYPE_POSTGRESQL;
   } else if (strcasecmp(p, "sqlite") == 0) {
      db_type = SQL_TYPE_SQLITE;
   } else if (strcasecmp(p, "sqlite3") == 0) {
      db_type = SQL_TYPE_SQLITE3;
   } else {
      Jmsg1(jcr, M_ABORT, 0, _("Unknown database type: %s\n"), p);
   }
#elif HAVE_MYSQL
   db_type = SQL_TYPE_MYSQL;
#elif HAVE_POSTGRESQL
   db_type = SQL_TYPE_POSTGRESQL;
#elif HAVE_SQLITE
   db_type = SQL_TYPE_SQLITE;
#elif HAVE_SQLITE3
   db_type = SQL_TYPE_SQLITE3;
#endif

   return db_init_database(jcr, db_name, db_user, db_password, db_address,
             db_port, db_socket, mult_db_connections);
}

dbid_list::dbid_list()
{
   memset(this, 0, sizeof(dbid_list));
   max_ids = 1000;
   DBId = (DBId_t *)malloc(max_ids * sizeof(DBId_t));
   num_ids = num_seen = tot_ids = 0;
   PurgedFiles = NULL;
}

dbid_list::~dbid_list()
{
   free(DBId);
}


/*
 * Called here to retrieve an integer from the database
 */
static int int_handler(void *ctx, int num_fields, char **row)
{
   uint32_t *val = (uint32_t *)ctx;

   Dmsg1(800, "int_handler starts with row pointing at %x\n", row);

   if (row[0]) {
      Dmsg1(800, "int_handler finds '%s'\n", row[0]);
      *val = str_to_int64(row[0]);
   } else {
      Dmsg0(800, "int_handler finds zero\n");
      *val = 0;
   }
   Dmsg0(800, "int_handler finishes\n");
   return 0;
}

/*
 * Called here to retrieve a 32/64 bit integer from the database.
 *   The returned integer will be extended to 64 bit.
 */
int db_int64_handler(void *ctx, int num_fields, char **row)
{
   db_int64_ctx *lctx = (db_int64_ctx *)ctx;

   if (row[0]) {
      lctx->value = str_to_int64(row[0]);
      lctx->count++;
   }
   return 0;
}

/*
 * Use to build a comma separated list of values from a query. "10,20,30"
 */
int db_list_handler(void *ctx, int num_fields, char **row)
{
   db_list_ctx *lctx = (db_list_ctx *)ctx;
   if (num_fields == 1 && row[0]) {
      if (lctx->list[0]) {
         pm_strcat(lctx->list, ",");
      }
      pm_strcat(lctx->list, row[0]);
      lctx->count++;
   }
   return 0;
}

/* NOTE!!! The following routines expect that the
 *  calling subroutine sets and clears the mutex
 */

/* Check that the tables correspond to the version we want */
bool check_tables_version(JCR *jcr, B_DB *mdb)
{
   const char *query = "SELECT VersionId FROM Version";

   bacula_db_version = 0;
   if (!db_sql_query(mdb, query, int_handler, (void *)&bacula_db_version)) {
      Jmsg(jcr, M_FATAL, 0, "%s", mdb->errmsg);
      return false;
   }
   if (bacula_db_version != BDB_VERSION) {
      Mmsg(mdb->errmsg, "Version error for database \"%s\". Wanted %d, got %d\n",
          mdb->db_name, BDB_VERSION, bacula_db_version);
      Jmsg(jcr, M_FATAL, 0, "%s", mdb->errmsg);
      return false;
   }
   return true;
}

/* Utility routine for queries. The database MUST be locked before calling here. */
int
QueryDB(const char *file, int line, JCR *jcr, B_DB *mdb, char *cmd)
{
   int status;

   sql_free_result(mdb);
   if ((status=sql_query(mdb, cmd)) != 0) {
      m_msg(file, line, &mdb->errmsg, _("query %s failed:\n%s\n"), cmd, sql_strerror(mdb));
      j_msg(file, line, jcr, M_FATAL, 0, "%s", mdb->errmsg);
      if (verbose) {
         j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return 0;
   }

   mdb->result = sql_store_result(mdb);

   return mdb->result != NULL;
}

/*
 * Utility routine to do inserts
 * Returns: 0 on failure
 *          1 on success
 */
int
InsertDB(const char *file, int line, JCR *jcr, B_DB *mdb, char *cmd)
{
   if (sql_query(mdb, cmd)) {
      m_msg(file, line, &mdb->errmsg,  _("insert %s failed:\n%s\n"), cmd, sql_strerror(mdb));
      j_msg(file, line, jcr, M_FATAL, 0, "%s", mdb->errmsg);
      if (verbose) {
         j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return 0;
   }
   if (mdb->have_insert_id) {
      mdb->num_rows = sql_affected_rows(mdb);
   } else {
      mdb->num_rows = 1;
   }
   if (mdb->num_rows != 1) {
      char ed1[30];
      m_msg(file, line, &mdb->errmsg, _("Insertion problem: affected_rows=%s\n"),
         edit_uint64(mdb->num_rows, ed1));
      if (verbose) {
         j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return 0;
   }
   mdb->changes++;
   return 1;
}

/* Utility routine for updates.
 *  Returns: 0 on failure
 *           1 on success
 */
int
UpdateDB(const char *file, int line, JCR *jcr, B_DB *mdb, char *cmd)
{

   if (sql_query(mdb, cmd)) {
      m_msg(file, line, &mdb->errmsg, _("update %s failed:\n%s\n"), cmd, sql_strerror(mdb));
      j_msg(file, line, jcr, M_ERROR, 0, "%s", mdb->errmsg);
      if (verbose) {
         j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return 0;
   }
   mdb->num_rows = sql_affected_rows(mdb);
   if (mdb->num_rows < 1) {
      char ed1[30];
      m_msg(file, line, &mdb->errmsg, _("Update failed: affected_rows=%s for %s\n"),
         edit_uint64(mdb->num_rows, ed1), cmd);
      if (verbose) {
//       j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return 0;
   }
   mdb->changes++;
   return 1;
}

/* Utility routine for deletes
 *
 * Returns: -1 on error
 *           n number of rows affected
 */
int
DeleteDB(const char *file, int line, JCR *jcr, B_DB *mdb, char *cmd)
{

   if (sql_query(mdb, cmd)) {
      m_msg(file, line, &mdb->errmsg, _("delete %s failed:\n%s\n"), cmd, sql_strerror(mdb));
      j_msg(file, line, jcr, M_ERROR, 0, "%s", mdb->errmsg);
      if (verbose) {
         j_msg(file, line, jcr, M_INFO, 0, "%s\n", cmd);
      }
      return -1;
   }
   mdb->changes++;
   return sql_affected_rows(mdb);
}


/*
 * Get record max. Query is already in mdb->cmd
 *  No locking done
 *
 * Returns: -1 on failure
 *          count on success
 */
int get_sql_record_max(JCR *jcr, B_DB *mdb)
{
   SQL_ROW row;
   int stat = 0;

   if (QUERY_DB(jcr, mdb, mdb->cmd)) {
      if ((row = sql_fetch_row(mdb)) == NULL) {
         Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
         stat = -1;
      } else {
         stat = str_to_int64(row[0]);
      }
      sql_free_result(mdb);
   } else {
      Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
      stat = -1;
   }
   return stat;
}

/*
 * Return pre-edited error message
 */
char *db_strerror(B_DB *mdb)
{
   return mdb->errmsg;
}

/*
 * Lock database, this can be called multiple times by the same
 *   thread without blocking, but must be unlocked the number of
 *   times it was locked.
 */
void _db_lock(const char *file, int line, B_DB *mdb)
{
   int errstat;
   if ((errstat=rwl_writelock(&mdb->lock)) != 0) {
      berrno be;
      e_msg(file, line, M_FATAL, 0, "rwl_writelock failure. stat=%d: ERR=%s\n",
           errstat, be.bstrerror(errstat));
   }
}

/*
 * Unlock the database. This can be called multiple times by the
 *   same thread up to the number of times that thread called
 *   db_lock()/
 */
void _db_unlock(const char *file, int line, B_DB *mdb)
{
   int errstat;
   if ((errstat=rwl_writeunlock(&mdb->lock)) != 0) {
      berrno be;
      e_msg(file, line, M_FATAL, 0, "rwl_writeunlock failure. stat=%d: ERR=%s\n",
           errstat, be.bstrerror(errstat));
   }
}

/*
 * Start a transaction. This groups inserts and makes things
 *  much more efficient. Usually started when inserting
 *  file attributes.
 */
void db_start_transaction(JCR *jcr, B_DB *mdb)
{
   if (!jcr->attr) {
      jcr->attr = get_pool_memory(PM_FNAME);
   }
   if (!jcr->ar) {
      jcr->ar = (ATTR_DBR *)malloc(sizeof(ATTR_DBR));
   }

#ifdef HAVE_SQLITE
   if (!mdb->allow_transactions) {
      return;
   }
   db_lock(mdb);
   /* Allow only 10,000 changes per transaction */
   if (mdb->transaction && mdb->changes > 10000) {
      db_end_transaction(jcr, mdb);
   }
   if (!mdb->transaction) {
      my_sqlite_query(mdb, "BEGIN");  /* begin transaction */
      Dmsg0(400, "Start SQLite transaction\n");
      mdb->transaction = 1;
   }
   db_unlock(mdb);
#endif

/*
 * This is turned off because transactions break
 * if multiple simultaneous jobs are run.
 */
#ifdef HAVE_POSTGRESQL
   if (!mdb->allow_transactions) {
      return;
   }
   db_lock(mdb);
   /* Allow only 25,000 changes per transaction */
   if (mdb->transaction && mdb->changes > 25000) {
      db_end_transaction(jcr, mdb);
   }
   if (!mdb->transaction) {
      db_sql_query(mdb, "BEGIN", NULL, NULL);  /* begin transaction */
      Dmsg0(400, "Start PosgreSQL transaction\n");
      mdb->transaction = 1;
   }
   db_unlock(mdb);
#endif

#ifdef HAVE_DBI
   if (db_type == SQL_TYPE_SQLITE) {
      if (!mdb->allow_transactions) {
         return;
      }
      db_lock(mdb);
      /* Allow only 10,000 changes per transaction */
      if (mdb->transaction && mdb->changes > 10000) {
         db_end_transaction(jcr, mdb);
      }
      if (!mdb->transaction) {
         //my_sqlite_query(mdb, "BEGIN");  /* begin transaction */
         db_sql_query(mdb, "BEGIN", NULL, NULL);  /* begin transaction */
         Dmsg0(400, "Start SQLite transaction\n");
         mdb->transaction = 1;
      }
      db_unlock(mdb);
   } else if (db_type == SQL_TYPE_POSTGRESQL) {
      if (!mdb->allow_transactions) {
         return;
      }
      db_lock(mdb);
      /* Allow only 25,000 changes per transaction */
      if (mdb->transaction && mdb->changes > 25000) {
         db_end_transaction(jcr, mdb);
      }
      if (!mdb->transaction) {
         db_sql_query(mdb, "BEGIN", NULL, NULL);  /* begin transaction */
         Dmsg0(400, "Start PosgreSQL transaction\n");
         mdb->transaction = 1;
      }
      db_unlock(mdb);
   }
#endif
}

void db_end_transaction(JCR *jcr, B_DB *mdb)
{
   /*
    * This can be called during thread cleanup and
    *   the db may already be closed.  So simply return.
    */
   if (!mdb) {
      return;
   }

   if (jcr && jcr->cached_attribute) {
      Dmsg0(400, "Flush last cached attribute.\n");
      if (!db_create_file_attributes_record(jcr, mdb, jcr->ar)) {
         Jmsg1(jcr, M_FATAL, 0, _("Attribute create error. %s"), db_strerror(jcr->db));
      }
      jcr->cached_attribute = false;
   }

#ifdef HAVE_SQLITE
   if (!mdb->allow_transactions) {
      return;
   }
   db_lock(mdb);
   if (mdb->transaction) {
      my_sqlite_query(mdb, "COMMIT"); /* end transaction */
      mdb->transaction = 0;
      Dmsg1(400, "End SQLite transaction changes=%d\n", mdb->changes);
   }
   mdb->changes = 0;
   db_unlock(mdb);
#endif

#ifdef HAVE_POSTGRESQL
   if (!mdb->allow_transactions) {
      return;
   }
   db_lock(mdb);
   if (mdb->transaction) {
      db_sql_query(mdb, "COMMIT", NULL, NULL); /* end transaction */
      mdb->transaction = 0;
      Dmsg1(400, "End PostgreSQL transaction changes=%d\n", mdb->changes);
   }
   mdb->changes = 0;
   db_unlock(mdb);
#endif

#ifdef HAVE_DBI
   if (db_type == SQL_TYPE_SQLITE) {
      if (!mdb->allow_transactions) {
         return;
      }
      db_lock(mdb);
      if (mdb->transaction) {
         //my_sqlite_query(mdb, "COMMIT"); /* end transaction */
         db_sql_query(mdb, "COMMIT", NULL, NULL); /* end transaction */
         mdb->transaction = 0;
         Dmsg1(400, "End SQLite transaction changes=%d\n", mdb->changes);
      }
      mdb->changes = 0;
      db_unlock(mdb);
   } else if (db_type == SQL_TYPE_POSTGRESQL) {
      if (!mdb->allow_transactions) {
         return;
      }
      db_lock(mdb);
      if (mdb->transaction) {
         db_sql_query(mdb, "COMMIT", NULL, NULL); /* end transaction */
         mdb->transaction = 0;
         Dmsg1(400, "End PostgreSQL transaction changes=%d\n", mdb->changes);
      }
      mdb->changes = 0;
      db_unlock(mdb);
   }
#endif
}

/*
 * Given a full filename, split it into its path
 *  and filename parts. They are returned in pool memory
 *  in the mdb structure.
 */
void split_path_and_file(JCR *jcr, B_DB *mdb, const char *fname)
{
   const char *p, *f;

   /* Find path without the filename.
    * I.e. everything after the last / is a "filename".
    * OK, maybe it is a directory name, but we treat it like
    * a filename. If we don't find a / then the whole name
    * must be a path name (e.g. c:).
    */
   for (p=f=fname; *p; p++) {
      if (IsPathSeparator(*p)) {
         f = p;                       /* set pos of last slash */
      }
   }
   if (IsPathSeparator(*f)) {                   /* did we find a slash? */
      f++;                            /* yes, point to filename */
   } else {                           /* no, whole thing must be path name */
      f = p;
   }

   /* If filename doesn't exist (i.e. root directory), we
    * simply create a blank name consisting of a single
    * space. This makes handling zero length filenames
    * easier.
    */
   mdb->fnl = p - f;
   if (mdb->fnl > 0) {
      mdb->fname = check_pool_memory_size(mdb->fname, mdb->fnl+1);
      memcpy(mdb->fname, f, mdb->fnl);    /* copy filename */
      mdb->fname[mdb->fnl] = 0;
   } else {
      mdb->fname[0] = 0;
      mdb->fnl = 0;
   }

   mdb->pnl = f - fname;
   if (mdb->pnl > 0) {
      mdb->path = check_pool_memory_size(mdb->path, mdb->pnl+1);
      memcpy(mdb->path, fname, mdb->pnl);
      mdb->path[mdb->pnl] = 0;
   } else {
      Mmsg1(&mdb->errmsg, _("Path length is zero. File=%s\n"), fname);
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      mdb->path[0] = 0;
      mdb->pnl = 0;
   }

   Dmsg2(500, "split path=%s file=%s\n", mdb->path, mdb->fname);
}

/*
 * List dashes as part of header for listing SQL results in a table
 */
void
list_dashes(B_DB *mdb, DB_LIST_HANDLER *send, void *ctx)
{
   SQL_FIELD  *field;
   int i, j;

   sql_field_seek(mdb, 0);
   send(ctx, "+");
   for (i = 0; i < sql_num_fields(mdb); i++) {
      field = sql_fetch_field(mdb);
      if (!field) {
         break;
      }
      for (j = 0; j < (int)field->max_length + 2; j++) {
         send(ctx, "-");
      }
      send(ctx, "+");
   }
   send(ctx, "\n");
}

/*
 * If full_list is set, we list vertically, otherwise, we
 * list on one line horizontally.
 */
void
list_result(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *send, void *ctx, e_list_type type)
{
   SQL_FIELD *field;
   SQL_ROW row;
   int i, col_len, max_len = 0;
   char buf[2000], ewc[30];

   Dmsg0(800, "list_result starts\n");
   if (mdb->result == NULL || sql_num_rows(mdb) == 0) {
      send(ctx, _("No results to list.\n"));
      return;
   }

   Dmsg1(800, "list_result starts looking at %d fields\n", sql_num_fields(mdb));
   /* determine column display widths */
   sql_field_seek(mdb, 0);
   for (i = 0; i < sql_num_fields(mdb); i++) {
      Dmsg1(800, "list_result processing field %d\n", i);
      field = sql_fetch_field(mdb);
      if (!field) {
         break;
      }
      col_len = cstrlen(field->name);
      if (type == VERT_LIST) {
         if (col_len > max_len) {
            max_len = col_len;
         }
      } else {
         if (IS_NUM(field->type) && (int)field->max_length > 0) { /* fixup for commas */
            field->max_length += (field->max_length - 1) / 3;
         }
         if (col_len < (int)field->max_length) {
            col_len = field->max_length;
         }
         if (col_len < 4 && !IS_NOT_NULL(field->flags)) {
            col_len = 4;                 /* 4 = length of the word "NULL" */
         }
         field->max_length = col_len;    /* reset column info */
      }
   }

   Dmsg0(800, "list_result finished first loop\n");
   if (type == VERT_LIST) {
      goto vertical_list;
   }

   Dmsg1(800, "list_result starts second loop looking at %d fields\n", sql_num_fields(mdb));
   list_dashes(mdb, send, ctx);
   send(ctx, "|");
   sql_field_seek(mdb, 0);
   for (i = 0; i < sql_num_fields(mdb); i++) {
      Dmsg1(800, "list_result looking at field %d\n", i);
      field = sql_fetch_field(mdb);
      if (!field) {
         break;
      }
      bsnprintf(buf, sizeof(buf), " %-*s |", (int)field->max_length, field->name);
      send(ctx, buf);
   }
   send(ctx, "\n");
   list_dashes(mdb, send, ctx);

   Dmsg1(800, "list_result starts third loop looking at %d fields\n", sql_num_fields(mdb));
   while ((row = sql_fetch_row(mdb)) != NULL) {
      sql_field_seek(mdb, 0);
      send(ctx, "|");
      for (i = 0; i < sql_num_fields(mdb); i++) {
         field = sql_fetch_field(mdb);
         if (!field) {
            break;
         }
         if (row[i] == NULL) {
            bsnprintf(buf, sizeof(buf), " %-*s |", (int)field->max_length, "NULL");
         } else if (IS_NUM(field->type) && !jcr->gui && is_an_integer(row[i])) {
            bsnprintf(buf, sizeof(buf), " %*s |", (int)field->max_length,
                      add_commas(row[i], ewc));
         } else {
            bsnprintf(buf, sizeof(buf), " %-*s |", (int)field->max_length, row[i]);
         }
         send(ctx, buf);
      }
      send(ctx, "\n");
   }
   list_dashes(mdb, send, ctx);
   return;

vertical_list:

   Dmsg1(800, "list_result starts vertical list at %d fields\n", sql_num_fields(mdb));
   while ((row = sql_fetch_row(mdb)) != NULL) {
      sql_field_seek(mdb, 0);
      for (i = 0; i < sql_num_fields(mdb); i++) {
         field = sql_fetch_field(mdb);
         if (!field) {
            break;
         }
         if (row[i] == NULL) {
            bsnprintf(buf, sizeof(buf), " %*s: %s\n", max_len, field->name, "NULL");
         } else if (IS_NUM(field->type) && !jcr->gui && is_an_integer(row[i])) {
            bsnprintf(buf, sizeof(buf), " %*s: %s\n", max_len, field->name,
                add_commas(row[i], ewc));
         } else {
            bsnprintf(buf, sizeof(buf), " %*s: %s\n", max_len, field->name, row[i]);
         }
         send(ctx, buf);
      }
      send(ctx, "\n");
   }
   return;
}

/* 
 * Open a new connexion to mdb catalog. This function is used
 * by batch and accurate mode.
 */
bool db_open_batch_connexion(JCR *jcr, B_DB *mdb)
{
   int multi_db=false;

#ifdef HAVE_BATCH_FILE_INSERT
   multi_db=true;               /* we force a new connexion only if batch insert is enabled */
#endif

   if (!jcr->db_batch) {
      jcr->db_batch = db_init_database(jcr, 
                                      mdb->db_name, 
                                      mdb->db_user,
                                      mdb->db_password, 
                                      mdb->db_address,
                                      mdb->db_port,
                                      mdb->db_socket,
                                      multi_db /* multi_db = true when using batch mode */);
      if (!jcr->db_batch) {
         Jmsg0(jcr, M_FATAL, 0, "Could not init batch connexion");
         return false;
      }

      if (!db_open_database(jcr, jcr->db_batch)) {
         Mmsg2(&jcr->db_batch->errmsg,  _("Could not open database \"%s\": ERR=%s\n"),
              jcr->db_batch->db_name, db_strerror(jcr->db_batch));
         Jmsg1(jcr, M_FATAL, 0, "%s", jcr->db_batch->errmsg);
         return false;
      }      
      Dmsg3(100, "initdb ref=%d connected=%d db=%p\n", jcr->db_batch->ref_count,
            jcr->db_batch->connected, jcr->db_batch->db);

   }
   return true;
}

/*
 * !!! WARNING !!! Use this function only when bacula is stopped.
 * ie, after a fatal signal and before exiting the program
 * Print information about a B_DB object.
 */
void _dbg_print_db(JCR *jcr, FILE *fp)
{
   B_DB *mdb = jcr->db;

   if (!mdb) {
      return;
   }

   fprintf(fp, "B_DB=%p db_name=%s db_user=%s connected=%i\n",
           mdb, NPRTB(mdb->db_name), NPRTB(mdb->db_user), mdb->connected);
   fprintf(fp, "\tcmd=\"%s\" changes=%i\n", NPRTB(mdb->cmd), mdb->changes);
   if (mdb->lock.valid == RWLOCK_VALID) { 
      fprintf(fp, "\tRWLOCK=%p w_active=%i w_wait=%i\n", &mdb->lock, mdb->lock.w_active, mdb->lock.w_wait);
   }
}

#endif /* HAVE_SQLITE3 || HAVE_MYSQL || HAVE_SQLITE || HAVE_POSTGRESQL*/
