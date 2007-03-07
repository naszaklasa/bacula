/*
 * Bacula User Agent specific configuration and defines
 *
 *     Kern Sibbald, Sep MM
 *
 *     Version $Id: console_conf.h 3676 2006-11-21 20:14:47Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

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

/*
 * Resource codes -- they must be sequential for indexing
 */

enum {
   R_CONSOLE   = 1001,
   R_DIRECTOR,
   R_FIRST     = R_CONSOLE,
   R_LAST      = R_DIRECTOR           /* Keep this updated */
};

/*
 * Some resource attributes
 */
enum {
   R_NAME     = 1020,
   R_ADDRESS,
   R_PASSWORD,
   R_TYPE,
   R_BACKUP
};


/* Definition of the contents of each Resource */

/* Console "globals" */
struct CONRES {
   RES   hdr;
   char *rc_file;                     /* startup file */
   char *hist_file;                   /* command history file */
   char *password;                    /* UA server password */
   int tls_enable;                    /* Enable TLS on all connections */
   int tls_require;                   /* Require TLS on all connections */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Client Certificate File */
   char *tls_keyfile;                 /* TLS Client Key File */
   char *director;                    /* bind to director */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};

/* Director */
struct DIRRES {
   RES   hdr;
   int   DIRport;                     /* UA server port */
   char *address;                     /* UA server address */
   char *password;                    /* UA server password */
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
   CONRES res_cons;
   RES hdr;
};
