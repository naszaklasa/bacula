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
 * SQL header file
 *
 *   by Kern E. Sibbald
 *
 *   Anyone who accesses the database will need to include
 *   this file.
 *
 * This file contains definitions common to sql.c and
 * the external world, and definitions destined only
 * for the external world. This is control with
 * the define __SQL_C, which is defined only in sql.c
 *
 *    Version $Id$
 */

/*
   Here is how database versions work.

   While I am working on a new release with database changes, the
   update scripts are in the src/cats directory under the names
   update_xxx_tables.in.  Most of the time, I make database updates
   in one go and immediately update the version, but not always.  If
   there are going to be several updates as is the case with version
   1.37, then I will often forgo changing the version until the last
   update otherwise I will end up with too many versions and a lot
   of confusion.

   When I am pretty sure there will be no more updates, I will
   change the version from 8 to 9 (in the present case), and when I
   am 100% sure there will be no more changes, the update script
   will be copied to the updatedb directory with the correct name
   (in the present case 8 to 9).

   Now, in principle, each of the different DB implementations
   can have a different version, but in practice they are all
   the same (simplifies things). The exception is the internal
   database, which is no longer used, and hence, no longer changes.
 */


#ifndef __SQL_H_
#define __SQL_H_ 1

enum {
   SQL_TYPE_MYSQL      = 0,
   SQL_TYPE_POSTGRESQL = 1,
   SQL_TYPE_SQLITE     = 2,
   SQL_TYPE_SQLITE3
};


typedef void (DB_LIST_HANDLER)(void *, const char *);
typedef int (DB_RESULT_HANDLER)(void *, int, char **);

#define db_lock(mdb)   _db_lock(__FILE__, __LINE__, mdb)
#define db_unlock(mdb) _db_unlock(__FILE__, __LINE__, mdb)

#ifdef __SQL_C

#if defined(BUILDING_CATS)
#ifdef HAVE_SQLITE

#define BDB_VERSION 11

#include <sqlite.h>

/* Define opaque structure for sqlite */
struct sqlite {
   char dummy;
};

#define IS_NUM(x)             ((x) == 1)
#define IS_NOT_NULL(x)        ((x) == 1)

typedef struct s_sql_field {
   char *name;                        /* name of column */
   int length;                        /* length */
   int max_length;                    /* max length */
   uint32_t type;                     /* type */
   uint32_t flags;                    /* flags */
} SQL_FIELD;

/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *                    S Q L I T E
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   struct sqlite *db;
   char **result;
   int status;
   int nrow;                          /* nrow returned from sqlite */
   int ncolumn;                       /* ncolum returned from sqlite */
   int num_rows;                      /* used by code */
   int row;                           /* seek row */
   int field;                         /* seek field */
   SQL_FIELD **fields;                /* defined fields */
   int ref_count;
   char *db_name;
   char *db_user;
   char *db_address;                  /* host name address */
   char *db_socket;                   /* socket for local access */
   char *db_password;
   int  db_port;                      /* port for host name address */
   bool connected;                    /* connection made to db */
   bool have_insert_id;               /* do not have insert id */
   bool fields_defined;               /* set when fields defined */
   char *sqlite_errmsg;               /* error message returned by sqlite */
   POOLMEM *errmsg;                   /* nicely edited error message */
   POOLMEM *cmd;                      /* SQL command string */
   POOLMEM *cached_path;              /* cached path name */
   int cached_path_len;               /* length of cached path */
   uint32_t cached_path_id;           /* cached path id */
   bool allow_transactions;           /* transactions allowed */
   bool transaction;                  /* transaction started */
   int changes;                       /* changes during transaction */
   POOLMEM *fname;                    /* Filename only */
   POOLMEM *path;                     /* Path only */
   POOLMEM *esc_name;                 /* Escaped file name */
   POOLMEM *esc_path;                 /* Escaped path name */
   int fnl;                           /* file name length */
   int pnl;                           /* path name length */
};


/*
 * "Generic" names for easier conversion
 *
 *                    S Q L I T E
 */
#define sql_store_result(x)   (x)->result
#define sql_free_result(x)    my_sqlite_free_table(x)
#define sql_fetch_row(x)      my_sqlite_fetch_row(x)
#define sql_query(x, y)       my_sqlite_query((x), (y))
#ifdef HAVE_SQLITE3
#define sql_insert_id(x,y)    sqlite3_last_insert_rowid((x)->db)
#define sql_close(x)          sqlite3_close((x)->db)
#else
#define sql_insert_id(x,y)    sqlite_last_insert_rowid((x)->db)
#define sql_close(x)          sqlite_close((x)->db)
#endif
#define sql_strerror(x)       (x)->sqlite_errmsg?(x)->sqlite_errmsg:"unknown"
#define sql_num_rows(x)       (x)->nrow
#define sql_data_seek(x, i)   (x)->row = (i)
#define sql_affected_rows(x)  1
#define sql_field_seek(x, y)  my_sqlite_field_seek((x), (y))
#define sql_fetch_field(x)    my_sqlite_fetch_field(x)
#define sql_num_fields(x)     ((x)->ncolumn)
#define SQL_ROW               char**

