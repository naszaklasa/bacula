/*
 * Resource codes -- they must be sequential for indexing   
 *
 *   Version $Id: stored_conf.h,v 1.24 2004/09/19 18:56:29 kerns Exp $
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

enum {
   R_DIRECTOR = 3001,
   R_STORAGE,
   R_DEVICE,
   R_MSGS,
   R_FIRST = R_DIRECTOR,
   R_LAST  = R_MSGS                   /* keep this updated */
};

enum {
   R_NAME = 3020,
   R_ADDRESS,
   R_PASSWORD,
   R_TYPE,
   R_BACKUP
};


/* Definition of the contents of each Resource */
struct DIRRES {
   RES   hdr;

   char *password;                    /* Director password */
   char *address;                     /* Director IP address or zero */
   int enable_ssl;                    /* Use SSL with this Director */
   int monitor;                       /* Have only access to status and .status functions */
};


/* Storage daemon "global" definitions */
struct s_res_store {
   RES   hdr;

   dlist *sdaddrs;
   dlist *sddaddrs;
   char *working_directory;           /* working directory for checkpoints */
   char *pid_directory;
   char *subsys_directory;
   int require_ssl;                   /* Require SSL on all connections */
   uint32_t max_concurrent_jobs;      /* maximum concurrent jobs to run */
   MSGS *messages;                    /* Daemon message handler */
   utime_t heartbeat_interval;        /* Interval to send hb to FD */
};
typedef struct s_res_store STORES;

/* Device specific definitions */
struct DEVRES {
   RES   hdr;

   char *media_type;                  /* User assigned media type */
   char *device_name;                 /* Archive device name */
   char *changer_name;                /* Changer device name */
   char *changer_command;             /* Changer command  -- external program */
   char *alert_command;               /* Alert command -- external program */
   char *spool_directory;             /* Spool file directory */
   uint32_t drive_index;              /* Autochanger drive index */
   uint32_t cap_bits;                 /* Capabilities of this device */
   uint32_t max_changer_wait;         /* Changer timeout */
   uint32_t max_rewind_wait;          /* maximum secs to wait for rewind */
   uint32_t max_open_wait;            /* maximum secs to wait for open */
   uint32_t max_open_vols;            /* maximum simultaneous open volumes */
   uint32_t min_block_size;           /* min block size */
   uint32_t max_block_size;           /* max block size */
   uint32_t max_volume_jobs;          /* max jobs to put on one volume */
   uint32_t max_network_buffer_size;  /* max network buf size */
   utime_t  vol_poll_interval;        /* interval between polling volume during mount */
   int64_t max_volume_files;          /* max files to put on one volume */
   int64_t max_volume_size;           /* max bytes to put on one volume */
   int64_t max_file_size;             /* max file size in bytes */
   int64_t volume_capacity;           /* advisory capacity */
   int64_t max_spool_size;            /* Max spool size for all jobs */
   int64_t max_job_spool_size;        /* Max spool size for any single job */
   DEVICE *dev;                       /* Pointer to phyical dev -- set at runtime */
};

union URES {
   DIRRES res_dir;
   STORES res_store;
   DEVRES res_dev;
   MSGS   res_msgs;
   RES    hdr;
};
