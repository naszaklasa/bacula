/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2008 Free Software Foundation Europe e.V.

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
 * Bacula Catalog Database routines specific to DBI
 *   These are DBI specific routines
 *
 *    João Henrique Freitas, December 2007
 *    based upon work done by Dan Langille, December 2003 and
 *    by Kern Sibbald, March 2000
 *
 *    Version $Id$
 */


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#ifdef HAVE_DBI

/* -----------------------------------------------------------------------
 *
 *   DBI dependent defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

/* List of open databases */
static BQUEUE db_list = {&db_list, &db_list};

/* Control allocated fields by my_dbi_getvalue */
static BQUEUE dbi_getvalue_list = {&dbi_getvalue_list, &dbi_getvalue_list};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Retrieve database type
 */
const char *
db_get_type(void)
{
   return "DBI";
}

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
   char db_driver[10];
   char db_driverdir[256];

   /* Constraint the db_driver */
   if(db_type  == -1) {
      Jmsg(jcr, M_FATAL, 0, _("A dbi driver for DBI must be supplied.\n"));
      return NULL;
   }

   /* Do the correct selection of driver.
    * Can be one of the varius supported by libdbi
    */
   switch (db_type) {
   case SQL_TYPE_MYSQL:
      bstrncpy(db_driver,"mysql", sizeof(db_driver));
      break;
   case SQL_TYPE_POSTGRESQL:
      bstrncpy(db_driver,"pgsql", sizeof(db_driver));
      break;
   case SQL_TYPE_SQLITE:
      bstrncpy(db_driver,"sqlite", sizeof(db_driver));
      break;
   case SQL_TYPE_SQLITE3:
      bstrncpy(db_driver,"sqlite3", sizeof(db_driver));
      break;
   }

   /* Set db_driverdir whereis is the libdbi drivers */
   bstrncpy(db_driverdir, DBI_DRIVER_DIR, 255);

   if (!db_user) {
      Jmsg(jcr, M_FATAL, 0, _("A user name for DBI must be supplied.\n"));
      return NULL;
   }
   P(mutex);                          /* lock DB queue */
   if (!mult_db_connections) {
      /* Look to see if DB already open */
      for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
         if (bstrcmp(mdb->db_name, db_name) &&
             bstrcmp(mdb->db_address, db_address) &&
             bstrcmp(mdb->db_driver, db_driver) &&
             mdb->db_port == db_port) {
            Dmsg4(100, "DB REopen %d %s %s erro: %d\n", mdb->ref_count, db_driver, db_name,
                  dbi_conn_error(mdb->db, NULL));
            mdb->ref_count++;
            V(mutex);
            return mdb;                  /* already open */
         }
      }
   }
   Dmsg0(100, "db_open first time\n");
   mdb = (B_DB *)malloc(sizeof(B_DB));
   memset(mdb, 0, sizeof(B_DB));
   mdb->db_name = bstrdup(db_name);
   mdb->db_user = bstrdup(db_user);
   if (db_password) {
      mdb->db_password = bstrdup(db_password);
   }
   if (db_address) {
      mdb->db_address  = bstrdup(db_address);
   }
   if (db_socket) {
      mdb->db_socket   = bstrdup(db_socket);
   }
   if (db_driverdir) {
      mdb->db_driverdir = bstrdup(db_driverdir);
   }
   if (db_driver) {
      mdb->db_driver    = bstrdup(db_driver);
   }
   mdb->db_type        = db_type;
   mdb->db_port        = db_port;
   mdb->have_insert_id = TRUE;
   mdb->errmsg         = get_pool_memory(PM_EMSG); /* get error message buffer */
   *mdb->errmsg        = 0;
   mdb->cmd            = get_pool_memory(PM_EMSG); /* get command buffer */
   mdb->cached_path    = get_pool_memory(PM_FNAME);
   mdb->cached_path_id = 0;
   mdb->ref_count      = 1;
   mdb->fname          = get_pool_memory(PM_FNAME);
   mdb->path           = get_pool_memory(PM_FNAME);
   mdb->esc_name       = get_pool_memory(PM_FNAME);
   mdb->esc_path      = get_pool_memory(PM_FNAME);
   mdb->allow_transactions = mult_db_connections;
   qinsert(&db_list, &mdb->bq);            /* put db in list */
   V(mutex);
   return mdb;
}

/*
 * Now actually open the database.  This can generate errors,
 *   which are returned in the errmsg
 *
 * DO NOT close the database or free(mdb) here  !!!!
 */
