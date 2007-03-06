/*
 * Director specific configuration and defines
 *
 *     Kern Sibbald, Feb MM
 *
 *    Version $Id: dird_conf.h,v 1.89.2.3 2006/01/11 10:24:12 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

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
   R_DEVICE,
   R_FIRST = R_DIRECTOR,
   R_LAST  = R_DEVICE                 /* keep this updated */
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
struct DEVICE;

/*
 *   Director Resource
 *
 */
class DIRRES {
public:
   RES   hdr;
   dlist *DIRaddrs;
   char *password;                    /* Password for UA access */
   char *query_file;                  /* SQL query file */
   char *working_directory;           /* WorkingDirectory */
   const char *scripts_directory;     /* ScriptsDirectory */
   char *pid_directory;               /* PidDirectory */
   char *subsys_directory;            /* SubsysDirectory */
   MSGS *messages;                    /* Daemon message handler */
   uint32_t MaxConcurrentJobs;        /* Max concurrent jobs for whole director */
   utime_t FDConnectTimeout;          /* timeout for connect in seconds */
   utime_t SDConnectTimeout;          /* timeout in seconds */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   int tls_verify_peer;              /* TLS Verify Client Certificate */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Server Certificate File */
   char *tls_keyfile;                 /* TLS Server Key File */
   char *tls_dhfile;                  /* TLS Diffie-Hellman Parameters */
   alist *tls_allowed_cns;            /* TLS Allowed Clients */
   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};

/*
 * Device Resource
 *  This resource is a bit different from the other resources
 *  because it is not defined in the Director 
 *  by DEVICE { ... }, but rather by a "reference" such as
 *  DEVICE = xxx; Then when the Director connects to the
 *  SD, it requests the information about the device.
 */
class DEVICE {
public:
   RES hdr;

   bool found;                        /* found with SD */
   int num_writers;                   /* number of writers */
   int max_writers;                   /* = 1 for files */
   int reserved;                      /* number of reserves */
   int num_drives;                    /* for autochanger */
   bool autochanger;                  /* set if device is autochanger */
   bool open;                         /* drive open */
   bool append;                       /* in append mode */
   bool read;                         /* in read mode */
   bool labeled;                      /* Volume name valid */
   bool offline;                      /* not available */
   bool autoselect;                   /* can be selected via autochanger */
   uint32_t PoolId;
   char ChangerName[MAX_NAME_LENGTH];
   char VolumeName[MAX_NAME_LENGTH];
   char MediaType[MAX_NAME_LENGTH];
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
class CONRES {
public:
   RES   hdr;
   char *password;                    /* UA server password */
   alist *ACL_lists[Num_ACL];         /* pointers to ACLs */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   int tls_verify_peer;              /* TLS Verify Client Certificate */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Server Certificate File */
   char *tls_keyfile;                 /* TLS Server Key File */
   char *tls_dhfile;                  /* TLS Diffie-Hellman Parameters */
   alist *tls_allowed_cns;            /* TLS Allowed Clients */
   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};


/*
 *   Catalog Resource
 *
 */
class CAT {
public:
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
class CLIENT {
public:
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
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */
   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};

/*
 *   Store Resource
 *
 */
class STORE {
public:
   RES   hdr;

   int   SDport;                      /* port where Directors connect */
   int   SDDport;                     /* data port for File daemon */
   char *address;
   char *password;
   char *media_type;
   alist *device;                     /* Alternate devices for this Storage */
   int  autochanger;                  /* set if autochanger */
   int  drives;                       /* number of drives in autochanger */
   uint32_t MaxConcurrentJobs;        /* Maximume concurrent jobs */
   uint32_t NumConcurrentJobs;        /* number of concurrent jobs running */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */
   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
   int64_t StorageId;                 /* Set from Storage DB record */
   int enabled;                       /* Set if device is enabled */

   /* Methods */
   char *dev_name() const;
   char *name() const;
};

inline char *STORE::dev_name() const
{ 
   DEVICE *dev = (DEVICE *)device->first();
   return dev->hdr.name;
}

inline char *STORE::name() const { return hdr.name; }


/*
 *   Job Resource
 */
class JOB {
public:
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
   utime_t FullMaxWaitTime;           /* Max Full job wait time */
   utime_t DiffMaxWaitTime;           /* Max Differential job wait time */
   utime_t IncMaxWaitTime;            /* Max Incremental job wait time */
   utime_t MaxStartDelay;             /* max start delay in seconds */
   int PrefixLinks;                   /* prefix soft links with Where path */
   int PruneJobs;                     /* Force pruning of Jobs */
   int PruneFiles;                    /* Force pruning of Files */
   int PruneVolumes;                  /* Force pruning of Volumes */
   int SpoolAttributes;               /* Set to spool attributes in SD */
   int spool_data;                    /* Set to spool data in SD */
   int rerun_failed_levels;           /* Upgrade to rerun failed levels */
   int PreferMountedVolumes;          /* Prefer vols mounted rather than new one */
   uint32_t MaxConcurrentJobs;        /* Maximume concurrent jobs */
   int RescheduleOnError;             /* Set to reschedule on error */
   int RescheduleTimes;               /* Number of times to reschedule job */
   utime_t RescheduleInterval;        /* Reschedule interval */
   utime_t JobRetention;              /* job retention period in seconds */
   int write_part_after_job;          /* Set to write part after job in SD */
   int enabled;                       /* Set if device is enabled */
   
   MSGS      *messages;               /* How and where to send messages */
   SCHED     *schedule;               /* When -- Automatic schedule */
   CLIENT    *client;                 /* Who to backup */
   FILESET   *fileset;                /* What to backup -- Fileset */
   alist     *storage;                /* Where is device -- list of Storage to be used */
   POOL      *pool;                   /* Where is media -- Media Pool */
   POOL      *full_pool;              /* Pool for Full backups */
   POOL      *inc_pool;               /* Pool for Incremental backups */
   POOL      *dif_pool;               /* Pool for Differental backups */
   union {
      JOB       *verify_job;          /* Job name to verify */
      JOB       *migration_job;       /* Job name to migrate */
   };
   JOB       *jobdefs;                /* Job defaults */
   alist     *run_cmds;               /* Run commands */
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
class FILESET {
public:
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
   int enable_vss;                    /* Enable Volume Shadow Copy */
};


/*
 *   Schedule Resource
 *
 */
class SCHED {
public:
   RES   hdr;

   RUN *run;
};

/*
 *   Counter Resource
 */
class COUNTER {
public:
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
class POOL {
public:
   RES   hdr;

   char *pool_type;                   /* Pool type */
   char *label_format;                /* Label format string */
   char *cleaning_prefix;             /* Cleaning label prefix */
   int   LabelType;                   /* Bacula/ANSI/IBM label type */
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
   utime_t MigrationTime;             /* Time to migrate to next pool */
   uint32_t MigrationHighBytes;       /* When migration starts */
   uint32_t MigrationLowBytes;        /* When migration stops */
   POOL  *NextPool;                   /* Next pool for migration */
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
   DEVICE     res_dev;
   RES        hdr;
};



/* Run structure contained in Schedule Resource */
class RUN {
public:
   RUN *next;                         /* points to next run record */
   int level;                         /* level override */
   int Priority;                      /* priority override */
   int job_type;
   bool spool_data;                   /* Data spooling override */
   bool spool_data_set;               /* Data spooling override given */
   bool write_part_after_job;         /* Write part after job override */
   bool write_part_after_job_set;     /* Write part after job override given */
   
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