#define sql_batch_start(x,y)    my_batch_start(x,y)
#define sql_batch_end(x,y,z)    my_batch_end(x,y,z)
#define sql_batch_insert(x,y,z) my_batch_insert(x,y,z)
#define sql_batch_lock_path_query       my_sqlite_batch_lock_query
#define sql_batch_lock_filename_query   my_sqlite_batch_lock_query
#define sql_batch_unlock_tables_query   my_sqlite_batch_unlock_query
#define sql_batch_fill_filename_query   my_sqlite_batch_fill_filename_query
#define sql_batch_fill_path_query       my_sqlite_batch_fill_path_query

/* In cats/sqlite.c */
void       my_sqlite_free_table(B_DB *mdb);
SQL_ROW    my_sqlite_fetch_row(B_DB *mdb);
int        my_sqlite_query(B_DB *mdb, const char *cmd);
void       my_sqlite_field_seek(B_DB *mdb, int field);
SQL_FIELD *my_sqlite_fetch_field(B_DB *mdb);
extern const char* my_sqlite_batch_lock_query;
extern const char* my_sqlite_batch_unlock_query;
extern const char* my_sqlite_batch_fill_filename_query;
extern const char* my_sqlite_batch_fill_path_query;


#else

/*                    S Q L I T E 3            */


#ifdef HAVE_SQLITE3


#define BDB_VERSION 11

#include <sqlite3.h>

/* Define opaque structure for sqlite */
struct sqlite3 {
   char dummy;
};

#define IS_NUM(x)             ((x) == 1)
#define IS_NOT_NULL(x)        ((x) == 1)

typedef struct s_sql_field {
   char *name;                        /* name of column */
   int length;                        /* length */
   int max_length;                    /* max length */
   uint32_t type;                     /* type */
   uint32_t flags;                    /* flags */
} SQL_FIELD;

/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *                    S Q L I T E
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   struct sqlite3 *db;
   char **result;
   int status;
   int nrow;                          /* nrow returned from sqlite */
   int ncolumn;                       /* ncolum returned from sqlite */
   int num_rows;                      /* used by code */
   int row;                           /* seek row */
   int field;                         /* seek field */
   SQL_FIELD **fields;                /* defined fields */
   int ref_count;
   char *db_name;
   char *db_user;
   char *db_address;                  /* host name address */
   char *db_socket;                   /* socket for local access */
   char *db_password;
   int  db_port;                      /* port for host name address */
   bool connected;                    /* connection made to db */
   bool have_insert_id;               /* do not have insert id */
   bool fields_defined;               /* set when fields defined */
   char *sqlite_errmsg;               /* error message returned by sqlite */
   POOLMEM *errmsg;                   /* nicely edited error message */
   POOLMEM *cmd;                      /* SQL command string */
   POOLMEM *cached_path;              /* cached path name */
   int cached_path_len;               /* length of cached path */
   uint32_t cached_path_id;           /* cached path id */
   bool allow_transactions;           /* transactions allowed */
   bool transaction;                  /* transaction started */
   int changes;                       /* changes during transaction */
   POOLMEM *fname;                    /* Filename only */
   POOLMEM *path;                     /* Path only */
   POOLMEM *esc_name;                 /* Escaped file name */
   POOLMEM *esc_path;                 /* Escaped path name */
   int fnl;                           /* file name length */
   int pnl;                           /* path name length */
};

/*
 * Conversion of sqlite 2 names to sqlite3
 */
#define sqlite_last_insert_rowid sqlite3_last_insert_rowid
#define sqlite_open sqlite3_open
#define sqlite_close sqlite3_close
#define sqlite_result sqlite3_result
#define sqlite_exec sqlite3_exec
#define sqlite_get_table sqlite3_get_table
#define sqlite_free_table sqlite3_free_table


/*
 * "Generic" names for easier conversion
 *
 *                    S Q L I T E 3
 */
#define sql_store_result(x)   (x)->result
#define sql_free_result(x)    my_sqlite_free_table(x)
#define sql_fetch_row(x)      my_sqlite_fetch_row(x)
#define sql_query(x, y)       my_sqlite_query((x), (y))
#ifdef HAVE_SQLITE3
#define sql_insert_id(x,y)    sqlite3_last_insert_rowid((x)->db)
#define sql_close(x)          sqlite3_close((x)->db)
#else
#define sql_insert_id(x,y)    sqlite_last_insert_rowid((x)->db)
#define sql_close(x)          sqlite_close((x)->db)
#endif
#define sql_strerror(x)       (x)->sqlite_errmsg?(x)->sqlite_errmsg:"unknown"
#define sql_num_rows(x)       (x)->nrow
#define sql_data_seek(x, i)   (x)->row = (i)
#define sql_affected_rows(x)  1
#define sql_field_seek(x, y)  my_sqlite_field_seek((x), (y))
#define sql_fetch_field(x)    my_sqlite_fetch_field(x)
#define sql_num_fields(x)     ((x)->ncolumn)
#define sql_batch_start(x,y)    my_batch_start(x,y)
#define sql_batch_end(x,y,z)    my_batch_end(x,y,z)
#define sql_batch_insert(x,y,z) my_batch_insert(x,y,z)
#define SQL_ROW               char**
#define sql_batch_lock_path_query       my_sqlite_batch_lock_query
#define sql_batch_lock_filename_query   my_sqlite_batch_lock_query
#define sql_batch_unlock_tables_query   my_sqlite_batch_unlock_query
#define sql_batch_fill_filename_query   my_sqlite_batch_fill_filename_query
#define sql_batch_fill_path_query       my_sqlite_batch_fill_path_query

