/*
 * Protypes for stored
 *
 *   Version $Id: protos.h,v 1.70 2004/10/07 15:26:37 kerns Exp $
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
   
/* From stored.c */
uint32_t new_VolSessionId();

/* From acquire.c */
DCR	*acquire_device_for_append(JCR *jcr);
DCR	*acquire_device_for_read(JCR *jcr);
bool	 release_device(JCR *jcr);
DCR	*new_dcr(JCR *jcr, DEVICE *dev);
void	 free_dcr(DCR *dcr);

/* From askdir.c */
enum get_vol_info_rw {
   GET_VOL_INFO_FOR_WRITE,
   GET_VOL_INFO_FOR_READ
};
bool	dir_get_volume_info(DCR *dcr, enum get_vol_info_rw);
bool	dir_find_next_appendable_volume(DCR *dcr);
bool	dir_update_volume_info(DCR *dcr, bool label);
bool	dir_ask_sysop_to_create_appendable_volume(DCR *dcr);
bool	dir_ask_sysop_to_mount_volume(DCR *dcr);
bool	dir_update_file_attributes(DCR *dcr, DEV_RECORD *rec);
bool	dir_send_job_status(JCR *jcr);
bool	dir_create_jobmedia_record(DCR *dcr);

/* authenticate.c */
int	authenticate_director(JCR *jcr);
int	authenticate_filed(JCR *jcr);

/* From autochanger.c */
int	 autoload_device(DCR *dcr, int writing, BSOCK *dir);
bool	 autochanger_list(DCR *dcr, BSOCK *dir);
void	 invalidate_slot_in_catalog(DCR *dcr);
char	*edit_device_codes(JCR *jcr, char *omsg, const char *imsg, const char *cmd);

/* From block.c */
void	dump_block(DEV_BLOCK *b, const char *msg);
DEV_BLOCK *new_block(DEVICE *dev);
DEV_BLOCK *dup_block(DEV_BLOCK *eblock);
void	init_block_write(DEV_BLOCK *block);
void	empty_block(DEV_BLOCK *block);
void	free_block(DEV_BLOCK *block);
bool	write_block_to_device(DCR *dcr);
bool	write_block_to_dev(DCR *dcr);
void	print_block_read_errors(JCR *jcr, DEV_BLOCK *block);
void	ser_block_header(DEV_BLOCK *block);

#define CHECK_BLOCK_NUMBERS    true
#define NO_BLOCK_NUMBER_CHECK  false
bool	read_block_from_device(DCR *dcr, bool check_block_numbers);
bool	read_block_from_dev(DCR *dcr, bool check_block_numbers);

/* From butil.c -- utilities for SD tool programs */
void	print_ls_output(const char *fname, const char *link, int type, struct stat *statp);
JCR    *setup_jcr(const char *name, char *dev_name, BSR *bsr,
		  const char *VolumeName, int mode);
void	display_tape_error_status(JCR *jcr, DEVICE *dev);


/* From dev.c */
DEVICE	*init_dev(DEVICE *dev, DEVRES *device);
int	 open_dev(DEVICE *dev, char *VolName, int mode);
void	 close_dev(DEVICE *dev);
void	 force_close_dev(DEVICE *dev);
bool	 truncate_dev(DEVICE *dev);
void	 term_dev(DEVICE *dev);
char *	 strerror_dev(DEVICE *dev);
void	 clrerror_dev(DEVICE *dev, int func);
bool	 update_pos_dev(DEVICE *dev);
bool	 rewind_dev(DEVICE *dev);
bool	 load_dev(DEVICE *dev);
bool	 offline_dev(DEVICE *dev);
int	 flush_dev(DEVICE *dev);
int	 weof_dev(DEVICE *dev, int num);
int	 write_block(DEVICE *dev);
uint32_t status_dev(DEVICE *dev);
int	 eod_dev(DEVICE *dev);
bool	 fsf_dev(DEVICE *dev, int num);
bool	 fsr_dev(DEVICE *dev, int num);
bool	 bsf_dev(DEVICE *dev, int num);
bool	 bsr_dev(DEVICE *dev, int num);
void	 attach_jcr_to_device(DEVICE *dev, JCR *jcr);
void	 detach_jcr_from_device(DEVICE *dev, JCR *jcr);
JCR	*next_attached_jcr(DEVICE *dev, JCR *jcr);
bool	 dev_can_write(DEVICE *dev);
bool	 offline_or_rewind_dev(DEVICE *dev);
bool	 reposition_dev(DEVICE *dev, uint32_t file, uint32_t block);
void	 init_dev_wait_timers(DEVICE *dev);
bool	 double_dev_wait_time(DEVICE *dev);