int
db_open_database(JCR *jcr, B_DB *mdb)
{
   int errstat;
   int dbstat;
   uint8_t len;
   const char *errmsg;
   char buf[10], *port;
   int numdrivers;
   char *db_name = NULL;
   char *db_dir = NULL;

   P(mutex);
   if (mdb->connected) {
      V(mutex);
      return 1;
   }
   mdb->connected = false;

   if ((errstat=rwl_init(&mdb->lock)) != 0) {
      berrno be;
      Mmsg1(&mdb->errmsg, _("Unable to initialize DB lock. ERR=%s\n"),
            be.bstrerror(errstat));
      V(mutex);
      return 0;
   }

   if (mdb->db_port) {
      bsnprintf(buf, sizeof(buf), "%d", mdb->db_port);
      port = buf;
   } else {
      port = NULL;
   }

   numdrivers = dbi_initialize_r(mdb->db_driverdir, &(mdb->instance));
   if (numdrivers < 0) {
      Mmsg2(&mdb->errmsg, _("Unable to locate the DBD drivers to DBI interface in: \n"
                               "db_driverdir=%s. It is probaly not found any drivers\n"),
                               mdb->db_driverdir,numdrivers);
      V(mutex);
      return 0;
   }
   mdb->db = (void **)dbi_conn_new_r(mdb->db_driver, mdb->instance);
   /* Can be many types of databases */
   switch (mdb->db_type) {
   case SQL_TYPE_MYSQL:
      dbi_conn_set_option(mdb->db, "host", mdb->db_address); /* default = localhost */
      dbi_conn_set_option(mdb->db, "port", port);            /* default port */
      dbi_conn_set_option(mdb->db, "username", mdb->db_user);     /* login name */
      dbi_conn_set_option(mdb->db, "password", mdb->db_password); /* password */
      dbi_conn_set_option(mdb->db, "dbname", mdb->db_name);       /* database name */
      break;
   case SQL_TYPE_POSTGRESQL:
      dbi_conn_set_option(mdb->db, "host", mdb->db_address);
      dbi_conn_set_option(mdb->db, "port", port);
      dbi_conn_set_option(mdb->db, "username", mdb->db_user);
      dbi_conn_set_option(mdb->db, "password", mdb->db_password);
      dbi_conn_set_option(mdb->db, "dbname", mdb->db_name);
      break;
   case SQL_TYPE_SQLITE:
      len = strlen(working_directory) + 5;
      db_dir = (char *)malloc(len);
      strcpy(db_dir, working_directory);
      strcat(db_dir, "/");
      len = strlen(mdb->db_name) + 5;
      db_name = (char *)malloc(len);
      strcpy(db_name, mdb->db_name);
      strcat(db_name, ".db");
      dbi_conn_set_option(mdb->db, "sqlite_dbdir", db_dir);
      dbi_conn_set_option(mdb->db, "dbname", db_name);
      break;
   case SQL_TYPE_SQLITE3:
      len = strlen(working_directory) + 5;
      db_dir = (char *)malloc(len);
      strcpy(db_dir, working_directory);
      strcat(db_dir, "/");
      len = strlen(mdb->db_name) + 5;
      db_name = (char *)malloc(len);
      strcpy(db_name, mdb->db_name);
      strcat(db_name, ".db");
      dbi_conn_set_option(mdb->db, "sqlite3_dbdir", db_dir);
      dbi_conn_set_option(mdb->db, "dbname", db_name);
      Dmsg2(500, "SQLITE: %s %s\n", db_dir, db_name);
      break;
   }

   /* If connection fails, try at 5 sec intervals for 30 seconds. */
   for (int retry=0; retry < 6; retry++) {

      dbstat = dbi_conn_connect(mdb->db);
      if ( dbstat == 0) {
         break;
      }

      dbi_conn_error(mdb->db, &errmsg);
      Dmsg1(50, "dbi error: %s\n", errmsg);

      bmicrosleep(5, 0);

   }

   if ( dbstat != 0 ) {
      Mmsg3(&mdb->errmsg, _("Unable to connect to DBI interface.\n"
                       "Type=%s Database=%s User=%s\n"
                       "It is probably not running or your password is incorrect.\n"),
                        mdb->db_driver, mdb->db_name, mdb->db_user);
      V(mutex);
      return 0;
   }

   Dmsg0(50, "dbi_real_connect done\n");
   Dmsg3(50, "db_user=%s db_name=%s db_password=%s\n",
                    mdb->db_user, mdb->db_name,
                    mdb->db_password==NULL?"(NULL)":mdb->db_password);

   mdb->connected = true;

   if (!check_tables_version(jcr, mdb)) {
      V(mutex);
      return 0;
   }

   switch (mdb->db_type) {
   case SQL_TYPE_MYSQL:
      /* Set connection timeout to 8 days specialy for batch mode */
      sql_query(mdb, "SET wait_timeout=691200");
      sql_query(mdb, "SET interactive_timeout=691200");
      break;
   case SQL_TYPE_POSTGRESQL:
      /* tell PostgreSQL we are using standard conforming strings
         and avoid warnings such as:
         WARNING:  nonstandard use of \\ in a string literal
      */
      sql_query(mdb, "SET datestyle TO 'ISO, YMD'");
      sql_query(mdb, "set standard_conforming_strings=on");
      break;
   }

   if(db_dir) {
      free(db_dir);
   }
   if(db_name) {
      free(db_name);
   }

   V(mutex);
   return 1;
}

void
db_close_database(JCR *jcr, B_DB *mdb)
{
   if (!mdb) {
      return;
   }
   db_end_transaction(jcr, mdb);
   P(mutex);
   sql_free_result(mdb);
   mdb->ref_count--;
   if (mdb->ref_count == 0) {
      qdchain(&mdb->bq);
      if (mdb->connected && mdb->db) {
         //sql_close(mdb);
         dbi_shutdown_r(mdb->instance);
         mdb->db = NULL;
         mdb->instance = NULL;
      }
      rwl_destroy(&mdb->lock);
      free_pool_memory(mdb->errmsg);
      free_pool_memory(mdb->cmd);
      free_pool_memory(mdb->cached_path);
      free_pool_memory(mdb->fname);
      free_pool_memory(mdb->path);
      free_pool_memory(mdb->esc_name);
      free_pool_memory(mdb->esc_path);
      if (mdb->db_name) {
         free(mdb->db_name);
      }
      if (mdb->db_user) {
         free(mdb->db_user);
      }
      if (mdb->db_password) {
         free(mdb->db_password);
      }
      if (mdb->db_address) {
         free(mdb->db_address);
      }
      if (mdb->db_socket) {
         free(mdb->db_socket);
      }
      if (mdb->db_driverdir) {
         free(mdb->db_driverdir);
      }
      if (mdb->db_driver) {
          free(mdb->db_driver);
      }
      free(mdb);
   }
   V(mutex);
}

