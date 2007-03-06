/*
 * Definitions for using the Device functions in Bacula
 *  Tape and File storage access
 *
 * Kern Sibbald, MM
 *
 *   Version $Id: dev.h,v 1.51.4.3 2005/02/27 21:53:29 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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


#ifndef __DEV_H
#define __DEV_H 1

#undef DCR                            /* used by Bacula */

/* #define NEW_LOCK 1 */

#define new_lock_device(dev)             _new_lock_device(__FILE__, __LINE__, (dev))
#define new_lock_device_state(dev,state) _new_lock_device(__FILE__, __LINE__, (dev), (state))
#define new_unlock_device(dev)           _new_unlock_device(__FILE__, __LINE__, (dev))

#define lock_device(d) _lock_device(__FILE__, __LINE__, (d))
#define unlock_device(d) _unlock_device(__FILE__, __LINE__, (d))
#define block_device(d, s) _block_device(__FILE__, __LINE__, (d), s)
#define unblock_device(d) _unblock_device(__FILE__, __LINE__, (d))
#define steal_device_lock(d, p, s) _steal_device_lock(__FILE__, __LINE__, (d), (p), s)
#define give_back_device_lock(d, p) _give_back_device_lock(__FILE__, __LINE__, (d), (p))

/* Arguments to open_dev() */
enum {
   OPEN_READ_WRITE = 0,
   OPEN_READ_ONLY,
   OPEN_WRITE_ONLY
};

/* Generic status bits returned from status_dev() */
#define BMT_TAPE           (1<<0)     /* is tape device */
#define BMT_EOF            (1<<1)     /* just read EOF */
#define BMT_BOT            (1<<2)     /* at beginning of tape */
#define BMT_EOT            (1<<3)     /* end of tape reached */
#define BMT_SM             (1<<4)     /* DDS setmark */
#define BMT_EOD            (1<<5)     /* DDS at end of data */
#define BMT_WR_PROT        (1<<6)     /* tape write protected */
#define BMT_ONLINE         (1<<7)     /* tape online */
#define BMT_DR_OPEN        (1<<8)     /* tape door open */
#define BMT_IM_REP_EN      (1<<9)     /* immediate report enabled */


/* Test capabilities */
#define dev_cap(dev, cap) ((dev)->capabilities & (cap))

/* Bits for device capabilities */
#define CAP_EOF            (1<<0)     /* has MTWEOF */
#define CAP_BSR            (1<<1)     /* has MTBSR */
#define CAP_BSF            (1<<2)     /* has MTBSF */
#define CAP_FSR            (1<<3)     /* has MTFSR */
#define CAP_FSF            (1<<4)     /* has MTFSF */
#define CAP_EOM            (1<<5)     /* has MTEOM */
#define CAP_REM            (1<<6)     /* is removable media */
#define CAP_RACCESS        (1<<7)     /* is random access device */
#define CAP_AUTOMOUNT      (1<<8)     /* Read device at start to see what is there */
#define CAP_LABEL          (1<<9)     /* Label blank tapes */
#define CAP_ANONVOLS       (1<<10)    /* Mount without knowing volume name */
#define CAP_ALWAYSOPEN     (1<<11)    /* always keep device open */
#define CAP_AUTOCHANGER    (1<<12)    /* AutoChanger */
#define CAP_OFFLINEUNMOUNT (1<<13)    /* Offline before unmount */
#define CAP_STREAM         (1<<14)    /* Stream device */
#define CAP_BSFATEOM       (1<<15)    /* Backspace file at EOM */
#define CAP_FASTFSF        (1<<16)    /* Fast forward space file */
#define CAP_TWOEOF         (1<<17)    /* Write two eofs for EOM */
#define CAP_CLOSEONPOLL    (1<<18)    /* Close device on polling */
#define CAP_POSITIONBLOCKS (1<<19)    /* Use block positioning */
#define CAP_MTIOCGET       (1<<20)    /* Basic support for fileno and blkno */
#define CAP_REQMOUNT       (1<<21)    /* Require mount to read files back (typically: DVD) */

