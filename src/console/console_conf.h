/*
 * Bacula User Agent specific configuration and defines
 *
 *     Kern Sibbald, Sep MM
 *
 *     Version $Id: console_conf.h,v 1.9 2004/06/15 10:40:43 kerns Exp $
 */

/*
 * Resource codes -- they must be sequential for indexing   
 */

enum {
   R_CONSOLE   = 1001,
   R_DIRECTOR,
   R_FIRST     = R_CONSOLE,
   R_LAST      = R_DIRECTOR	      /* Keep this updated */
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
   RES	 hdr;
   char *rc_file;		      /* startup file */
   char *hist_file;		      /* command history file */
   int require_ssl;		      /* Require SSL on all connections */
   char *password;		      /* UA server password */
};

/* Director */
struct DIRRES {
   RES	 hdr;
   int	 DIRport;		      /* UA server port */
   char *address;		      /* UA server address */
   char *password;		      /* UA server password */
   int	enable_ssl;		      /* Use SSL */
};


/* Define the Union of all the above
 * resource structure definitions.
 */
union URES {
   DIRRES res_dir;
   CONRES res_cons;
   RES hdr;
};
