/*
 * Director specific configuration and defines
 *
 *     Kern Sibbald, Feb MM
 *
 *    Version $Id: dird_conf.h,v 1.70.4.1 2005/02/14 10:02:21 kerns Exp $
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

/* NOTE:  #includes at the end of this file */

/*
 * Resource codes -- they must be sequential for indexing   
 */
enum {
   R_DIRECTOR = 1001,
   R_CLIENT,
   R_JOB,
   R_STORAGE,
   R_CATALOG,
   R_SCHEDULE,
   R_FILESET,
   R_POOL,
   R_MSGS,
   R_COUNTER,
   R_CONSOLE,
   R_JOBDEFS,
   R_FIRST = R_DIRECTOR,
   R_LAST  = R_JOBDEFS                /* keep this updated */
};


/*
 * Some resource attributes
 */
enum {
   R_NAME = 1020,
   R_ADDRESS,
   R_PASSWORD,
   R_TYPE,
   R_BACKUP
};


/* Used for certain KeyWord tables */
struct s_kw {       
   const char *name;
   int token;   
};

/* Job Level keyword structure */
struct s_jl {
   const char *level_name;                  /* level keyword */
   int  level;                        /* level */
   int  job_type;                     /* JobType permitting this level */
};

/* Job Type keyword structure */
struct s_jt {
   const char *type_name;
   int job_type;
};

/* Definition of the contents of each Resource */
/* Needed for forward references */
struct SCHED;
struct CLIENT;
struct FILESET;
struct POOL;
struct RUN;

/* 
 *   Director Resource  
 *
 */
struct DIRRES {
   RES   hdr;
   dlist *DIRaddrs;
   char *password;                    /* Password for UA access */
   int enable_ssl;                    /* Use SSL for UA */
   char *query_file;                  /* SQL query file */
   char *working_directory;           /* WorkingDirectory */
   char *pid_directory;               /* PidDirectory */
   char *subsys_directory;            /* SubsysDirectory */
   int require_ssl;                   /* Require SSL for all connections */
   MSGS *messages;                    /* Daemon message handler */
   uint32_t MaxConcurrentJobs;        /* Max concurrent jobs for whole director */
   utime_t FDConnectTimeout;          /* timeout for connect in seconds */
   utime_t SDConnectTimeout;          /* timeout in seconds */
};


/*
 * Console ACL positions
 */
enum {
   Job_ACL = 0,
   Client_ACL,
   Storage_ACL,
   Schedule_ACL,
   Run_ACL,
   Pool_ACL,
   Command_ACL,
   FileSet_ACL,
   Catalog_ACL,
   Num_ACL                            /* keep last */
};

/* 
 *    Console Resource
 */
struct CONRES {
   RES   hdr;
   char *password;                    /* UA server password */
   int enable_ssl;                    /* Use SSL */
   alist *ACL_lists[Num_ACL];         /* pointers to ACLs */
};


/*
 *   Catalog Resource
 *
 */
struct CAT {
   RES   hdr;

   int   db_port;                     /* Port -- not yet implemented */
   char *db_address;                  /* host name for remote access */
   char *db_socket;                   /* Socket for local access */
   char *db_password;
   char *db_user;
   char *db_name;
   int   mult_db_connections;         /* set if multiple connections wanted */
};


/*
 *   Client Resource
 *
 */
struct CLIENT {
   RES   hdr;

   int   FDport;                      /* Where File daemon listens */
   int   AutoPrune;                   /* Do automatic pruning? */
   utime_t FileRetention;             /* file retention period in seconds */
   utime_t JobRetention;              /* job retention period in seconds */
   char *address;
   char *password;
   CAT *catalog;                      /* Catalog resource */
   uint32_t MaxConcurrentJobs;        /* Maximume concurrent jobs */
   uint32_t NumConcurrentJobs;        /* number of concurrent jobs running */
   int enable_ssl;                    /* Use SSL */
};

/*
 *   Store Resource
 * 
 */
struct STORE {
   RES   hdr;

