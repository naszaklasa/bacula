/*
 *   Main configuration file parser for Bacula Directors,
 *    some parts may be split into separate files such as
 *    the schedule configuration (run_config.c).
 *
 *   Note, the configuration file parser consists of three parts
 *
 *   1. The generic lexical scanner in lib/lex.c and lib/lex.h
 *
 *   2. The generic config  scanner in lib/parse_config.c and 
 *      lib/parse_config.h.
 *      These files contain the parser code, some utility
 *      routines, and the common store routines (name, int,
 *      string).
 *
 *   3. The daemon specific file, which contains the Resource
 *      definitions as well as any specific store routines
 *      for the resource records.
 *
 *     Kern Sibbald, January MM
 *
 *     Version $Id: dird_conf.c,v 1.108.4.1.2.2 2005/04/13 11:40:44 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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

#include "bacula.h"
#include "dird.h"

/* Define the first and last resource ID record
 * types. Note, these should be unique for each
 * daemon though not a requirement.
 */
int r_first = R_FIRST;
int r_last  = R_LAST;
static RES *sres_head[R_LAST - R_FIRST + 1];
RES **res_head = sres_head;

/* Imported subroutines */
extern void store_run(LEX *lc, RES_ITEM *item, int index, int pass);
extern void store_finc(LEX *lc, RES_ITEM *item, int index, int pass);
extern void store_inc(LEX *lc, RES_ITEM *item, int index, int pass);


/* Forward referenced subroutines */

void store_jobtype(LEX *lc, RES_ITEM *item, int index, int pass);
void store_level(LEX *lc, RES_ITEM *item, int index, int pass);
void store_replace(LEX *lc, RES_ITEM *item, int index, int pass);
void store_acl(LEX *lc, RES_ITEM *item, int index, int pass);


/* We build the current resource here as we are
 * scanning the resource configuration definition,
 * then move it to allocated memory when the resource
 * scan is complete.
 */
URES res_all;
int  res_all_size = sizeof(res_all);


/* Definition of records permitted within each
 * resource with the routine to process the record 
 * information.  NOTE! quoted names must be in lower case.
 */ 
/* 
 *    Director Resource
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM dir_items[] = {
   {"name",        store_name,     ITEM(res_dir.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(res_dir.hdr.desc), 0, 0, 0},
   {"messages",    store_res,      ITEM(res_dir.messages), R_MSGS, 0, 0},
   {"dirport",     store_addresses_port,    ITEM(res_dir.DIRaddrs),  0, ITEM_DEFAULT, 9101},
   {"diraddress",  store_addresses_address, ITEM(res_dir.DIRaddrs),  0, ITEM_DEFAULT, 9101},
   {"diraddresses",store_addresses,         ITEM(res_dir.DIRaddrs),  0, ITEM_DEFAULT, 9101},
   {"queryfile",   store_dir,      ITEM(res_dir.query_file), 0, ITEM_REQUIRED, 0},
   {"workingdirectory", store_dir, ITEM(res_dir.working_directory), 0, ITEM_REQUIRED, 0},
   {"piddirectory",store_dir,     ITEM(res_dir.pid_directory), 0, ITEM_REQUIRED, 0},
   {"subsysdirectory", store_dir,  ITEM(res_dir.subsys_directory), 0, 0, 0},
   {"requiressl",  store_yesno,    ITEM(res_dir.require_ssl), 1, ITEM_DEFAULT, 0},
   {"enablessl",   store_yesno,    ITEM(res_dir.enable_ssl), 1, ITEM_DEFAULT, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_dir.MaxConcurrentJobs), 0, ITEM_DEFAULT, 1},
   {"password",    store_password, ITEM(res_dir.password), 0, ITEM_REQUIRED, 0},
   {"fdconnecttimeout", store_time,ITEM(res_dir.FDConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {"sdconnecttimeout", store_time,ITEM(res_dir.SDConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {NULL, NULL, NULL, 0, 0, 0}
};

/* 
 *    Console Resource
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM con_items[] = {
   {"name",        store_name,     ITEM(res_con.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(res_con.hdr.desc), 0, 0, 0},
   {"enablessl",   store_yesno,    ITEM(res_con.enable_ssl), 1, ITEM_DEFAULT, 0},
   {"password",    store_password, ITEM(res_con.password), 0, ITEM_REQUIRED, 0},
   {"jobacl",      store_acl,      ITEM(res_con.ACL_lists), Job_ACL, 0, 0},
   {"clientacl",   store_acl,      ITEM(res_con.ACL_lists), Client_ACL, 0, 0},
   {"storageacl",  store_acl,      ITEM(res_con.ACL_lists), Storage_ACL, 0, 0},
   {"scheduleacl", store_acl,      ITEM(res_con.ACL_lists), Schedule_ACL, 0, 0},
   {"runacl",      store_acl,      ITEM(res_con.ACL_lists), Run_ACL, 0, 0},
   {"poolacl",     store_acl,      ITEM(res_con.ACL_lists), Pool_ACL, 0, 0},
   {"commandacl",  store_acl,      ITEM(res_con.ACL_lists), Command_ACL, 0, 0},
   {"filesetacl",  store_acl,      ITEM(res_con.ACL_lists), FileSet_ACL, 0, 0},
   {"catalogacl",  store_acl,      ITEM(res_con.ACL_lists), Catalog_ACL, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0}
};


/* 
 *    Client or File daemon resource
 *
 *   name          handler     value                 code flags    default_value
 */