/* In cats/sqlite.c */
void       my_sqlite_free_table(B_DB *mdb);
SQL_ROW    my_sqlite_fetch_row(B_DB *mdb);
int        my_sqlite_query(B_DB *mdb, const char *cmd);
void       my_sqlite_field_seek(B_DB *mdb, int field);
SQL_FIELD *my_sqlite_fetch_field(B_DB *mdb);
extern const char* my_sqlite_batch_lock_query;
extern const char* my_sqlite_batch_unlock_query;
extern const char* my_sqlite_batch_fill_filename_query;
extern const char* my_sqlite_batch_fill_path_query;


#else

#ifdef HAVE_MYSQL

#define BDB_VERSION 11

#include <mysql.h>

/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *
 *                     M Y S Q L
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   MYSQL mysql;
   MYSQL *db;
   MYSQL_RES *result;
   int status;
   my_ulonglong num_rows;
   int ref_count;
   char *db_name;
   char *db_user;
   char *db_password;
   char *db_address;                  /* host address */
   char *db_socket;                   /* socket for local access */
   int db_port;                       /* port of host address */
   int have_insert_id;                /* do have insert_id() */
   bool connected;
   POOLMEM *errmsg;                   /* nicely edited error message */
   POOLMEM *cmd;                      /* SQL command string */
   POOLMEM *cached_path;
   int cached_path_len;               /* length of cached path */
   uint32_t cached_path_id;
   bool allow_transactions;           /* transactions allowed */ 
   int changes;                       /* changes made to db */
   POOLMEM *fname;                    /* Filename only */
   POOLMEM *path;                     /* Path only */
   POOLMEM *esc_name;                 /* Escaped file name */
   POOLMEM *esc_path;                 /* Escaped path name */
   int fnl;                           /* file name length */
   int pnl;                           /* path name length */
};

#define DB_STATUS int

/* "Generic" names for easier conversion */
#define sql_store_result(x)   mysql_store_result((x)->db)
#define sql_use_result(x)     mysql_use_result((x)->db)
#define sql_free_result(x)    my_mysql_free_result(x)
#define sql_fetch_row(x)      mysql_fetch_row((x)->result)
#define sql_query(x, y)       mysql_query((x)->db, (y))
#define sql_strerror(x)       mysql_error((x)->db)
#define sql_num_rows(x)       mysql_num_rows((x)->result)
#define sql_data_seek(x, i)   mysql_data_seek((x)->result, (i))
#define sql_affected_rows(x)  mysql_affected_rows((x)->db)
#define sql_insert_id(x,y)    mysql_insert_id((x)->db)
#define sql_field_seek(x, y)  mysql_field_seek((x)->result, (y))
#define sql_fetch_field(x)    mysql_fetch_field((x)->result)
#define sql_num_fields(x)     (int)mysql_num_fields((x)->result)
#define SQL_ROW               MYSQL_ROW
#define SQL_FIELD             MYSQL_FIELD

#define sql_batch_start(x,y)    my_batch_start(x,y)
#define sql_batch_end(x,y,z)    my_batch_end(x,y,z)
#define sql_batch_insert(x,y,z) my_batch_insert(x,y,z)
#define sql_batch_lock_path_query       my_mysql_batch_lock_path_query
#define sql_batch_lock_filename_query   my_mysql_batch_lock_filename_query
#define sql_batch_unlock_tables_query   my_mysql_batch_unlock_tables_query
#define sql_batch_fill_filename_query   my_mysql_batch_fill_filename_query
#define sql_batch_fill_path_query       my_mysql_batch_fill_path_query


extern const char* my_mysql_batch_lock_path_query;
extern const char* my_mysql_batch_lock_filename_query;
extern const char* my_mysql_batch_unlock_tables_query;
extern const char* my_mysql_batch_fill_filename_query;
extern const char* my_mysql_batch_fill_path_query;
extern void  my_mysql_free_result(B_DB *mdb);

#else

#ifdef HAVE_POSTGRESQL

#define BDB_VERSION 11

#include <libpq-fe.h>

/* TEMP: the following is taken from select OID, typname from pg_type; */
#define IS_NUM(x)        ((x) == 20 || (x) == 21 || (x) == 23 || (x) == 700 || (x) == 701)
#define IS_NOT_NULL(x)   ((x) == 1)

typedef char **POSTGRESQL_ROW;
typedef struct pg_field {
   char         *name;
   int           max_length;
   unsigned int  type;
   unsigned int  flags;       // 1 == not null
} POSTGRESQL_FIELD;


