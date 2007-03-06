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
 *    Version $Id: cats.h,v 1.48 2004/09/22 21:36:57 kerns Exp $
 */

/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

#ifndef __SQL_H_
#define __SQL_H_ 1


typedef void (DB_LIST_HANDLER)(void *, const char *);
typedef int (DB_RESULT_HANDLER)(void *, int, char **);

#define db_lock(mdb)   _db_lock(__FILE__, __LINE__, mdb)
#define db_unlock(mdb) _db_unlock(__FILE__, __LINE__, mdb)

#ifdef __SQL_C

#ifdef HAVE_SQLITE

#define BDB_VERSION 8

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
typedef struct s_db {
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
   POOLMEM *esc_name;                 /* Escaped file/path name */
   int fnl;                           /* file name length */
   int pnl;                           /* path name length */
} B_DB;


/* 
 * "Generic" names for easier conversion   
 *
 *                    S Q L I T E
 */
#define sql_store_result(x)   (x)->result
#define sql_free_result(x)    my_sqlite_free_table(x)
#define sql_fetch_row(x)      my_sqlite_fetch_row(x)
#define sql_query(x, y)       my_sqlite_query((x), (y))
#define sql_close(x)          sqlite_close((x)->db)  
#define sql_strerror(x)       (x)->sqlite_errmsg?(x)->sqlite_errmsg:"unknown"
#define sql_num_rows(x)       (x)->nrow
#define sql_data_seek(x, i)   (x)->row = (i)
#define sql_affected_rows(x)  1
#define sql_insert_id(x,y)    sqlite_last_insert_rowid((x)->db)
#define sql_field_seek(x, y)  my_sqlite_field_seek((x), (y))
#define sql_fetch_field(x)    my_sqlite_fetch_field(x)
#define sql_num_fields(x)     ((x)->ncolumn)
#define SQL_ROW               char**   



/* In cats/sqlite.c */
void       my_sqlite_free_table(B_DB *mdb);
SQL_ROW    my_sqlite_fetch_row(B_DB *mdb);
int        my_sqlite_query(B_DB *mdb, char *cmd);
void       my_sqlite_field_seek(B_DB *mdb, int field);
SQL_FIELD *my_sqlite_fetch_field(B_DB *mdb);


#else

#ifdef HAVE_MYSQL

#define BDB_VERSION 8

#include <mysql.h>

/*
 * This is the "real" definition that should only be
 * used inside sql.c and associated database interface
 * subroutines.
 *
 *                     M Y S Q L
 */
typedef struct s_db {
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
   int changes;                       /* changes made to db */
   POOLMEM *fname;                    /* Filename only */
   POOLMEM *path;                     /* Path only */
   POOLMEM *esc_name;                 /* Escaped file/path name */
   int fnl;                           /* file name length */
   int pnl;                           /* path name length */
} B_DB;

#define DB_STATUS int

/* "Generic" names for easier conversion */
#define sql_store_result(x)   mysql_store_result((x)->db)
#define sql_free_result(x)    mysql_free_result((x)->result)
#define sql_fetch_row(x)      mysql_fetch_row((x)->result)
#define sql_query(x, y)       mysql_query((x)->db, (y))
#define sql_close(x)          mysql_close((x)->db)  
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

#else

#ifdef HAVE_POSTGRESQL

#define BDB_VERSION 8

#include <libpq-fe.h>

/* TEMP: the following is taken from select OID, typname from pg_type; */
#define IS_NUM(x)             ((x) == 20 || (x) == 21 || (x) == 23 || (x) == 700 || (x) == 701)
#define IS_NOT_NULL(x)        ((x) == 1)

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
typedef struct s_db {
   BQUEUE bq;                         /* queue control */
   brwlock_t lock;                    /* transaction lock */
   PGconn *db;
   PGresult *result;
   int status;
   POSTGRESQL_ROW row;
   POSTGRESQL_FIELD *fields;
   int num_rows;
   int num_fields;
   int row_number;            /* what row number did we get via my_postgresql_data_seek? */
   int field_number;          /* what field number did we get via my_postgresql_field_seek? */
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
   POOLMEM *esc_name;             /* Escaped file/path name */
   int fnl;                       /* file name length */
   int pnl;                       /* path name length */
} B_DB;

void               my_postgresql_free_result(B_DB *mdb);
POSTGRESQL_ROW     my_postgresql_fetch_row  (B_DB *mdb);
int                my_postgresql_query      (B_DB *mdb, const char *query);
void               my_postgresql_data_seek  (B_DB *mdb, int row);
int                my_postgresql_currval    (B_DB *mdb, char *table_name);
void               my_postgresql_field_seek (B_DB *mdb, int row);
POSTGRESQL_FIELD * my_postgresql_fetch_field(B_DB *mdb);


/* "Generic" names for easier conversion */
#define sql_store_result(x)   ((x)->result)
#define sql_free_result(x)    my_postgresql_free_result(x)
#define sql_fetch_row(x)      my_postgresql_fetch_row(x)
#define sql_query(x, y)       my_postgresql_query((x), (y))
#define sql_close(x)          PQfinish((x)->db)  
#define sql_strerror(x)       PQresultErrorMessage((x)->result)
#define sql_num_rows(x)       ((unsigned) PQntuples((x)->result))
#define sql_data_seek(x, i)   my_postgresql_data_seek((x), (i))
#define sql_affected_rows(x)  ((unsigned) atoi(PQcmdTuples((x)->result)))
#define sql_insert_id(x,y)    my_postgresql_currval((x), (y))
#define sql_field_seek(x, y)  my_postgresql_field_seek((x), (y))
#define sql_fetch_field(x)    my_postgresql_fetch_field(x)
#define sql_num_fields(x)     ((x)->num_fields)
#define SQL_ROW               POSTGRESQL_ROW
#define SQL_FIELD             POSTGRESQL_FIELD

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
typedef struct s_db {
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
} B_DB;

