/*
 * Bacula Catalog Database routines written specifically
 *  for Bacula.  Note, these routines are VERY dumb and
 *  do not provide all the functionality of an SQL database.
 *  The purpose of these routines is to ensure that Bacula
 *  can limp along if no real database is loaded on the
 *  system.
 *
 *    Kern Sibbald, January MMI
 *
 *    Version $Id: bdb.c 3709 2006-11-27 10:03:06Z kerns $
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#ifdef HAVE_BACULA_DB

uint32_t bacula_db_version = 0;

/* List of open databases */
static BQUEUE db_list = {&db_list, &db_list};
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


#define DB_CONTROL_FILENAME  "control.db"
#define DB_JOBS_FILENAME     "jobs.db"
#define DB_POOLS_FILENAME    "pools.db"
#define DB_MEDIA_FILENAME    "media.db"
#define DB_JOBMEDIA_FILENAME "jobmedia.db"
#define DB_CLIENT_FILENAME   "client.db"
#define DB_FILESET_FILENAME  "fileset.db"

static POOLMEM *make_filename(B_DB *mdb, char *name)
{
   char sep;
   POOLMEM *dbf;

   dbf = get_pool_memory(PM_FNAME);
   if (IsPathSeparator(working_directory[strlen(working_directory)-1])) {
      sep = 0;
   } else {
      sep = '/';
   }
   Mmsg(dbf, "%s%c%s-%s", working_directory, sep, mdb->db_name, name);
   return dbf;
}

int bdb_write_control_file(B_DB *mdb)
{
   mdb->control.time = time(NULL);
   lseek(mdb->cfd, 0, SEEK_SET);
   if (write(mdb->cfd, &mdb->control, sizeof(mdb->control)) != sizeof(mdb->control)) {
      Mmsg1(&mdb->errmsg, "Error writing control file. ERR=%s\n", strerror(errno));
      Emsg0(M_FATAL, 0, mdb->errmsg);
      return 0;
   }
   return 1;
}

/*
 * Retrieve database type
 */
const char *
db_get_type(void)
{
   return "Internal";
}

/*
 * Initialize database data structure. In principal this should
 * never have errors, or it is really fatal.
 */
B_DB *
db_init_database(JCR *jcr, char const *db_name, char const *db_user, char const *db_password,
                 char const *db_address, int db_port, char const *db_socket,
                 int mult_db_connections)
{
   B_DB *mdb;
   P(mutex);                          /* lock DB queue */
   /* Look to see if DB already open */
   for (mdb=NULL; (mdb=(B_DB *)qnext(&db_list, &mdb->bq)); ) {
      if (strcmp(mdb->db_name, db_name) == 0) {
         Dmsg2(200, "DB REopen %d %s\n", mdb->ref_count, db_name);
         mdb->ref_count++;
         V(mutex);
         return mdb;                  /* already open */
      }
   }

   Dmsg0(200, "db_open first time\n");
   mdb = (B_DB *)malloc(sizeof(B_DB));
   memset(mdb, 0, sizeof(B_DB));
   Dmsg0(200, "DB struct init\n");
   mdb->db_name = bstrdup(db_name);
   mdb->errmsg = get_pool_memory(PM_EMSG);
   *mdb->errmsg = 0;
   mdb->cmd = get_pool_memory(PM_EMSG);  /* command buffer */
   mdb->ref_count = 1;
   mdb->cached_path = get_pool_memory(PM_FNAME);
   mdb->cached_path_id = 0;
   qinsert(&db_list, &mdb->bq);       /* put db in list */
   Dmsg0(200, "Done db_open_database()\n");
   mdb->cfd = -1;
   V(mutex);
   Jmsg(jcr, M_WARNING, 0, _("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"));
   Jmsg(jcr, M_WARNING, 0, _("WARNING!!!! The Internal Database is NOT OPERATIONAL!\n"));
   Jmsg(jcr, M_WARNING, 0, _("You should use SQLite, PostgreSQL, or MySQL\n"));

   return mdb;
}

/*
 * Now actually open the database.  This can generate errors,
 * which are returned in the errmsg
 */