/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *
 *                     P O S T G R E S Q L
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   PGconn *db;
   PGresult *result;
   int status;
   POSTGRESQL_ROW row;
   POSTGRESQL_FIELD *fields;
   int num_rows;
   int row_size;                  /* size of malloced rows */
   int num_fields;
   int fields_size;               /* size of malloced fields */
   int row_number;                /* row number from my_postgresql_data_seek */
   int field_number;              /* field number from my_postgresql_field_seek */
   int ref_count;
   char *db_name;
   char *db_user;
   char *db_password;
   char *db_address;              /* host address */
   char *db_socket;               /* socket for local access */
   int db_port;                   /* port of host address */
   int have_insert_id;            /* do have insert_id() */
   bool connected;
   POOLMEM *errmsg;               /* nicely edited error message */
   POOLMEM *cmd;                  /* SQL command string */
   POOLMEM *cached_path;
   int cached_path_len;           /* length of cached path */
   uint32_t cached_path_id;
   bool allow_transactions;       /* transactions allowed */
   bool transaction;              /* transaction started */
   int changes;                   /* changes made to db */
   POOLMEM *fname;                /* Filename only */
   POOLMEM *path;                 /* Path only */
   POOLMEM *esc_name;             /* Escaped file name */
   POOLMEM *esc_path;             /* Escaped path name */
   int fnl;                       /* file name length */
   int pnl;                       /* path name length */
};

void               my_postgresql_free_result(B_DB *mdb);
POSTGRESQL_ROW     my_postgresql_fetch_row  (B_DB *mdb);
int                my_postgresql_query      (B_DB *mdb, const char *query);
void               my_postgresql_data_seek  (B_DB *mdb, int row);
int                my_postgresql_currval    (B_DB *mdb, const char *table_name);
void               my_postgresql_field_seek (B_DB *mdb, int row);
POSTGRESQL_FIELD * my_postgresql_fetch_field(B_DB *mdb);

int my_postgresql_batch_start(JCR *jcr, B_DB *mdb);
int my_postgresql_batch_end(JCR *jcr, B_DB *mdb, const char *error);
typedef struct ATTR_DBR ATTR_DBR;
int my_postgresql_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);
char *my_postgresql_copy_escape(char *dest, char *src, size_t len);

extern const char* my_pg_batch_lock_path_query;
extern const char* my_pg_batch_lock_filename_query;
extern const char* my_pg_batch_unlock_tables_query;
extern const char* my_pg_batch_fill_filename_query;
extern const char* my_pg_batch_fill_path_query;

/* "Generic" names for easier conversion */
#define sql_store_result(x)   ((x)->result)
#define sql_free_result(x)    my_postgresql_free_result(x)
#define sql_fetch_row(x)      my_postgresql_fetch_row(x)
#define sql_query(x, y)       my_postgresql_query((x), (y))
#define sql_close(x)          PQfinish((x)->db)
#define sql_strerror(x)       PQerrorMessage((x)->db)
#define sql_num_rows(x)       ((unsigned) PQntuples((x)->result))
#define sql_data_seek(x, i)   my_postgresql_data_seek((x), (i))
#define sql_affected_rows(x)  ((unsigned) atoi(PQcmdTuples((x)->result)))
#define sql_insert_id(x,y)    my_postgresql_currval((x), (y))
#define sql_field_seek(x, y)  my_postgresql_field_seek((x), (y))
#define sql_fetch_field(x)    my_postgresql_fetch_field(x)
#define sql_num_fields(x)     ((x)->num_fields)

#define sql_batch_start(x,y)    my_postgresql_batch_start(x,y)
#define sql_batch_end(x,y,z)    my_postgresql_batch_end(x,y,z)
#define sql_batch_insert(x,y,z) my_postgresql_batch_insert(x,y,z)
#define sql_batch_lock_path_query       my_pg_batch_lock_path_query
#define sql_batch_lock_filename_query   my_pg_batch_lock_filename_query
#define sql_batch_unlock_tables_query   my_pg_batch_unlock_tables_query
#define sql_batch_fill_filename_query   my_pg_batch_fill_filename_query
#define sql_batch_fill_path_query       my_pg_batch_fill_path_query

#define SQL_ROW               POSTGRESQL_ROW
#define SQL_FIELD             POSTGRESQL_FIELD

#else

#ifdef HAVE_DBI

#define BDB_VERSION 11

#include <dbi/dbi.h>

#ifdef HAVE_BATCH_FILE_INSERT
#include <dbi/dbi-dev.h>
#endif //HAVE_BATCH_FILE_INSERT

#define IS_NUM(x)        ((x) == 1 || (x) == 2 )
#define IS_NOT_NULL(x)   ((x) == (1 << 0))

typedef char **DBI_ROW;
typedef struct dbi_field {
   char         *name;
   int           max_length;
   unsigned int  type;
   unsigned int  flags;       // 1 == not null
} DBI_FIELD;

typedef struct dbi_field_get {
   BQUEUE bq;
   char *value;
} DBI_FIELD_GET;

/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *
 *                     D B I
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   dbi_conn *db;
   dbi_result *result;
   dbi_inst instance;
   dbi_error_flag status;
   DBI_ROW row;
   DBI_FIELD *fields;
   DBI_FIELD_GET *field_get;
   int num_rows;
   int row_size;                  /* size of malloced rows */
   int num_fields;
   int fields_size;               /* size of malloced fields */
   int row_number;                /* row number from my_postgresql_data_seek */
   int field_number;              /* field number from my_postgresql_field_seek */
   int ref_count;
   int db_type;                   /* DBI driver defined */
   char *db_driverdir ;           /* DBI driver dir */
   char *db_driver;               /* DBI type database */
   char *db_name;
   char *db_user;
   char *db_password;
   char *db_address;              /* host address */
   char *db_socket;               /* socket for local access */
   int db_port;                   /* port of host address */
   int have_insert_id;            /* do have insert_id() */
   bool connected;
   POOLMEM *errmsg;               /* nicely edited error message */
   POOLMEM *cmd;                  /* SQL command string */
   POOLMEM *cached_path;
   int cached_path_len;           /* length of cached path */
   uint32_t cached_path_id;
   bool allow_transactions;       /* transactions allowed */
   bool transaction;              /* transaction started */
   int changes;                   /* changes made to db */
   POOLMEM *fname;                /* Filename only */
   POOLMEM *path;                 /* Path only */
   POOLMEM *esc_name;             /* Escaped file name */
   POOLMEM *esc_path;             /* Escaped path name */
   int fnl;                       /* file name length */
   int pnl;                       /* path name length */
};

