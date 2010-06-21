/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2010 Free Software Foundation Europe e.V.

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
 * Bacula Catalog Database routines specific to Ingres
 *   These are Ingres specific routines
 *
 *    Stefan Reddig, June 2009
 *    based uopn work done 
 *    by Dan Langille, December 2003 and
 *    by Kern Sibbald, March 2000
 *
 */


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#ifdef HAVE_INGRES

#include "myingres.h"

/* -----------------------------------------------------------------------
 *
 *   Ingres dependent defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

/* List of open databases */  /* SRE: needed for ingres? */
static BQUEUE db_list = {&db_list, &db_list};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Retrieve database type
 */
const char *
db_get_type(void)
{
   return "Ingres";
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

   if (!db_user) {
      Jmsg(jcr, M_FATAL, 0, _("A user name for Ingres must be supplied.\n"));
      return NULL;
   }
   P(mutex);                          /* lock DB queue */
   if (!mult_db_connections) {
      /* Look to see if DB already open */
      for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
         if (bstrcmp(mdb->db_name, db_name) &&
             bstrcmp(mdb->db_address, db_address) &&
             mdb->db_port == db_port) {
            Dmsg2(100, "DB REopen %d %s\n", mdb->ref_count, db_name);
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

/* Check that the database correspond to the encoding we want */
static bool check_database_encoding(JCR *jcr, B_DB *mdb)
{
/* SRE: TODO! Needed?
 SQL_ROW row;
   int ret=false;

   if (!db_sql_query(mdb, "SELECT getdatabaseencoding()", NULL, NULL)) {
      Jmsg(jcr, M_ERROR, 0, "%s", mdb->errmsg);
      return false;
   }

   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
      Jmsg(jcr, M_ERROR, 0, "Can't check database encoding %s", mdb->errmsg);
   } else {
      ret = bstrcmp(row[0], "SQL_ASCII");
      if (!ret) {
         Mmsg(mdb->errmsg, 
              _("Encoding error for database \"%s\". Wanted SQL_ASCII, got %s\n"),
              mdb->db_name, row[0]);
         Jmsg(jcr, M_WARNING, 0, "%s", mdb->errmsg);
         Dmsg1(50, "%s", mdb->errmsg);
      } 
   }
   return ret;
*/
    return true;
}

/*
 * Check for errors in DBMS work
 */
static int sql_check(B_DB *mdb)
{
    int errorcode;

    if ((errorcode = INGcheck()) < 0) {
        /* TODO: fill mdb->errmsg */
        Mmsg(mdb->errmsg, "Something went wrong - still searching!\n");
    } else if (errorcode > 0) {
	/* just a warning, proceed */
    }
    return errorcode;
}

/*
 * Now actually open the database.  This can generate errors,
 *   which are returned in the errmsg
 *
 * DO NOT close the database or free(mdb) here !!!!
 */
int
db_open_database(JCR *jcr, B_DB *mdb)
{
   int errstat;
   char buf[10], *port;

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

   mdb->db = INGconnectDB(mdb->db_name, mdb->db_user, mdb->db_password);

   Dmsg0(50, "Ingres real CONNECT done\n");
   Dmsg3(50, "db_user=%s db_name=%s db_password=%s\n", mdb->db_user, mdb->db_name,
            mdb->db_password==NULL?"(NULL)":mdb->db_password);

   if (sql_check(mdb)) {
      Mmsg2(&mdb->errmsg, _("Unable to connect to Ingres server.\n"
            "Database=%s User=%s\n"
            "It is probably not running or your password is incorrect.\n"),
             mdb->db_name, mdb->db_user);
      V(mutex);
      return 0;
   }

   mdb->connected = true;

   if (!check_tables_version(jcr, mdb)) {
      V(mutex);
      return 0;
   }

   //sql_query(mdb, "SET datestyle TO 'ISO, YMD'");
   
   /* check that encoding is SQL_ASCII */
   check_database_encoding(jcr, mdb);

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
         sql_close(mdb);
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
      free(mdb);
   }
   V(mutex);
}

void db_check_backend_thread_safe()
{ }



void db_thread_cleanup()
{ }

/*
 * Return the next unique index (auto-increment) for
 * the given table.  Return NULL on error.
 *
 * For Ingres, NULL causes the auto-increment value     SRE: true?
 *  to be updated.
 */
int db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index)
{
   strcpy(index, "NULL");
   return 1;
}


/*
 * Escape strings so that Ingres is happy
 *
 *   NOTE! len is the length of the old string. Your new
 *         string must be long enough (max 2*old+1) to hold
 *         the escaped output.
 */
void
db_escape_string(JCR *jcr, B_DB *mdb, char *snew, char *old, int len)
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
      if (mdb->result != NULL) {
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

/*
 * Close database connection
 */
void my_ingres_close(B_DB *mdb)
{
    INGdisconnectDB(mdb->db);
    //SRE: error handling? 
}

INGRES_ROW my_ingres_fetch_row(B_DB *mdb)
{
   int j;
   INGRES_ROW row = NULL; // by default, return NULL

   if (!mdb->result) {
      return row;
   }
   if (mdb->result->num_rows <= 0) {
      return row;
   }

   Dmsg0(500, "my_ingres_fetch_row start\n");

   if (!mdb->row || mdb->row_size < mdb->num_fields) {
      int num_fields = mdb->num_fields;
      Dmsg1(500, "we have need space of %d bytes\n", sizeof(char *) * mdb->num_fields);

      if (mdb->row) {
         Dmsg0(500, "my_ingres_fetch_row freeing space\n");
         free(mdb->row);
      }
      num_fields += 20;                  /* add a bit extra */
      mdb->row = (INGRES_ROW)malloc(sizeof(char *) * num_fields);
      mdb->row_size = num_fields;

      // now reset the row_number now that we have the space allocated
      mdb->row_number = 1;
   }

   // if still within the result set
   if (mdb->row_number <= mdb->num_rows) {
      Dmsg2(500, "my_ingres_fetch_row row number '%d' is acceptable (0..%d)\n", mdb->row_number, mdb->num_rows);
      // get each value from this row
      for (j = 0; j < mdb->num_fields; j++) {
         mdb->row[j] = INGgetvalue(mdb->result, mdb->row_number, j);
         Dmsg2(500, "my_ingres_fetch_row field '%d' has value '%s'\n", j, mdb->row[j]);
      }
      // increment the row number for the next call
      mdb->row_number++;

      row = mdb->row;
   } else {
      Dmsg2(500, "my_ingres_fetch_row row number '%d' is NOT acceptable (0..%d)\n", mdb->row_number, mdb->num_rows);
   }

   Dmsg1(500, "my_ingres_fetch_row finishes returning %p\n", row);

   return row;
}


int my_ingres_max_length(B_DB *mdb, int field_num) {
   //
   // for a given column, find the max length
   //
   int max_length;
   int i;
   int this_length;

   max_length = 0;
   for (i = 0; i < mdb->num_rows; i++) {
      if (INGgetisnull(mdb->result, i, field_num)) {
          this_length = 4;        // "NULL"
      } else {
          this_length = cstrlen(INGgetvalue(mdb->result, i, field_num));
      }

      if (max_length < this_length) {
          max_length = this_length;
      }
   }

   return max_length;
}

INGRES_FIELD * my_ingres_fetch_field(B_DB *mdb)
{
   int i;

   Dmsg0(500, "my_ingres_fetch_field starts\n");

   if (!mdb->fields || mdb->fields_size < mdb->num_fields) {
      if (mdb->fields) {
         free(mdb->fields);
      }
      Dmsg1(500, "allocating space for %d fields\n", mdb->num_fields);
      mdb->fields = (INGRES_FIELD *)malloc(sizeof(INGRES_FIELD) * mdb->num_fields);
      mdb->fields_size = mdb->num_fields;

      for (i = 0; i < mdb->num_fields; i++) {
         Dmsg1(500, "filling field %d\n", i);
         strcpy(mdb->fields[i].name,INGfname(mdb->result, i));
         mdb->fields[i].max_length = my_ingres_max_length(mdb, i);
         mdb->fields[i].type       = INGftype(mdb->result, i);
         mdb->fields[i].flags      = 0;

         Dmsg4(500, "my_ingres_fetch_field finds field '%s' has length='%d' type='%d' and IsNull=%d\n",
            mdb->fields[i].name, mdb->fields[i].max_length, mdb->fields[i].type,
            mdb->fields[i].flags);
      } // end for
   } // end if

   // increment field number for the next time around

   Dmsg0(500, "my_ingres_fetch_field finishes\n");
   return &mdb->fields[mdb->field_number++];
}

void my_ingres_data_seek(B_DB *mdb, int row)
{
   // set the row number to be returned on the next call
   // to my_ingres_fetch_row
   mdb->row_number = row;
}

void my_ingres_field_seek(B_DB *mdb, int field)
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
int my_ingres_query(B_DB *mdb, const char *query)
{
   Dmsg0(500, "my_ingres_query started\n");
   // We are starting a new query.  reset everything.
   mdb->num_rows     = -1;
   mdb->row_number   = -1;
   mdb->field_number = -1;

   int cols = -1;

   if (mdb->result) {
      INGclear(mdb->result);  /* hmm, someone forgot to free?? */
      mdb->result = NULL;
   }

   Dmsg1(500, "my_ingres_query starts with '%s'\n", query);

   /* TODO: differentiate between SELECTs and other queries */

   if ((cols = INGgetCols(query)) <= 0) {
      if (cols < 0 ) {
         Dmsg0(500,"my_ingres_query: neg.columns: no DML stmt!\n");
      }
      Dmsg0(500,"my_ingres_query (non SELECT) starting...\n");
      /* non SELECT */
      mdb->num_rows = INGexec(mdb->db, query);
      if (INGcheck()) {
        Dmsg0(500,"my_ingres_query (non SELECT) went wrong\n");
        mdb->status = 1;
      } else {
        Dmsg0(500,"my_ingres_query (non SELECT) seems ok\n");
        mdb->status = 0;
      }
   } else {
      /* SELECT */
      Dmsg0(500,"my_ingres_query (SELECT) starting...\n");
      mdb->result = INGquery(mdb->db, query);
      if ( mdb->result != NULL ) {
        Dmsg1(500, "we have a result\n", query);

        // how many fields in the set?
        mdb->num_fields = (int)INGnfields(mdb->result);
        Dmsg1(500, "we have %d fields\n", mdb->num_fields);

        mdb->num_rows = INGntuples(mdb->result);
        Dmsg1(500, "we have %d rows\n", mdb->num_rows);

        mdb->status = 0;                  /* succeed */
      } else {
        Dmsg0(500, "No resultset...\n");
        mdb->status = 1; /* failed */
      }
   }

   Dmsg0(500, "my_ingres_query finishing\n");
   return mdb->status;
}

void my_ingres_free_result(B_DB *mdb)
{
   
   db_lock(mdb);
   if (mdb->result) {
      INGclear(mdb->result);
      mdb->result = NULL;
   }

   if (mdb->row) {
      free(mdb->row);
      mdb->row = NULL;
   }

   if (mdb->fields) {
      free(mdb->fields);
      mdb->fields = NULL;
   }
   db_unlock(mdb);
}

int my_ingres_currval(B_DB *mdb, const char *table_name)
{
   // TODO!
   return -1;
}

#ifdef HAVE_BATCH_FILE_INSERT

int my_ingres_batch_start(JCR *jcr, B_DB *mdb)
{
    //TODO!
   return ING_ERROR;
}

/* set error to something to abort operation */
int my_ingres_batch_end(JCR *jcr, B_DB *mdb, const char *error)
{
    //TODO!
   return ING_ERROR;
}

int my_ingres_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
    //TODO!
   return ING_ERROR;
}

#endif /* HAVE_BATCH_FILE_INSERT */

/*
 * Escape strings so that Ingres is happy on COPY
 *
 *   NOTE! len is the length of the old string. Your new
 *         string must be long enough (max 2*old+1) to hold
 *         the escaped output.
 */
char *my_ingres_copy_escape(char *dest, char *src, size_t len)
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

#ifdef HAVE_BATCH_FILE_INSERT
const char *my_ingres_batch_lock_path_query = 
   "BEGIN; LOCK TABLE Path IN SHARE ROW EXCLUSIVE MODE";


const char *my_ingres_batch_lock_filename_query = 
   "BEGIN; LOCK TABLE Filename IN SHARE ROW EXCLUSIVE MODE";

const char *my_ingres_batch_unlock_tables_query = "COMMIT";

const char *my_ingres_batch_fill_path_query = 
   "INSERT INTO Path (Path) "
    "SELECT a.Path FROM "
     "(SELECT DISTINCT Path FROM batch) AS a "
      "WHERE NOT EXISTS (SELECT Path FROM Path WHERE Path = a.Path) ";


const char *my_ingres_batch_fill_filename_query = 
   "INSERT INTO Filename (Name) "
    "SELECT a.Name FROM "
     "(SELECT DISTINCT Name FROM batch) as a "
      "WHERE NOT EXISTS "
       "(SELECT Name FROM Filename WHERE Name = a.Name)";
#endif /* HAVE_BATCH_FILE_INSERT */

#endif /* HAVE_INGRES */
