/*
 * Protypes for stored
 *
 *   Version $Id: protos.h,v 1.105.2.4 2006/03/24 16:35:23 kerns Exp $
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

/* From stored.c */
uint32_t new_VolSessionId();

/* From acquire.c */
DCR     *acquire_device_for_append(DCR *dcr);
DCR     *acquire_device_for_read(DCR *dcr);
bool     release_device(DCR *dcr);
DCR     *new_dcr(JCR *jcr, DEVICE *dev);
void     free_dcr(DCR *dcr);

/* From askdir.c */
enum get_vol_info_rw {
   GET_VOL_INFO_FOR_WRITE,
   GET_VOL_INFO_FOR_READ
};
bool    dir_get_volume_info(DCR *dcr, enum get_vol_info_rw);
bool    dir_find_next_appendable_volume(DCR *dcr);
bool    dir_update_volume_info(DCR *dcr, bool label);
bool    dir_ask_sysop_to_create_appendable_volume(DCR *dcr);
bool    dir_ask_sysop_to_mount_volume(DCR *dcr);
bool    dir_update_file_attributes(DCR *dcr, DEV_RECORD *rec);
bool    dir_send_job_status(JCR *jcr);
bool    dir_create_jobmedia_record(DCR *dcr);
bool    dir_update_device(JCR *jcr, DEVICE *dev);
bool    dir_update_changer(JCR *jcr, AUTOCHANGER *changer);

/* authenticate.c */
int     authenticate_director(JCR *jcr);
int     authenticate_filed(JCR *jcr);

/* From autochanger.c */
bool     init_autochangers();
int      autoload_device(DCR *dcr, int writing, BSOCK *dir);
bool     autochanger_cmd(DCR *dcr, BSOCK *dir, const char *cmd);
bool     unload_autochanger(DCR *dcr, int loaded);
char    *edit_device_codes(DCR *dcr, char *omsg, const char *imsg, const char *cmd);
int      get_autochanger_loaded_slot(DCR *dcr);

/* From block.c */
void    dump_block(DEV_BLOCK *b, const char *msg);
DEV_BLOCK *new_block(DEVICE *dev);
DEV_BLOCK *dup_block(DEV_BLOCK *eblock);
void    init_block_write(DEV_BLOCK *block);
void    empty_block(DEV_BLOCK *block);
void    free_block(DEV_BLOCK *block);
bool    write_block_to_device(DCR *dcr);
bool    write_block_to_dev(DCR *dcr);
void    print_block_read_errors(JCR *jcr, DEV_BLOCK *block);
void    ser_block_header(DEV_BLOCK *block);

#define CHECK_BLOCK_NUMBERS    true
#define NO_BLOCK_NUMBER_CHECK  false
bool    read_block_from_device(DCR *dcr, bool check_block_numbers);
bool    read_block_from_dev(DCR *dcr, bool check_block_numbers);

/* From butil.c -- utilities for SD tool programs */
void    print_ls_output(const char *fname, const char *link, int type, struct stat *statp);
JCR    *setup_jcr(const char *name, char *dev_name, BSR *bsr,
                  const char *VolumeName, int mode);
void    display_tape_error_status(JCR *jcr, DEVICE *dev);


/* From dev.c */
DEVICE  *init_dev(JCR *jcr, DEVRES *device);
off_t    lseek_dev(DEVICE *dev, off_t offset, int whence);
bool     can_open_mounted_dev(DEVICE *dev);
bool     truncate_dev(DCR *dcr);
void     term_dev(DEVICE *dev);
char *   strerror_dev(DEVICE *dev);
void     clrerror_dev(DEVICE *dev, int func);
bool     update_pos_dev(DEVICE *dev);
bool     rewind_dev(DEVICE *dev);
bool     load_dev(DEVICE *dev);
bool     offline_dev(DEVICE *dev);
int      flush_dev(DEVICE *dev);
int      weof_dev(DEVICE *dev, int num);
int      write_block(DEVICE *dev);
uint32_t status_dev(DEVICE *dev);
bool     eod_dev(DEVICE *dev);
bool     fsf_dev(DEVICE *dev, int num);
bool     bsf_dev(DEVICE *dev, int num);
bool     bsr_dev(DEVICE *dev, int num);
void     attach_jcr_to_device(DEVICE *dev, JCR *jcr);
void     detach_jcr_from_device(DEVICE *dev, JCR *jcr);
JCR     *next_attached_jcr(DEVICE *dev, JCR *jcr);
bool     offline_or_rewind_dev(DEVICE *dev);
bool     reposition_dev(DEVICE *dev, uint32_t file, uint32_t block);
void     init_device_wait_timers(DCR *dcr);
void     init_jcr_device_wait_timers(JCR *jcr);
bool     double_dev_wait_time(DEVICE *dev);

/* Get info about device */
char *   dev_vol_name(DEVICE *dev);
uint32_t dev_block(DEVICE *dev);
uint32_t dev_file(DEVICE *dev);

/* From dvd.c */
int  dvd_open_next_part(DCR *dcr);
bool dvd_write_part(DCR *dcr); 
bool dvd_close_job(DCR *dcr);
bool mount_dev(DEVICE* dev, int timeout);
bool unmount_dev(DEVICE* dev, int timeout);
void update_free_space_dev(DEVICE *dev);
void make_mounted_dvd_filename(DEVICE *dev, POOL_MEM &archive_name);
void make_spooled_dvd_filename(DEVICE *dev, POOL_MEM &archive_name);
bool truncate_dvd_dev(DCR *dcr);
bool check_can_write_on_non_blank_dvd(DCR *dcr);

