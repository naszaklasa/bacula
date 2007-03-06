/*
 * Bacula JCR Structure definition for Daemons and the Library
 *  This definition consists of a "Global" definition common
 *  to all daemons and used by the library routines, and a
 *  daemon specific part that is enabled with #defines.
 *
 * Kern Sibbald, Nov MM
 *
 *   Version $Id: jcr.h,v 1.76.4.1.2.1 2005/04/05 17:23:50 kerns Exp $
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
 
#ifndef __JCR_H_
#define __JCR_H_ 1

/* Backup/Verify level code. These are stored in the DB */
#define L_FULL                   'F'  /* Full backup */
#define L_INCREMENTAL            'I'  /* since last backup */
#define L_DIFFERENTIAL           'D'  /* since last full backup */
#define L_SINCE                  'S'
#define L_VERIFY_CATALOG         'C'  /* verify from catalog */
#define L_VERIFY_INIT            'V'  /* verify save (init DB) */
#define L_VERIFY_VOLUME_TO_CATALOG 'O'  /* verify Volume to catalog entries */
#define L_VERIFY_DISK_TO_CATALOG 'd'  /* verify Disk attributes to catalog */
#define L_VERIFY_DATA            'A'  /* verify data on volume */
#define L_BASE                   'B'  /* Base level job */
#define L_NONE                   ' '  /* None, for Restore and Admin */


/* Job Types. These are stored in the DB */
#define JT_BACKUP                'B'  /* Backup Job */
#define JT_VERIFY                'V'  /* Verify Job */
#define JT_RESTORE               'R'  /* Restore Job */
#define JT_CONSOLE               'C'  /* console program */
#define JT_SYSTEM                'S'  /* internal system "job" */
#define JT_ADMIN                 'D'  /* admin job */
#define JT_ARCHIVE               'A'  /* Archive Job */
#define JT_COPY                  'Y'  /* Copy Job */
#define JT_MIGRATION             'M'  /* Migration Job */

/* Job Status. Some of these are stored in the DB */
#define JS_Created               'C'  /* created but not yet running */
#define JS_Running               'R'  /* running */
#define JS_Blocked               'B'  /* blocked */
#define JS_Terminated            'T'  /* terminated normally */
#define JS_ErrorTerminated       'E'  /* Job terminated in error */
#define JS_Error                 'e'  /* Non-fatal error */
#define JS_FatalError            'f'  /* Fatal error */
#define JS_Differences           'D'  /* Verify differences */
#define JS_Canceled              'A'  /* canceled by user */
#define JS_WaitFD                'F'  /* waiting on File daemon */
#define JS_WaitSD                'S'  /* waiting on the Storage daemon */
#define JS_WaitMedia             'm'  /* waiting for new media */
#define JS_WaitMount             'M'  /* waiting for Mount */
#define JS_WaitStoreRes          's'  /* Waiting for storage resource */
#define JS_WaitJobRes            'j'  /* Waiting for job resource */
#define JS_WaitClientRes         'c'  /* Waiting for Client resource */
#define JS_WaitMaxJobs           'd'  /* Waiting for maximum jobs */
#define JS_WaitStartTime         't'  /* Waiting for start time */
#define JS_WaitPriority          'p'  /* Waiting for higher priority jobs to finish */

#define job_canceled(jcr) \
  (jcr->JobStatus == JS_Canceled || \
   jcr->JobStatus == JS_ErrorTerminated || \
   jcr->JobStatus == JS_FatalError)

#define foreach_jcr(jcr) \
   for ((jcr)=NULL; ((jcr)=get_next_jcr(jcr)); ) 



struct JCR;
typedef void (JCR_free_HANDLER)(JCR *jcr);

