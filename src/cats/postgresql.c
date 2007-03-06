/*
 * Bacula Catalog Database routines specific to PostgreSQL
 *   These are PostgreSQL specific routines
 *
 *    Dan Langille, December 2003
 *    based upon work done by Kern Sibbald, March 2000
 *
 *    Version $Id: postgresql.c,v 1.21 2004/09/22 21:36:57 kerns Exp $
 */

/*
   Copyright (C) 2003-2004 Kern Sibbald and John Walker

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

#ifdef HAVE_POSTGRESQL

#include "postgres_ext.h"       /* needed for NAMEDATALEN */

/* -----------------------------------------------------------------------
 *
 *   PostgreSQL dependent defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

/* List of open databases */
static BQUEUE db_list = {&db_list, &db_list};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
      Jmsg(jcr, M_FATAL, 0, _("A user name for PostgreSQL must be supplied.\n"));
      return NULL;
   }
   P(mutex);			      /* lock DB queue */
   if (!mult_db_connections) {
      /* Look to see if DB already open */
      for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
	 if (strcmp(mdb->db_name, db_name) == 0) {
            Dmsg2(100, "DB REopen %d %s\n", mdb->ref_count, db_name);
	    mdb->ref_count++;
	    V(mutex);
	    return mdb; 		 /* already open */
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
   mdb->errmsg	       = get_pool_memory(PM_EMSG); /* get error message buffer */
   *mdb->errmsg        = 0;
   mdb->cmd	       = get_pool_memory(PM_EMSG); /* get command buffer */
   mdb->cached_path    = get_pool_memory(PM_FNAME);
   mdb->cached_path_id = 0;
   mdb->ref_count      = 1;
   mdb->fname	       = get_pool_memory(PM_FNAME);
   mdb->path	       = get_pool_memory(PM_FNAME);
   mdb->esc_name       = get_pool_memory(PM_FNAME);
   mdb->allow_transactions = mult_db_connections;
   qinsert(&db_list, &mdb->bq); 	   /* put db in list */
   V(mutex);
   return mdb;
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
      Mmsg1(&mdb->errmsg, _("Unable to initialize DB lock. ERR=%s\n"), 
	    strerror(errstat));
      V(mutex);
      return 0;
   }

   if (mdb->db_port) {
      bsnprintf(buf, sizeof(buf), "%d", mdb->db_port);
      port = buf;
   } else {
      port = NULL;
   }

   /* If connection fails, try at 5 sec intervals for 30 seconds. */
   for (int retry=0; retry < 6; retry++) {
      /* connect to the database */
      mdb->db = PQsetdbLogin(
	   mdb->db_address,	      /* default = localhost */
	   port,		      /* default port */
	   NULL,		      /* pg options */
	   NULL,		      /* tty, ignored */
	   mdb->db_name,	      /* database name */
	   mdb->db_user,	      /* login name */
	   mdb->db_password);	      /* password */

      /* If no connect, try once more in case it is a timing problem */
      if (PQstatus(mdb->db) == CONNECTION_OK) {
	 break;   
      }
      bmicrosleep(5, 0);
   }

   Dmsg0(50, "pg_real_connect done\n");
   Dmsg3(50, "db_user=%s db_name=%s db_password=%s\n", mdb->db_user, mdb->db_name, 
            mdb->db_password==NULL?"(NULL)":mdb->db_password);
  
   if (PQstatus(mdb->db) != CONNECTION_OK) {
      Mmsg2(&mdb->errmsg, _("Unable to connect to PostgreSQL server.\n"
            "Database=%s User=%s\n"
            "It is probably not running or your password is incorrect.\n"),
	     mdb->db_name, mdb->db_user);
      V(mutex);
      return 0;
   }

   if (!check_tables_version(jcr, mdb)) {
      V(mutex);
      return 0;
   }

   mdb->connected = true;
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
	 sql_close(mdb);
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
      my_postgresql_free_result(mdb);	    
      free(mdb);
   }
   V(mutex);
}

/*
 * Return the next unique index (auto-increment) for
 * the given table.  Return NULL on error.
 *  
 * For PostgreSQL, NULL causes the auto-increment value
 *  to be updated.
 */
