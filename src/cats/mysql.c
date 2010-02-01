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
 * Bacula Catalog Database routines specific to MySQL
 *   These are MySQL specific routines -- hopefully all
 *    other files are generic.
 *
 *    Kern Sibbald, March 2000
 *
 *    Version $Id$
 */


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#ifdef HAVE_MYSQL

/* -----------------------------------------------------------------------
 *
 *   MySQL dependent defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

/* List of open databases */
static BQUEUE db_list = {&db_list, &db_list};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Retrieve database type
 */
const char *
db_get_type(void)
{
   return "MySQL";
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
      Jmsg(jcr, M_FATAL, 0, _("A user name for MySQL must be supplied.\n"));
      return NULL;
   }
   P(mutex);                          /* lock DB queue */
   /* Look to see if DB already open */
   if (!mult_db_connections) {
      for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
         if (bstrcmp(mdb->db_name, db_name) &&
             bstrcmp(mdb->db_address, db_address) &&
             mdb->db_port == db_port) {
            Dmsg2(100, "DB REopen %d %s\n", mdb->ref_count, db_name);
            mdb->ref_count++;
            V(mutex);
            Dmsg3(100, "initdb ref=%d connected=%d db=%p\n", mdb->ref_count,
                  mdb->connected, mdb->db);
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
      mdb->db_address = bstrdup(db_address);
   }
   if (db_socket) {
      mdb->db_socket = bstrdup(db_socket);
   }
   mdb->db_port = db_port;
   mdb->have_insert_id = true;
   mdb->errmsg = get_pool_memory(PM_EMSG); /* get error message buffer */
   *mdb->errmsg = 0;
   mdb->cmd = get_pool_memory(PM_EMSG);    /* get command buffer */
   mdb->cached_path = get_pool_memory(PM_FNAME);
   mdb->cached_path_id = 0;
   mdb->ref_count = 1;
   mdb->fname = get_pool_memory(PM_FNAME);
   mdb->path = get_pool_memory(PM_FNAME);
   mdb->esc_name = get_pool_memory(PM_FNAME);
   mdb->esc_path = get_pool_memory(PM_FNAME);
   mdb->allow_transactions = mult_db_connections;
   qinsert(&db_list, &mdb->bq);            /* put db in list */
   Dmsg3(100, "initdb ref=%d connected=%d db=%p\n", mdb->ref_count,
         mdb->connected, mdb->db);
   V(mutex);
   return mdb;
}

/*
 * Now actually open the database.  This can generate errors,
 *  which are returned in the errmsg
 *
 * DO NOT close the database or free(mdb) here !!!!
 */