#endif /* HAVE_MYSQL */
#endif /* HAVE_SQLITE */
#endif /* HAVE_POSTGRESQL */

/* Use for better error location printing */
#define UPDATE_DB(jcr, db, cmd) UpdateDB(__FILE__, __LINE__, jcr, db, cmd)
#define INSERT_DB(jcr, db, cmd) InsertDB(__FILE__, __LINE__, jcr, db, cmd)
#define QUERY_DB(jcr, db, cmd) QueryDB(__FILE__, __LINE__, jcr, db, cmd)
#define DELETE_DB(jcr, db, cmd) DeleteDB(__FILE__, __LINE__, jcr, db, cmd)


#else    /* not __SQL_C */

/* This is a "dummy" definition for use outside of sql.c
 */
typedef struct s_db {     
   int dummy;                         /* for SunOS compiler */
} B_DB;  

#endif /*  __SQL_C */

extern uint32_t bacula_db_version;

/* ***FIXME*** FileId_t should *really* be uint64_t
 *  but at the current time, this breaks MySQL.
 */
typedef uint32_t FileId_t;
typedef uint32_t DBId_t;              /* general DB id type */
typedef uint32_t JobId_t;
      
#define faddr_t long


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
   time_t SchedTime;                  /* Time job scheduled */
   time_t StartTime;                  /* Job start time */
   time_t EndTime;                    /* Job termination time */
   utime_t JobTDate;                  /* Backup time/date in seconds */
   uint32_t VolSessionId;
   uint32_t VolSessionTime;
   uint32_t JobFiles;
   uint32_t JobErrors;
   uint32_t JobMissingFiles;
   uint64_t JobBytes;

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
   /* Extra stuff not in DB */
   faddr_t rec_addr;
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
};


/* Volume Parameter structure */
struct VOL_PARAMS {
   char VolumeName[MAX_NAME_LENGTH];  /* Volume name */
   uint32_t VolIndex;                 /* Volume seqence no. */ 
   uint32_t FirstIndex;               /* First index this Volume */
   uint32_t LastIndex;                /* Last index this Volume */
   uint32_t StartFile;                /* File for start of data */
   uint32_t EndFile;                  /* End file on Volume */
   uint32_t StartBlock;               /* start block on tape */
   uint32_t EndBlock;                 /* last block */
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
/*   int Status; */
   char SIG[50];
   int SigType;                       /* NO_SIG/MD5_SIG/SHA1_SIG */
};

/* Pool record -- same format as database */
struct POOL_DBR {
   DBId_t PoolId;
   char Name[MAX_NAME_LENGTH];        /* Pool name */
   uint32_t NumVols;                  /* total number of volumes */
   uint32_t MaxVols;                  /* max allowed volumes */
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
   char PoolType[MAX_NAME_LENGTH];             
   char LabelFormat[MAX_NAME_LENGTH];
   /* Extra stuff not in DB */
   faddr_t rec_addr;
};

/* Media record -- same as the database */
struct MEDIA_DBR {
   DBId_t MediaId;                    /* Unique volume id */
   char VolumeName[MAX_NAME_LENGTH];  /* Volume name */
   char MediaType[MAX_NAME_LENGTH];   /* Media type */
   DBId_t PoolId;                     /* Pool id */
   time_t   FirstWritten;             /* Time Volume first written */
   time_t   LastWritten;              /* Time Volume last written */
   time_t   LabelDate;                /* Date/Time Volume labeled */
   uint32_t VolJobs;                  /* number of jobs on this medium */
   uint32_t VolFiles;                 /* Number of files */
   uint32_t VolBlocks;                /* Number of blocks */
   uint32_t VolMounts;                /* Number of times mounted */
   uint32_t VolErrors;                /* Number of read/write errors */
   uint32_t VolWrites;                /* Number of writes */
   uint32_t VolReads;                 /* Number of reads */
   uint64_t VolBytes;                 /* Number of bytes written */
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
   int32_t  InChanger;                /* Volume currently in changer */
   uint32_t EndFile;                  /* Last file on volume */
   uint32_t EndBlock;                 /* Last block on volume */
   char VolStatus[20];                /* Volume status */
   /* Extra stuff not in DB */
   faddr_t rec_addr;                  /* found record address */
   /* Since the database returns times as strings, this is how we pass
    *   them back.
    */
   char    cFirstWritten[MAX_TIME_LENGTH]; /* FirstWritten returned from DB */
   char    cLastWritten[MAX_TIME_LENGTH];  /* LastWritten returned from DB */
   char    cLabelData[MAX_TIME_LENGTH];    /* LabelData returned from DB */
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



#include "protos.h"
#include "jcr.h"

/*
 * Some functions exported by sql.c for use withing the 
 *   cats directory.
 */
void list_result(B_DB *mdb, DB_LIST_HANDLER *send, void *ctx, e_list_type type);
void list_dashes(B_DB *mdb, DB_LIST_HANDLER *send, void *ctx);
int get_sql_record_max(JCR *jcr, B_DB *mdb);
int check_tables_version(JCR *jcr, B_DB *mdb);
void _db_unlock(const char *file, int line, B_DB *mdb);
void _db_lock(const char *file, int line, B_DB *mdb);
 
#endif /* __SQL_H_ */
