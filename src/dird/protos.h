/*
 * Director external function prototypes
 *
 *   Version $Id: protos.h,v 1.56.2.1 2005/02/14 10:02:21 kerns Exp $
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

/* bsr.c */
RBSR *new_bsr();
void free_bsr(RBSR *bsr);
int complete_bsr(UAContext *ua, RBSR *bsr);
uint32_t write_bsr_file(UAContext *ua, RBSR *bsr);
void add_findex(RBSR *bsr, uint32_t JobId, int32_t findex);
RBSR_FINDEX *new_findex();


/* catreq.c */
extern void catalog_request(JCR *jcr, BSOCK *bs, char *buf);
extern void catalog_update(JCR *jcr, BSOCK *bs, char *buf);

/* dird_conf.c */
extern const char *level_to_str(int level);

/* expand.c */
int variable_expansion(JCR *jcr, char *inp, POOLMEM **exp);


/* fd_cmds.c */
extern int connect_to_file_daemon(JCR *jcr, int retry_interval,
				  int max_retry_time, int verbose);
extern int send_include_list(JCR *jcr);
extern int send_exclude_list(JCR *jcr);
extern int send_bootstrap_file(JCR *jcr);
extern int send_level_command(JCR *jcr);
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
extern int response(JCR *jcr, BSOCK *fd, char *resp, const char *cmd, e_prtmsg prtmsg);

/* job.c */
extern void set_jcr_defaults(JCR *jcr, JOB *job);
extern void create_unique_job_name(JCR *jcr, const char *base_name);
extern void update_job_end_record(JCR *jcr);
extern bool get_or_create_client_record(JCR *jcr);
extern bool get_or_create_fileset_record(JCR *jcr, FILESET_DBR *fsr);
extern JobId_t run_job(JCR *jcr);
extern int cancel_job(UAContext *ua, JCR *jcr);
extern void init_jcr_job_record(JCR *jcr);
extern void copy_storage(JCR *new_jcr, JCR *old_jcr);
extern void set_storage(JCR *jcr, STORE *store);

/* mountreq.c */
extern void mount_request(JCR *jcr, BSOCK *bs, char *buf);

/* msgchan.c */
extern bool connect_to_storage_daemon(JCR *jcr, int retry_interval,    
			      int max_retry_time, int verbose);
extern int start_storage_daemon_job(JCR *jcr);
extern int start_storage_daemon_message_thread(JCR *jcr);
extern int bget_dirmsg(BSOCK *bs);
extern void wait_for_storage_daemon_termination(JCR *jcr);

/* next_vol.c */
int find_next_volume_for_append(JCR *jcr, MEDIA_DBR *mr, bool create);
bool has_volume_expired(JCR *jcr, MEDIA_DBR *mr);
void check_if_volume_valid_or_recyclable(JCR *jcr, MEDIA_DBR *mr, const char **reason);

/* newvol.c */
bool newVolume(JCR *jcr, MEDIA_DBR *mr);

/* ua_acl.c */
bool acl_access_ok(UAContext *ua, int acl, char *item);
bool acl_access_ok(UAContext *ua, int acl, char *item, int len);

/* ua_cmds.c */
int do_a_command(UAContext *ua, const char *cmd);
int do_a_dot_command(UAContext *ua, const char *cmd);
int qmessagescmd(UAContext *ua, const char *cmd);
int open_db(UAContext *ua);
void close_db(UAContext *ua);
enum e_pool_op {
   POOL_OP_UPDATE, 
   POOL_OP_CREATE
};
int create_pool(JCR *jcr, B_DB *db, POOL *pool, e_pool_op op);
void set_pool_dbr_defaults_in_media_dbr(MEDIA_DBR *mr, POOL_DBR *pr);

/* ua_input.c */
int get_cmd(UAContext *ua, const char *prompt);
int get_pint(UAContext *ua, const char *prompt);
int get_yesno(UAContext *ua, const char *prompt);
void parse_ua_args(UAContext *ua);

/* ua_label.c */
bool is_volume_name_legal(UAContext *ua, const char *name);

/* ua_output.c */
void prtit(void *ctx, const char *msg);
int complete_jcr_for_job(JCR *jcr, JOB *job, POOL *pool);
RUN *find_next_run(RUN *run, JOB *job, time_t &runtime);

/* ua_server.c */
void bsendmsg(void *sock, const char *fmt, ...);
UAContext *new_ua_context(JCR *jcr);
JCR *new_control_jcr(const char *base_name, int job_type);
void free_ua_context(UAContext *ua);

/* ua_select.c */
STORE	*select_storage_resource(UAContext *ua);
JOB	*select_job_resource(UAContext *ua);
JOB	*select_restore_job_resource(UAContext *ua);
CLIENT	*select_client_resource(UAContext *ua);
FILESET *select_fileset_resource(UAContext *ua);
int	select_pool_and_media_dbr(UAContext *ua, POOL_DBR *pr, MEDIA_DBR *mr);
int	select_media_dbr(UAContext *ua, MEDIA_DBR *mr);
bool	select_pool_dbr(UAContext *ua, POOL_DBR *pr);
int	select_client_dbr(UAContext *ua, CLIENT_DBR *cr);

void	start_prompt(UAContext *ua, const char *msg);
void	add_prompt(UAContext *ua, const char *prompt);
int	do_prompt(UAContext *ua, const char *automsg, const char *msg, char *prompt, int max_prompt);
CAT    *get_catalog_resource(UAContext *ua);	       
STORE  *get_storage_resource(UAContext *ua, int use_default);
int	get_media_type(UAContext *ua, char *MediaType, int max_media);
bool	get_pool_dbr(UAContext *ua, POOL_DBR *pr);
int	get_client_dbr(UAContext *ua, CLIENT_DBR *cr);
POOL   *get_pool_resource(UAContext *ua);
POOL   *select_pool_resource(UAContext *ua);
CLIENT *get_client_resource(UAContext *ua);
int	get_job_dbr(UAContext *ua, JOB_DBR *jr);

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
int purge_jobs_from_volume(UAContext *ua, MEDIA_DBR *mr);

/* ua_run.c */
extern int run_cmd(UAContext *ua, const char *cmd);