void               my_dbi_free_result(B_DB *mdb);
DBI_ROW            my_dbi_fetch_row  (B_DB *mdb);
int                my_dbi_query      (B_DB *mdb, const char *query);
void               my_dbi_data_seek  (B_DB *mdb, int row);
void               my_dbi_field_seek (B_DB *mdb, int row);
DBI_FIELD *        my_dbi_fetch_field(B_DB *mdb);
const char *       my_dbi_strerror   (B_DB *mdb);
int                my_dbi_getisnull  (dbi_result *result, int row_number, int column_number);
char *             my_dbi_getvalue   (dbi_result *result, int row_number, unsigned int column_number);
//int                my_dbi_getvalue   (dbi_result *result, int row_number, unsigned int column_number, char *value);
int                my_dbi_sql_insert_id(B_DB *mdb, char *table_name);

int my_dbi_batch_start(JCR *jcr, B_DB *mdb);
int my_dbi_batch_end(JCR *jcr, B_DB *mdb, const char *error);
typedef struct ATTR_DBR ATTR_DBR;
int my_dbi_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar);
char *my_postgresql_copy_escape(char *dest, char *src, size_t len);
// typedefs for libdbi work with postgresql copy insert
typedef int (*custom_function_insert_t)(void*, const char*, int);
typedef char* (*custom_function_error_t)(void*);
typedef int (*custom_function_end_t)(void*, const char*);

extern const char* my_dbi_batch_lock_path_query[4];
extern const char* my_dbi_batch_lock_filename_query[4];
extern const char* my_dbi_batch_unlock_tables_query[4];
extern const char* my_dbi_batch_fill_filename_query[4];
extern const char* my_dbi_batch_fill_path_query[4];

/* "Generic" names for easier conversion */
#define sql_store_result(x)   (x)->result
#define sql_free_result(x)    my_dbi_free_result(x)
#define sql_fetch_row(x)      my_dbi_fetch_row(x)
#define sql_query(x, y)       my_dbi_query((x), (y))
#define sql_close(x)          dbi_conn_close((x)->db)
#define sql_strerror(x)       my_dbi_strerror(x)
#define sql_num_rows(x)       dbi_result_get_numrows((x)->result)
#define sql_data_seek(x, i)   my_dbi_data_seek((x), (i))
/* #define sql_affected_rows(x)  dbi_result_get_numrows_affected((x)->result) */
#define sql_affected_rows(x)  1
#define sql_insert_id(x,y)    my_dbi_sql_insert_id((x), (y))
#define sql_field_seek(x, y)  my_dbi_field_seek((x), (y))
#define sql_fetch_field(x)    my_dbi_fetch_field(x)
#define sql_num_fields(x)     ((x)->num_fields)
#define sql_batch_start(x,y)    my_dbi_batch_start(x,y)
#define sql_batch_end(x,y,z)    my_dbi_batch_end(x,y,z)
#define sql_batch_insert(x,y,z) my_dbi_batch_insert(x,y,z)
#define sql_batch_lock_path_query       my_dbi_batch_lock_path_query[db_type]
#define sql_batch_lock_filename_query   my_dbi_batch_lock_filename_query[db_type]
#define sql_batch_unlock_tables_query   my_dbi_batch_unlock_tables_query[db_type]
#define sql_batch_fill_filename_query   my_dbi_batch_fill_filename_query[db_type]
#define sql_batch_fill_path_query       my_dbi_batch_fill_path_query[db_type]

#define SQL_ROW               DBI_ROW
#define SQL_FIELD             DBI_FIELD


#else  /* USE BACULA DB routines */

#define HAVE_BACULA_DB 1

/* Change this each time there is some incompatible
 * file format change!!!!
 */
#define BDB_VERSION 13                /* file version number */

struct s_control {
   int bdb_version;                   /* Version number */
   uint32_t JobId;                    /* next Job Id */
   uint32_t PoolId;                   /* next Pool Id */
   uint32_t MediaId;                  /* next Media Id */
   uint32_t JobMediaId;               /* next JobMedia Id */
   uint32_t ClientId;                 /* next Client Id */
   uint32_t FileSetId;                /* nest FileSet Id */
   time_t time;                       /* time file written */
};


/* This is the REAL definition for using the
 *  Bacula internal DB
 */