/* Test state */
#define dev_state(dev, st_state) ((dev)->state & (st_state))

/* Device state bits */
#define ST_OPENED          (1<<0)     /* set when device opened */
#define ST_TAPE            (1<<1)     /* is a tape device */
#define ST_FILE            (1<<2)     /* is a file device */
#define ST_FIFO            (1<<3)     /* is a fifo device */
#define ST_DVD             (1<<4)     /* is a DVD device */  
#define ST_PROG            (1<<5)     /* is a program device */
#define ST_LABEL           (1<<6)     /* label found */
#define ST_MALLOC          (1<<7)     /* dev packet malloc'ed in init_dev() */
#define ST_APPEND          (1<<8)     /* ready for Bacula append */
#define ST_READ            (1<<9)     /* ready for Bacula read */
#define ST_EOT             (1<<10)    /* at end of tape */
#define ST_WEOT            (1<<11)    /* Got EOT on write */
#define ST_EOF             (1<<12)    /* Read EOF i.e. zero bytes */
#define ST_NEXTVOL         (1<<13)    /* Start writing on next volume */
#define ST_SHORT           (1<<14)    /* Short block read */
#define ST_MOUNTED         (1<<15)    /* the device is mounted to the mount point */

/* dev_blocked states (mutually exclusive) */
enum {
   BST_NOT_BLOCKED = 0,               /* not blocked */
   BST_UNMOUNTED,                     /* User unmounted device */
   BST_WAITING_FOR_SYSOP,             /* Waiting for operator to mount tape */
   BST_DOING_ACQUIRE,                 /* Opening/validating/moving tape */
   BST_WRITING_LABEL,                  /* Labeling a tape */
   BST_UNMOUNTED_WAITING_FOR_SYSOP,    /* Closed by user during mount request */
   BST_MOUNT                           /* Mount request */
};

/* Volume Catalog Information structure definition */
struct VOLUME_CAT_INFO {
   /* Media info for the current Volume */
   uint32_t VolCatJobs;               /* number of jobs on this Volume */
   uint32_t VolCatFiles;              /* Number of files */
   uint32_t VolCatBlocks;             /* Number of blocks */
   uint64_t VolCatBytes;              /* Number of bytes written */
   uint32_t VolCatParts;              /* Number of parts written */
   uint32_t VolCatMounts;             /* Number of mounts this volume */
   uint32_t VolCatErrors;             /* Number of errors this volume */
   uint32_t VolCatWrites;             /* Number of writes this volume */
   uint32_t VolCatReads;              /* Number of reads this volume */
   uint64_t VolCatRBytes;             /* Number of bytes read */
   uint32_t VolCatRecycles;           /* Number of recycles this volume */
   uint32_t EndFile;                  /* Last file number */
   uint32_t EndBlock;                 /* Last block number */
   int32_t  LabelType;                /* Bacula/ANSI/IBM */
   int32_t  Slot;                     /* Slot in changer */
   bool     InChanger;                /* Set if vol in current magazine */
   uint32_t VolCatMaxJobs;            /* Maximum Jobs to write to volume */
   uint32_t VolCatMaxFiles;           /* Maximum files to write to volume */
   uint64_t VolCatMaxBytes;           /* Max bytes to write to volume */
   uint64_t VolCatCapacityBytes;      /* capacity estimate */
   uint64_t VolReadTime;              /* time spent reading */
   uint64_t VolWriteTime;             /* time spent writing this Volume */
   char VolCatStatus[20];             /* Volume status */
   char VolCatName[MAX_NAME_LENGTH];  /* Desired volume to mount */
};


typedef struct s_steal_lock {
   pthread_t  no_wait_id;             /* id of no wait thread */
   int        dev_blocked;            /* state */
   int        dev_prev_blocked;       /* previous blocked state */
} bsteal_lock_t;

