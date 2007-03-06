/*
 * Bacula Catalog Database routines specific to SQLite
 *
 *    Kern Sibbald, January 2002
 *
 *    Version $Id: sqlite.c,v 1.27 2004/09/22 21:36:57 kerns Exp $
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


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C 		      /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#ifdef HAVE_SQLITE

/* -----------------------------------------------------------------------
 *
 *    SQLite dependent defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

extern const char *working_directory;

/* List of open databases */
static BQUEUE db_list = {&db_list, &db_list};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);


/*
 * Initialize database data structure. In principal this should
 * never have errors, or it is really fatal.
 */
B_DB *
db_init_database(JCR *jcr, const char *db_name, const char *db_user, const char *db_password,
		 const char *db_address, int db_port, const char *db_socket, 
		 int mult_db_connections)
{
   B_DB *mdb;

   P(mutex);			      /* lock DB queue */
   /* Look to see if DB already open */
   if (!mult_db_connections) {
      for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
	 if (strcmp(mdb->db_name, db_name) == 0) {
            Dmsg2(300, "DB REopen %d %s\n", mdb->ref_count, db_name);
	    mdb->ref_count++;
	    V(mutex);
	    return mdb; 		 /* already open */
	 }
      }
   }
   Dmsg0(300, "db_open first time\n");
   mdb = (B_DB *) malloc(sizeof(B_DB));
   memset(mdb, 0, sizeof(B_DB));
   mdb->db_name = bstrdup(db_name);
   mdb->have_insert_id = TRUE; 
   mdb->errmsg = get_pool_memory(PM_EMSG); /* get error message buffer */
   *mdb->errmsg = 0;
   mdb->cmd = get_pool_memory(PM_EMSG);    /* get command buffer */
   mdb->cached_path = get_pool_memory(PM_FNAME);
   mdb->cached_path_id = 0;
   mdb->ref_count = 1;
   mdb->fname = get_pool_memory(PM_FNAME);
   mdb->path = get_pool_memory(PM_FNAME);
   mdb->esc_name = get_pool_memory(PM_FNAME);
   mdb->allow_transactions = mult_db_connections;
   qinsert(&db_list, &mdb->bq); 	   /* put db in list */
   V(mutex);
   return mdb;
}

/*
 * Now actually open the database.  This can generate errors,
 * which are returned in the errmsg
 *
 * DO NOT close the database or free(mdb) here !!!!
 */
int
db_open_database(JCR *jcr, B_DB *mdb)
{
   char *db_name;
   int len;
   struct stat statbuf;
   int errstat;

   P(mutex);
   if (mdb->connected) {
      V(mutex);
      return 1;
   }
   mdb->connected = FALSE;

   if ((errstat=rwl_init(&mdb->lock)) != 0) {
      Mmsg1(&mdb->errmsg, _("Unable to initialize DB lock. ERR=%s\n"), 
	    strerror(errstat));
      V(mutex);
      return 0;
   }

   /* open the database */
   len = strlen(working_directory) + strlen(mdb->db_name) + 5; 
   db_name = (char *)malloc(len);
   strcpy(db_name, working_directory);
   strcat(db_name, "/");
   strcat(db_name, mdb->db_name);
   strcat(db_name, ".db");
   if (stat(db_name, &statbuf) != 0) {
      Mmsg1(&mdb->errmsg, _("Database %s does not exist, please create it.\n"), 
	 db_name);
      free(db_name);
      V(mutex);
      return 0;
   }
   mdb->db = sqlite_open(
	db_name,		      /* database name */
	644,			      /* mode */
	&mdb->sqlite_errmsg);	      /* error message */

   Dmsg0(300, "sqlite_open\n");
  
   if (mdb->db == NULL) {
      Mmsg2(&mdb->errmsg, _("Unable to open Database=%s. ERR=%s\n"),
         db_name, mdb->sqlite_errmsg ? mdb->sqlite_errmsg : _("unknown"));
      free(db_name);
      V(mutex);
      return 0;
   }
   free(db_name);
   if (!check_tables_version(jcr, mdb)) {
      V(mutex);
      return 0;
   }

   mdb->connected = TRUE;
   V(mutex);
   return 1;
}

void
db_close_database(JCR *jcr, B_DB *mdb)
{
   if (!mdb) {
      return;
   }
   P(mutex);
   mdb->ref_count--;
   if (mdb->ref_count == 0) {
      qdchain(&mdb->bq);
      if (mdb->connected && mdb->db) {
	 sqlite_close(mdb->db);
      }
      rwl_destroy(&mdb->lock);	     
      free_pool_memory(mdb->errmsg);
      free_pool_memory(mdb->cmd);
      free_pool_memory(mdb->cached_path);
      free_pool_memory(mdb->fname);
      free_pool_memory(mdb->path);
      free_pool_memory(mdb->esc_name);
      if (mdb->db_name) {
	 free(mdb->db_name);
      }
      free(mdb);
   }
   V(mutex);
}

/*
 * Return the next unique index (auto-increment) for
 * the given table.  Return 0 on error.
 */