struct B_DB {
   BQUEUE bq;                         /* queue control */
/* pthread_mutex_t mutex;  */         /* single thread lock */
   brwlock_t lock;                    /* transaction lock */
   int ref_count;                     /* number of times opened */
   struct s_control control;          /* control file structure */
   int cfd;                           /* control file device */
   FILE *jobfd;                       /* Jobs records file descriptor */
   FILE *poolfd;                      /* Pool records fd */
   FILE *mediafd;                     /* Media records fd */
   FILE *jobmediafd;                  /* JobMedia records fd */
   FILE *clientfd;                    /* Client records fd */
   FILE *filesetfd;                   /* FileSet records fd */
   char *db_name;                     /* name of database */
   POOLMEM *errmsg;                   /* nicely edited error message */
   POOLMEM *cmd;                      /* Command string */
   POOLMEM *cached_path;
   int cached_path_len;               /* length of cached path */
   uint32_t cached_path_id;
};

#endif /* HAVE_SQLITE3 */
#endif /* HAVE_MYSQL */
#endif /* HAVE_SQLITE */
#endif /* HAVE_POSTGRESQL */
#endif /* HAVE_DBI */
#endif

/* Use for better error location printing */
#define UPDATE_DB(jcr, db, cmd) UpdateDB(__FILE__, __LINE__, jcr, db, cmd)
#define INSERT_DB(jcr, db, cmd) InsertDB(__FILE__, __LINE__, jcr, db, cmd)
#define QUERY_DB(jcr, db, cmd) QueryDB(__FILE__, __LINE__, jcr, db, cmd)
#define DELETE_DB(jcr, db, cmd) DeleteDB(__FILE__, __LINE__, jcr, db, cmd)


#else    /* not __SQL_C */

/* This is a "dummy" definition for use outside of sql.c
 */
struct B_DB {
   int dummy;                         /* for SunOS compiler */
};

#endif /*  __SQL_C */

/* ==============================================================
 *
 *  What follows are definitions that are used "globally" for all
 *   the different SQL engines and both inside and external to the
 *   cats directory.
 */

extern uint32_t bacula_db_version;

#define faddr_t long

/*
 * Structure used when calling db_get_query_ids()
 *  allows the subroutine to return a list of ids.
 */
class dbid_list : public SMARTALLOC {
public:
   DBId_t *DBId;                      /* array of DBIds */
   char *PurgedFiles;                 /* Array of PurgedFile flags */
   int num_ids;                       /* num of ids actually stored */
   int max_ids;                       /* size of id array */
   int num_seen;                      /* number of ids processed */
   int tot_ids;                       /* total to process */

   dbid_list();                       /* in sql.c */
   ~dbid_list();                      /* in sql.c */
};




/* Job information passed to create job record and update
 * job record at end of job. Note, although this record
 * contains all the fields found in the Job database record,
 * it also contains fields found in the JobMedia record.
 */
/* Job record */
struct JOB_DBR {
   JobId_t JobId;
   char Job[MAX_NAME_LENGTH];         /* Job unique name */
   char Name[MAX_NAME_LENGTH];        /* Job base name */
   int JobType;                       /* actually char(1) */
   int JobLevel;                      /* actually char(1) */
   int JobStatus;                     /* actually char(1) */
   DBId_t ClientId;                   /* Id of client */
   DBId_t PoolId;                     /* Id of pool */
   DBId_t FileSetId;                  /* Id of FileSet */
   DBId_t PriorJobId;                 /* Id of migrated (prior) job */
   time_t SchedTime;                  /* Time job scheduled */
   time_t StartTime;                  /* Job start time */
   time_t EndTime;                    /* Job termination time of orig job */
   time_t RealEndTime;                /* Job termination time of this job */
   utime_t JobTDate;                  /* Backup time/date in seconds */
   uint32_t VolSessionId;
   uint32_t VolSessionTime;
   uint32_t JobFiles;
   uint32_t JobErrors;
   uint32_t JobMissingFiles;
   uint64_t JobBytes;
   uint64_t ReadBytes;
   int PurgedFiles;
   int HasBase;

   /* Note, FirstIndex, LastIndex, Start/End File and Block
    * are only used in the JobMedia record.
    */
   uint32_t FirstIndex;               /* First index this Volume */
   uint32_t LastIndex;                /* Last index this Volume */
   uint32_t StartFile;
   uint32_t EndFile;
   uint32_t StartBlock;
   uint32_t EndBlock;

   char cSchedTime[MAX_TIME_LENGTH];
   char cStartTime[MAX_TIME_LENGTH];
   char cEndTime[MAX_TIME_LENGTH];
   char cRealEndTime[MAX_TIME_LENGTH];
   /* Extra stuff not in DB */
   int limit;                         /* limit records to display */
   faddr_t rec_addr;
   uint32_t FileIndex;                /* added during Verify */
};

/* Job Media information used to create the media records
 * for each Volume used for the job.
 */
/* JobMedia record */
struct JOBMEDIA_DBR {
   DBId_t JobMediaId;                 /* record id */
   JobId_t  JobId;                    /* JobId */
   DBId_t MediaId;                    /* MediaId */
   uint32_t FirstIndex;               /* First index this Volume */
   uint32_t LastIndex;                /* Last index this Volume */
   uint32_t StartFile;                /* File for start of data */
   uint32_t EndFile;                  /* End file on Volume */
   uint32_t StartBlock;               /* start block on tape */
   uint32_t EndBlock;                 /* last block */
   uint32_t Copy;                     /* identical copy */
};