void db_thread_cleanup()
{ }

/*
 * Return the next unique index (auto-increment) for
 * the given table.  Return NULL on error.
 *
 */
int db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index)
{
   strcpy(index, "NULL");
   return 1;
}


/*
 * Escape strings so that DBI is happy
 *
 *   NOTE! len is the length of the old string. Your new
 *         string must be long enough (max 2*old+1) to hold
 *         the escaped output.
 *
 * dbi_conn_quote_string_copy receives a pointer to pointer.
 * We need copy the value of pointer to snew because libdbi change the
 * pointer
 */
void
db_escape_string(JCR *jcr, B_DB *mdb, char *snew, char *old, int len)
{
   char *inew;
   char *pnew;

   if (len == 0) {
      snew[0] = 0;
   } else {
      /* correct the size of old basead in len
       * and copy new string to inew
       */
      inew = (char *)malloc(sizeof(char) * len + 1);
      bstrncpy(inew,old,len + 1);
      /* escape the correct size of old */
      dbi_conn_escape_string_copy(mdb->db, inew, &pnew);
      free(inew);
      /* copy the escaped string to snew */
      bstrncpy(snew, pnew, 2 * len + 1);
   }

   Dmsg2(500, "dbi_conn_escape_string_copy %p %s\n",snew,snew);

}

/*
 * Submit a general SQL command (cmd), and for each row returned,
 *  the sqlite_handler is called with the ctx.
 */
bool db_sql_query(B_DB *mdb, const char *query, DB_RESULT_HANDLER *result_handler, void *ctx)
{
   SQL_ROW row;

   Dmsg0(500, "db_sql_query started\n");

   db_lock(mdb);
   if (sql_query(mdb, query) != 0) {
      Mmsg(mdb->errmsg, _("Query failed: %s: ERR=%s\n"), query, sql_strerror(mdb));
      db_unlock(mdb);
      Dmsg0(500, "db_sql_query failed\n");
      return false;
   }
   Dmsg0(500, "db_sql_query succeeded. checking handler\n");

   if (result_handler != NULL) {
      Dmsg0(500, "db_sql_query invoking handler\n");
      if ((mdb->result = sql_store_result(mdb)) != NULL) {
         int num_fields = sql_num_fields(mdb);

         Dmsg0(500, "db_sql_query sql_store_result suceeded\n");
         while ((row = sql_fetch_row(mdb)) != NULL) {

            Dmsg0(500, "db_sql_query sql_fetch_row worked\n");
            if (result_handler(ctx, num_fields, row))
               break;
         }

        sql_free_result(mdb);
      }
   }
   db_unlock(mdb);

   Dmsg0(500, "db_sql_query finished\n");

   return true;
}



DBI_ROW my_dbi_fetch_row(B_DB *mdb)
{
   int j;
   DBI_ROW row = NULL; // by default, return NULL

   Dmsg0(500, "my_dbi_fetch_row start\n");
   if ((!mdb->row || mdb->row_size < mdb->num_fields) && mdb->num_rows > 0) {
      int num_fields = mdb->num_fields;
      Dmsg1(500, "we have need space of %d bytes\n", sizeof(char *) * mdb->num_fields);

      if (mdb->row) {
         Dmsg0(500, "my_dbi_fetch_row freeing space\n");
         Dmsg2(500, "my_dbi_free_row row: '%p' num_fields: '%d'\n", mdb->row, mdb->num_fields);
         if (mdb->num_rows != 0) {
            for(j = 0; j < mdb->num_fields; j++) {
               Dmsg2(500, "my_dbi_free_row row '%p' '%d'\n", mdb->row[j], j);
                  if(mdb->row[j]) {
                     free(mdb->row[j]);
                  }
            }
         }
         free(mdb->row);
      }
      //num_fields += 20;                  /* add a bit extra */
      mdb->row = (DBI_ROW)malloc(sizeof(char *) * num_fields);
      mdb->row_size = num_fields;

      // now reset the row_number now that we have the space allocated
      mdb->row_number = 1;
   }

   // if still within the result set
   if (mdb->row_number <= mdb->num_rows && mdb->row_number != DBI_ERROR_BADPTR) {
      Dmsg2(500, "my_dbi_fetch_row row number '%d' is acceptable (1..%d)\n", mdb->row_number, mdb->num_rows);
      // get each value from this row
      for (j = 0; j < mdb->num_fields; j++) {
         mdb->row[j] = my_dbi_getvalue(mdb->result, mdb->row_number, j);
         // allocate space to queue row
         mdb->field_get = (DBI_FIELD_GET *)malloc(sizeof(DBI_FIELD_GET));
         // store the pointer in queue
         mdb->field_get->value = mdb->row[j];
         Dmsg4(500, "my_dbi_fetch_row row[%d] field: '%p' in queue: '%p' has value: '%s'\n",
               j, mdb->row[j], mdb->field_get->value, mdb->row[j]);
         // insert in queue to future free
         qinsert(&dbi_getvalue_list, &mdb->field_get->bq);
      }
      // increment the row number for the next call
      mdb->row_number++;

      row = mdb->row;
   } else {
      Dmsg2(500, "my_dbi_fetch_row row number '%d' is NOT acceptable (1..%d)\n", mdb->row_number, mdb->num_rows);
   }

   Dmsg1(500, "my_dbi_fetch_row finishes returning %p\n", row);

   return row;
}