/* Job Control Record (JCR) */
struct JCR {
   /* Global part of JCR common to all daemons */
   JCR *next;
   JCR *prev;
   volatile int use_count;            /* use count */
   pthread_t my_thread_id;            /* id of thread controlling jcr */
   pthread_mutex_t mutex;             /* jcr mutex */
   BSOCK *dir_bsock;                  /* Director bsock or NULL if we are him */
   BSOCK *store_bsock;                /* Storage connection socket */
   BSOCK *file_bsock;                 /* File daemon connection socket */
   JCR_free_HANDLER *daemon_free_jcr; /* Local free routine */
   dlist *msg_queue;                  /* Queued messages */
   alist job_end_push;                /* Job end pushed calls */
   bool dequeuing;                    /* dequeuing messages */
   POOLMEM *VolumeName;               /* Volume name desired -- pool_memory */
   POOLMEM *errmsg;                   /* edited error message */
   char Job[MAX_NAME_LENGTH];         /* Unique name of this Job */
   uint32_t JobId;                    /* Director's JobId */
   uint32_t VolSessionId;
   uint32_t VolSessionTime;
   uint32_t JobFiles;                 /* Number of files written, this job */
   uint32_t JobErrors;                /* */
   uint64_t JobBytes;                 /* Number of bytes processed this job */
   uint64_t ReadBytes;                /* Bytes read -- before compression */
   uint32_t Errors;                   /* Number of non-fatal errors */
   volatile int JobStatus;            /* ready, running, blocked, terminated */ 
   int JobType;                       /* backup, restore, verify ... */
   int JobLevel;                      /* Job level */
   int JobPriority;                   /* Job priority */
   time_t sched_time;                 /* job schedule time, i.e. when it should start */
   time_t start_time;                 /* when job actually started */
   time_t run_time;                   /* used for computing speed */
   time_t end_time;                   /* job end time */
   POOLMEM *client_name;              /* client name */
   POOLMEM *RestoreBootstrap;         /* Bootstrap file to restore */
   char *sd_auth_key;                 /* SD auth key */
   MSGS *jcr_msgs;                    /* Copy of message resource -- actually used */
   uint32_t ClientId;                 /* Client associated with Job */
   char *where;                       /* prefix to restore files to */
   int cached_pnl;                    /* cached path length */
   POOLMEM *cached_path;              /* cached path */
   bool prefix_links;                 /* Prefix links with Where path */
   bool gui;                          /* set if gui using console */
   bool authenticated;                /* set when client authenticated */

   /* Daemon specific part of JCR */
   /* This should be empty in the library */

#ifdef DIRECTOR_DAEMON
#define MAX_STORE 2
   /* Director Daemon specific part of JCR */
   pthread_t SD_msg_chan;             /* Message channel thread id */
   pthread_cond_t term_wait;          /* Wait for job termination */
   workq_ele_t *work_item;            /* Work queue item if scheduled */
   volatile bool sd_msg_thread_done;  /* Set when Storage message thread terms */
   BSOCK *ua;                         /* User agent */
   JOB *job;                          /* Job resource */
   JOB *verify_job;                   /* Job resource of verify target job */
   alist *storage[MAX_STORE];         /* Storage possibilities */
   STORE *store;                      /* Storage daemon selected */
   CLIENT *client;                    /* Client resource */
   POOL *pool;                        /* Pool resource */
   POOL *full_pool;                   /* Full backup pool resource */
   POOL *inc_pool;                    /* Incremental backup pool resource */
   POOL *dif_pool;                    /* Differential backup pool resource */
   FILESET *fileset;                  /* FileSet resource */
   CAT *catalog;                      /* Catalog resource */
   MSGS *messages;                    /* Default message handler */
   uint32_t SDJobFiles;               /* Number of files written, this job */
   uint64_t SDJobBytes;               /* Number of bytes processed this job */
   uint32_t SDErrors;                 /* Number of non-fatal errors */
   volatile int SDJobStatus;          /* Storage Job Status */
   volatile int FDJobStatus;          /* File daemon Job Status */
   uint32_t ExpectedFiles;            /* Expected restore files */
   B_DB *db;                          /* database pointer */
   uint32_t MediaId;                  /* DB record IDs associated with this job */
   uint32_t PoolId;                   /* Pool record id */
   FileId_t FileId;                   /* Last file id inserted */
   uint32_t FileIndex;                /* Last FileIndex processed */
   POOLMEM *fname;                    /* name to put into catalog */
   POOLMEM *stime;                    /* start time for incremental/differential */
   JOB_DBR jr;                        /* Job DB record for current job */
   JOB_DBR verify_jr;                 /* Pointer to target job */
   uint32_t RestoreJobId;             /* Id specified by UA */
   POOLMEM *client_uname;             /* client uname */ 
   int replace;                       /* Replace option */
   int saveMaxConcurrentJobs;         /* save for restore jobs */
   int NumVols;                       /* Number of Volume used in pool */
   int reschedule_count;              /* Number of times rescheduled */
   bool spool_data;                   /* Spool data in SD */
   bool acquired_resource_locks;      /* set if resource locks acquired */
   bool term_wait_inited;             /* Set when cond var inited */
   bool fn_printed;                   /* printed filename */
#endif /* DIRECTOR_DAEMON */


#ifdef FILE_DAEMON
   /* File Daemon specific part of JCR */
   uint32_t num_files_examined;       /* files examined this job */
   POOLMEM *last_fname;               /* last file saved/verified */
   /*********FIXME********* add missing files and files to be retried */
   int incremental;                   /* set if incremental for SINCE */
   time_t mtime;                      /* begin time for SINCE */
   int listing;                       /* job listing in estimate */
   long Ticket;                       /* Ticket */
   char *big_buf;                     /* I/O buffer */
   POOLMEM *compress_buf;             /* Compression buffer */
   int32_t compress_buf_size;         /* Length of compression buffer */
   int replace;                       /* Replace options */
   int buf_size;                      /* length of buffer */
   void *ff;                          /* Find Files packet */
   char stored_addr[MAX_NAME_LENGTH]; /* storage daemon address */
   uint32_t StartFile;
   uint32_t EndFile;
   uint32_t StartBlock;
   uint32_t EndBlock;
   pthread_t heartbeat_id;            /* id of heartbeat thread */
   volatile BSOCK *hb_bsock;          /* duped SD socket */
   POOLMEM *RunAfterJob;              /* Command to run after job */
   DIRRES* director;                  /* Director resource */
#endif /* FILE_DAEMON */


#ifdef STORAGE_DAEMON
   /* Storage Daemon specific part of JCR */
   JCR *next_dev;                     /* next JCR attached to device */
   JCR *prev_dev;                     /* previous JCR attached to device */
   pthread_cond_t job_start_wait;     /* Wait for FD to start Job */
   int type;
   DCR *dcr;                          /* device context record */
   DEVRES *device;                    /* device resource to use */
   POOLMEM *job_name;                 /* base Job name (not unique) */
   POOLMEM *fileset_name;             /* FileSet */
   POOLMEM *fileset_md5;              /* MD5 for FileSet */
   VOL_LIST *VolList;                 /* list to read */
   int32_t NumVolumes;                /* number of volumes used */
   int32_t CurVolume;                 /* current volume number */
   int label_errors;                  /* count of label errors */
   bool session_opened;
   long Ticket;                       /* ticket for this job */
   bool ignore_label_errors;          /* ignore Volume label errors */
   bool spool_attributes;             /* set if spooling attributes */
   bool no_attributes;                /* set if no attributes wanted */
   bool spool_data;                   /* set to spool data */
   int CurVol;                        /* Current Volume count */
   DIRRES* director;                  /* Director resource */
   
