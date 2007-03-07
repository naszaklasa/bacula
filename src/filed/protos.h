/*
 *   Version $Id: protos.h 3673 2006-11-21 17:03:47Z kerns $
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

extern bool blast_data_to_storage_daemon(JCR *jcr, char *addr);
extern void do_verify_volume(JCR *jcr);
extern void do_restore(JCR *jcr);
extern int authenticate_director(JCR *jcr);
extern int authenticate_storagedaemon(JCR *jcr);
extern int make_estimate(JCR *jcr);

/* From verify.c */
int digest_file(JCR *jcr, FF_PKT *ff_pkt, DIGEST *digest);
void do_verify(JCR *jcr);

/* From heartbeat.c */
void start_heartbeat_monitor(JCR *jcr);
void stop_heartbeat_monitor(JCR *jcr);
void start_dir_heartbeat(JCR *jcr);
void stop_dir_heartbeat(JCR *jcr);

/* From acl.c */
int bacl_get(JCR *jcr, int acltype);
int bacl_set(JCR *jcr, int acltype);