/* Volume Parameter structure */
struct VOL_PARAMS {
   char VolumeName[MAX_NAME_LENGTH];  /* Volume name */
   char MediaType[MAX_NAME_LENGTH];   /* Media Type */
   char Storage[MAX_NAME_LENGTH];     /* Storage name */
   uint32_t VolIndex;                 /* Volume seqence no. */
   uint32_t FirstIndex;               /* First index this Volume */
   uint32_t LastIndex;                /* Last index this Volume */
   int32_t Slot;                      /* Slot */
   uint64_t StartAddr;                /* Start address */
   uint64_t EndAddr;                  /* End address */
   int32_t InChanger;                 /* InChanger flag */
// uint32_t Copy;                     /* identical copy */
// uint32_t Stripe;                   /* RAIT strip number */
};


/* Attributes record -- NOT same as in database because
 *  in general, this "record" creates multiple database
 *  records (e.g. pathname, filename, fileattributes).
 */
struct ATTR_DBR {
   char *fname;                       /* full path & filename */
   char *link;                        /* link if any */
   char *attr;                        /* attributes statp */
   uint32_t FileIndex;
   uint32_t Stream;
   JobId_t  JobId;
   DBId_t ClientId;
   DBId_t PathId;
   DBId_t FilenameId;
   FileId_t FileId;
   char *Digest;
   int DigestType;
};


/* File record -- same format as database */
struct FILE_DBR {
   FileId_t FileId;
   uint32_t FileIndex;
   JobId_t  JobId;
   DBId_t FilenameId;
   DBId_t PathId;
   JobId_t  MarkId;
   char LStat[256];
   char Digest[BASE64_SIZE(CRYPTO_DIGEST_MAX_SIZE)];
   int DigestType;                    /* NO_SIG/MD5_SIG/SHA1_SIG */
};

/* Pool record -- same format as database */
struct POOL_DBR {
   DBId_t PoolId;
   char Name[MAX_NAME_LENGTH];        /* Pool name */
   uint32_t NumVols;                  /* total number of volumes */
   uint32_t MaxVols;                  /* max allowed volumes */
   int32_t LabelType;                 /* Bacula/ANSI/IBM */
   int32_t UseOnce;                   /* set to use once only */
   int32_t UseCatalog;                /* set to use catalog */
   int32_t AcceptAnyVolume;           /* set to accept any volume sequence */
   int32_t AutoPrune;                 /* set to prune automatically */
   int32_t Recycle;                   /* default Vol recycle flag */
   utime_t  VolRetention;             /* retention period in seconds */
   utime_t  VolUseDuration;           /* time in secs volume can be used */
   uint32_t MaxVolJobs;               /* Max Jobs on Volume */
   uint32_t MaxVolFiles;              /* Max files on Volume */
   uint64_t MaxVolBytes;              /* Max bytes on Volume */
   DBId_t RecyclePoolId;              /* RecyclePool destination when media is purged */
   DBId_t ScratchPoolId;              /* ScratchPool source when media is needed */
   char PoolType[MAX_NAME_LENGTH];
   char LabelFormat[MAX_NAME_LENGTH];
   /* Extra stuff not in DB */
   faddr_t rec_addr;
};

class DEVICE_DBR {
public:
   DBId_t DeviceId;
   char Name[MAX_NAME_LENGTH];        /* Device name */
   DBId_t MediaTypeId;                /* MediaType */
   DBId_t StorageId;                  /* Storage id if autochanger */
   uint32_t DevMounts;                /* Number of times mounted */
   uint32_t DevErrors;                /* Number of read/write errors */
   uint64_t DevReadBytes;             /* Number of bytes read */
   uint64_t DevWriteBytes;            /* Number of bytew written */
   uint64_t DevReadTime;              /* time spent reading volume */
   uint64_t DevWriteTime;             /* time spent writing volume */
   uint64_t DevReadTimeSincCleaning;  /* read time since cleaning */
   uint64_t DevWriteTimeSincCleaning; /* write time since cleaning */
   time_t   CleaningDate;             /* time last cleaned */
   utime_t  CleaningPeriod;           /* time between cleanings */
};

class STORAGE_DBR {
public:
   DBId_t StorageId;
   char Name[MAX_NAME_LENGTH];        /* Device name */
   int AutoChanger;                   /* Set if autochanger */

   /* Not in database */
   bool created;                      /* set if created by db_create ... */
};

class MEDIATYPE_DBR {
public:
   DBId_t MediaTypeId;
   char MediaType[MAX_NAME_LENGTH];   /* MediaType string */
   int ReadOnly;                      /* Set if read-only */
};