static RES_ITEM cli_items[] = {
   {"name",     store_name,       ITEM(res_client.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,     ITEM(res_client.hdr.desc), 0, 0, 0},
   {"address",  store_str,        ITEM(res_client.address),  0, ITEM_REQUIRED, 0},
   {"fdaddress",  store_str,      ITEM(res_client.address),  0, 0, 0},
   {"fdport",   store_pint,       ITEM(res_client.FDport),   0, ITEM_DEFAULT, 9102},
   {"password", store_password,   ITEM(res_client.password), 0, ITEM_REQUIRED, 0},
   {"fdpassword", store_password,   ITEM(res_client.password), 0, 0, 0},
   {"catalog",  store_res,        ITEM(res_client.catalog),  R_CATALOG, ITEM_REQUIRED, 0},
   {"fileretention", store_time,  ITEM(res_client.FileRetention), 0, ITEM_DEFAULT, 60*60*24*60},
   {"jobretention",  store_time,  ITEM(res_client.JobRetention),  0, ITEM_DEFAULT, 60*60*24*180},
   {"autoprune", store_yesno,     ITEM(res_client.AutoPrune), 1, ITEM_DEFAULT, 1},
   {"enablessl", store_yesno,     ITEM(res_client.enable_ssl), 1, ITEM_DEFAULT, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_client.MaxConcurrentJobs), 0, ITEM_DEFAULT, 1},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Storage daemon resource
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM store_items[] = {
   {"name",        store_name,     ITEM(res_store.hdr.name),   0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(res_store.hdr.desc),   0, 0, 0},
   {"sdport",      store_pint,     ITEM(res_store.SDport),     0, ITEM_DEFAULT, 9103},
   {"address",     store_str,      ITEM(res_store.address),    0, ITEM_REQUIRED, 0},
   {"sdaddress",   store_str,      ITEM(res_store.address),    0, 0, 0},
   {"password",    store_password, ITEM(res_store.password),   0, ITEM_REQUIRED, 0},
   {"sdpassword",  store_password, ITEM(res_store.password),   0, 0, 0},
   {"device",      store_strname,  ITEM(res_store.dev_name),   0, ITEM_REQUIRED, 0},
   {"sddevicename", store_strname, ITEM(res_store.dev_name),   0, 0, 0},
   {"mediatype",   store_strname,  ITEM(res_store.media_type), 0, ITEM_REQUIRED, 0},
   {"autochanger", store_yesno,    ITEM(res_store.autochanger), 1, ITEM_DEFAULT, 0},
   {"enablessl",   store_yesno,    ITEM(res_store.enable_ssl),  1, ITEM_DEFAULT, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_store.MaxConcurrentJobs), 0, ITEM_DEFAULT, 1},
   {"sddport", store_pint, ITEM(res_store.SDDport), 0, 0, 0}, /* deprecated */
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* 
 *    Catalog Resource Directives
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM cat_items[] = {
   {"name",     store_name,     ITEM(res_cat.hdr.name),    0, ITEM_REQUIRED, 0},
   {"description", store_str,   ITEM(res_cat.hdr.desc),    0, 0, 0},
   {"address",  store_str,      ITEM(res_cat.db_address),  0, 0, 0},
   {"dbaddress", store_str,     ITEM(res_cat.db_address),  0, 0, 0},
   {"dbport",   store_pint,     ITEM(res_cat.db_port),      0, 0, 0},
   /* keep this password as store_str for the moment */
   {"password", store_str,      ITEM(res_cat.db_password), 0, 0, 0},
   {"dbpassword", store_str,    ITEM(res_cat.db_password), 0, 0, 0},
   {"user",     store_str,      ITEM(res_cat.db_user),     0, 0, 0},
   {"dbname",   store_str,      ITEM(res_cat.db_name),     0, ITEM_REQUIRED, 0},
   {"dbsocket", store_str,      ITEM(res_cat.db_socket),   0, 0, 0}, 
   /* Turned off for the moment */
   {"multipleconnections", store_yesno, ITEM(res_cat.mult_db_connections), 0, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* 
 *    Job Resource Directives
 *
 *   name          handler     value                 code flags    default_value
 */
RES_ITEM job_items[] = {
   {"name",      store_name,    ITEM(res_job.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,   ITEM(res_job.hdr.desc), 0, 0, 0},
   {"type",      store_jobtype, ITEM(res_job.JobType),  0, ITEM_REQUIRED, 0},
   {"level",     store_level,   ITEM(res_job.JobLevel),    0, 0, 0},
   {"messages",  store_res,     ITEM(res_job.messages), R_MSGS, ITEM_REQUIRED, 0},
   {"storage",   store_alist_res, ITEM(res_job.storage),  R_STORAGE, ITEM_REQUIRED, MAX_STORE},
   {"pool",      store_res,     ITEM(res_job.pool),     R_POOL, ITEM_REQUIRED, 0},
   {"fullbackuppool",  store_res, ITEM(res_job.full_pool),   R_POOL, 0, 0},
   {"incrementalbackuppool",  store_res, ITEM(res_job.inc_pool), R_POOL, 0, 0},
   {"differentialbackuppool", store_res, ITEM(res_job.dif_pool), R_POOL, 0, 0},
   {"client",    store_res,     ITEM(res_job.client),   R_CLIENT, ITEM_REQUIRED, 0},
   {"fileset",   store_res,     ITEM(res_job.fileset),  R_FILESET, ITEM_REQUIRED, 0},
   {"schedule",  store_res,     ITEM(res_job.schedule), R_SCHEDULE, 0, 0},
   {"verifyjob", store_res,     ITEM(res_job.verify_job), R_JOB, 0, 0},
   {"jobdefs",   store_res,     ITEM(res_job.jobdefs),  R_JOBDEFS, 0, 0},
   {"where",    store_dir,      ITEM(res_job.RestoreWhere), 0, 0, 0},
   {"bootstrap",store_dir,      ITEM(res_job.RestoreBootstrap), 0, 0, 0},
   {"writebootstrap",store_dir, ITEM(res_job.WriteBootstrap), 0, 0, 0},
   {"replace",  store_replace,  ITEM(res_job.replace), 0, ITEM_DEFAULT, REPLACE_ALWAYS},
   {"maxruntime",   store_time, ITEM(res_job.MaxRunTime), 0, 0, 0},
   {"maxwaittime",  store_time, ITEM(res_job.MaxWaitTime), 0, 0, 0},
   {"maxstartdelay",store_time, ITEM(res_job.MaxStartDelay), 0, 0, 0},
   {"jobretention", store_time, ITEM(res_job.JobRetention),  0, 0, 0},
   {"prefixlinks", store_yesno, ITEM(res_job.PrefixLinks), 1, ITEM_DEFAULT, 0},
   {"prunejobs",   store_yesno, ITEM(res_job.PruneJobs), 1, ITEM_DEFAULT, 0},
   {"prunefiles",  store_yesno, ITEM(res_job.PruneFiles), 1, ITEM_DEFAULT, 0},
   {"prunevolumes",store_yesno, ITEM(res_job.PruneVolumes), 1, ITEM_DEFAULT, 0},
   {"spoolattributes",store_yesno, ITEM(res_job.SpoolAttributes), 1, ITEM_DEFAULT, 0},
   {"spooldata",   store_yesno, ITEM(res_job.spool_data), 1, ITEM_DEFAULT, 0},
   {"rerunfailedlevels",   store_yesno, ITEM(res_job.rerun_failed_levels), 1, ITEM_DEFAULT, 0},
   {"runbeforejob", store_str,  ITEM(res_job.RunBeforeJob), 0, 0, 0},
   {"runafterjob",  store_str,  ITEM(res_job.RunAfterJob),  0, 0, 0},
   {"runafterfailedjob",  store_str,  ITEM(res_job.RunAfterFailedJob),  0, 0, 0},
   {"clientrunbeforejob", store_str,  ITEM(res_job.ClientRunBeforeJob), 0, 0, 0},
   {"clientrunafterjob",  store_str,  ITEM(res_job.ClientRunAfterJob),  0, 0, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_job.MaxConcurrentJobs), 0, ITEM_DEFAULT, 1},
   {"rescheduleonerror", store_yesno, ITEM(res_job.RescheduleOnError), 1, ITEM_DEFAULT, 0},
   {"rescheduleinterval", store_time, ITEM(res_job.RescheduleInterval), 0, ITEM_DEFAULT, 60 * 30},
   {"rescheduletimes", store_pint, ITEM(res_job.RescheduleTimes), 0, 0, 0},
   {"priority",   store_pint, ITEM(res_job.Priority), 0, ITEM_DEFAULT, 10},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* FileSet resource
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM fs_items[] = {
   {"name",        store_name, ITEM(res_fs.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,  ITEM(res_fs.hdr.desc), 0, 0, 0},
   {"include",     store_inc,  NULL,                  0, ITEM_NO_EQUALS, 0},
   {"exclude",     store_inc,  NULL,                  1, ITEM_NO_EQUALS, 0},
   {"ignorefilesetchanges", store_yesno, ITEM(res_fs.ignore_fs_changes), 1, ITEM_DEFAULT, 0},
   {NULL,          NULL,       NULL,                  0, 0, 0} 
};

/* Schedule -- see run_conf.c */
/* Schedule
 *
 *   name          handler     value                 code flags    default_value
 */
static RES_ITEM sch_items[] = {
   {"name",     store_name,  ITEM(res_sch.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str, ITEM(res_sch.hdr.desc), 0, 0, 0},
   {"run",      store_run,   ITEM(res_sch.run),      0, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Pool resource
 *
 *   name             handler     value                        code flags default_value
 */
static RES_ITEM pool_items[] = {
   {"name",            store_name,    ITEM(res_pool.hdr.name),      0, ITEM_REQUIRED, 0},
   {"description",     store_str,     ITEM(res_pool.hdr.desc),      0, 0,     0},
   {"pooltype",        store_strname, ITEM(res_pool.pool_type),     0, ITEM_REQUIRED, 0},
   {"labelformat",     store_strname, ITEM(res_pool.label_format),  0, 0,     0},
   {"cleaningprefix",  store_strname, ITEM(res_pool.cleaning_prefix),  0, 0,     0},
   {"usecatalog",      store_yesno, ITEM(res_pool.use_catalog),     1, ITEM_DEFAULT,  1},
   {"usevolumeonce",   store_yesno, ITEM(res_pool.use_volume_once), 1, 0,        0},
   {"purgeoldestvolume", store_yesno, ITEM(res_pool.purge_oldest_volume), 1, 0, 0},
   {"recycleoldestvolume", store_yesno, ITEM(res_pool.recycle_oldest_volume), 1, 0, 0},
   {"recyclecurrentvolume", store_yesno, ITEM(res_pool.recycle_current_volume), 1, 0, 0},
   {"maximumvolumes",  store_pint,  ITEM(res_pool.max_volumes),     0, 0,        0},
   {"maximumvolumejobs", store_pint,  ITEM(res_pool.MaxVolJobs),    0, 0,       0},
   {"maximumvolumefiles", store_pint, ITEM(res_pool.MaxVolFiles),   0, 0,       0},
   {"maximumvolumebytes", store_size, ITEM(res_pool.MaxVolBytes),   0, 0,       0},
   {"acceptanyvolume", store_yesno, ITEM(res_pool.accept_any_volume), 1, ITEM_DEFAULT,     1},
   {"catalogfiles",    store_yesno, ITEM(res_pool.catalog_files),   1, ITEM_DEFAULT,  1},
   {"volumeretention", store_time,  ITEM(res_pool.VolRetention),    0, ITEM_DEFAULT, 60*60*24*365},
   {"volumeuseduration", store_time,  ITEM(res_pool.VolUseDuration),0, 0, 0},
   {"autoprune",       store_yesno, ITEM(res_pool.AutoPrune), 1, ITEM_DEFAULT, 1},
   {"recycle",         store_yesno, ITEM(res_pool.Recycle),     1, ITEM_DEFAULT, 1},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* 
 * Counter Resource
 *   name             handler     value                        code flags default_value
 */
static RES_ITEM counter_items[] = {
   {"name",            store_name,    ITEM(res_counter.hdr.name),        0, ITEM_REQUIRED, 0},
   {"description",     store_str,     ITEM(res_counter.hdr.desc),        0, 0,     0},
   {"minimum",         store_int,     ITEM(res_counter.MinValue),        0, ITEM_DEFAULT, 0},
   {"maximum",         store_pint,    ITEM(res_counter.MaxValue),        0, ITEM_DEFAULT, INT32_MAX},
   {"wrapcounter",     store_res,     ITEM(res_counter.WrapCounter),     R_COUNTER, 0, 0},
   {"catalog",         store_res,     ITEM(res_counter.Catalog),         R_CATALOG, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};


/* Message resource */
extern RES_ITEM msgs_items[];

/* 
 * This is the master resource definition.  
 * It must have one item for each of the resources.
 *
 *  NOTE!!! keep it in the same order as the R_codes
 *    or eliminate all resources[rindex].name
 *
 *  name             items        rcode        res_head
 */
RES_TABLE resources[] = {
   {"director",      dir_items,   R_DIRECTOR},
   {"client",        cli_items,   R_CLIENT},
   {"job",           job_items,   R_JOB},
   {"storage",       store_items, R_STORAGE},
   {"catalog",       cat_items,   R_CATALOG},
   {"schedule",      sch_items,   R_SCHEDULE},
   {"fileset",       fs_items,    R_FILESET},
   {"pool",          pool_items,  R_POOL},
   {"messages",      msgs_items,  R_MSGS},
   {"counter",       counter_items, R_COUNTER},
   {"console",       con_items,   R_CONSOLE},
   {"jobdefs",       job_items,   R_JOBDEFS},
   {NULL,            NULL,        0}
};


/* Keywords (RHS) permitted in Job Level records   
 *
 *   level_name      level              job_type
 */
struct s_jl joblevels[] = {
   {"Full",          L_FULL,            JT_BACKUP},
   {"Base",          L_BASE,            JT_BACKUP},
   {"Incremental",   L_INCREMENTAL,     JT_BACKUP},
   {"Differential",  L_DIFFERENTIAL,    JT_BACKUP},
   {"Since",         L_SINCE,           JT_BACKUP},
   {"Catalog",       L_VERIFY_CATALOG,  JT_VERIFY},
   {"InitCatalog",   L_VERIFY_INIT,     JT_VERIFY},
   {"VolumeToCatalog", L_VERIFY_VOLUME_TO_CATALOG,   JT_VERIFY},
   {"DiskToCatalog", L_VERIFY_DISK_TO_CATALOG,   JT_VERIFY},
   {"Data",          L_VERIFY_DATA,     JT_VERIFY},
   {" ",             L_NONE,            JT_ADMIN},
   {" ",             L_NONE,            JT_RESTORE},
   {NULL,            0,                          0}
};

/* Keywords (RHS) permitted in Job type records   
 *
 *   type_name       job_type
 */
struct s_jt jobtypes[] = {
   {"backup",        JT_BACKUP},
   {"admin",         JT_ADMIN},
   {"verify",        JT_VERIFY},
   {"restore",       JT_RESTORE},
   {NULL,            0}
};

#ifdef old_deprecated_code

/* Keywords (RHS) permitted in Backup and Verify records */
static struct s_kw BakVerFields[] = {
   {"client",        'C'},
   {"fileset",       'F'},
   {"level",         'L'}, 
   {NULL,            0}
};

/* Keywords (RHS) permitted in Restore records */
static struct s_kw RestoreFields[] = {
   {"client",        'C'},
   {"fileset",       'F'},
   {"jobid",         'J'},            /* JobId to restore */
   {"where",         'W'},            /* root of restore */
   {"replace",       'R'},            /* replacement options */
   {"bootstrap",     'B'},            /* bootstrap file */
   {NULL,              0}
};
#endif

/* Options permitted in Restore replace= */
struct s_kw ReplaceOptions[] = {
   {"always",         REPLACE_ALWAYS},
   {"ifnewer",        REPLACE_IFNEWER},
   {"ifolder",        REPLACE_IFOLDER},
   {"never",          REPLACE_NEVER},
   {NULL,               0}
};

const char *level_to_str(int level)
{
   int i;
   static char level_no[30];
   const char *str = level_no;

   bsnprintf(level_no, sizeof(level_no), "%d", level);    /* default if not found */
   for (i=0; joblevels[i].level_name; i++) {
      if (level == joblevels[i].level) {
         str = joblevels[i].level_name;
         break;
      }
   }
   return str;
}

/* Dump contents of resource */
void dump_resource(int type, RES *reshdr, void sendit(void *sock, const char *fmt, ...), void *sock)
{
   URES *res = (URES *)reshdr;
   bool recurse = true;
   char ed1[100], ed2[100];

   if (res == NULL) {
      sendit(sock, "No %s resource defined\n", res_to_str(type));
      return;
   }
   if (type < 0) {                    /* no recursion */
      type = - type;
      recurse = false;
   }
   switch (type) {
   case R_DIRECTOR:
      sendit(sock, "Director: name=%s MaxJobs=%d FDtimeout=%s SDtimeout=%s\n", 
         reshdr->name, res->res_dir.MaxConcurrentJobs, 
         edit_uint64(res->res_dir.FDConnectTimeout, ed1),
         edit_uint64(res->res_dir.SDConnectTimeout, ed2));
      if (res->res_dir.query_file) {
         sendit(sock, "   query_file=%s\n", res->res_dir.query_file);
      }
      if (res->res_dir.messages) {
         sendit(sock, "  --> ");
         dump_resource(-R_MSGS, (RES *)res->res_dir.messages, sendit, sock);
      }
      break;
   case R_CONSOLE:
      sendit(sock, "Console: name=%s SSL=%d\n", 
         res->res_con.hdr.name, res->res_con.enable_ssl);
      break;
   case R_COUNTER:
      if (res->res_counter.WrapCounter) {
         sendit(sock, "Counter: name=%s min=%d max=%d cur=%d wrapcntr=%s\n",
            res->res_counter.hdr.name, res->res_counter.MinValue, 
            res->res_counter.MaxValue, res->res_counter.CurrentValue,
            res->res_counter.WrapCounter->hdr.name);
      } else {
         sendit(sock, "Counter: name=%s min=%d max=%d\n",
            res->res_counter.hdr.name, res->res_counter.MinValue, 
            res->res_counter.MaxValue);
      }
      if (res->res_counter.Catalog) {
         sendit(sock, "  --> ");
         dump_resource(-R_CATALOG, (RES *)res->res_counter.Catalog, sendit, sock);
      }
      break;

   case R_CLIENT:
      sendit(sock, "Client: name=%s address=%s FDport=%d MaxJobs=%u\n",
         res->res_client.hdr.name, res->res_client.address, res->res_client.FDport,
         res->res_client.MaxConcurrentJobs);
      sendit(sock, "      JobRetention=%s FileRetention=%s AutoPrune=%d\n",
         edit_utime(res->res_client.JobRetention, ed1, sizeof(ed1)),
         edit_utime(res->res_client.FileRetention, ed2, sizeof(ed2)),
         res->res_client.AutoPrune);
      if (res->res_client.catalog) {
         sendit(sock, "  --> ");
         dump_resource(-R_CATALOG, (RES *)res->res_client.catalog, sendit, sock);
      }
      break;
   case R_STORAGE:
      sendit(sock, "Storage: name=%s address=%s SDport=%d MaxJobs=%u\n\
      DeviceName=%s MediaType=%s\n",
         res->res_store.hdr.name, res->res_store.address, res->res_store.SDport,
         res->res_store.MaxConcurrentJobs,
         res->res_store.dev_name, res->res_store.media_type);
      break;
   case R_CATALOG:
      sendit(sock, "Catalog: name=%s address=%s DBport=%d db_name=%s\n\
      db_user=%s\n",
         res->res_cat.hdr.name, NPRT(res->res_cat.db_address),
         res->res_cat.db_port, res->res_cat.db_name, NPRT(res->res_cat.db_user));
      break;
   case R_JOB:
   case R_JOBDEFS:
      sendit(sock, "%s: name=%s JobType=%d level=%s Priority=%d MaxJobs=%u\n", 
         type == R_JOB ? "Job" : "JobDefs",
         res->res_job.hdr.name, res->res_job.JobType, 
         level_to_str(res->res_job.JobLevel), res->res_job.Priority,
         res->res_job.MaxConcurrentJobs);
      sendit(sock, "     Resched=%d Times=%d Interval=%s Spool=%d\n",
          res->res_job.RescheduleOnError, res->res_job.RescheduleTimes,
          edit_uint64_with_commas(res->res_job.RescheduleInterval, ed1),
          res->res_job.spool_data);
      if (res->res_job.client) {
         sendit(sock, "  --> ");
         dump_resource(-R_CLIENT, (RES *)res->res_job.client, sendit, sock);
      }
      if (res->res_job.fileset) {
         sendit(sock, "  --> ");
         dump_resource(-R_FILESET, (RES *)res->res_job.fileset, sendit, sock);
      }
      if (res->res_job.schedule) {
         sendit(sock, "  --> ");
         dump_resource(-R_SCHEDULE, (RES *)res->res_job.schedule, sendit, sock);
      }
      if (res->res_job.RestoreWhere) {
         sendit(sock, "  --> Where=%s\n", NPRT(res->res_job.RestoreWhere));
      }
      if (res->res_job.RestoreBootstrap) {
         sendit(sock, "  --> Bootstrap=%s\n", NPRT(res->res_job.RestoreBootstrap));
      }
      if (res->res_job.RunBeforeJob) {
         sendit(sock, "  --> RunBefore=%s\n", NPRT(res->res_job.RunBeforeJob));
      }
      if (res->res_job.RunAfterJob) {
         sendit(sock, "  --> RunAfter=%s\n", NPRT(res->res_job.RunAfterJob));
      }
      if (res->res_job.RunAfterFailedJob) {
         sendit(sock, "  --> RunAfterFailed=%s\n", NPRT(res->res_job.RunAfterFailedJob));
      }
      if (res->res_job.WriteBootstrap) {
         sendit(sock, "  --> WriteBootstrap=%s\n", NPRT(res->res_job.WriteBootstrap));
      }
      if (res->res_job.storage[0]) {
         sendit(sock, "  --> ");
         /* ***FIXME*** */
//         dump_resource(-R_STORAGE, (RES *)res->res_job.storage, sendit, sock);
      }
      if (res->res_job.pool) {
         sendit(sock, "  --> ");
         dump_resource(-R_POOL, (RES *)res->res_job.pool, sendit, sock);
      }
      if (res->res_job.full_pool) {
         sendit(sock, "  --> ");
         dump_resource(-R_POOL, (RES *)res->res_job.full_pool, sendit, sock);
      }
      if (res->res_job.inc_pool) {
         sendit(sock, "  --> ");
         dump_resource(-R_POOL, (RES *)res->res_job.inc_pool, sendit, sock);
      }
      if (res->res_job.dif_pool) {
         sendit(sock, "  --> ");
         dump_resource(-R_POOL, (RES *)res->res_job.dif_pool, sendit, sock);
      }
      if (res->res_job.verify_job) {
         sendit(sock, "  --> ");
         dump_resource(-type, (RES *)res->res_job.verify_job, sendit, sock);
      }
      break;
      if (res->res_job.messages) {
         sendit(sock, "  --> ");
         dump_resource(-R_MSGS, (RES *)res->res_job.messages, sendit, sock);
      }
      break;
   case R_FILESET:
   {
      int i, j, k;
      sendit(sock, "FileSet: name=%s\n", res->res_fs.hdr.name);
      for (i=0; i<res->res_fs.num_includes; i++) {
         INCEXE *incexe = res->res_fs.include_items[i];
         for (j=0; j<incexe->num_opts; j++) {
            FOPTS *fo = incexe->opts_list[j];
            sendit(sock, "      O %s\n", fo->opts);
            for (k=0; k<fo->regex.size(); k++) {
               sendit(sock, "      R %s\n", fo->regex.get(k));
            }
            for (k=0; k<fo->regexdir.size(); k++) {
               sendit(sock, "      RD %s\n", fo->regexdir.get(k));
            }
            for (k=0; k<fo->regexfile.size(); k++) {
               sendit(sock, "      RF %s\n", fo->regexfile.get(k));
            }
            for (k=0; k<fo->wild.size(); k++) {
               sendit(sock, "      W %s\n", fo->wild.get(k));
            }
            for (k=0; k<fo->wilddir.size(); k++) {
               sendit(sock, "      WD %s\n", fo->wilddir.get(k));
            }
            for (k=0; k<fo->wildfile.size(); k++) {
               sendit(sock, "      WF %s\n", fo->wildfile.get(k));
            }
            for (k=0; k<fo->base.size(); k++) {
               sendit(sock, "      B %s\n", fo->base.get(k));
            }
            for (k=0; k<fo->fstype.size(); k++) {
               sendit(sock, "      X %s\n", fo->fstype.get(k));
            }
            if (fo->reader) {
               sendit(sock, "      D %s\n", fo->reader);
            }
            if (fo->writer) {
               sendit(sock, "      T %s\n", fo->writer);
            }
            sendit(sock, "      N\n");
         }
         for (j=0; j<incexe->name_list.size(); j++) {
            sendit(sock, "      I %s\n", incexe->name_list.get(j));
         }
         if (incexe->name_list.size()) {
            sendit(sock, "      N\n");
         }
      }

      for (i=0; i<res->res_fs.num_excludes; i++) {
         INCEXE *incexe = res->res_fs.exclude_items[i];
         for (j=0; j<incexe->name_list.size(); j++) {
            sendit(sock, "      E %s\n", incexe->name_list.get(j));
         }
         if (incexe->name_list.size()) {
            sendit(sock, "      N\n");
         }
      }
      break;
   }
   case R_SCHEDULE:
      if (res->res_sch.run) {
         int i;
         RUN *run = res->res_sch.run;
         char buf[1000], num[30];
         sendit(sock, "Schedule: name=%s\n", res->res_sch.hdr.name);
         if (!run) {
            break;
         }
next_run:
         sendit(sock, "  --> Run Level=%s\n", level_to_str(run->level));
         bstrncpy(buf, "      hour=", sizeof(buf));
         for (i=0; i<24; i++) {
            if (bit_is_set(i, run->hour)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         bstrncpy(buf, "      mday=", sizeof(buf));
         for (i=0; i<31; i++) {
            if (bit_is_set(i, run->mday)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         bstrncpy(buf, "      month=", sizeof(buf));
         for (i=0; i<12; i++) {
            if (bit_is_set(i, run->month)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         bstrncpy(buf, "      wday=", sizeof(buf));
         for (i=0; i<7; i++) {
            if (bit_is_set(i, run->wday)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         bstrncpy(buf, "      wom=", sizeof(buf));
         for (i=0; i<5; i++) {
            if (bit_is_set(i, run->wom)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         bstrncpy(buf, "      woy=", sizeof(buf));
         for (i=0; i<54; i++) {
            if (bit_is_set(i, run->woy)) {
               bsnprintf(num, sizeof(num), "%d ", i);
               bstrncat(buf, num, sizeof(buf));
            }
         }
         bstrncat(buf, "\n", sizeof(buf));
         sendit(sock, buf);
         sendit(sock, "      mins=%d\n", run->minute);
         if (run->pool) {
            sendit(sock, "     --> ");
            dump_resource(-R_POOL, (RES *)run->pool, sendit, sock);
         }
         if (run->storage) {
            sendit(sock, "     --> ");
            dump_resource(-R_STORAGE, (RES *)run->storage, sendit, sock);
         }
         if (run->msgs) {
            sendit(sock, "     --> ");
            dump_resource(-R_MSGS, (RES *)run->msgs, sendit, sock);
         }
         /* If another Run record is chained in, go print it */
         if (run->next) {
            run = run->next;
            goto next_run;
         }
      } else {
         sendit(sock, "Schedule: name=%s\n", res->res_sch.hdr.name);
      }
      break;
   case R_POOL:
      sendit(sock, "Pool: name=%s PoolType=%s\n", res->res_pool.hdr.name,
              res->res_pool.pool_type);
      sendit(sock, "      use_cat=%d use_once=%d acpt_any=%d cat_files=%d\n",
              res->res_pool.use_catalog, res->res_pool.use_volume_once,
              res->res_pool.accept_any_volume, res->res_pool.catalog_files);
      sendit(sock, "      max_vols=%d auto_prune=%d VolRetention=%s\n",
              res->res_pool.max_volumes, res->res_pool.AutoPrune,
              edit_utime(res->res_pool.VolRetention, ed1, sizeof(ed1)));
      sendit(sock, "      VolUse=%s recycle=%d LabelFormat=%s\n", 
              edit_utime(res->res_pool.VolUseDuration, ed1, sizeof(ed1)),
              res->res_pool.Recycle,
              NPRT(res->res_pool.label_format));
      sendit(sock, "      CleaningPrefix=%s\n",
              NPRT(res->res_pool.cleaning_prefix));
      sendit(sock, "      recyleOldest=%d MaxVolJobs=%d MaxVolFiles=%d\n",
              res->res_pool.purge_oldest_volume, 
              res->res_pool.MaxVolJobs, res->res_pool.MaxVolFiles);
      break;
   case R_MSGS:
      sendit(sock, "Messages: name=%s\n", res->res_msgs.hdr.name);
      if (res->res_msgs.mail_cmd) 
         sendit(sock, "      mailcmd=%s\n", res->res_msgs.mail_cmd);
      if (res->res_msgs.operator_cmd) 
         sendit(sock, "      opcmd=%s\n", res->res_msgs.operator_cmd);
      break;
   default:
      sendit(sock, "Unknown resource type %d in dump_resource.\n", type);
      break;
   }
   if (recurse && res->res_dir.hdr.next) {
      dump_resource(type, res->res_dir.hdr.next, sendit, sock);
   }
}

/*
 * Free all the members of an INCEXE structure
 */
static void free_incexe(INCEXE *incexe)
{
   incexe->name_list.destroy();
   for (int i=0; i<incexe->num_opts; i++) {
      FOPTS *fopt = incexe->opts_list[i];
      fopt->regex.destroy();
      fopt->regexdir.destroy();
      fopt->regexfile.destroy();
      fopt->wild.destroy();
      fopt->wilddir.destroy();
      fopt->wildfile.destroy();
      fopt->base.destroy();
      fopt->fstype.destroy();
      if (fopt->reader) {
         free(fopt->reader);
      }
      if (fopt->writer) {
         free(fopt->writer);
      }
      free(fopt);
   }
   if (incexe->opts_list) {
      free(incexe->opts_list);
   }
   free(incexe);
}

/* 
 * Free memory of resource -- called when daemon terminates.
 * NB, we don't need to worry about freeing any references
 * to other resources as they will be freed when that 
 * resource chain is traversed.  Mainly we worry about freeing
 * allocated strings (names).
 */
void free_resource(RES *sres, int type)
{
   int num;
   RES *nres;                         /* next resource if linked */
   URES *res = (URES *)sres;

   if (res == NULL)
      return;

   /* common stuff -- free the resource name and description */
   nres = (RES *)res->res_dir.hdr.next;
   if (res->res_dir.hdr.name) {
      free(res->res_dir.hdr.name);
   }
   if (res->res_dir.hdr.desc) {
      free(res->res_dir.hdr.desc);
   }

   switch (type) {
   case R_DIRECTOR:
      if (res->res_dir.working_directory) {
         free(res->res_dir.working_directory);
      }
      if (res->res_dir.pid_directory) {
         free(res->res_dir.pid_directory);
      }
      if (res->res_dir.subsys_directory) {
         free(res->res_dir.subsys_directory);
      }
      if (res->res_dir.password) {
         free(res->res_dir.password);
      }
      if (res->res_dir.query_file) {
         free(res->res_dir.query_file);
      }
      if (res->res_dir.DIRaddrs) {
         free_addresses(res->res_dir.DIRaddrs);
      }
      break;
   case R_COUNTER:
       break;
   case R_CONSOLE:
      if (res->res_con.password) {
         free(res->res_con.password);
      }
      for (int i=0; i<Num_ACL; i++) {
         if (res->res_con.ACL_lists[i]) {
            delete res->res_con.ACL_lists[i];
            res->res_con.ACL_lists[i] = NULL;
         }
      }
      break;
   case R_CLIENT:
      if (res->res_client.address) {
         free(res->res_client.address);
      }
      if (res->res_client.password) {
         free(res->res_client.password);
      }
      break;
   case R_STORAGE:
      if (res->res_store.address) {
         free(res->res_store.address);
      }
      if (res->res_store.password) {
         free(res->res_store.password);
      }
      if (res->res_store.media_type) {
         free(res->res_store.media_type);
      }
      if (res->res_store.dev_name) {
         free(res->res_store.dev_name);
      }
      break;
   case R_CATALOG:
      if (res->res_cat.db_address) {
         free(res->res_cat.db_address);
      }
      if (res->res_cat.db_socket) {
         free(res->res_cat.db_socket);
      }
      if (res->res_cat.db_user) {
         free(res->res_cat.db_user);
      }
      if (res->res_cat.db_name) {
         free(res->res_cat.db_name);
      }
      if (res->res_cat.db_password) {
         free(res->res_cat.db_password);
      }
      break;
   case R_FILESET:
      if ((num=res->res_fs.num_includes)) {
         while (--num >= 0) {   
            free_incexe(res->res_fs.include_items[num]);
         }
         free(res->res_fs.include_items);
      }
      res->res_fs.num_includes = 0;
      if ((num=res->res_fs.num_excludes)) {
         while (--num >= 0) {   
            free_incexe(res->res_fs.exclude_items[num]);
         }
         free(res->res_fs.exclude_items);
      }
      res->res_fs.num_excludes = 0;
      break;
   case R_POOL:
      if (res->res_pool.pool_type) {
         free(res->res_pool.pool_type);
      }
      if (res->res_pool.label_format) {
         free(res->res_pool.label_format);
      }
      if (res->res_pool.cleaning_prefix) {
         free(res->res_pool.cleaning_prefix);
      }
      break;
   case R_SCHEDULE:
      if (res->res_sch.run) {
         RUN *nrun, *next;
         nrun = res->res_sch.run;
         while (nrun) {
            next = nrun->next;
            free(nrun);
            nrun = next;
         }
      }
      break;
   case R_JOB:
   case R_JOBDEFS:
      if (res->res_job.RestoreWhere) {
         free(res->res_job.RestoreWhere);
      }
      if (res->res_job.RestoreBootstrap) {
         free(res->res_job.RestoreBootstrap);
      }
      if (res->res_job.WriteBootstrap) {
         free(res->res_job.WriteBootstrap);
      }
      if (res->res_job.RunBeforeJob) {
         free(res->res_job.RunBeforeJob);
      }
      if (res->res_job.RunAfterJob) {
         free(res->res_job.RunAfterJob);
      }
      if (res->res_job.RunAfterFailedJob) {
         free(res->res_job.RunAfterFailedJob);
      }
      if (res->res_job.ClientRunBeforeJob) {
         free(res->res_job.ClientRunBeforeJob);
      }
      if (res->res_job.ClientRunAfterJob) {
         free(res->res_job.ClientRunAfterJob);
      }
      for (int i=0; i < MAX_STORE; i++) {
         if (res->res_job.storage[i]) {
            delete (alist *)res->res_job.storage[i];
         }
      }
      break;
   case R_MSGS:
      if (res->res_msgs.mail_cmd) {
         free(res->res_msgs.mail_cmd);
      }
      if (res->res_msgs.operator_cmd) {
         free(res->res_msgs.operator_cmd);
      }
      free_msgs_res((MSGS *)res);  /* free message resource */
      res = NULL;
      break;
   default:
      printf("Unknown resource type %d in free_resource.\n", type);
   }
   /* Common stuff again -- free the resource, recurse to next one */
   if (res) {
      free(res);
   }
   if (nres) {
      free_resource(nres, type);
   }
}

/*
 * Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * pointers because they may not have been defined until 
 * later in pass 1.
 */
void save_resource(int type, RES_ITEM *items, int pass)
{
   URES *res;
   int rindex = type - r_first;
   int i, size;
   int error = 0;
   
   /* Check Job requirements after applying JobDefs */
   if (type != R_JOB && type != R_JOBDEFS) {
      /* 
       * Ensure that all required items are present
       */
      for (i=0; items[i].name; i++) {
         if (items[i].flags & ITEM_REQUIRED) {
           if (!bit_is_set(i, res_all.res_dir.hdr.item_present)) {  
               Emsg2(M_ERROR_TERM, 0, "%s item is required in %s resource, but not found.\n",
                    items[i].name, resources[rindex]);
            }
         }
         /* If this triggers, take a look at lib/parse_conf.h */
         if (i >= MAX_RES_ITEMS) {
            Emsg1(M_ERROR_TERM, 0, "Too many items in %s resource\n", resources[rindex]);
         }
      }
   } else if (type == R_JOB) {
      /*
       * Ensure that the name item is present
       */
      if (items[0].flags & ITEM_REQUIRED) {
         if (!bit_is_set(0, res_all.res_dir.hdr.item_present)) {
             Emsg2(M_ERROR_TERM, 0, "%s item is required in %s resource, but not found.\n",
                   items[0].name, resources[rindex]);
         }
      }
   }

   /*
    * During pass 2 in each "store" routine, we looked up pointers 
    * to all the resources referrenced in the current resource, now we
    * must copy their addresses from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      switch (type) {
      /* Resources not containing a resource */
      case R_CONSOLE:
      case R_CATALOG:
      case R_STORAGE:
      case R_POOL:
      case R_MSGS:
      case R_FILESET:
         break;

      /* Resources containing another resource */
      case R_DIRECTOR:
         if ((res = (URES *)GetResWithName(R_DIRECTOR, res_all.res_dir.hdr.name)) == NULL) {
            Emsg1(M_ERROR_TERM, 0, "Cannot find Director resource %s\n", res_all.res_dir.hdr.name);
         }
         res->res_dir.messages = res_all.res_dir.messages;
         break;
      case R_JOB:
      case R_JOBDEFS:
         if ((res = (URES *)GetResWithName(type, res_all.res_dir.hdr.name)) == NULL) {
            Emsg1(M_ERROR_TERM, 0, "Cannot find Job resource %s\n", 
                  res_all.res_dir.hdr.name);
         }
         res->res_job.messages   = res_all.res_job.messages;
         res->res_job.schedule   = res_all.res_job.schedule;
         res->res_job.client     = res_all.res_job.client;
         res->res_job.fileset    = res_all.res_job.fileset;
         for (int i=0; i < MAX_STORE; i++) {
            res->res_job.storage[i] = res_all.res_job.storage[i];
         }
         res->res_job.pool       = res_all.res_job.pool;
         res->res_job.full_pool  = res_all.res_job.full_pool;
         res->res_job.inc_pool   = res_all.res_job.inc_pool;
         res->res_job.dif_pool   = res_all.res_job.dif_pool;
         res->res_job.verify_job = res_all.res_job.verify_job;
         res->res_job.jobdefs    = res_all.res_job.jobdefs;
         break;
      case R_COUNTER:
         if ((res = (URES *)GetResWithName(R_COUNTER, res_all.res_counter.hdr.name)) == NULL) {
            Emsg1(M_ERROR_TERM, 0, "Cannot find Counter resource %s\n", res_all.res_counter.hdr.name);
         }
         res->res_counter.Catalog = res_all.res_counter.Catalog;
         res->res_counter.WrapCounter = res_all.res_counter.WrapCounter;
         break;

      case R_CLIENT:
         if ((res = (URES *)GetResWithName(R_CLIENT, res_all.res_client.hdr.name)) == NULL) {
            Emsg1(M_ERROR_TERM, 0, "Cannot find Client resource %s\n", res_all.res_client.hdr.name);
         }
         res->res_client.catalog = res_all.res_client.catalog;
         break;
      case R_SCHEDULE:
         /*
          * Schedule is a bit different in that it contains a RUN record
          * chain which isn't a "named" resource. This chain was linked
          * in by run_conf.c during pass 2, so here we jam the pointer 
          * into the Schedule resource.                         
          */
         if ((res = (URES *)GetResWithName(R_SCHEDULE, res_all.res_client.hdr.name)) == NULL) {
            Emsg1(M_ERROR_TERM, 0, "Cannot find Schedule resource %s\n", res_all.res_client.hdr.name);
         }
         res->res_sch.run = res_all.res_sch.run;
         break;
      default:
         Emsg1(M_ERROR, 0, "Unknown resource type %d in save_resource.\n", type);
         error = 1;
         break;
      }
      /* Note, the resource name was already saved during pass 1,
       * so here, we can just release it.
       */
      if (res_all.res_dir.hdr.name) {
         free(res_all.res_dir.hdr.name);
         res_all.res_dir.hdr.name = NULL;
      }
      if (res_all.res_dir.hdr.desc) {
         free(res_all.res_dir.hdr.desc);
         res_all.res_dir.hdr.desc = NULL;
      }
      return;
   }

   /*
    * The following code is only executed during pass 1   
    */
   switch (type) {
   case R_DIRECTOR:
      size = sizeof(DIRRES);
      break;
   case R_CONSOLE:
      size = sizeof(CONRES);
      break;
   case R_CLIENT:
      size =sizeof(CLIENT);
      break;
   case R_STORAGE:
      size = sizeof(STORE); 
      break;
   case R_CATALOG:
      size = sizeof(CAT);
      break;
   case R_JOB:
   case R_JOBDEFS:
      size = sizeof(JOB);
      break;
   case R_FILESET:
      size = sizeof(FILESET);
      break;
   case R_SCHEDULE:
      size = sizeof(SCHED);
      break;
   case R_POOL:
      size = sizeof(POOL);
      break;
   case R_MSGS:
      size = sizeof(MSGS);
      break;
   case R_COUNTER:
      size = sizeof(COUNTER);
      break;
   default:
      printf("Unknown resource type %d in save_resrouce.\n", type);
      error = 1;
      size = 1;
      break;
   }
   /* Common */
   if (!error) {
      res = (URES *)malloc(size);
      memcpy(res, &res_all, size);
      if (!res_head[rindex]) {
         res_head[rindex] = (RES *)res; /* store first entry */
         Dmsg3(900, "Inserting first %s res: %s index=%d\n", res_to_str(type),
               res->res_dir.hdr.name, rindex);
      } else {
         RES *next;
         /* Add new res to end of chain */
         for (next=res_head[rindex]; next->next; next=next->next) {
            if (strcmp(next->name, res->res_dir.hdr.name) == 0) {
               Emsg2(M_ERROR_TERM, 0,
                  _("Attempt to define second %s resource named \"%s\" is not permitted.\n"),
                  resources[rindex].name, res->res_dir.hdr.name);
            }
         }
         next->next = (RES *)res;
         Dmsg4(900, "Inserting %s res: %s index=%d pass=%d\n", res_to_str(type),
               res->res_dir.hdr.name, rindex, pass);
      }
   }
}

/* 
 * Store JobType (backup, verify, restore)
 *
 */
void store_jobtype(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int token, i;   

   token = lex_get_token(lc, T_NAME);
   /* Store the type both pass 1 and pass 2 */
   for (i=0; jobtypes[i].type_name; i++) {
      if (strcasecmp(lc->str, jobtypes[i].type_name) == 0) {
         *(int *)(item->value) = jobtypes[i].job_type;
         i = 0;
         break;
      }
   }
   if (i != 0) {
      scan_err1(lc, "Expected a Job Type keyword, got: %s", lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/* 
 * Store Job Level (Full, Incremental, ...)
 *
 */
void store_level(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int token, i;

   token = lex_get_token(lc, T_NAME);
   /* Store the level pass 2 so that type is defined */
   for (i=0; joblevels[i].level_name; i++) {
      if (strcasecmp(lc->str, joblevels[i].level_name) == 0) {
         *(int *)(item->value) = joblevels[i].level;
         i = 0;
         break;
      }
   }
   if (i != 0) {
      scan_err1(lc, "Expected a Job Level keyword, got: %s", lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

void store_replace(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int token, i;
   token = lex_get_token(lc, T_NAME);
   /* Scan Replacement options */
   for (i=0; ReplaceOptions[i].name; i++) {
      if (strcasecmp(lc->str, ReplaceOptions[i].name) == 0) {
         *(int *)(item->value) = ReplaceOptions[i].token;
         i = 0;
         break;
      }
   }
   if (i != 0) {
      scan_err1(lc, "Expected a Restore replacement option, got: %s", lc->str);
   }
   scan_to_eol(lc);
   set_bit(index, res_all.hdr.item_present);
}

/* 
 * Store ACL (access control list)
 *
 */
void store_acl(LEX *lc, RES_ITEM *item, int index, int pass)
{
   int token;

   for (;;) {
      token = lex_get_token(lc, T_NAME);
      if (pass == 1) {
         if (((alist **)item->value)[item->code] == NULL) {   
            ((alist **)item->value)[item->code] = New(alist(10, owned_by_alist)); 
            Dmsg1(900, "Defined new ACL alist at %d\n", item->code);
         }
         ((alist **)item->value)[item->code]->append(bstrdup(lc->str));
         Dmsg2(900, "Appended to %d %s\n", item->code, lc->str);
      }
      token = lex_get_token(lc, T_ALL);
      if (token == T_COMMA) {
         continue;                    /* get another ACL */
      }
      break;
   }
   set_bit(index, res_all.hdr.item_present);
}
