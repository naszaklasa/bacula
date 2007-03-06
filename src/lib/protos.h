/*
 * Prototypes for lib directory of Bacula
 *
 *   Version $Id: protos.h,v 1.109.2.6 2006/05/02 14:48:16 kerns Exp $
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

class JCR;

/* attr.c */
ATTR     *new_attr();
void      free_attr(ATTR *attr);
int       unpack_attributes_record(JCR *jcr, int32_t stream, char *rec, ATTR *attr);
void      build_attr_output_fnames(JCR *jcr, ATTR *attr);
void      print_ls_output(JCR *jcr, ATTR *attr);

/* base64.c */
void      base64_init            (void);
int       to_base64              (intmax_t value, char *where);
int       from_base64            (intmax_t *value, char *where);
int       bin_to_base64          (char *buf, char *bin, int len);

/* bsys.c */
char     *bstrncpy               (char *dest, const char *src, int maxlen);
char     *bstrncpy               (char *dest, POOL_MEM &src, int maxlen);
char     *bstrncat               (char *dest, const char *src, int maxlen);
char     *bstrncat               (char *dest, POOL_MEM &src, int maxlen);
int       cstrlen                (const char *str);
void     *b_malloc               (const char *file, int line, size_t size);
#ifndef DEBUG
void     *bmalloc                (size_t size);
#endif
void     *brealloc               (void *buf, size_t size);
void     *bcalloc                (size_t size1, size_t size2);
int       bsnprintf              (char *str, int32_t size, const char *format, ...);
int       bvsnprintf             (char *str, int32_t size, const char *format, va_list ap);
int       pool_sprintf           (char *pool_buf, const char *fmt, ...);
void      create_pid_file        (char *dir, const char *progname, int port);
int       delete_pid_file        (char *dir, const char *progname, int port);
void      drop                   (char *uid, char *gid);
int       bmicrosleep            (time_t sec, long usec);
char     *bfgets                 (char *s, int size, FILE *fd);
void      make_unique_filename   (POOLMEM **name, int Id, char *what);
#ifndef HAVE_STRTOLL
long long int strtoll            (const char *ptr, char **endptr, int base);
#endif
void      read_state_file(char *dir, const char *progname, int port);
int       bstrerror(int errnum, char *buf, size_t bufsiz);

/* bnet.c */
int32_t    bnet_recv             (BSOCK *bsock);
bool       bnet_send             (BSOCK *bsock);
bool       bnet_fsend            (BSOCK *bs, const char *fmt, ...);
bool       bnet_set_buffer_size  (BSOCK *bs, uint32_t size, int rw);
bool       bnet_sig              (BSOCK *bs, int sig);
int        bnet_tls_server       (TLS_CONTEXT *ctx, BSOCK *bsock,
                                  alist *verify_list);
int        bnet_tls_client       (TLS_CONTEXT *ctx, BSOCK *bsock);
BSOCK *    bnet_connect          (JCR *jcr, int retry_interval,
               int max_retry_time, const char *name, char *host, char *service,
               int port, int verbose);
void       bnet_close            (BSOCK *bsock);
BSOCK *    init_bsock            (JCR *jcr, int sockfd, const char *who, const char *ip,
                                  int port, struct sockaddr *client_addr);
BSOCK *    dup_bsock             (BSOCK *bsock);
void       term_bsock            (BSOCK *bsock);
const char *bnet_strerror         (BSOCK *bsock);
const char *bnet_sig_to_ascii     (BSOCK *bsock);
int        bnet_wait_data        (BSOCK *bsock, int sec);
int        bnet_wait_data_intr   (BSOCK *bsock, int sec);
int        bnet_despool_to_bsock (BSOCK *bsock, void update(ssize_t size), ssize_t size);
bool       is_bnet_stop          (BSOCK *bsock);
int        is_bnet_error         (BSOCK *bsock);
void       bnet_suppress_error_messages(BSOCK *bsock, bool flag);
dlist *bnet_host2ipaddrs(const char *host, int family, const char **errstr);
int        bnet_set_blocking     (BSOCK *sock);
int        bnet_set_nonblocking  (BSOCK *sock);
void       bnet_restore_blocking (BSOCK *sock, int flags);