int my_dbi_max_length(B_DB *mdb, int field_num) {
   //
   // for a given column, find the max length
   //
   int max_length;
   int i;
   int this_length;
   char *cbuf = NULL;

   max_length = 0;
   for (i = 0; i < mdb->num_rows; i++) {
      if (my_dbi_getisnull(mdb->result, i, field_num)) {
          this_length = 4;        // "NULL"
      } else {
         cbuf = my_dbi_getvalue(mdb->result, i, field_num);
         this_length = cstrlen(cbuf);
         // cbuf is always free
         free(cbuf);
      }

      if (max_length < this_length) {
          max_length = this_length;
      }
   }

   return max_length;
}

DBI_FIELD * my_dbi_fetch_field(B_DB *mdb)
{
   int     i;
   int     dbi_index;

   Dmsg0(500, "my_dbi_fetch_field starts\n");

   if (!mdb->fields || mdb->fields_size < mdb->num_fields) {
      if (mdb->fields) {
         free(mdb->fields);
      }
      Dmsg1(500, "allocating space for %d fields\n", mdb->num_fields);
      mdb->fields = (DBI_FIELD *)malloc(sizeof(DBI_FIELD) * mdb->num_fields);
      mdb->fields_size = mdb->num_fields;

      for (i = 0; i < mdb->num_fields; i++) {
         // num_fileds is starting at 1, increment i by 1
         dbi_index = i + 1;
         Dmsg1(500, "filling field %d\n", i);
         mdb->fields[i].name       = (char *)dbi_result_get_field_name(mdb->result, dbi_index);
         mdb->fields[i].max_length = my_dbi_max_length(mdb, i);
         mdb->fields[i].type       = dbi_result_get_field_type_idx(mdb->result, dbi_index);
         mdb->fields[i].flags      = dbi_result_get_field_attribs_idx(mdb->result, dbi_index);

         Dmsg4(500, "my_dbi_fetch_field finds field '%s' has length='%d' type='%d' and IsNull=%d\n",
            mdb->fields[i].name, mdb->fields[i].max_length, mdb->fields[i].type,
            mdb->fields[i].flags);
      } // end for
   } // end if

   // increment field number for the next time around

   Dmsg0(500, "my_dbi_fetch_field finishes\n");
   return &mdb->fields[mdb->field_number++];
}

void my_dbi_data_seek(B_DB *mdb, int row)
{
   // set the row number to be returned on the next call
   // to my_dbi_fetch_row
   mdb->row_number = row;
}

void my_dbi_field_seek(B_DB *mdb, int field)
{
   mdb->field_number = field;
}

/*
 * Note, if this routine returns 1 (failure), Bacula expects
 *  that no result has been stored.
 *
 *  Returns:  0  on success
 *            1  on failure
 *
 */
int my_dbi_query(B_DB *mdb, const char *query)
{
   const char *errmsg;
   Dmsg1(500, "my_dbi_query started %s\n", query);
   // We are starting a new query.  reset everything.
   mdb->num_rows     = -1;
   mdb->row_number   = -1;
   mdb->field_number = -1;

   if (mdb->result) {
      dbi_result_free(mdb->result);  /* hmm, someone forgot to free?? */
      mdb->result = NULL;
   }

   mdb->result = (void **)dbi_conn_query(mdb->db, query);

   if (!mdb->result) {
      Dmsg2(50, "Query failed: %s %p\n", query, mdb->result);
      goto bail_out;
   }

   mdb->status = (dbi_error_flag) dbi_conn_error(mdb->db, &errmsg);

   if (mdb->status == DBI_ERROR_NONE) {
      Dmsg1(500, "we have a result\n", query);

      // how many fields in the set?
      // num_fields starting at 1
      mdb->num_fields = dbi_result_get_numfields(mdb->result);
      Dmsg1(500, "we have %d fields\n", mdb->num_fields);
      // if no result num_rows is 0
      mdb->num_rows = dbi_result_get_numrows(mdb->result);
      Dmsg1(500, "we have %d rows\n", mdb->num_rows);

      mdb->status = (dbi_error_flag) 0;                  /* succeed */
   } else {
      Dmsg1(50, "Result status failed: %s\n", query);
      goto bail_out;
   }

   Dmsg0(500, "my_dbi_query finishing\n");
   return mdb->status;

bail_out:
   mdb->status = (dbi_error_flag) dbi_conn_error(mdb->db,&errmsg);
   //dbi_conn_error(mdb->db, &errmsg);
   Dmsg4(500, "my_dbi_query we failed dbi error: "
                   "'%s' '%p' '%d' flag '%d''\n", errmsg, mdb->result, mdb->result, mdb->status);
   dbi_result_free(mdb->result);
   mdb->result = NULL;
   mdb->status = (dbi_error_flag) 1;                   /* failed */
   return mdb->status;
}