   int   SDport;                      /* port where Directors connect */
   int   SDDport;                     /* data port for File daemon */
   char *address;
   char *password;
   char *media_type;
   char *dev_name;   
   int  autochanger;                  /* set if autochanger */
   uint32_t MaxConcurrentJobs;        /* Maximume concurrent jobs */
   uint32_t NumConcurrentJobs;        /* number of concurrent jobs running */
   int enable_ssl;                    /* Use SSL */
};


#define MAX_STORE 2                   /* Max storage directives in Job */

/*
 *   Job Resource
 */
struct JOB {
   RES   hdr;

   int   JobType;                     /* job type (backup, verify, restore */
   int   JobLevel;                    /* default backup/verify level */
   int   Priority;                    /* Job priority */
   int   RestoreJobId;                /* What -- JobId to restore */
   char *RestoreWhere;                /* Where on disk to restore -- directory */
   char *RestoreBootstrap;            /* Bootstrap file */
   char *RunBeforeJob;                /* Run program before Job */
   char *RunAfterJob;                 /* Run program after Job */
   char *RunAfterFailedJob;           /* Run program after Job that errs */
   char *ClientRunBeforeJob;          /* Run client program before Job */
   char *ClientRunAfterJob;           /* Run client program after Job */
   char *WriteBootstrap;              /* Where to write bootstrap Job updates */
   int   replace;                     /* How (overwrite, ..) */
   utime_t MaxRunTime;                /* max run time in seconds */
   utime_t MaxWaitTime;               /* max blocking time in seconds */
   utime_t MaxStartDelay;             /* max start delay in seconds */
   int PrefixLinks;                   /* prefix soft links with Where path */
   int PruneJobs;                     /* Force pruning of Jobs */
   int PruneFiles;                    /* Force pruning of Files */
   int PruneVolumes;                  /* Force pruning of Volumes */
   int SpoolAttributes;               /* Set to spool attributes in SD */
   int spool_data;                    /* Set to spool data in SD */
   int rerun_failed_levels;           /* Upgrade to rerun failed levels */
   uint32_t MaxConcurrentJobs;        /* Maximume concurrent jobs */
   int RescheduleOnError;             /* Set to reschedule on error */
   int RescheduleTimes;               /* Number of times to reschedule job */
   utime_t RescheduleInterval;        /* Reschedule interval */
   utime_t JobRetention;              /* job retention period in seconds */
  
   MSGS      *messages;               /* How and where to send messages */
   SCHED     *schedule;               /* When -- Automatic schedule */
   CLIENT    *client;                 /* Who to backup */
   FILESET   *fileset;                /* What to backup -- Fileset */
   alist     *storage[MAX_STORE];     /* Where is device -- Storage daemon */
   POOL      *pool;                   /* Where is media -- Media Pool */
   POOL      *full_pool;              /* Pool for Full backups */
   POOL      *inc_pool;               /* Pool for Incremental backups */
   POOL      *dif_pool;               /* Pool for Differental backups */
   JOB       *verify_job;             /* Job name to verify */
   JOB       *jobdefs;                /* Job defaults */
   uint32_t NumConcurrentJobs;        /* number of concurrent jobs running */
};

#undef  MAX_FOPTS
#define MAX_FOPTS 34

/* File options structure */
struct FOPTS {
   char opts[MAX_FOPTS];              /* options string */
   alist regex;                       /* regex string(s) */
   alist regexdir;                    /* regex string(s) for directories */
   alist regexfile;                   /* regex string(s) for files */
   alist wild;                        /* wild card strings */
   alist wilddir;                     /* wild card strings for directories */
   alist wildfile;                    /* wild card strings for files */
   alist base;                        /* list of base names */
   alist fstype;                      /* file system type limitation */
   char *reader;                      /* reader program */
   char *writer;                      /* writer program */
};


/* This is either an include item or an exclude item */
struct INCEXE {
   FOPTS *current_opts;               /* points to current options structure */
   FOPTS **opts_list;                 /* options list */
   int num_opts;                      /* number of options items */
   alist name_list;                   /* filename list -- holds char * */
};