/* Media record -- same as the database */
struct MEDIA_DBR {
   DBId_t MediaId;                    /* Unique volume id */
   char VolumeName[MAX_NAME_LENGTH];  /* Volume name */
   char MediaType[MAX_NAME_LENGTH];   /* Media type */
   DBId_t PoolId;                     /* Pool id */
   time_t   FirstWritten;             /* Time Volume first written this usage */
   time_t   LastWritten;              /* Time Volume last written */
   time_t   LabelDate;                /* Date/Time Volume labeled */
   time_t   InitialWrite;             /* Date/Time Volume first written */
   int32_t  LabelType;                /* Label (Bacula/ANSI/IBM) */
   uint32_t VolJobs;                  /* number of jobs on this medium */
   uint32_t VolFiles;                 /* Number of files */
   uint32_t VolBlocks;                /* Number of blocks */
   uint32_t VolMounts;                /* Number of times mounted */
   uint32_t VolErrors;                /* Number of read/write errors */
   uint32_t VolWrites;                /* Number of writes */
   uint32_t VolReads;                 /* Number of reads */
   uint64_t VolBytes;                 /* Number of bytes written */
   uint32_t VolParts;                 /* Number of parts written */
   uint64_t MaxVolBytes;              /* Max bytes to write to Volume */
   uint64_t VolCapacityBytes;         /* capacity estimate */
   uint64_t VolReadTime;              /* time spent reading volume */
   uint64_t VolWriteTime;             /* time spent writing volume */
   utime_t  VolRetention;             /* Volume retention in seconds */
   utime_t  VolUseDuration;           /* time in secs volume can be used */
   uint32_t MaxVolJobs;               /* Max Jobs on Volume */
   uint32_t MaxVolFiles;              /* Max files on Volume */
   int32_t  Recycle;                  /* recycle yes/no */
   int32_t  Slot;                     /* slot in changer */
   int32_t  Enabled;                  /* 0=disabled, 1=enabled, 2=archived */
   int32_t  InChanger;                /* Volume currently in changer */
   DBId_t   StorageId;                /* Storage record Id */
   uint32_t EndFile;                  /* Last file on volume */
   uint32_t EndBlock;                 /* Last block on volume */
   uint32_t RecycleCount;             /* Number of times recycled */
   char     VolStatus[20];            /* Volume status */
   DBId_t   DeviceId;                 /* Device where Vol last written */
   DBId_t   LocationId;               /* Where Volume is -- user defined */
   DBId_t   ScratchPoolId;            /* Where to move if scratch */
   DBId_t   RecyclePoolId;            /* Where to move when recycled */
   /* Extra stuff not in DB */
   faddr_t rec_addr;                  /* found record address */
   /* Since the database returns times as strings, this is how we pass
    *   them back.
    */
   char    cFirstWritten[MAX_TIME_LENGTH]; /* FirstWritten returned from DB */
   char    cLastWritten[MAX_TIME_LENGTH];  /* LastWritten returned from DB */
   char    cLabelDate[MAX_TIME_LENGTH];    /* LabelData returned from DB */
   char    cInitialWrite[MAX_TIME_LENGTH]; /* InitialWrite returned from DB */
   bool    set_first_written;
   bool    set_label_date;
};

/* Client record -- same as the database */
struct CLIENT_DBR {
   DBId_t ClientId;                   /* Unique Client id */
   int AutoPrune;
   utime_t FileRetention;
   utime_t JobRetention;
   char Name[MAX_NAME_LENGTH];        /* Client name */
   char Uname[256];                   /* Uname for client */
};

/* Counter record as in database */
struct COUNTER_DBR {
   char Counter[MAX_NAME_LENGTH];
   int32_t MinValue;
   int32_t MaxValue;
   int32_t CurrentValue;
   char WrapCounter[MAX_NAME_LENGTH];
};


/* FileSet record -- same as the database */
struct FILESET_DBR {
   DBId_t FileSetId;                  /* Unique FileSet id */
   char FileSet[MAX_NAME_LENGTH];     /* FileSet name */
   char MD5[50];                      /* MD5 signature of include/exclude */
   time_t CreateTime;                 /* date created */
   /*
    * This is where we return CreateTime
    */
   char cCreateTime[MAX_TIME_LENGTH]; /* CreateTime as returned from DB */
   /* Not in DB but returned by db_create_fileset() */
   bool created;                      /* set when record newly created */
};

/* Call back context for getting a 32/64 bit value from the database */
struct db_int64_ctx {
   int64_t value;                     /* value returned */
   int count;                         /* number of values seen */
};

/* Call back context for getting a list of comma separated strings from the database */
class db_list_ctx {
public:
   POOLMEM *list;                     /* list */
   int count;                         /* number of values seen */

   db_list_ctx() { list = get_pool_memory(PM_FNAME); *list = 0; count = 0; }
   ~db_list_ctx() { free_pool_memory(list); list = NULL; }
};


#include "protos.h"
#include "jcr.h"
#include "sql_cmds.h"

/*
 * Exported globals from sql.c
 */
extern int DLL_IMP_EXP db_type;        /* SQL engine type index */

/*
 * Some functions exported by sql.c for use within the
 *   cats directory.
 */
void list_result(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *send, void *ctx, e_list_type type);
void list_dashes(B_DB *mdb, DB_LIST_HANDLER *send, void *ctx);
int get_sql_record_max(JCR *jcr, B_DB *mdb);
bool check_tables_version(JCR *jcr, B_DB *mdb);
void _db_unlock(const char *file, int line, B_DB *mdb);
void _db_lock(const char *file, int line, B_DB *mdb);
const char *db_get_type(void);

void print_dashes(B_DB *mdb);
void print_result(B_DB *mdb);
int QueryDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);
int InsertDB(const char *file, int line, JCR *jcr, B_DB *db, char *select_cmd);
int DeleteDB(const char *file, int line, JCR *jcr, B_DB *db, char *delete_cmd);
int UpdateDB(const char *file, int line, JCR *jcr, B_DB *db, char *update_cmd);
void split_path_and_file(JCR *jcr, B_DB *mdb, const char *fname);
#endif /* __SQL_H_ */