void my_dbi_free_result(B_DB *mdb)
{

   DBI_FIELD_GET *f;
   db_lock(mdb);
   if (mdb->result) {
      Dmsg1(500, "my_dbi_free_result result '%p'\n", mdb->result);
      dbi_result_free(mdb->result);
   }

   mdb->result = NULL;

   if (mdb->row) {
      free(mdb->row);
   }

   /* now is time to free all value return by my_dbi_get_value
    * this is necessary because libdbi don't free memory return by yours results
    * and Bacula has some routine wich call more than once time my_dbi_fetch_row
    *
    * Using a queue to store all pointer allocate is a good way to free all things
    * when necessary
    */
   while((f=(DBI_FIELD_GET *)qremove(&dbi_getvalue_list))) {
      Dmsg2(500, "my_dbi_free_result field value: '%p' in queue: '%p'\n", f->value, f);
      free(f->value);
      free(f);
   }

   mdb->row = NULL;

   if (mdb->fields) {
      free(mdb->fields);
      mdb->fields = NULL;
   }
   db_unlock(mdb);
   Dmsg0(500, "my_dbi_free_result finish\n");

}

const char *my_dbi_strerror(B_DB *mdb)
{
   const char *errmsg;

   dbi_conn_error(mdb->db, &errmsg);

   return errmsg;
}

#ifdef HAVE_BATCH_FILE_INSERT

/*
 * This can be a bit strang but is the one way to do
 *
 * Returns 1 if OK
 *         0 if failed
 */
int my_dbi_batch_start(JCR *jcr, B_DB *mdb)
{
   char *query = "COPY batch FROM STDIN";

   Dmsg0(500, "my_dbi_batch_start started\n");

   switch (mdb->db_type) {
   case SQL_TYPE_MYSQL:
      db_lock(mdb);
      if (my_dbi_query(mdb,
                              "CREATE TEMPORARY TABLE batch ("
                                  "FileIndex integer,"
                                  "JobId integer,"
                                  "Path blob,"
                                  "Name blob,"
                                  "LStat tinyblob,"
                                  "MD5 tinyblob)") == 1)
      {
         Dmsg0(500, "my_dbi_batch_start failed\n");
         return 1;
      }
      db_unlock(mdb);
      Dmsg0(500, "my_dbi_batch_start finishing\n");
      return 1;
      break;
   case SQL_TYPE_POSTGRESQL:

      if (my_dbi_query(mdb, "CREATE TEMPORARY TABLE batch ("
                                  "fileindex int,"
                                  "jobid int,"
                                  "path varchar,"
                                  "name varchar,"
                                  "lstat varchar,"
                                  "md5 varchar)") == 1)
      {
         Dmsg0(500, "my_dbi_batch_start failed\n");
         return 1;
      }

      // We are starting a new query.  reset everything.
      mdb->num_rows     = -1;
      mdb->row_number   = -1;
      mdb->field_number = -1;

      my_dbi_free_result(mdb);

      for (int i=0; i < 10; i++) {
         my_dbi_query(mdb, query);
         if (mdb->result) {
            break;
         }
         bmicrosleep(5, 0);
      }
      if (!mdb->result) {
         Dmsg1(50, "Query failed: %s\n", query);
         goto bail_out;
      }

      mdb->status = (dbi_error_flag)dbi_conn_error(mdb->db, NULL);
      //mdb->status = DBI_ERROR_NONE;

      if (mdb->status == DBI_ERROR_NONE) {
         // how many fields in the set?
         mdb->num_fields = dbi_result_get_numfields(mdb->result);
         mdb->num_rows   = dbi_result_get_numrows(mdb->result);
         mdb->status = (dbi_error_flag) 1;
      } else {
         Dmsg1(50, "Result status failed: %s\n", query);
         goto bail_out;
      }

      Dmsg0(500, "my_postgresql_batch_start finishing\n");

      return mdb->status;
      break;
   case SQL_TYPE_SQLITE:
      db_lock(mdb);
      if (my_dbi_query(mdb, "CREATE TEMPORARY TABLE batch ("
                                  "FileIndex integer,"
                                  "JobId integer,"
                                  "Path blob,"
                                  "Name blob,"
                                  "LStat tinyblob,"
                                  "MD5 tinyblob)") == 1)
      {
         Dmsg0(500, "my_dbi_batch_start failed\n");
         goto bail_out;
      }
      db_unlock(mdb);
      Dmsg0(500, "my_dbi_batch_start finishing\n");
      return 1;
      break;
   case SQL_TYPE_SQLITE3:
      db_lock(mdb);
      if (my_dbi_query(mdb, "CREATE TEMPORARY TABLE batch ("
                                  "FileIndex integer,"
                                  "JobId integer,"
                                  "Path blob,"
                                  "Name blob,"
                                  "LStat tinyblob,"
                                  "MD5 tinyblob)") == 1)
      {
         Dmsg0(500, "my_dbi_batch_start failed\n");
         goto bail_out;
      }
      db_unlock(mdb);
      Dmsg0(500, "my_dbi_batch_start finishing\n");
      return 1;
      break;
   }

bail_out:
   Mmsg1(&mdb->errmsg, _("error starting batch mode: %s"), my_dbi_strerror(mdb));
   mdb->status = (dbi_error_flag) 0;
   my_dbi_free_result(mdb);
   mdb->result = NULL;
   return mdb->status;
}

