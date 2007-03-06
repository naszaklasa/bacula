/*
 *   Version $Id: parse_conf.h,v 1.34.2.1 2007/01/24 01:59:02 robertnelson Exp $
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

struct RES_ITEM;                    /* Declare forward referenced structure */
class RES;                         /* Declare forware referenced structure */
typedef void (MSG_RES_HANDLER)(LEX *lc, RES_ITEM *item, int index, int pass);

/* This is the structure that defines
 * the record types (items) permitted within each
 * resource. It is used to define the configuration
 * tables.
 */
struct RES_ITEM {
   const char *name;                  /* Resource name i.e. Director, ... */
   MSG_RES_HANDLER *handler;          /* Routine storing the resource item */
   union {
      char **value;                   /* Where to store the item */
      char **charvalue;
      uint32_t ui32value;
      int32_t i32value;
      uint64_t ui64value;
      int64_t i64value;
      bool boolvalue;
      utime_t utimevalue;
      RES *resvalue;
      RES **presvalue;
   };
   int  code;                         /* item code/additional info */
   int  flags;                        /* flags: default, required, ... */
   int  default_value;                /* default value */
};

/* For storing name_addr items in res_items table */
#define ITEM(x) {(char **)&res_all.x}

#define MAX_RES_ITEMS 70              /* maximum resource items per RES */

/* This is the universal header that is
 * at the beginning of every resource
 * record.
 */
class RES {
public:
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
class MSGS {
public:
   RES   hdr;
   char *mail_cmd;                    /* mail command */
   char *operator_cmd;                /* Operator command */
   DEST *dest_chain;                  /* chain of destinations */
   char send_msg[nbytes_for_bits(M_MAX+1)];  /* bit array of types */

   /* Methods */
   char *name() const;
};

inline char *MSGS::name() const { return hdr.name; }


/* Define the Union of all the above common
 * resource structure definitions.
 */
union CURES {
   MSGS  res_msgs;
   RES hdr;
};


/* Configuration routines */
int   parse_config(const char *cf, LEX_ERROR_HANDLER *scan_error = NULL, int err_type=M_ERROR_TERM);
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
#ifdef HAVE_TYPEOF
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
void store_bit(LEX *lc, RES_ITEM *item, int index, int pass);
void store_bool(LEX *lc, RES_ITEM *item, int index, int pass);
void store_time(LEX *lc, RES_ITEM *item, int index, int pass);
void store_size(LEX *lc, RES_ITEM *item, int index, int pass);
void store_defs(LEX *lc, RES_ITEM *item, int index, int pass);
void store_label(LEX *lc, RES_ITEM *item, int index, int pass);