int
db_open_database(JCR *jcr, B_DB *mdb)
{
   char *dbf;
   int fd, badctl;
   off_t filend;
   int errstat;

   Dmsg1(200, "db_open_database() %s\n", mdb->db_name);

   P(mutex);

   if ((errstat=rwl_init(&mdb->lock)) != 0) {
      Mmsg1(&mdb->errmsg, _("Unable to initialize DB lock. ERR=%s\n"), strerror(errstat));
      V(mutex);
      return 0;
   }

   Dmsg0(200, "make_filename\n");
   dbf = make_filename(mdb, DB_CONTROL_FILENAME);
   mdb->cfd = open(dbf, O_CREAT|O_RDWR, 0600);
   free_memory(dbf);
   if (mdb->cfd < 0) {
      Mmsg2(&mdb->errmsg, _("Unable to open Catalog DB control file %s: ERR=%s\n"),
         dbf, strerror(errno));
      V(mutex);
      return 0;
   }
   Dmsg0(200, "DB open\n");
   /* See if the file was previously written */
   filend = lseek(mdb->cfd, 0, SEEK_END);
   if (filend == 0) {                 /* No, initialize everything */
      Dmsg0(200, "Init DB files\n");
      memset(&mdb->control, 0, sizeof(mdb->control));
      mdb->control.bdb_version = BDB_VERSION;
      bdb_write_control_file(mdb);

      /* Create Jobs File */
      dbf = make_filename(mdb, DB_JOBS_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);

      /* Create Pools File */
      dbf = make_filename(mdb, DB_POOLS_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);

      /* Create Media File */
      dbf = make_filename(mdb, DB_MEDIA_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);

      /* Create JobMedia File */
      dbf = make_filename(mdb, DB_JOBMEDIA_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);

      /* Create Client File */
      dbf = make_filename(mdb, DB_CLIENT_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);

      /* Create FileSet File */
      dbf = make_filename(mdb, DB_FILESET_FILENAME);
      fd = open(dbf, O_CREAT|O_RDWR, 0600);
      free_memory(dbf);
      close(fd);
   }

   Dmsg0(200, "Read control file\n");
   badctl = 0;
   lseek(mdb->cfd, 0, SEEK_SET);      /* seek to begining of control file */
   if (read(mdb->cfd, &mdb->control, sizeof(mdb->control)) != sizeof(mdb->control)) {
      Mmsg1(&mdb->errmsg, _("Error reading catalog DB control file. ERR=%s\n"), strerror(errno));
      badctl = 1;
   } else if (mdb->control.bdb_version != BDB_VERSION) {
      Mmsg2(&mdb->errmsg, _("Error, catalog DB control file wrong version. "
"Wanted %d, got %d\n"
"Please reinitialize the working directory.\n"),
         BDB_VERSION, mdb->control.bdb_version);
      badctl = 1;
   }
   bacula_db_version = mdb->control.bdb_version;
   if (badctl) {
      V(mutex);
      return 0;
   }
   V(mutex);
   return 1;
}

void db_close_database(JCR *jcr, B_DB *mdb)
{
   P(mutex);
   mdb->ref_count--;
   if (mdb->ref_count == 0) {
      qdchain(&mdb->bq);
      /*  close file descriptors */
      if (mdb->cfd >= 0) {
         close(mdb->cfd);
      }
      free(mdb->db_name);
      if (mdb->jobfd) {
         fclose(mdb->jobfd);
      }
      if (mdb->poolfd) {
         fclose(mdb->poolfd);
      }
      if (mdb->mediafd) {
         fclose(mdb->mediafd);
      }
      if (mdb->jobmediafd) {
         fclose(mdb->jobmediafd);
      }
      if (mdb->clientfd) {
         fclose(mdb->clientfd);
      }
      if (mdb->filesetfd) {
         fclose(mdb->filesetfd);
      }
      rwl_destroy(&mdb->lock);
      free_pool_memory(mdb->errmsg);
      free_pool_memory(mdb->cmd);
      free_pool_memory(mdb->cached_path);
      free(mdb);
   }
   V(mutex);
}


void db_escape_string(char *snew, char *old, int len)
{
   memset(snew, 0, len);
   bstrncpy(snew, old, len);
}

char *db_strerror(B_DB *mdb)
{
   return mdb->errmsg;
}

int db_sql_query(B_DB *mdb, char const *query, DB_RESULT_HANDLER *result_handler, void *ctx)
{
   return 1;
}

/*
 * Open the Jobs file for reading/writing
 */
int bdb_open_jobs_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->jobfd) {
      dbf = make_filename(mdb, DB_JOBS_FILENAME);
      mdb->jobfd = fopen(dbf, "r+b");
      if (!mdb->jobfd) {
         Mmsg2(&mdb->errmsg, "Error opening DB Jobs file %s: ERR=%s\n",
            dbf, strerror(errno));
         Emsg0(M_FATAL, 0, mdb->errmsg);
         free_memory(dbf);
         return 0;
      }
      free_memory(dbf);
   }
   return 1;
}