/* set error to something to abort operation */
int my_dbi_batch_end(JCR *jcr, B_DB *mdb, const char *error)
{
   int res = 0;
   int count = 30;
   int (*custom_function)(void*, const char*) = NULL;
   dbi_conn_t *myconn = (dbi_conn_t *)(mdb->db);

   Dmsg0(500, "my_dbi_batch_end started\n");

   if (!mdb) {                  /* no files ? */
      return 0;
   }

   switch (mdb->db_type) {
   case SQL_TYPE_MYSQL:
      if(mdb) {
         mdb->status = (dbi_error_flag) 0;
      }
      break;
   case SQL_TYPE_POSTGRESQL:
      custom_function = (custom_function_end_t)dbi_driver_specific_function(dbi_conn_get_driver(mdb->db), "PQputCopyEnd");


      do {
         res = (*custom_function)(myconn->connection, error);
      } while (res == 0 && --count > 0);

      if (res == 1) {
         Dmsg0(500, "ok\n");
         mdb->status = (dbi_error_flag) 1;
      }

      if (res <= 0) {
         Dmsg0(500, "we failed\n");
         mdb->status = (dbi_error_flag) 0;
         //Mmsg1(&mdb->errmsg, _("error ending batch mode: %s"), PQerrorMessage(mdb->db));
       }
      break;
   case SQL_TYPE_SQLITE:
      if(mdb) {
         mdb->status = (dbi_error_flag) 0;
      }
      break;
   case SQL_TYPE_SQLITE3:
      if(mdb) {
         mdb->status = (dbi_error_flag) 0;
      }
      break;
   }

   Dmsg0(500, "my_dbi_batch_end finishing\n");

   return true;
}

/*
 * This function is big and use a big switch.
 * In near future is better split in small functions
 * and refactory.
 *
 */
int my_dbi_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   int res;
   int count=30;
   dbi_conn_t *myconn = (dbi_conn_t *)(mdb->db);
   int (*custom_function)(void*, const char*, int) = NULL;
   char* (*custom_function_error)(void*) = NULL;
   size_t len;
   char *digest;
   char ed1[50];

   Dmsg0(500, "my_dbi_batch_insert started \n");

   mdb->esc_name = check_pool_memory_size(mdb->esc_name, mdb->fnl*2+1);
   mdb->esc_path = check_pool_memory_size(mdb->esc_path, mdb->pnl*2+1);

   if (ar->Digest == NULL || ar->Digest[0] == 0) {
      digest = "0";
   } else {
      digest = ar->Digest;
   }

   switch (mdb->db_type) {
   case SQL_TYPE_MYSQL:
      db_escape_string(jcr, mdb, mdb->esc_name, mdb->fname, mdb->fnl);
      db_escape_string(jcr, mdb, mdb->esc_path, mdb->path, mdb->pnl);
      len = Mmsg(mdb->cmd, "INSERT INTO batch VALUES (%u,%s,'%s','%s','%s','%s')",
                      ar->FileIndex, edit_int64(ar->JobId,ed1), mdb->esc_path,
                      mdb->esc_name, ar->attr, digest);

      if (my_dbi_query(mdb,mdb->cmd) == 1)
      {
         Dmsg0(500, "my_dbi_batch_insert failed\n");
         goto bail_out;
      }

      Dmsg0(500, "my_dbi_batch_insert finishing\n");

      return 1;
      break;
   case SQL_TYPE_POSTGRESQL:
      my_postgresql_copy_escape(mdb->esc_name, mdb->fname, mdb->fnl);
      my_postgresql_copy_escape(mdb->esc_path, mdb->path, mdb->pnl);
      len = Mmsg(mdb->cmd, "%u\t%s\t%s\t%s\t%s\t%s\n",
                     ar->FileIndex, edit_int64(ar->JobId, ed1), mdb->esc_path,
                     mdb->esc_name, ar->attr, digest);

      /* libdbi don't support CopyData and we need call a postgresql
       * specific function to do this work
       */
      Dmsg2(500, "my_dbi_batch_insert :\n %s \ncmd_size: %d",mdb->cmd, len);
      if ((custom_function = (custom_function_insert_t)dbi_driver_specific_function(dbi_conn_get_driver(mdb->db),
            "PQputCopyData")) != NULL) {
         do {
            res = (*custom_function)(myconn->connection, mdb->cmd, len);
         } while (res == 0 && --count > 0);

         if (res == 1) {
            Dmsg0(500, "ok\n");
            mdb->changes++;
            mdb->status = (dbi_error_flag) 1;
         }

         if (res <= 0) {
            Dmsg0(500, "my_dbi_batch_insert failed\n");
            goto bail_out;
         }

         Dmsg0(500, "my_dbi_batch_insert finishing\n");
         return mdb->status;
      } else {
         // ensure to detect a PQerror
         custom_function_error = (custom_function_error_t)dbi_driver_specific_function(dbi_conn_get_driver(mdb->db), "PQerrorMessage");
         Dmsg1(500, "my_dbi_batch_insert failed\n PQerrorMessage: %s", (*custom_function_error)(myconn->connection));
         goto bail_out;
      }
      break;
   case SQL_TYPE_SQLITE:
      db_escape_string(jcr, mdb, mdb->esc_name, mdb->fname, mdb->fnl);
      db_escape_string(jcr, mdb, mdb->esc_path, mdb->path, mdb->pnl);
      len = Mmsg(mdb->cmd, "INSERT INTO batch VALUES (%u,%s,'%s','%s','%s','%s')",
                      ar->FileIndex, edit_int64(ar->JobId,ed1), mdb->esc_path,
                      mdb->esc_name, ar->attr, digest);
      if (my_dbi_query(mdb,mdb->cmd) == 1)
      {
         Dmsg0(500, "my_dbi_batch_insert failed\n");
         goto bail_out;
      }

      Dmsg0(500, "my_dbi_batch_insert finishing\n");

      return 1;
      break;
   case SQL_TYPE_SQLITE3:
      db_escape_string(jcr, mdb, mdb->esc_name, mdb->fname, mdb->fnl);
      db_escape_string(jcr, mdb, mdb->esc_path, mdb->path, mdb->pnl);
      len = Mmsg(mdb->cmd, "INSERT INTO batch VALUES (%u,%s,'%s','%s','%s','%s')",
                      ar->FileIndex, edit_int64(ar->JobId,ed1), mdb->esc_path,
                      mdb->esc_name, ar->attr, digest);
      if (my_dbi_query(mdb,mdb->cmd) == 1)
      {
         Dmsg0(500, "my_dbi_batch_insert failed\n");
         goto bail_out;
      }

      Dmsg0(500, "my_dbi_batch_insert finishing\n");

      return 1;
      break;
   }

