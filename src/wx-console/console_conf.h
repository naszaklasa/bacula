/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
/*
 * Version $Id: console_conf.h,v 1.4 2004/07/18 09:33:26 kerns Exp $
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
struct s_res_con {
   RES   hdr;
   char *rc_file;                     /* startup file */
   char *hist_file;                   /* command history file */
   int require_ssl;                   /* Require SSL on all connections */
   char *password;                    /* UA server password */
};
typedef struct s_res_con CONRES;

/* Director */
struct s_res_dir {
   RES   hdr;
   int   DIRport;                     /* UA server port */
   char *address;                     /* UA server address */
   char *password;                    /* UA server password */
   int  enable_ssl;                   /* Use SSL */
};
typedef struct s_res_dir DIRRES;


/* Define the Union of all the above
 * resource structure definitions.
 */
union u_res {
   struct s_res_dir     res_dir;
   struct s_res_con     res_cons;
   RES hdr;
};

typedef union u_res URES;

#endif // CONSOLECONF_H