/* From device.c */
bool     open_device(DCR *dcr);
void     close_device(DEVICE *dev);
void     force_close_device(DEVICE *dev);
bool     first_open_device(DCR *dcr);
bool     fixup_device_block_write_error(DCR *dcr);
void     _lock_device(const char *file, int line, DEVICE *dev);
void     _unlock_device(const char *file, int line, DEVICE *dev);
void     _block_device(const char *file, int line, DEVICE *dev, int state);
void     _unblock_device(const char *file, int line, DEVICE *dev);
void     _steal_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold, int state);
void     _give_back_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold);
void     set_new_volume_parameters(DCR *dcr);
void     set_new_file_parameters(DCR *dcr);
bool     is_device_unmounted(DEVICE *dev);
void     dev_lock(DEVICE *dev);
void     dev_unlock(DEVICE *dev);

/* From dircmd.c */
void     *handle_connection_request(void *arg);


/* From fd_cmds.c */
void     run_job(JCR *jcr);
bool     bootstrap_cmd(JCR *jcr);

/* From job.c */
void     stored_free_jcr(JCR *jcr);
void     connection_from_filed(void *arg);
void     handle_filed_connection(BSOCK *fd, char *job_name);

/* From label.c */
int      read_dev_volume_label(DCR *dcr);
int      read_dvd_volume_label(DCR *dcr, bool write);
void     create_session_label(DCR *dcr, DEV_RECORD *rec, int label);
void     create_volume_label(DEVICE *dev, const char *VolName, const char *PoolName);
bool     write_new_volume_label_to_dev(DCR *dcr, const char *VolName, const char *PoolName);
#define ANSI_VOL_LABEL 0
#define ANSI_EOF_LABEL 1
#define ANSI_EOV_LABEL 2
bool     write_ansi_ibm_labels(DCR *dcr, int type, const char *VolName);
int      read_ansi_ibm_label(DCR *dcr);
bool     write_session_label(DCR *dcr, int label);
bool     write_volume_label_to_block(DCR *dcr);
bool     rewrite_volume_label(DCR *dcr, bool recycle);
void     dump_volume_label(DEVICE *dev);
void     dump_label_record(DEVICE *dev, DEV_RECORD *rec, int verbose);
bool     unser_volume_label(DEVICE *dev, DEV_RECORD *rec);
bool     unser_session_label(SESSION_LABEL *label, DEV_RECORD *rec);

/* From match_bsr.c */
int      match_bsr(BSR *bsr, DEV_RECORD *rec, VOLUME_LABEL *volrec,
              SESSION_LABEL *sesrec);
int      match_bsr_block(BSR *bsr, DEV_BLOCK *block);
void     position_bsr_block(BSR *bsr, DEV_BLOCK *block);
BSR     *find_next_bsr(BSR *root_bsr, DEVICE *dev);
bool     match_set_eof(BSR *bsr, DEV_RECORD *rec);

/* From mount.c */
bool     mount_next_write_volume(DCR *dcr, bool release);
bool     mount_next_read_volume(DCR *dcr);
void     mark_volume_in_error(DCR *dcr);

/* From parse_bsr.c */
BSR     *parse_bsr(JCR *jcr, char *lf);
void     dump_bsr(BSR *bsr, bool recurse);
void     free_bsr(BSR *bsr);
VOL_LIST *new_restore_volume();
int      add_restore_volume(JCR *jcr, VOL_LIST *vol);
void     free_restore_volume_list(JCR *jcr);
void     create_restore_volume_list(JCR *jcr);

/* From record.c */
const char *FI_to_ascii(char *buf, int fi);
const char *stream_to_ascii(char *buf, int stream, int fi);
bool        write_record_to_block(DEV_BLOCK *block, DEV_RECORD *rec);
bool        can_write_record_to_block(DEV_BLOCK *block, DEV_RECORD *rec);
bool        read_record_from_block(DEV_BLOCK *block, DEV_RECORD *rec);
DEV_RECORD *new_record();
void        free_record(DEV_RECORD *rec);
void        empty_record(DEV_RECORD *rec);

/* From read_record.c */
bool read_records(DCR *dcr,
       bool record_cb(DCR *dcr, DEV_RECORD *rec),
       bool mount_cb(DCR *dcr));

/* From reserve.c */
void    init_reservations_lock();
void    term_reservations_lock();
void    lock_reservations();
void    unlock_reservations();
void    release_volume(DCR *dcr);
VOLRES *new_volume(DCR *dcr, const char *VolumeName);
VOLRES *find_volume(const char *VolumeName);
bool    free_volume(DEVICE *dev);
void    free_unused_volume(DCR *dcr);
void    create_volume_list();
void    free_volume_list();
void    list_volumes(BSOCK *user);
bool    is_volume_in_use(DCR *dcr);
void    send_drive_reserve_messages(JCR *jcr, BSOCK *user);
bool    find_suitable_device_for_job(JCR *jcr, RCTX &rctx);
int     search_res_for_device(RCTX &rctx);
void    release_msgs(JCR *jcr);

/* From spool.c */
bool    begin_data_spool          (DCR *dcr);
bool    discard_data_spool        (DCR *dcr);
bool    commit_data_spool         (DCR *dcr);
bool    are_attributes_spooled    (JCR *jcr);
bool    begin_attribute_spool     (JCR *jcr);
bool    discard_attribute_spool   (JCR *jcr);
bool    commit_attribute_spool    (JCR *jcr);
bool    write_block_to_spool_file (DCR *dcr);
void    list_spool_stats          (BSOCK *bs);

/* From wait.c */
int wait_for_sysop(DCR *dcr);
bool wait_for_device(JCR *jcr, bool first);
