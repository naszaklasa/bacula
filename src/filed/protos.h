/*
 *   Version $Id: protos.h,v 1.9 2004/07/04 19:22:15 kerns Exp $
 */

/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

extern bool blast_data_to_storage_daemon(JCR *jcr, char *addr);
extern void do_verify(JCR *jcr);
extern void do_verify_volume(JCR *jcr);
extern void do_restore(JCR *jcr);
extern int authenticate_director(JCR *jcr);
extern int authenticate_storagedaemon(JCR *jcr);
extern int make_estimate(JCR *jcr);

/* From heartbeat.c */
void start_heartbeat_monitor(JCR *jcr);
void stop_heartbeat_monitor(JCR *jcr);
void start_dir_heartbeat(JCR *jcr);
void stop_dir_heartbeat(JCR *jcr);
