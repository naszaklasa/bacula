/*
 *   Version $Id: parse_conf.h,v 1.26.2.1 2006/03/28 16:42:19 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

struct RES_ITEM;                    /* Declare forward referenced structure */
typedef void (MSG_RES_HANDLER)(LEX *lc, RES_ITEM *item, int index, int pass);

/* This is the structure that defines
 * the record types (items) permitted within each
 * resource. It is used to define the configuration
 * tables.
 */
struct RES_ITEM {
   const char *name;                  /* Resource name i.e. Director, ... */
   MSG_RES_HANDLER *handler;          /* Routine storing the resource item */
   char **value;                      /* Where to store the item */
   int  code;                         /* item code/additional info */
   int  flags;                        /* flags: default, required, ... */
   int  default_value;                /* default value */
};

/* For storing name_addr items in res_items table */
#define ITEM(x) ((char **)&res_all.x)

#define MAX_RES_ITEMS 70              /* maximum resource items per RES */

/* This is the universal header that is
 * at the beginning of every resource
 * record.
 */
struct RES {
   RES *next;                         /* pointer to next resource of this type */
   char *name;                        /* resource name */
   char *desc;                        /* resource description */
   int   rcode;                       /* resource id or type */
   int   refcnt;                      /* reference count for releasing */
   char  item_present[MAX_RES_ITEMS]; /* set if item is present in conf file */
};


/*
 * Master Resource configuration structure definition
 * This is the structure that defines the
 * resources that are available to this daemon.
 */
struct RES_TABLE {
   const char *name;                  /* resource name */
   RES_ITEM *items;                   /* list of resource keywords */
   int rcode;                         /* code if needed */
};

/* Common Resource definitions */

#define MAX_RES_NAME_LENGTH MAX_NAME_LENGTH-1       /* maximum resource name length */

#define ITEM_REQUIRED    0x1          /* item required */
#define ITEM_DEFAULT     0x2          /* default supplied */
#define ITEM_NO_EQUALS   0x4          /* Don't scan = after name */

/* Message Resource */
struct MSGS {
   RES   hdr;
   char *mail_cmd;                    /* mail command */
   char *operator_cmd;                /* Operator command */
   DEST *dest_chain;                  /* chain of destinations */
   char send_msg[nbytes_for_bits(M_MAX+1)];  /* bit array of types */
};


/* Define the Union of all the above common
 * resource structure definitions.
 */
union CURES {
   MSGS  res_msgs;
   RES hdr;
};


/* Configuration routines */
int   parse_config(const char *cf, LEX_ERROR_HANDLER *scan_error = NULL);
void    free_config_resources(void);
RES   **save_config_resources(void);
RES   **new_res_head();


/* Resource routines */
RES *GetResWithName(int rcode, char *name);
RES *GetNextRes(int rcode, RES *res);
void b_LockRes(const char *file, int line);
void b_UnlockRes(const char *file, int line);
void dump_resource(int type, RES *res, void sendmsg(void *sock, const char *fmt, ...), void *sock);
void free_resource(RES *res, int type);
void init_resource(int type, RES_ITEM *item);
void save_resource(int type, RES_ITEM *item, int pass);
const char *res_to_str(int rcode);

/* Loop through each resource of type, returning in var */
#if defined(__GNUC__)
#define foreach_res(var, type) \
        for((var)=NULL; ((var)=(typeof(var))GetNextRes((type), (RES *)var));)
#else 
#define foreach_res(var, type) \
    for(var=NULL; (*((void **)&(var))=(void *)GetNextRes((type), (RES *)var));)
#endif



void store_str(LEX *lc, RES_ITEM *item, int index, int pass);
void store_dir(LEX *lc, RES_ITEM *item, int index, int pass);
void store_password(LEX *lc, RES_ITEM *item, int index, int pass);
void store_name(LEX *lc, RES_ITEM *item, int index, int pass);
void store_strname(LEX *lc, RES_ITEM *item, int index, int pass);
void store_res(LEX *lc, RES_ITEM *item, int index, int pass);
void store_alist_res(LEX *lc, RES_ITEM *item, int index, int pass);
void store_alist_str(LEX *lc, RES_ITEM *item, int index, int pass);
void store_int(LEX *lc, RES_ITEM *item, int index, int pass);
void store_pint(LEX *lc, RES_ITEM *item, int index, int pass);
void store_msgs(LEX *lc, RES_ITEM *item, int index, int pass);
void store_int64(LEX *lc, RES_ITEM *item, int index, int pass);
void store_yesno(LEX *lc, RES_ITEM *item, int index, int pass);
void store_time(LEX *lc, RES_ITEM *item, int index, int pass);
void store_size(LEX *lc, RES_ITEM *item, int index, int pass);
void store_defs(LEX *lc, RES_ITEM *item, int index, int pass);
void store_label(LEX *lc, RES_ITEM *item, int index, int pass);
