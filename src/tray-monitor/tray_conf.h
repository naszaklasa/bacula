/*
 * Tray Monitor specific configuration and defines
 *
 *   Adapted from dird_conf.c
 *
 *     Nicolas Boichat, August MMIV
 *
 *    Version $Id: tray_conf.h 3685 2006-11-22 14:26:40Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

/* NOTE:  #includes at the end of this file */

/*
 * Resource codes -- they must be sequential for indexing
 */
enum rescode {
   R_MONITOR = 1001,
   R_DIRECTOR,
   R_CLIENT,
   R_STORAGE,
   R_FIRST = R_MONITOR,
   R_LAST  = R_STORAGE                /* keep this updated */
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

/* Director */
struct DIRRES {
   RES   hdr;
   int   DIRport;                     /* UA server port */
   char *address;                     /* UA server address */
   int enable_ssl;                    /* Use SSL */
};

/*
 *   Tray Monitor Resource
 *
 */
struct MONITOR {
   RES   hdr;
   int require_ssl;                   /* Require SSL for all connections */
   MSGS *messages;                    /* Daemon message handler */
   char *password;                    /* UA server password */
   utime_t RefreshInterval;           /* Status refresh interval */
   utime_t FDConnectTimeout;          /* timeout for connect in seconds */
   utime_t SDConnectTimeout;          /* timeout in seconds */
};


/*
 *   Client Resource
 *
 */
struct CLIENT {
   RES   hdr;

   int   FDport;                      /* Where File daemon listens */
   char *address;
   char *password;
   int enable_ssl;                    /* Use SSL */
};

/*
 *   Store Resource
 *
 */
struct STORE {
   RES   hdr;

   int   SDport;                      /* port where Directors connect */
   char *address;
   char *password;
   int enable_ssl;                    /* Use SSL */
};



/* Define the Union of all the above
 * resource structure definitions.
 */
union URES {
   MONITOR    res_monitor;
   DIRRES     res_dir;
   CLIENT     res_client;
   STORE      res_store;
   RES        hdr;
};