/* 
 *   FileSet Resource
 *
 */
struct FILESET {
   RES   hdr;

   bool new_include;                  /* Set if new include used */
   INCEXE **include_items;            /* array of incexe structures */
   int num_includes;                  /* number in array */
   INCEXE **exclude_items;
   int num_excludes;
   bool have_MD5;                     /* set if MD5 initialized */
   struct MD5Context md5c;            /* MD5 of include/exclude */
   char MD5[30];                      /* base 64 representation of MD5 */
   int ignore_fs_changes;             /* Don't force Full if FS changed */
};

 
/* 
 *   Schedule Resource
 *
 */
struct SCHED {
   RES   hdr;

   RUN *run;
};

/*
 *   Counter Resource
 */
struct COUNTER {
   RES   hdr;

   int32_t  MinValue;                 /* Minimum value */
   int32_t  MaxValue;                 /* Maximum value */
   int32_t  CurrentValue;             /* Current value */
   COUNTER *WrapCounter;              /* Wrap counter name */
   CAT     *Catalog;                  /* Where to store */
   bool     created;                  /* Created in DB */
};

/*
 *   Pool Resource   
 *
 */
struct POOL {
   RES   hdr;

   char *pool_type;                   /* Pool type */
   char *label_format;                /* Label format string */
   char *cleaning_prefix;             /* Cleaning label prefix */
   int   use_catalog;                 /* maintain catalog for media */
   int   catalog_files;               /* maintain file entries in catalog */
   int   use_volume_once;             /* write on volume only once */
   int   accept_any_volume;           /* accept any volume */
   int   purge_oldest_volume;         /* purge oldest volume */
   int   recycle_oldest_volume;       /* attempt to recycle oldest volume */
   int   recycle_current_volume;      /* attempt recycle of current volume */
   uint32_t max_volumes;              /* max number of volumes */
   utime_t VolRetention;              /* volume retention period in seconds */
   utime_t VolUseDuration;            /* duration volume can be used */
   uint32_t MaxVolJobs;               /* Maximum jobs on the Volume */
   uint32_t MaxVolFiles;              /* Maximum files on the Volume */
   uint64_t MaxVolBytes;              /* Maximum bytes on the Volume */
   int   AutoPrune;                   /* default for pool auto prune */
   int   Recycle;                     /* default for media recycle yes/no */
};


/* Define the Union of all the above
 * resource structure definitions.
 */
union URES {
   DIRRES     res_dir;
   CONRES     res_con;
   CLIENT     res_client;
   STORE      res_store;
   CAT        res_cat;
   JOB        res_job;
   FILESET    res_fs;
   SCHED      res_sch;
   POOL       res_pool;
   MSGS       res_msgs;
   COUNTER    res_counter;
   RES        hdr;
};



/* Run structure contained in Schedule Resource */
struct RUN {
   RUN *next;                         /* points to next run record */
   int level;                         /* level override */
   int Priority;                      /* priority override */
   int job_type;  
   bool spool_data;                   /* Data spooling override */
   bool spool_data_set;               /* Data spooling override given */
   POOL *pool;                        /* Pool override */
   POOL *full_pool;                   /* Pool override */
   POOL *inc_pool;                    /* Pool override */
   POOL *dif_pool;                    /* Pool override */
   STORE *storage;                    /* Storage override */
   MSGS *msgs;                        /* Messages override */
   char *since;
   int level_no;
   int minute;                        /* minute to run job */
   time_t last_run;                   /* last time run */
   time_t next_run;                   /* next time to run */
   char hour[nbytes_for_bits(24)];    /* bit set for each hour */
   char mday[nbytes_for_bits(31)];    /* bit set for each day of month */
   char month[nbytes_for_bits(12)];   /* bit set for each month */
   char wday[nbytes_for_bits(7)];     /* bit set for each day of the week */
   char wom[nbytes_for_bits(5)];      /* week of month */
   char woy[nbytes_for_bits(54)];     /* week of year */
};