/* Get info about device */
char *	 dev_name(DEVICE *dev);
char *	 dev_vol_name(DEVICE *dev);
uint32_t dev_block(DEVICE *dev);
uint32_t dev_file(DEVICE *dev);
bool	 dev_is_tape(DEVICE *dev);

/* From device.c */
bool	 open_device(DCR *dcr);
bool	 first_open_device(DEVICE *dev);
bool	 fixup_device_block_write_error(DCR *dcr);
void	 _lock_device(const char *file, int line, DEVICE *dev);
void	 _unlock_device(const char *file, int line, DEVICE *dev);
void	 _block_device(const char *file, int line, DEVICE *dev, int state);
void	 _unblock_device(const char *file, int line, DEVICE *dev);
void	 _steal_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold, int state);
void	 _give_back_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold);
void	 set_new_volume_parameters(DCR *dcr);
void	 set_new_file_parameters(DCR *dcr);
bool	 device_is_unmounted(DEVICE *dev);
void	 dev_lock(DEVICE *dev);
void	 dev_unlock(DEVICE *dev);
const char *edit_blocked_reason(DEVICE *dev);

/* From dircmd.c */
void	 *handle_connection_request(void *arg); 


/* From fd_cmds.c */
void	 run_job(JCR *jcr);
bool	 bootstrap_cmd(JCR *jcr);

/* From job.c */
void	 stored_free_jcr(JCR *jcr);
void	 connection_from_filed(void *arg);     
void	 handle_filed_connection(BSOCK *fd, char *job_name);

/* From label.c */
int	 read_dev_volume_label(DCR *dcr);
void	 create_session_label(DCR *dcr, DEV_RECORD *rec, int label);
void	 create_volume_label(DEVICE *dev, const char *VolName, const char *PoolName);
bool	 write_new_volume_label_to_dev(DCR *dcr, const char *VolName, const char *PoolName);
bool	 write_session_label(DCR *dcr, int label);
bool	 write_volume_label_to_block(DCR *dcr);
void	 dump_volume_label(DEVICE *dev);
void	 dump_label_record(DEVICE *dev, DEV_RECORD *rec, int verbose);
bool	 unser_volume_label(DEVICE *dev, DEV_RECORD *rec);
bool	 unser_session_label(SESSION_LABEL *label, DEV_RECORD *rec);

/* From match_bsr.c */
int	 match_bsr(BSR *bsr, DEV_RECORD *rec, VOLUME_LABEL *volrec, 
	      SESSION_LABEL *sesrec);
int	 match_bsr_block(BSR *bsr, DEV_BLOCK *block);
void	 position_bsr_block(BSR *bsr, DEV_BLOCK *block);
BSR	*find_next_bsr(BSR *root_bsr, DEVICE *dev);
bool	 match_set_eof(BSR *bsr, DEV_RECORD *rec);

/* From mount.c */
bool	 mount_next_write_volume(DCR *dcr, bool release);
bool	 mount_next_read_volume(DCR *dcr);
void	 release_volume(DCR *ddr);
void	 mark_volume_in_error(DCR *dcr);

/* From parse_bsr.c */
BSR	*parse_bsr(JCR *jcr, char *lf);
void	 dump_bsr(BSR *bsr, bool recurse);
void	 free_bsr(BSR *bsr);
VOL_LIST *new_vol();
int	 add_vol(JCR *jcr, VOL_LIST *vol);
void	 free_vol_list(JCR *jcr);
void	 create_vol_list(JCR *jcr);

/* From record.c */
const char *FI_to_ascii(int fi);
const char *stream_to_ascii(int stream, int fi);
bool	    write_record_to_block(DEV_BLOCK *block, DEV_RECORD *rec);
bool	    can_write_record_to_block(DEV_BLOCK *block, DEV_RECORD *rec);
bool	    read_record_from_block(DEV_BLOCK *block, DEV_RECORD *rec); 
DEV_RECORD *new_record();
void	    free_record(DEV_RECORD *rec);
void	    empty_record(DEV_RECORD *rec);

/* From read_record.c */
bool read_records(DCR *dcr,
       bool record_cb(DCR *dcr, DEV_RECORD *rec),
       bool mount_cb(DCR *dcr));

/* From spool.c */
bool	begin_data_spool	  (JCR *jcr);
bool	discard_data_spool	  (JCR *jcr);
bool	commit_data_spool	  (JCR *jcr);
bool	are_attributes_spooled	  (JCR *jcr);
bool	begin_attribute_spool	  (JCR *jcr);
bool	discard_attribute_spool   (JCR *jcr);
bool	commit_attribute_spool	  (JCR *jcr);
bool	write_block_to_spool_file (DCR *dcr);
void	list_spool_stats	  (BSOCK *bs);