bail_out:
  Mmsg1(&mdb->errmsg, _("error inserting batch mode: %s"), my_dbi_strerror(mdb));
  mdb->status = (dbi_error_flag) 0;
  my_dbi_free_result(mdb);
  return mdb->status;
}

/*
 * Escape strings so that PostgreSQL is happy on COPY
 *
 *   NOTE! len is the length of the old string. Your new
 *         string must be long enough (max 2*old+1) to hold
 *         the escaped output.
 */
char *my_postgresql_copy_escape(char *dest, char *src, size_t len)
{
   /* we have to escape \t, \n, \r, \ */
   char c = '\0' ;

   while (len > 0 && *src) {
      switch (*src) {
      case '\n':
         c = 'n';
         break;
      case '\\':
         c = '\\';
         break;
      case '\t':
         c = 't';
         break;
      case '\r':
         c = 'r';
         break;
      default:
         c = '\0' ;
      }

      if (c) {
         *dest = '\\';
         dest++;
         *dest = c;
      } else {
         *dest = *src;
      }

      len--;
      src++;
      dest++;
   }

   *dest = '\0';
   return dest;
}

#endif /* HAVE_BATCH_FILE_INSERT */

/* my_dbi_getisnull
 * like PQgetisnull
 * int PQgetisnull(const PGresult *res,
 *              int row_number,
 *               int column_number);
 *
 *  use dbi_result_seek_row to search in result set
 */
int my_dbi_getisnull(dbi_result *result, int row_number, int column_number) {
   int i;

   if(row_number == 0) {
      row_number++;
   }

   column_number++;

   if(dbi_result_seek_row(result, row_number)) {

      i = dbi_result_field_is_null_idx(result,column_number);

      return i;
   } else {

      return 0;
   }

}
/* my_dbi_getvalue
 * like PQgetvalue;
 * char *PQgetvalue(const PGresult *res,
 *                int row_number,
 *                int column_number);
 *
 * use dbi_result_seek_row to search in result set
 * use example to return only strings
 */