/* bget_msg.c */
int      bget_msg(BSOCK *sock);

/* bpipe.c */
BPIPE *          open_bpipe(char *prog, int wait, const char *mode);
int              close_wpipe(BPIPE *bpipe);
int              close_bpipe(BPIPE *bpipe);

/* cram-md5.c */
int cram_md5_get_auth(BSOCK *bs, char *password, int *tls_remote_need);
int cram_md5_auth(BSOCK *bs, char *password, int tls_local_need);
void hmac_md5(uint8_t* text, int text_len, uint8_t*  key,
              int key_len, uint8_t *hmac);

/* crc32.c */

uint32_t bcrc32(uint8_t *buf, int len);

/* daemon.c */
void     daemon_start            ();

/* edit.c */
uint64_t         str_to_uint64(char *str);
int64_t          str_to_int64(char *str);
#define          str_to_int32(str) ((int32_t)str_to_int64(str))
char *           edit_uint64_with_suffix   (uint64_t val, char *buf);
char *           edit_uint64_with_commas   (uint64_t val, char *buf);
char *           add_commas              (char *val, char *buf);
char *           edit_uint64             (uint64_t val, char *buf);
char *           edit_int64              (int64_t val, char *buf);
bool             duration_to_utime       (char *str, utime_t *value);
bool             size_to_uint64(char *str, int str_len, uint64_t *rtn_value);
char             *edit_utime             (utime_t val, char *buf, int buf_len);
bool             is_a_number             (const char *num);
bool             is_an_integer           (const char *n);
bool             is_name_valid           (char *name, POOLMEM **msg);

/* jcr.c (most definitions are in src/jcr.h) */
void init_last_jobs_list();
void term_last_jobs_list();
void lock_last_jobs_list();
void unlock_last_jobs_list();
bool read_last_jobs_list(int fd, uint64_t addr);
uint64_t write_last_jobs_list(int fd, uint64_t addr);
void write_state_file(char *dir, const char *progname, int port);
void job_end_push(JCR *jcr, void job_end_cb(JCR *jcr,void *), void *ctx);
void lock_jobs();
void unlock_jobs();
JCR *jcr_walk_start();
JCR *jcr_walk_next(JCR *prev_jcr);
void jcr_walk_end(JCR *jcr);


/* lex.c */
LEX *     lex_close_file         (LEX *lf);
LEX *     lex_open_file          (LEX *lf, const char *fname, LEX_ERROR_HANDLER *scan_error);
int       lex_get_char           (LEX *lf);
void      lex_unget_char         (LEX *lf);
const char *  lex_tok_to_str     (int token);
int       lex_get_token          (LEX *lf, int expect);
void      lex_set_default_error_handler (LEX *lf);

/* message.c */
void       my_name_is            (int argc, char *argv[], const char *name);
void       init_msg              (JCR *jcr, MSGS *msg);
void       term_msg              (void);
void       close_msg             (JCR *jcr);
void       add_msg_dest          (MSGS *msg, int dest, int type, char *where, char *dest_code);
void       rem_msg_dest          (MSGS *msg, int dest, int type, char *where);
void       Jmsg                  (JCR *jcr, int type, time_t mtime, const char *fmt, ...);
void       dispatch_message      (JCR *jcr, int type, time_t mtime, char *buf);
void       init_console_msg      (const char *wd);
void       free_msgs_res         (MSGS *msgs);
void       dequeue_messages      (JCR *jcr);
void       set_trace             (int trace_flag);

/* bnet_server.c */
void       bnet_thread_server(dlist *addr, int max_clients, workq_t *client_wq,
                   void *handle_client_request(void *bsock));
void       bnet_stop_thread_server(pthread_t tid);
void             bnet_server             (int port, void handle_client_request(BSOCK *bsock));
int              net_connect             (int port);
BSOCK *          bnet_bind               (int port);
BSOCK *          bnet_accept             (BSOCK *bsock, char *who);

/* idcache.c */
char *getuser(uid_t uid);
void free_getuser_cache();
char *getgroup (gid_t gid);
void free_getgroup_cache();