int db_next_index(JCR *jcr, B_DB *mdb, char *table, char *index)
{
   strcpy(index, "NULL");
   return 1;
}   


/*
 * Escape strings so that PostgreSQL is happy
 *
 *   NOTE! len is the length of the old string. Your new
 *	   string must be long enough (max 2*old+1) to hold
 *	   the escaped output.
 */
void
db_escape_string(char *snew, char *old, int len)
{
   PQescapeString(snew, old, len);
}

/*
 * Submit a general SQL command (cmd), and for each row returned,
 *  the sqlite_handler is called with the ctx.
 */
int db_sql_query(B_DB *mdb, const char *query, DB_RESULT_HANDLER *result_handler, void *ctx)
{
   SQL_ROW row;

   Dmsg0(500, "db_sql_query started\n");
  
   db_lock(mdb);
   if (sql_query(mdb, query) != 0) {
      Mmsg(mdb->errmsg, _("Query failed: %s: ERR=%s\n"), query, sql_strerror(mdb));
      db_unlock(mdb);
      Dmsg0(500, "db_sql_query failed\n");
      return 0;
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

   return 1;
}



POSTGRESQL_ROW my_postgresql_fetch_row(B_DB *mdb) 
{
   int j;
   POSTGRESQL_ROW row = NULL; // by default, return NULL

   Dmsg0(500, "my_postgresql_fetch_row start\n");

   if (mdb->row_number == -1 || mdb->row == NULL) {
      Dmsg1(500, "we have need space of %d bytes\n", sizeof(char *) * mdb->num_fields);

      if (mdb->row != NULL) {
         Dmsg0(500, "my_postgresql_fetch_row freeing space\n");
	 free(mdb->row);
	 mdb->row = NULL;
      }

      mdb->row = (POSTGRESQL_ROW) malloc(sizeof(char *) * mdb->num_fields);

      // now reset the row_number now that we have the space allocated
      mdb->row_number = 0;
   }

   // if still within the result set
   if (mdb->row_number < mdb->num_rows) {
      Dmsg2(500, "my_postgresql_fetch_row row number '%d' is acceptable (0..%d)\n", mdb->row_number, mdb->num_rows);
      // get each value from this row
      for (j = 0; j < mdb->num_fields; j++) {
	 mdb->row[j] = PQgetvalue(mdb->result, mdb->row_number, j);
         Dmsg2(500, "my_postgresql_fetch_row field '%d' has value '%s'\n", j, mdb->row[j]);
      }
      // increment the row number for the next call
      mdb->row_number++;

      row = mdb->row;
   } else {
      Dmsg2(500, "my_postgresql_fetch_row row number '%d' is NOT acceptable (0..%d)\n", mdb->row_number, mdb->num_rows);
   }

   Dmsg1(500, "my_postgresql_fetch_row finishes returning %x\n", row);

   return row;
}

int my_postgresql_max_length(B_DB *mdb, int field_num) {
   //
   // for a given column, find the max length
   //
   int max_length;
   int i;
   int this_length;

   max_length = 0;
   for (i = 0; i < mdb->num_rows; i++) {
      if (PQgetisnull(mdb->result, i, field_num)) {
          this_length = 4;        // "NULL"
      } else {
	  this_length = strlen(PQgetvalue(mdb->result, i, field_num));
      }
		      
      if (max_length < this_length) {
	  max_length = this_length;
      }
   }

   return max_length;
}

POSTGRESQL_FIELD * my_postgresql_fetch_field(B_DB *mdb) 
{
   int	   i;

   Dmsg0(500, "my_postgresql_fetch_field starts\n");
   if (mdb->fields == NULL) {
      Dmsg1(500, "allocating space for %d fields\n", mdb->num_fields);
      mdb->fields = (POSTGRESQL_FIELD *)malloc(sizeof(POSTGRESQL_FIELD) * mdb->num_fields);

      for (i = 0; i < mdb->num_fields; i++) {
         Dmsg1(500, "filling field %d\n", i);
	 mdb->fields[i].name	       = PQfname(mdb->result, i);
	 mdb->fields[i].max_length = my_postgresql_max_length(mdb, i);
	 mdb->fields[i].type	   = PQftype(mdb->result, i);
	 mdb->fields[i].flags	   = 0;

         Dmsg4(500, "my_postgresql_fetch_field finds field '%s' has length='%d' type='%d' and IsNull=%d\n", 
	    mdb->fields[i].name, mdb->fields[i].max_length, mdb->fields[i].type,
	    mdb->fields[i].flags);
      } // end for
   } // end if

   // increment field number for the next time around

   Dmsg0(500, "my_postgresql_fetch_field finishes\n");
   return &mdb->fields[mdb->field_number++];
}

void my_postgresql_data_seek(B_DB *mdb, int row) 
{
   // set the row number to be returned on the next call
   // to my_postgresql_fetch_row
   mdb->row_number = row;
}

void my_postgresql_field_seek(B_DB *mdb, int field)
{
   mdb->field_number = field;
}

/*
 * Note, if this routine returns 1 (failure), Bacula expects
 *  that no result has been stored.
 */
int my_postgresql_query(B_DB *mdb, const char *query) {
   Dmsg0(500, "my_postgresql_query started\n");
   // We are starting a new query.  reset everything.
   mdb->num_rows     = -1;
   mdb->row_number   = -1;
   mdb->field_number = -1;

   if (mdb->result != NULL) {
      PQclear(mdb->result);  /* hmm, someone forgot to free?? */
   }

   Dmsg1(500, "my_postgresql_query starts with '%s'\n", query);
   mdb->result = PQexec(mdb->db, query);
   mdb->status = PQresultStatus(mdb->result);
   if (mdb->status == PGRES_TUPLES_OK || mdb->status == PGRES_COMMAND_OK) {
      Dmsg1(500, "we have a result\n", query);

      // how many fields in the set?
      mdb->num_fields = (int) PQnfields(mdb->result);
      Dmsg1(500, "we have %d fields\n", mdb->num_fields);

      mdb->num_rows   = PQntuples(mdb->result);
      Dmsg1(500, "we have %d rows\n", mdb->num_rows);

      mdb->status = 0;
   } else {
      Dmsg1(500, "we failed\n", query);
      mdb->status = 1;
   }

   Dmsg0(500, "my_postgresql_query finishing\n");

   return mdb->status;
}

void my_postgresql_free_result (B_DB *mdb) 
{
   if (mdb->result) {
      PQclear(mdb->result);
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
}

int my_postgresql_currval(B_DB *mdb, char *table_name) 
{
   // Obtain the current value of the sequence that
   // provides the serial value for primary key of the table.

   // currval is local to our session.	It is not affected by
   // other transactions.

   // Determine the name of the sequence.
   // PostgreSQL automatically creates a sequence using
   // <table>_<column>_seq.
   // At the time of writing, all tables used this format for
   // for their primary key: <table>id
   // Except for basefiles which has a primary key on baseid.
   // Therefore, we need to special case that one table.

   // everything else can use the PostgreSQL formula.

   char      sequence[NAMEDATALEN-1];
   char      query   [NAMEDATALEN+50];
   PGresult *result;
   int	     id = 0;

   if (strcasecmp(table_name, "basefiles") == 0) {
      bstrncpy(sequence, "basefiles_baseid", sizeof(sequence));
   } else {
      bstrncpy(sequence, table_name, sizeof(sequence));
      bstrncat(sequence, "_",        sizeof(sequence));
      bstrncat(sequence, table_name, sizeof(sequence));
      bstrncat(sequence, "id",       sizeof(sequence));
   }

   bstrncat(sequence, "_seq", sizeof(sequence));
   bsnprintf(query, sizeof(query), "SELECT currval('%s')", sequence);

// Mmsg(query, "SELECT currval('%s')", sequence);
   Dmsg1(500, "my_postgresql_currval invoked with '%s'\n", query);
   result = PQexec(mdb->db, query);

   Dmsg0(500, "exec done");

   if (PQresultStatus(result) == PGRES_TUPLES_OK) {
      Dmsg0(500, "getting value");
      id = atoi(PQgetvalue(result, 0, 0));
      Dmsg2(500, "got value '%s' which became %d\n", PQgetvalue(result, 0, 0), id);
   } else {
      Mmsg1(&mdb->errmsg, _("error fetching currval: %s\n"), PQerrorMessage(mdb->db));
   }

   PQclear(result);

   return id;
}


#endif /* HAVE_POSTGRESQL */