struct DEVRES;                        /* Device resource defined in stored_conf.h */

/*
 * Device structure definition. There is one of these for
 *  each physical device. Everything here is "global" to
 *  that device and effects all jobs using the device.
 */
class DEVICE {
public:
   DEVICE *next;
   DEVICE *prev;
   dlist *attached_dcrs;              /* attached DCR list */
   pthread_mutex_t mutex;             /* access control */
   pthread_mutex_t spool_mutex;       /* mutex for updating spool_size */
   pthread_cond_t wait;               /* thread wait variable */
   pthread_cond_t wait_next_vol;      /* wait for tape to be mounted */
   pthread_t no_wait_id;              /* this thread must not wait */
   int dev_blocked;                   /* set if we must wait (i.e. change tape) */
   int dev_prev_blocked;              /* previous blocked state */
   int num_waiting;                   /* number of threads waiting */
   int num_writers;                   /* number of writing threads */
   int reserved_device;               /* number of device reservations */

   /* New access control in process of being implemented */
   brwlock_t lock;                    /* New mutual exclusion lock */

   int use_count;                     /* usage count on this device */
   int fd;                            /* file descriptor */
   int capabilities;                  /* capabilities mask */
   int state;                         /* state mask */
   int dev_errno;                     /* Our own errno */
   int mode;                          /* read/write modes */
   int openmode;                      /* parameter passed to open_dev (useful to reopen the device) */
   uint32_t drive_index;              /* Autochanger drive index */
   POOLMEM *dev_name;                 /* device name */
   char *errmsg;                      /* nicely edited error message */
   uint32_t block_num;                /* current block number base 0 */
   uint32_t file;                     /* current file number base 0 */
   uint64_t file_addr;                /* Current file read/write address */
   uint64_t file_size;                /* Current file size */
   uint32_t EndBlock;                 /* last block written */
   uint32_t EndFile;                  /* last file written */
   uint32_t min_block_size;           /* min block size */
   uint32_t max_block_size;           /* max block size */
   uint64_t max_volume_size;          /* max bytes to put on one volume */
   uint64_t max_file_size;            /* max file size to put in one file on volume */
   uint64_t volume_capacity;          /* advisory capacity */
   uint64_t max_spool_size;           /* maximum spool file size */
   uint64_t spool_size;               /* current spool size */
   uint32_t max_rewind_wait;          /* max secs to allow for rewind */
   uint32_t max_open_wait;            /* max secs to allow for open */
   uint32_t max_open_vols;            /* max simultaneous open volumes */
   
   uint64_t max_part_size;            /* max part size */
   uint64_t part_size;                /* current part size */
   uint32_t part;                     /* current part number */
   uint64_t part_start;               /* current part start address (relative to the whole volume) */
   uint32_t num_parts;                /* number of parts (total) */
   uint64_t free_space;               /* current free space on medium (without the current part) */
   int free_space_errno;              /* indicates:
                                       * - free_space_errno == 0: ignore free_space.
                                       * - free_space_errno < 0: an error occured. 
                                       * - free_space_errno > 0: free_space is valid. */
   
   utime_t  vol_poll_interval;        /* interval between polling Vol mount */
   DEVRES *device;                    /* pointer to Device Resource */
   btimer_t *tid;                     /* timer id */

   VOLUME_CAT_INFO VolCatInfo;        /* Volume Catalog Information */
   VOLUME_LABEL VolHdr;               /* Actual volume label */

   /* Device wait times ***FIXME*** look at durations */
   char BadVolName[MAX_NAME_LENGTH];  /* Last wrong Volume mounted */
   bool poll;                         /* set to poll Volume */
   int min_wait;
   int max_wait;
   int max_num_wait;
   int wait_sec;
   int rem_wait_sec;
   int num_wait;

