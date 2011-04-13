/*
 * Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#ifndef CONSOLECONF_H
#define CONSOLECONF_H

#include "bacula.h"
#include "jcr.h"

/*
 * Resource codes -- they must be sequential for indexing
 */
#define R_FIRST                       1001

#define R_CONSOLE                     1001
#define R_DIRECTOR                    1002

#define R_LAST                        R_DIRECTOR

/*
 * Some resource attributes
 */
#define R_NAME                        1020
#define R_ADDRESS                     1021
#define R_PASSWORD                    1022
#define R_TYPE                        1023
#define R_BACKUP                      1024


/* Definition of the contents of each Resource */

/* Console "globals" */
struct CONRES {
   RES   hdr;
   char *rc_file;                     /* startup file */
   char *hist_file;                   /* command history file */
   char *password;                    /* UA server password */
   bool tls_enable;                   /* Enable TLS on all connections */
   bool tls_require;                  /* Require TLS on all connections */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};

/* Director */
struct DIRRES {
   RES   hdr;
   uint32_t DIRport;                  /* UA server port */
   char *address;                     /* UA server address */
   char *password;                    /* UA server password */
   bool tls_enable;                   /* Enable TLS on all connections */
   bool tls_require;                  /* Require TLS on all connections */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};


/* Define the Union of all the above
 * resource structure definitions.
 */
union u_res {
   DIRRES res_dir;
   CONRES res_cons;
   RES hdr;
};

typedef union u_res URES;

#endif // CONSOLECONF_H
