/*
 * Bacula File Daemon specific configuration
 *
 *     Kern Sibbald, Sep MM
 *
 *   Version $Id: filed_conf.h,v 1.18 2005/07/08 10:02:57 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

/*
 * Resource codes -- they must be sequential for indexing
 */
#define R_FIRST                       1001

#define R_DIRECTOR                    1001
#define R_CLIENT                      1002
#define R_MSGS                        1003

#define R_LAST                        R_MSGS

/*
 * Some resource attributes
 */
#define R_NAME                        1020
#define R_ADDRESS                     1021
#define R_PASSWORD                    1022
#define R_TYPE                        1023


/* Definition of the contents of each Resource */
struct DIRRES {
   RES   hdr;
   char *password;                    /* Director password */
   char *address;                     /* Director address or zero */
   int monitor;                       /* Have only access to status and .status functions */
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

struct CLIENT {
   RES   hdr;
   dlist *FDaddrs;
   char *working_directory;
   char *pid_directory;
   char *subsys_directory;
   char *scripts_directory;
   MSGS *messages;                    /* daemon message handler */
   int MaxConcurrentJobs;
   utime_t heartbeat_interval;        /* Interval to send heartbeats to Dir */
   utime_t SDConnectTimeout;          /* timeout in seconds */
   uint32_t max_network_buffer_size;  /* max network buf size */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};



/* Define the Union of all the above
 * resource structure definitions.
 */
union URES {
   DIRRES res_dir;
   CLIENT res_client;
   MSGS   res_msgs;
   RES    hdr;
};
