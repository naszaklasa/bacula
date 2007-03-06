/*
 * Prototypes for finlib directory of Bacula
 *
 *   Version $Id: protos.h,v 1.30 2005/05/17 14:37:05 thorstenengel Exp $
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
/* from attribs.c */
void    encode_stat       (char *buf, FF_PKT *ff_pkt, int data_stream);
int     decode_stat       (char *buf, struct stat *statp, int32_t *LinkFI);
int32_t decode_LinkFI     (char *buf, struct stat *statp);
int     encode_attribsEx  (JCR *jcr, char *attribsEx, FF_PKT *ff_pkt);
bool    set_attributes    (JCR *jcr, ATTR *attr, BFILE *ofd);
int     select_data_stream(FF_PKT *ff_pkt);

/* from create_file.c */
int    create_file       (JCR *jcr, ATTR *attr, BFILE *ofd, int replace);

/* From find.c */
FF_PKT *init_find_files();
void  set_find_options(FF_PKT *ff, int incremental, time_t mtime);
int   find_files(JCR *jcr, FF_PKT *ff, int sub(FF_PKT *ff_pkt, void *hpkt, bool), void *pkt);
int   match_files(JCR *jcr, FF_PKT *ff, int sub(FF_PKT *ff_pkt, void *hpkt, bool), void *pkt);
int   term_find_files(FF_PKT *ff);
int   get_win32_driveletters(FF_PKT *ff, char* szDrives);

/* From match.c */
void  init_include_exclude_files(FF_PKT *ff);
void  term_include_exclude_files(FF_PKT *ff);
void  add_fname_to_include_list(FF_PKT *ff, int prefixed, const char *fname);
void  add_fname_to_exclude_list(FF_PKT *ff, const char *fname);
int   file_is_excluded(FF_PKT *ff, const char *file);
int   file_is_included(FF_PKT *ff, const char *file);
struct s_included_file *get_next_included_file(FF_PKT *ff,
                           struct s_included_file *inc);

/* From find_one.c */
int   find_one_file(JCR *jcr, FF_PKT *ff, 
               int handle_file(FF_PKT *ff_pkt, void *hpkt, bool top_level),
               void *pkt, char *p, dev_t parent_device, bool top_level);
int   term_find_one(FF_PKT *ff);


/* From get_priv.c */
int enable_backup_privileges(JCR *jcr, int ignore_errors);


/* from makepath.c */
int make_path(JCR *jcr, const char *argpath, int mode,
           int parent_mode, uid_t owner, gid_t group,
           int preserve_existing, char *verbose_fmt_string);

/* from fstype.c */
bool fstype(const char *fname, char *fs, int fslen);

/* from bfile.c -- see bfile.h */