   int is_tape() const;
   int is_file() const;
   int is_fifo() const;
   int is_dvd() const;
   int is_open() const;
   int is_labeled() const;
   int is_busy() const;               /* either reading or writing */
   int at_eof() const;
   int at_eom() const;
   int at_eot() const;
   int can_append() const;
   int can_read() const;
   const char *strerror() const;
   const char *archive_name() const;
   void set_eof();
   void set_eot();
   void set_append();
   void set_read();
   void clear_append();
   void clear_read();
};

/* Note, these return int not bool! */
inline int DEVICE::is_tape() const { return state & ST_TAPE; }
inline int DEVICE::is_file() const { return state & ST_FILE; }
inline int DEVICE::is_fifo() const { return state & ST_FIFO; }
inline int DEVICE::is_dvd()  const { return state & ST_DVD; }
inline int DEVICE::is_open() const { return state & ST_OPENED; }
inline int DEVICE::is_labeled() const { return state & ST_LABEL; }
inline int DEVICE::is_busy() const { return state & ST_READ || num_writers; }
inline int DEVICE::at_eof() const { return state & ST_EOF; }
inline int DEVICE::at_eot() const { return state & ST_EOT; }
inline int DEVICE::can_append() const { return state & ST_APPEND; }
inline int DEVICE::can_read() const { return state & ST_READ; }
inline void DEVICE::set_append() { state |= ST_APPEND; }
inline void DEVICE::set_read() { state |= ST_READ; }
inline void DEVICE::clear_append() { state &= ~ST_APPEND; }
inline void DEVICE::clear_read() { state &= ~ST_READ; }
inline const char *DEVICE::strerror() const { return errmsg; }
inline const char *DEVICE::archive_name() const { return dev_name; }
 

/*
 * Device Context (or Control) Record.
 *  There is one of these records for each Job that is using
 *  the device. Items in this record are "local" to the Job and
 *  do not affect other Jobs. Note, a job can have multiple
 *  DCRs open, each pointing to a different device. 
 */
class DCR {
public:
   dlink dev_link;                    /* link to attach to dev */
   JCR *jcr;                          /* pointer to JCR */
   DEVICE *dev;                       /* pointer to device */
   DEVRES *device;                    /* pointer to device resource */
   DEV_BLOCK *block;                  /* pointer to block */
   DEV_RECORD *rec;                   /* pointer to record */
   int spool_fd;                      /* fd if spooling */
   bool spool_data;                   /* set to spool data */
   bool spooling;                     /* set when actually spooling */
   bool dev_locked;                   /* set if dev already locked */
   bool NewVol;                       /* set if new Volume mounted */
   bool WroteVol;                     /* set if Volume written */
   bool NewFile;                      /* set when EOF written */
   bool reserved_device;              /* set if reserve done */
   uint32_t VolFirstIndex;            /* First file index this Volume */
   uint32_t VolLastIndex;             /* Last file index this Volume */
   uint32_t FileIndex;                /* Current File Index */
   uint32_t EndFile;                  /* End file written */
   uint32_t StartFile;                /* Start write file */
   uint32_t StartBlock;               /* Start write block */
   uint32_t EndBlock;                 /* Ending block written */
   int64_t spool_size;                /* Current spool size */
   int64_t max_spool_size;            /* Max job spool size */
   char VolumeName[MAX_NAME_LENGTH];  /* Volume name */
   char pool_name[MAX_NAME_LENGTH];   /* pool name */
   char pool_type[MAX_NAME_LENGTH];   /* pool type */
   char media_type[MAX_NAME_LENGTH];  /* media type */
   char dev_name[MAX_NAME_LENGTH];    /* dev name */
   VOLUME_CAT_INFO VolCatInfo;        /* Catalog info for desired volume */
};


/* Get some definition of function to position
 *  to the end of the medium in MTEOM. System
 *  dependent. Arrgggg!
 */
#ifndef MTEOM
#ifdef  MTSEOD
#define MTEOM MTSEOD
#endif
#ifdef MTEOD
#undef MTEOM
#define MTEOM MTEOD
#endif
#endif

#endif