int
db_open_database(JCR *jcr, B_DB *mdb)
{
   int errstat;

   P(mutex);
   if (mdb->connected) {
      V(mutex);
      return 1;
   }

   if ((errstat=rwl_init(&mdb->lock)) != 0) {
      berrno be;
      Mmsg1(&mdb->errmsg, _("Unable to initialize DB lock. ERR=%s\n"),
            be.bstrerror(errstat));
      V(mutex);
      return 0;
   }

   /* connect to the database */
#ifdef xHAVE_EMBEDDED_MYSQL
// mysql_server_init(0, NULL, NULL);
#endif
   mysql_init(&mdb->mysql);

   Dmsg0(50, "mysql_init done\n");
   /* If connection fails, try at 5 sec intervals for 30 seconds. */
   for (int retry=0; retry < 6; retry++) {
      mdb->db = mysql_real_connect(
           &(mdb->mysql),                /* db */
           mdb->db_address,              /* default = localhost */
           mdb->db_user,                 /*  login name */
           mdb->db_password,             /*  password */
           mdb->db_name,                 /* database name */
           mdb->db_port,                 /* default port */
           mdb->db_socket,               /* default = socket */
           CLIENT_FOUND_ROWS);           /* flags */

      /* If no connect, try once more in case it is a timing problem */
      if (mdb->db != NULL) {
         break;
      }
      bmicrosleep(5,0);
   }

   mdb->mysql.reconnect = 1;             /* so connection does not timeout */
   Dmsg0(50, "mysql_real_connect done\n");
   Dmsg3(50, "db_user=%s db_name=%s db_password=%s\n", mdb->db_user, mdb->db_name,
            mdb->db_password==NULL?"(NULL)":mdb->db_password);

   if (mdb->db == NULL) {
      Mmsg2(&mdb->errmsg, _("Unable to connect to MySQL server.\n"
"Database=%s User=%s\n"
"MySQL connect failed either server not running or your authorization is incorrect.\n"),
         mdb->db_name, mdb->db_user);
#if MYSQL_VERSION_ID >= 40101
      Dmsg3(50, "Error %u (%s): %s\n",
            mysql_errno(&(mdb->mysql)), mysql_sqlstate(&(mdb->mysql)),
            mysql_error(&(mdb->mysql)));
#else
      Dmsg2(50, "Error %u: %s\n",
            mysql_errno(&(mdb->mysql)), mysql_error(&(mdb->mysql)));
#endif
      V(mutex);
      return 0;
   }

   mdb->connected = true;
   if (!check_tables_version(jcr, mdb)) {
      V(mutex);
      return 0;
   }

   Dmsg3(100, "opendb ref=%d connected=%d db=%p\n", mdb->ref_count,
         mdb->connected, mdb->db);

   /* Set connection timeout to 8 days specialy for batch mode */
   sql_query(mdb, "SET wait_timeout=691200");
   sql_query(mdb, "SET interactive_timeout=691200");

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
   Dmsg3(100, "closedb ref=%d connected=%d db=%p\n", mdb->ref_count,
         mdb->connected, mdb->db);
   if (mdb->ref_count == 0) {
      qdchain(&mdb->bq);
      if (mdb->connected) {
         Dmsg1(100, "close db=%p\n", mdb->db);
         mysql_close(&mdb->mysql);

#ifdef xHAVE_EMBEDDED_MYSQL
//       mysql_server_end();
#endif
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

/*
 * This call is needed because the message channel thread
 *  opens a database on behalf of a jcr that was created in
 *  a different thread. MySQL then allocates thread specific
 *  data, which is NOT freed when the original jcr thread
 *  closes the database.  Thus the msgchan must call here
 *  to cleanup any thread specific data that it created.
 */
void db_thread_cleanup()
{ 
#ifndef HAVE_WIN32
   my_thread_end();
#endif
}

/*
 * Return the next unique index (auto-increment) for
 * the given table.  Return NULL on error.
 *
 * For MySQL, NULL causes the auto-increment value
 *  to be updated.
 */
int db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index)
{
   strcpy(index, "NULL");
   return 1;
}


/*
 * Escape strings so that MySQL is happy
 *
 *   NOTE! len is the length of the old string. Your new
 *         string must be long enough (max 2*old+1) to hold
 *         the escaped output.
 */
void
db_escape_string(JCR *jcr, B_DB *mdb, char *snew, char *old, int len)
{
   mysql_real_escape_string(mdb->db, snew, old, len);
}

/*
 * Submit a general SQL command (cmd), and for each row returned,
 *  the sqlite_handler is called with the ctx.
 */
bool db_sql_query(B_DB *mdb, const char *query, DB_RESULT_HANDLER *result_handler, void *ctx)
{
   SQL_ROW row;
   bool send = true;

   db_lock(mdb);
   if (sql_query(mdb, query) != 0) {
      Mmsg(mdb->errmsg, _("Query failed: %s: ERR=%s\n"), query, sql_strerror(mdb));
      db_unlock(mdb);
      return false;
   }
   if (result_handler != NULL) {
      if ((mdb->result = sql_use_result(mdb)) != NULL) {
         int num_fields = sql_num_fields(mdb);

         /* We *must* fetch all rows */
         while ((row = sql_fetch_row(mdb)) != NULL) {
            if (send) {
               /* the result handler returns 1 when it has
                *  seen all the data it wants.  However, we
                *  loop to the end of the data.
                */
               if (result_handler(ctx, num_fields, row)) {
                  send = false;
               }
            }
         }

         sql_free_result(mdb);
      }
   }
   db_unlock(mdb);
   return true;

}

void my_mysql_free_result(B_DB *mdb)
{
   db_lock(mdb);
   if (mdb->result) {
      mysql_free_result(mdb->result);
      mdb->result = NULL;
   }
   db_unlock(mdb);
}

#ifdef HAVE_BATCH_FILE_INSERT
const char *my_mysql_batch_lock_path_query = 
   "LOCK TABLES Path write, batch write, Path as p write";


const char *my_mysql_batch_lock_filename_query = 
   "LOCK TABLES Filename write, batch write, Filename as f write";

const char *my_mysql_batch_unlock_tables_query = "UNLOCK TABLES";

const char *my_mysql_batch_fill_path_query = 
   "INSERT INTO Path (Path) "
    "SELECT a.Path FROM " 
     "(SELECT DISTINCT Path FROM batch) AS a WHERE NOT EXISTS "
     "(SELECT Path FROM Path AS p WHERE p.Path = a.Path)";     

const char *my_mysql_batch_fill_filename_query = 
   "INSERT INTO Filename (Name) "
    "SELECT a.Name FROM " 
     "(SELECT DISTINCT Name FROM batch) AS a WHERE NOT EXISTS "
     "(SELECT Name FROM Filename AS f WHERE f.Name = a.Name)";
#endif /* HAVE_BATCH_FILE_INSERT */

#endif /* HAVE_MYSQL */