char *my_dbi_getvalue(dbi_result *result, int row_number, unsigned int column_number) {

   char *buf = NULL;
   const char *errmsg;
   const char *field_name;
   unsigned short dbitype;
   size_t field_length;
   int64_t num;

   /* correct the index for dbi interface
    * dbi index begins 1
    * I prefer do not change others functions
    */
   Dmsg3(600, "my_dbi_getvalue pre-starting result '%p' row number '%d' column number '%d'\n",
                                result, row_number, column_number);

   column_number++;

   if(row_number == 0) {
     row_number++;
   }

   Dmsg3(600, "my_dbi_getvalue starting result '%p' row number '%d' column number '%d'\n",
                        result, row_number, column_number);

   if(dbi_result_seek_row(result, row_number)) {

      field_name = dbi_result_get_field_name(result, column_number);
      field_length = dbi_result_get_field_length(result, field_name);
      dbitype = dbi_result_get_field_type_idx(result,column_number);

      Dmsg3(500, "my_dbi_getvalue start: type: '%d' "
            "field_length bytes: '%d' fieldname: '%s'\n",
            dbitype, field_length, field_name);

      if(field_length) {
         //buf = (char *)malloc(sizeof(char *) * field_length + 1);
         buf = (char *)malloc(field_length + 1);
      } else {
         /* if numbers */
         buf = (char *)malloc(sizeof(char *) * 50);
      }

      switch (dbitype) {
      case DBI_TYPE_INTEGER:
         num = dbi_result_get_longlong(result, field_name);
         edit_int64(num, buf);
         field_length = strlen(buf);
         break;
      case DBI_TYPE_STRING:
         if(field_length) {
            field_length = bsnprintf(buf, field_length + 1, "%s",
            dbi_result_get_string(result, field_name));
         } else {
            buf[0] = 0;
         }
         break;
      case DBI_TYPE_BINARY:
         /* dbi_result_get_binary return a NULL pointer if value is empty
         * following, change this to what Bacula espected
         */
         if(field_length) {
            field_length = bsnprintf(buf, field_length + 1, "%s",
                  dbi_result_get_binary(result, field_name));
         } else {
            buf[0] = 0;
         }
         break;
      case DBI_TYPE_DATETIME:
         time_t last;
         struct tm tm;

         last = dbi_result_get_datetime(result, field_name);

         if(last == -1) {
                field_length = bsnprintf(buf, 20, "0000-00-00 00:00:00");
         } else {
            (void)localtime_r(&last, &tm);
            field_length = bsnprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d",
                  (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
         }
         break;
      }

   } else {
      dbi_conn_error(dbi_result_get_conn(result), &errmsg);
      Dmsg1(500, "my_dbi_getvalue error: %s\n", errmsg);
   }

   Dmsg3(500, "my_dbi_getvalue finish buffer: '%p' num bytes: '%d' data: '%s'\n",
      buf, field_length, buf);

   // don't worry about this buf
   return buf;
}

int my_dbi_sql_insert_id(B_DB *mdb, char *table_name)
{
   /*
    Obtain the current value of the sequence that
    provides the serial value for primary key of the table.

    currval is local to our session.  It is not affected by
    other transactions.

    Determine the name of the sequence.
    PostgreSQL automatically creates a sequence using
    <table>_<column>_seq.
    At the time of writing, all tables used this format for
    for their primary key: <table>id
    Except for basefiles which has a primary key on baseid.
    Therefore, we need to special case that one table.

    everything else can use the PostgreSQL formula.
   */

   char      sequence[30];
   uint64_t    id = 0;

   if (mdb->db_type == SQL_TYPE_POSTGRESQL) {

      if (strcasecmp(table_name, "basefiles") == 0) {
         bstrncpy(sequence, "basefiles_baseid", sizeof(sequence));
      } else {
         bstrncpy(sequence, table_name, sizeof(sequence));
         bstrncat(sequence, "_",        sizeof(sequence));
         bstrncat(sequence, table_name, sizeof(sequence));
         bstrncat(sequence, "id",       sizeof(sequence));
      }

      bstrncat(sequence, "_seq", sizeof(sequence));
      id = dbi_conn_sequence_last(mdb->db, NT_(sequence));
   } else {
      id = dbi_conn_sequence_last(mdb->db, NT_(table_name));
   }

   return id;
}

#ifdef HAVE_BATCH_FILE_INSERT
const char *my_dbi_batch_lock_path_query[4] = {
   /* Mysql */
   "LOCK TABLES Path write, batch write, Path as p write",
   /* Postgresql */
   "BEGIN; LOCK TABLE Path IN SHARE ROW EXCLUSIVE MODE",
   /* SQLite */
   "BEGIN",
   /* SQLite3 */
   "BEGIN"};

const char *my_dbi_batch_lock_filename_query[4] = {
   /* Mysql */
   "LOCK TABLES Filename write, batch write, Filename as f write",
   /* Postgresql */
   "BEGIN; LOCK TABLE Filename IN SHARE ROW EXCLUSIVE MODE",
   /* SQLite */
   "BEGIN",
   /* SQLite3 */
   "BEGIN"};

const char *my_dbi_batch_unlock_tables_query[4] = {
   /* Mysql */
   "UNLOCK TABLES",
   /* Postgresql */
   "COMMIT",
   /* SQLite */
   "COMMIT",
   /* SQLite3 */
   "COMMIT"};

const char *my_dbi_batch_fill_path_query[4] = {
   /* Mysql */
   "INSERT INTO Path (Path) "
   "SELECT a.Path FROM "
   "(SELECT DISTINCT Path FROM batch) AS a WHERE NOT EXISTS "
   "(SELECT Path FROM Path AS p WHERE p.Path = a.Path)",
   /* Postgresql */
   "INSERT INTO Path (Path) "
   "SELECT a.Path FROM "
   "(SELECT DISTINCT Path FROM batch) AS a "
   "WHERE NOT EXISTS (SELECT Path FROM Path WHERE Path = a.Path) ",
   /* SQLite */
   "INSERT INTO Path (Path)"
   " SELECT DISTINCT Path FROM batch"
   " EXCEPT SELECT Path FROM Path",
   /* SQLite3 */
   "INSERT INTO Path (Path)"
   " SELECT DISTINCT Path FROM batch"
   " EXCEPT SELECT Path FROM Path"};

const char *my_dbi_batch_fill_filename_query[4] = {
   /* Mysql */
   "INSERT INTO Filename (Name) "
   "SELECT a.Name FROM "
   "(SELECT DISTINCT Name FROM batch) AS a WHERE NOT EXISTS "
   "(SELECT Name FROM Filename AS f WHERE f.Name = a.Name)",
   /* Postgresql */
   "INSERT INTO Filename (Name) "
   "SELECT a.Name FROM "
   "(SELECT DISTINCT Name FROM batch) as a "
   "WHERE NOT EXISTS "
   "(SELECT Name FROM Filename WHERE Name = a.Name)",
   /* SQLite */
   "INSERT INTO Filename (Name)"
   " SELECT DISTINCT Name FROM batch "
   " EXCEPT SELECT Name FROM Filename",
   /* SQLite3 */
   "INSERT INTO Filename (Name)"
   " SELECT DISTINCT Name FROM batch "
   " EXCEPT SELECT Name FROM Filename"};

#endif /* HAVE_BATCH_FILE_INSERT */

#endif /* HAVE_DBI */
