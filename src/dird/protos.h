/*
 * Director external function prototypes
 *
 *   Version $Id: protos.h,v 1.75.2.5 2006/03/04 11:10:17 kerns Exp $
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

/* admin.c */
extern bool do_admin_init(JCR *jcr);
extern bool do_admin(JCR *jcr);
extern void admin_cleanup(JCR *jcr, int TermCode);


/* authenticate.c */
extern bool authenticate_storage_daemon(JCR *jcr, STORE *store);
extern int authenticate_file_daemon(JCR *jcr);
extern int authenticate_user_agent(UAContext *ua);

/* autoprune.c */
extern int do_autoprune(JCR *jcr);
extern int prune_volumes(JCR *jcr);

/* autorecycle.c */
extern int recycle_oldest_purged_volume(JCR *jcr, bool InChanger, MEDIA_DBR *mr);
extern int recycle_volume(JCR *jcr, MEDIA_DBR *mr);
extern int find_recycled_volume(JCR *jcr, bool InChanger, MEDIA_DBR *mr);

/* backup.c */
extern int wait_for_job_termination(JCR *jcr);
extern bool do_backup_init(JCR *jcr);
extern bool do_backup(JCR *jcr);
extern void backup_cleanup(JCR *jcr, int TermCode);
extern void update_bootstrap_file(JCR *jcr);

/* bsr.c */
RBSR *new_bsr();
void free_bsr(RBSR *bsr);
bool complete_bsr(UAContext *ua, RBSR *bsr);
uint32_t write_bsr_file(UAContext *ua, RESTORE_CTX &rx);
void add_findex(RBSR *bsr, uint32_t JobId, int32_t findex);
void add_findex_all(RBSR *bsr, uint32_t JobId);
RBSR_FINDEX *new_findex();
void make_unique_restore_filename(UAContext *ua, POOLMEM **fname);


/* catreq.c */
extern void catalog_request(JCR *jcr, BSOCK *bs);
extern void catalog_update(JCR *jcr, BSOCK *bs);

/* dird_conf.c */
extern const char *level_to_str(int level);

/* expand.c */
int variable_expansion(JCR *jcr, char *inp, POOLMEM **exp);


/* fd_cmds.c */
extern int connect_to_file_daemon(JCR *jcr, int retry_interval,
                                  int max_retry_time, int verbose);
extern bool send_include_list(JCR *jcr);
extern bool send_exclude_list(JCR *jcr);
extern bool send_bootstrap_file(JCR *jcr);
extern bool send_level_command(JCR *jcr);
extern int get_attributes_and_put_in_catalog(JCR *jcr);
extern int get_attributes_and_compare_to_catalog(JCR *jcr, JobId_t JobId);
extern int put_file_into_catalog(JCR *jcr, long file_index, char *fname,
                          char *link, char *attr, int stream);
extern void get_level_since_time(JCR *jcr, char *since, int since_len);
extern int send_run_before_and_after_commands(JCR *jcr);

/* getmsg.c */
enum e_prtmsg {
   DISPLAY_ERROR,
   NO_DISPLAY
};
extern bool response(JCR *jcr, BSOCK *fd, char *resp, const char *cmd, e_prtmsg prtmsg);

/* job.c */
extern void set_jcr_defaults(JCR *jcr, JOB *job);
extern void create_unique_job_name(JCR *jcr, const char *base_name);
extern void update_job_end_record(JCR *jcr);
extern bool get_or_create_client_record(JCR *jcr);
extern bool get_or_create_fileset_record(JCR *jcr);
extern JobId_t run_job(JCR *jcr);
extern bool cancel_job(UAContext *ua, JCR *jcr);
extern void init_jcr_job_record(JCR *jcr);
extern void copy_storage(JCR *new_jcr, JCR *old_jcr);
extern void set_storage(JCR *jcr, STORE *store);

/* mac.c */
extern bool do_mac(JCR *jcr);
extern bool do_mac_init(JCR *jcr);
extern void mac_cleanup(JCR *jcr, int TermCode);


/* mountreq.c */
extern void mount_request(JCR *jcr, BSOCK *bs, char *buf);

/* msgchan.c */
extern bool connect_to_storage_daemon(JCR *jcr, int retry_interval,
                              int max_retry_time, int verbose);
extern bool start_storage_daemon_job(JCR *jcr, alist *rstore, alist *wstore);
extern int start_storage_daemon_message_thread(JCR *jcr);
extern int bget_dirmsg(BSOCK *bs);
extern void wait_for_storage_daemon_termination(JCR *jcr);

/* next_vol.c */
int find_next_volume_for_append(JCR *jcr, MEDIA_DBR *mr, int index, bool create);
bool has_volume_expired(JCR *jcr, MEDIA_DBR *mr);
void check_if_volume_valid_or_recyclable(JCR *jcr, MEDIA_DBR *mr, const char **reason);

/* newvol.c */
bool newVolume(JCR *jcr, MEDIA_DBR *mr);

/* python.c */
int generate_job_event(JCR *jcr, const char *event);