int db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index)
{
   SQL_ROW row;

   db_lock(mdb);

   Mmsg(mdb->cmd,
"SELECT id FROM NextId WHERE TableName=\"%s\"", table);
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      Mmsg(mdb->errmsg, _("next_index query error: ERR=%s\n"), sql_strerror(mdb));
      db_unlock(mdb);
      return 0;
   }
   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg(mdb->errmsg, _("Error fetching index: ERR=%s\n"), sql_strerror(mdb));
      db_unlock(mdb);
      return 0;
   }
   bstrncpy(index, row[0], 28);
   sql_free_result(mdb);

   Mmsg(mdb->cmd,
"UPDATE NextId SET id=id+1 WHERE TableName=\"%s\"", table);
   if (!QUERY_DB(jcr, mdb, mdb->cmd)) {
      Mmsg(mdb->errmsg, _("next_index update error: ERR=%s\n"), sql_strerror(mdb));
      db_unlock(mdb);
      return 0;
   }
   sql_free_result(mdb);

   db_unlock(mdb);
   return 1;
}   


/*
 * Escape strings so that SQLite is happy
 *
 *   NOTE! len is the length of the old string. Your new
 *	   string must be long enough (max 2*old+1) to hold
 *	   the escaped output.
 */
void
db_escape_string(char *snew, char *old, int len)
{
   char *n, *o;

   n = snew;
   o = old;
   while (len--) {
      switch (*o) {
      case '\'':
         *n++ = '\'';
         *n++ = '\'';
	 o++;
	 break;
      case 0:
         *n++ = '\\';
	 *n++ = 0;
	 o++;
	 break;
      default:
	 *n++ = *o++;
	 break;
      }
   }
   *n = 0;
}

struct rh_data {
   DB_RESULT_HANDLER *result_handler;
   void *ctx;
};

/*  
 * Convert SQLite's callback into Bacula DB callback  
 */
static int sqlite_result(void *arh_data, int num_fields, char **rows, char **col_names)
{
   struct rh_data *rh_data = (struct rh_data *)arh_data;   

   if (rh_data->result_handler) {
      (*(rh_data->result_handler))(rh_data->ctx, num_fields, rows);
   }
   return 0;
}

/*
 * Submit a general SQL command (cmd), and for each row returned,
 *  the sqlite_handler is called with the ctx.
 */
int db_sql_query(B_DB *mdb, const char *query, DB_RESULT_HANDLER *result_handler, void *ctx)
{
   struct rh_data rh_data;
   int stat;

   db_lock(mdb);
   if (mdb->sqlite_errmsg) {
      actuallyfree(mdb->sqlite_errmsg);
      mdb->sqlite_errmsg = NULL;
   }
   rh_data.result_handler = result_handler;
   rh_data.ctx = ctx;
   stat = sqlite_exec(mdb->db, query, sqlite_result, (void *)&rh_data, &mdb->sqlite_errmsg);
   if (stat != 0) {
      Mmsg(mdb->errmsg, _("Query failed: %s: ERR=%s\n"), query, sql_strerror(mdb));
      db_unlock(mdb);
      return 0;
   }
   db_unlock(mdb);
   return 1;
}

/*
 * Submit a sqlite query and retrieve all the data
 */
int my_sqlite_query(B_DB *mdb, char *cmd) 
{
   int stat;

   if (mdb->sqlite_errmsg) {
      actuallyfree(mdb->sqlite_errmsg);
      mdb->sqlite_errmsg = NULL;
   }
   stat = sqlite_get_table(mdb->db, cmd, &mdb->result, &mdb->nrow, &mdb->ncolumn,
	    &mdb->sqlite_errmsg);
   mdb->row = 0;		      /* row fetched */
   return stat;
}

/* Fetch one row at a time */
SQL_ROW my_sqlite_fetch_row(B_DB *mdb)
{
   if (mdb->row >= mdb->nrow) {
      return NULL;
   }
   mdb->row++;
   return &mdb->result[mdb->ncolumn * mdb->row];
}

void my_sqlite_free_table(B_DB *mdb)
{
   int i;

   if (mdb->fields_defined) {
      for (i=0; i < sql_num_fields(mdb); i++) {
	 free(mdb->fields[i]);
      }
      free(mdb->fields);
      mdb->fields_defined = false;
   }
   sqlite_free_table(mdb->result);
   mdb->nrow = mdb->ncolumn = 0; 
}

void my_sqlite_field_seek(B_DB *mdb, int field)
{
   int i, j;
   if (mdb->result == NULL) {
      return;
   }
   /* On first call, set up the fields */
   if (!mdb->fields_defined && sql_num_fields(mdb) > 0) {
      mdb->fields = (SQL_FIELD **)malloc(sizeof(SQL_FIELD) * mdb->ncolumn);
      for (i=0; i < sql_num_fields(mdb); i++) {
	 mdb->fields[i] = (SQL_FIELD *)malloc(sizeof(SQL_FIELD));
	 mdb->fields[i]->name = mdb->result[i];
	 mdb->fields[i]->length = strlen(mdb->fields[i]->name);
	 mdb->fields[i]->max_length = mdb->fields[i]->length;
	 for (j=1; j <= mdb->nrow; j++) {
	    int len;
	    if (mdb->result[i + mdb->ncolumn *j]) {
	       len = (uint32_t)strlen(mdb->result[i + mdb->ncolumn * j]);
	    } else {
	       len = 0;
	    }
	    if (len > mdb->fields[i]->max_length) {
	       mdb->fields[i]->max_length = len;
	    }
	 }
	 mdb->fields[i]->type = 0;
	 mdb->fields[i]->flags = 1;	   /* not null */
      }
      mdb->fields_defined = TRUE;
   }
   if (field > sql_num_fields(mdb)) {
      field = sql_num_fields(mdb);
    }
    mdb->field = field;

}

SQL_FIELD *my_sqlite_fetch_field(B_DB *mdb)
{
   return mdb->fields[mdb->field++];
}



#endif /* HAVE_SQLITE */
