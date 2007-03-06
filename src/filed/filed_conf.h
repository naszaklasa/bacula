/*
 * Bacula File Daemon specific configuration
 *
 *     Kern Sibbald, Sep MM
 *
 *   Version $Id: filed_conf.h,v 1.13 2004/08/22 17:37:44 nboichat Exp $
 */
/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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
   int enable_ssl;                    /* Use SSL for this Director */
   int monitor;                       /* Have only access to status and .status functions */
};

struct CLIENT {
   RES   hdr;
   dlist *FDaddrs;
   char *working_directory;
   char *pid_directory;
   char *subsys_directory;
   int require_ssl;                   /* Require SSL on all connections */
   MSGS *messages;                    /* daemon message handler */
   int MaxConcurrentJobs;
   utime_t heartbeat_interval;        /* Interval to send heartbeats to Dir */
   utime_t SDConnectTimeout;          /* timeout in seconds */
   uint32_t max_network_buffer_size;  /* max network buf size */
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
