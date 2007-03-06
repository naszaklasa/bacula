/*
 * Bacula GNOME User Agent specific configuration and defines
 *
 *     Kern Sibbald, March 2002
 *
 *     Version $Id: console_conf.h,v 1.8 2005/07/09 13:55:34 kerns Exp $
 */

#ifndef __CONSOLE_CONF_H_
#define __CONSOLE_CONF_H_

/*
 * Resource codes -- they must be sequential for indexing
 */

enum {
   R_DIRECTOR = 1001,
   R_CONSOLE,
   R_CONSOLE_FONT,
   R_FIRST = R_DIRECTOR,
   R_LAST = R_CONSOLE_FONT            /* Keep this updated */
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


/* Definition of the contents of each Resource */
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

struct CONFONTRES {
   RES   hdr;
   char *fontface;                    /* Console Font specification */
   int require_ssl;                   /* Require SSL on all connections */
};

struct CONRES {
   RES   hdr;
   char *password;                    /* UA server password */
   int tls_enable;                    /* Enable TLS on all connections */
   int tls_require;                   /* Require TLS on all connections */
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
   DIRRES dir_res;
   CONRES con_res;
   CONFONTRES con_font;
   RES hdr;
};

typedef union u_res URES;

#endif