   uint32_t FileId;                   /* Last file id inserted */

   /* Parmaters for Open Read Session */
   BSR *bsr;                          /* Bootstrap record -- has everything */
   bool mount_next_volume;            /* set to cause next volume mount */
   uint32_t read_VolSessionId;
   uint32_t read_VolSessionTime;
   uint32_t read_StartFile;
   uint32_t read_EndFile;
   uint32_t read_StartBlock;
   uint32_t read_EndBlock;

#endif /* STORAGE_DAEMON */

}; 



/* 
 * Structure for all daemons that keeps some summary
 *  info on the last job run.
 */
struct s_last_job {
   dlink link;
   int Errors;                        /* FD/SD errors */
   int JobType;
   int JobStatus;
   int JobLevel;
   uint32_t JobId;
   uint32_t VolSessionId;
   uint32_t VolSessionTime;
   uint32_t JobFiles;
   uint64_t JobBytes;
   time_t start_time;
   time_t end_time;
   char Job[MAX_NAME_LENGTH];
};

extern struct s_last_job last_job;               
extern dlist *last_jobs;


/* The following routines are found in lib/jcr.c */
extern bool init_jcr_subsystem(void);
extern JCR *new_jcr(int size, JCR_free_HANDLER *daemon_free_jcr);
extern void free_locked_jcr(JCR *jcr);
extern JCR *get_jcr_by_id(uint32_t JobId);
extern JCR *get_jcr_by_session(uint32_t SessionId, uint32_t SessionTime);
extern JCR *get_jcr_by_partial_name(char *Job);
extern JCR *get_jcr_by_full_name(char *Job);
extern JCR *get_next_jcr(JCR *jcr);
extern void set_jcr_job_status(JCR *jcr, int JobStatus);

#ifdef TRACE_JCR_CHAIN
extern void b_lock_jcr_chain(const char *filen, int line);
extern void b_unlock_jcr_chain(const char *filen, int line);
#define lock_jcr_chain() b_lock_jcr_chain(__FILE__, __LINE__);
#define unlock_jcr_chain() b_unlock_jcr_chain(__FILE__, __LINE__);
#else
extern void lock_jcr_chain();
extern void unlock_jcr_chain();
#endif

#ifdef DEBUG
extern void b_free_jcr(const char *file, int line, JCR *jcr);
#define free_jcr(jcr) b_free_jcr(__FILE__, __LINE__, (jcr))
#else
extern void free_jcr(JCR *jcr);
#endif

#endif /* __JCR_H_ */