/* python.c */
typedef int (EVENT_HANDLER)(JCR *jcr, const char *event);
void init_python_interpreter(const char *progname, const char *scripts,
                             const char *module);
void term_python_interpreter();
//extern EVENT_HANDLER *generate_daemon_event;
int generate_daemon_event(JCR *jcr, const char *event);
void lock_python();
void unlock_python();

/* signal.c */
void             init_signals             (void terminate(int sig));
void             init_stack_dump          (void);

/* scan.c */
void             strip_leading_space     (char *str);
void             strip_trailing_junk     (char *str);
void             strip_trailing_newline  (char *str);
void             strip_trailing_slashes  (char *dir);
bool             skip_spaces             (char **msg);
bool             skip_nonspaces          (char **msg);
int              fstrsch                 (const char *a, const char *b);
char            *next_arg(char **s);
int              parse_args(POOLMEM *cmd, POOLMEM **args, int *argc,
                        char **argk, char **argv, int max_args);
void            split_path_and_filename(const char *fname, POOLMEM **path,
                        int *pnl, POOLMEM **file, int *fnl);
int             bsscanf(const char *buf, const char *fmt, ...);


/* tls.c */
int              init_tls                (void);
int              cleanup_tls             (void);

TLS_CONTEXT      *new_tls_context        (const char *ca_certfile,
                                          const char *ca_certdir,
                                          const char *certfile,
                                          const char *keyfile,
                                          TLS_PEM_PASSWD_CB *pem_callback,
                                          const void *pem_userdata,
                                          const char *dhfile,
                                          bool verify_peer);
void             free_tls_context        (TLS_CONTEXT *ctx);
#ifdef HAVE_TLS
bool             tls_postconnect_verify_host  (TLS_CONNECTION *tls,
                                               const char *host);
bool             tls_postconnect_verify_cn    (TLS_CONNECTION *tls,
                                               alist *verify_list);
TLS_CONNECTION   *new_tls_connection     (TLS_CONTEXT *ctx, int fd);
void             free_tls_connection     (TLS_CONNECTION *tls);
bool             tls_bsock_connect       (BSOCK *bsock);
bool             tls_bsock_accept        (BSOCK *bsock);
void             tls_bsock_shutdown      (BSOCK *bsock);
int              tls_bsock_writen        (BSOCK *bsock, char *ptr, int32_t nbytes);
int              tls_bsock_readn         (BSOCK *bsock, char *ptr, int32_t nbytes);
#endif /* HAVE_TLS */


/* util.c */
int              is_buf_zero             (char *buf, int len);
void             lcase                   (char *str);
void             bash_spaces             (char *str);
void             bash_spaces             (POOL_MEM &pm);
void             unbash_spaces           (char *str);
void             unbash_spaces           (POOL_MEM &pm);
char *           encode_time             (time_t time, char *buf);
char *           encode_mode             (mode_t mode, char *buf);
int              do_shell_expansion      (char *name, int name_len);
void             jobstatus_to_ascii      (int JobStatus, char *msg, int maxlen);
int              run_program             (char *prog, int wait, POOLMEM *results);
int              run_program_full_output (char *prog, int wait, POOLMEM *results);
const char *     job_type_to_str         (int type);
const char *     job_status_to_str       (int stat);
const char *     job_level_to_str        (int level);
void             make_session_key        (char *key, char *seed, int mode);
POOLMEM         *edit_job_codes(JCR *jcr, char *omsg, char *imsg, const char *to);
void             set_working_directory(char *wd);


/* watchdog.c */
int start_watchdog(void);
int stop_watchdog(void);
watchdog_t *new_watchdog(void);
bool register_watchdog(watchdog_t *wd);
bool unregister_watchdog(watchdog_t *wd);

/* timers.c */
btimer_t *start_child_timer(pid_t pid, uint32_t wait);
void stop_child_timer(btimer_t *wid);
btimer_t *start_thread_timer(pthread_t tid, uint32_t wait);
void stop_thread_timer(btimer_t *wid);
btimer_t *start_bsock_timer(BSOCK *bs, uint32_t wait);
void stop_bsock_timer(btimer_t *wid);