/*
 * Open the JobMedia file for reading/writing
 */
int bdb_open_jobmedia_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->jobmediafd) {
      dbf = make_filename(mdb, DB_JOBMEDIA_FILENAME);
      mdb->jobmediafd = fopen(dbf, "r+b");
      if (!mdb->jobmediafd) {
         Mmsg2(&mdb->errmsg, "Error opening DB JobMedia file %s: ERR=%s\n",
            dbf, strerror(errno));
         Emsg0(M_FATAL, 0, mdb->errmsg);
         free_memory(dbf);
         return 0;
      }
      free_memory(dbf);
   }
   return 1;
}


/*
 * Open the Pools file for reading/writing
 */
int bdb_open_pools_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->poolfd) {
      dbf = make_filename(mdb, DB_POOLS_FILENAME);
      mdb->poolfd = fopen(dbf, "r+b");
      if (!mdb->poolfd) {
         Mmsg2(&mdb->errmsg, "Error opening DB Pools file %s: ERR=%s\n",
            dbf, strerror(errno));
         Emsg0(M_FATAL, 0, mdb->errmsg);
         free_memory(dbf);
         return 0;
      }
      Dmsg1(200, "Opened pool file %s\n", dbf);
      free_memory(dbf);
   }
   return 1;
}

/*
 * Open the Client file for reading/writing
 */
int bdb_open_client_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->clientfd) {
      dbf = make_filename(mdb, DB_CLIENT_FILENAME);
      mdb->clientfd = fopen(dbf, "r+b");
      if (!mdb->clientfd) {
         Mmsg2(&mdb->errmsg, "Error opening DB Clients file %s: ERR=%s\n",
            dbf, strerror(errno));
         Emsg0(M_FATAL, 0, mdb->errmsg);
         free_memory(dbf);
         return 0;
      }
      free_memory(dbf);
   }
   return 1;
}

/*
 * Open the FileSet file for reading/writing
 */
int bdb_open_fileset_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->filesetfd) {
      dbf = make_filename(mdb, DB_CLIENT_FILENAME);
      mdb->filesetfd = fopen(dbf, "r+b");
      if (!mdb->filesetfd) {
         Mmsg2(&mdb->errmsg, "Error opening DB FileSet file %s: ERR=%s\n",
            dbf, strerror(errno));
         Emsg0(M_FATAL, 0, mdb->errmsg);
         free_memory(dbf);
         return 0;
      }
      free_memory(dbf);
   }
   return 1;
}



/*
 * Open the Media file for reading/writing
 */
int bdb_open_media_file(B_DB *mdb)
{
   char *dbf;

   if (!mdb->mediafd) {
      dbf = make_filename(mdb, DB_MEDIA_FILENAME);
      mdb->mediafd = fopen(dbf, "r+b");
      if (!mdb->mediafd) {
         Mmsg2(&mdb->errmsg, "Error opening DB Media file %s: ERR=%s\n",
            dbf, strerror(errno));
         free_memory(dbf);
         return 0;
      }
      free_memory(dbf);
   }
   return 1;
}


void _db_lock(const char *file, int line, B_DB *mdb)
{
   int errstat;
   if ((errstat=rwl_writelock(&mdb->lock)) != 0) {
      e_msg(file, line, M_ABORT, 0, "rwl_writelock failure. ERR=%s\n",
           strerror(errstat));
   }
}

void _db_unlock(const char *file, int line, B_DB *mdb)
{
   int errstat;
   if ((errstat=rwl_writeunlock(&mdb->lock)) != 0) {
      e_msg(file, line, M_ABORT, 0, "rwl_writeunlock failure. ERR=%s\n",
           strerror(errstat));
   }
}

/*
 * Start a transaction. This groups inserts and makes things
 *  much more efficient. Usually started when inserting
 *  file attributes.
 */
void db_start_transaction(JCR *jcr, B_DB *mdb)
{
}

void db_end_transaction(JCR *jcr, B_DB *mdb)
{
}

bool db_update_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *sr)
{ return true; }

void
db_list_pool_records(JCR *jcr, B_DB *mdb, POOL_DBR *pdbr, 
                     DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

int db_int64_handler(void *ctx, int num_fields, char **row)
{ return 0; }


#endif /* HAVE_BACULA_DB */