/* restore.c */
extern bool do_restore(JCR *jcr);
extern bool do_restore_init(JCR *jcr);
extern void restore_cleanup(JCR *jcr, int TermCode);


/* ua_acl.c */
bool acl_access_ok(UAContext *ua, int acl, char *item);
bool acl_access_ok(UAContext *ua, int acl, char *item, int len);

/* ua_cmds.c */
int do_a_command(UAContext *ua, const char *cmd);
int do_a_dot_command(UAContext *ua, const char *cmd);
int qmessagescmd(UAContext *ua, const char *cmd);
bool open_db(UAContext *ua);
void close_db(UAContext *ua);
enum e_pool_op {
   POOL_OP_UPDATE,
   POOL_OP_CREATE
};
int create_pool(JCR *jcr, B_DB *db, POOL *pool, e_pool_op op);
void set_pool_dbr_defaults_in_media_dbr(MEDIA_DBR *mr, POOL_DBR *pr);
void set_pooldbr_from_poolres(POOL_DBR *pr, POOL *pool, e_pool_op op);

/* ua_input.c */
int get_cmd(UAContext *ua, const char *prompt);
bool get_pint(UAContext *ua, const char *prompt);
bool get_yesno(UAContext *ua, const char *prompt);
void parse_ua_args(UAContext *ua);

/* ua_label.c */
bool is_volume_name_legal(UAContext *ua, const char *name);
int get_num_drives_from_SD(UAContext *ua);

/* ua_output.c */
void prtit(void *ctx, const char *msg);
int complete_jcr_for_job(JCR *jcr, JOB *job, POOL *pool);
RUN *find_next_run(RUN *run, JOB *job, time_t &runtime, int ndays);

/* ua_restore.c */
int get_next_jobid_from_list(char **p, JobId_t *JobId);

/* ua_server.c */
void bsendmsg(void *sock, const char *fmt, ...);
UAContext *new_ua_context(JCR *jcr);
JCR *new_control_jcr(const char *base_name, int job_type);
void free_ua_context(UAContext *ua);

/* ua_select.c */
STORE   *select_storage_resource(UAContext *ua);
JOB     *select_job_resource(UAContext *ua);
JOB     *select_restore_job_resource(UAContext *ua);
CLIENT  *select_client_resource(UAContext *ua);
FILESET *select_fileset_resource(UAContext *ua);
int     select_pool_and_media_dbr(UAContext *ua, POOL_DBR *pr, MEDIA_DBR *mr);
int     select_media_dbr(UAContext *ua, MEDIA_DBR *mr);
bool    select_pool_dbr(UAContext *ua, POOL_DBR *pr);
int     select_client_dbr(UAContext *ua, CLIENT_DBR *cr);

void    start_prompt(UAContext *ua, const char *msg);
void    add_prompt(UAContext *ua, const char *prompt);
int     do_prompt(UAContext *ua, const char *automsg, const char *msg, char *prompt, int max_prompt);
CAT    *get_catalog_resource(UAContext *ua);
STORE  *get_storage_resource(UAContext *ua, bool use_default);
int     get_storage_drive(UAContext *ua, STORE *store);
int     get_media_type(UAContext *ua, char *MediaType, int max_media);
bool    get_pool_dbr(UAContext *ua, POOL_DBR *pr);
int     get_client_dbr(UAContext *ua, CLIENT_DBR *cr);
POOL   *get_pool_resource(UAContext *ua);
POOL   *select_pool_resource(UAContext *ua);
CLIENT *get_client_resource(UAContext *ua);
int     get_job_dbr(UAContext *ua, JOB_DBR *jr);

int find_arg_keyword(UAContext *ua, const char **list);
int find_arg(UAContext *ua, const char *keyword);
int find_arg_with_value(UAContext *ua, const char *keyword);
int do_keyword_prompt(UAContext *ua, const char *msg, const char **list);
int confirm_retention(UAContext *ua, utime_t *ret, const char *msg);
bool get_level_from_name(JCR *jcr, const char *level_name);

/* ua_tree.c */
bool user_select_files_from_tree(TREE_CTX *tree);
int insert_tree_handler(void *ctx, int num_fields, char **row);

/* ua_prune.c */
int prune_files(UAContext *ua, CLIENT *client);
int prune_jobs(UAContext *ua, CLIENT *client, int JobType);
int prune_volume(UAContext *ua, MEDIA_DBR *mr);

/* ua_purge.c */
bool mark_media_purged(UAContext *ua, MEDIA_DBR *mr);
void purge_files_from_volume(UAContext *ua, MEDIA_DBR *mr );
int purge_jobs_from_volume(UAContext *ua, MEDIA_DBR *mr);
void purge_files_from_job(UAContext *ua, JOB_DBR *jr);


/* ua_run.c */
extern int run_cmd(UAContext *ua, const char *cmd);

/* verify.c */
extern bool do_verify(JCR *jcr);
extern bool do_verify_init(JCR *jcr);
extern void verify_cleanup(JCR *jcr, int TermCode);
