/*
 * Bacula GNOME User Agent specific configuration and defines
 *
 *     Kern Sibbald, March 2002
 *
 *     Version $Id: console_conf.h,v 1.6 2004/06/15 10:40:46 kerns Exp $
 */

#ifndef __CONSOLE_CONF_H_
#define __CONSOLE_CONF_H_

/*
 * Resource codes -- they must be sequential for indexing   
 */

enum {
   R_DIRECTOR = 1001,
   R_CONSOLE,
   R_CONSOLE_FONT
};

#define R_FIRST     R_DIRECTOR
#define R_LAST	    R_CONSOLE_FONT

/*
 * Some resource attributes
 */
#define R_NAME			      1020
#define R_ADDRESS		      1021
#define R_PASSWORD		      1022
#define R_TYPE			      1023
#define R_BACKUP		      1024


/* Definition of the contents of each Resource */
struct s_res_dir {
   RES	 hdr;
   int	 DIRport;		      /* UA server port */
   char *address;		      /* UA server address */
   char *password;		      /* UA server password */
   int enable_ssl;		      /* Use SSL */
};
typedef struct s_res_dir DIRRES;

struct s_con_font {
   RES	 hdr;
   char *fontface;		      /* Console Font specification */
   int require_ssl;		      /* Require SSL on all connections */
};
typedef struct s_con_font CONFONTRES;

struct s_con_res {
   RES	 hdr;
   char *password;		      /* UA server password */
   int require_ssl;		      /* Require SSL on all connections */
};
typedef struct s_con_res CONRES;


/* Define the Union of all the above
 * resource structure definitions.
 */
union u_res {
   struct s_res_dir	dir_res;
   struct s_con_font	con_font;
   struct s_con_res	con_res;
   RES hdr;
};

typedef union u_res URES;

#endif
