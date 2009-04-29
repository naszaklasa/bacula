/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2009 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *  Version $Id $
 *
 */

#include "bacula.h"
#include "filed.h"

static int dbglvl=200;

typedef struct PrivateCurFile {
   hlink link;
   char *fname;
   utime_t ctime;
   utime_t mtime;
   bool seen;
} CurFile;

bool accurate_mark_file_as_seen(JCR *jcr, char *fname)
{
   if (!jcr->accurate || !jcr->file_list) {
      return false;
   }
   /* TODO: just use elt->seen = 1 */
   CurFile *temp = (CurFile *)jcr->file_list->lookup(fname);
   if (temp) {
      temp->seen = 1;              /* records are in memory */
      Dmsg1(dbglvl, "marked <%s> as seen\n", fname);
   } else {
      Dmsg1(dbglvl, "<%s> not found to be marked as seen\n", fname);
   }
   return true;
}

static bool accurate_mark_file_as_seen(JCR *jcr, CurFile *elt)
{
   /* TODO: just use elt->seen = 1 */
   CurFile *temp = (CurFile *)jcr->file_list->lookup(elt->fname);
   if (temp) {
      temp->seen = 1;              /* records are in memory */
   }
   return true;
}

static bool accurate_lookup(JCR *jcr, char *fname, CurFile *ret)
{
   bool found=false;
   ret->seen = 0;

   CurFile *temp = (CurFile *)jcr->file_list->lookup(fname);
   if (temp) {
      memcpy(ret, temp, sizeof(CurFile));
      found=true;
      Dmsg1(dbglvl, "lookup <%s> ok\n", fname);
   }

   return found;
}

static bool accurate_init(JCR *jcr, int nbfile)
{
   CurFile *elt = NULL;
   jcr->file_list = (htable *)malloc(sizeof(htable));
   jcr->file_list->init(elt, &elt->link, nbfile);
   return true;
}

/* This function is called at the end of backup
 * We walk over all hash disk element, and we check
 * for elt.seen.
 */
bool accurate_send_deleted_list(JCR *jcr)
{
   CurFile *elt;
   FF_PKT *ff_pkt;
   int stream = STREAM_UNIX_ATTRIBUTES;

   if (!jcr->accurate || jcr->get_JobLevel() == L_FULL) {
      goto bail_out;
   }

   if (jcr->file_list == NULL) {
      goto bail_out;
   }

   ff_pkt = init_find_files();
   ff_pkt->type = FT_DELETED;

   foreach_htable(elt, jcr->file_list) {
      if (elt->seen || plugin_check_file(jcr, elt->fname)) {
         continue;
      }
      Dmsg2(dbglvl, "deleted fname=%s seen=%i\n", elt->fname, elt->seen);
      ff_pkt->fname = elt->fname;
      ff_pkt->statp.st_mtime = elt->mtime;
      ff_pkt->statp.st_ctime = elt->ctime;
      encode_and_send_attributes(jcr, ff_pkt, stream);
//    free(elt->fname);
   }

   term_find_files(ff_pkt);
bail_out:
   /* TODO: clean htable when this function is not reached ? */
   if (jcr->file_list) {
      jcr->file_list->destroy();
      free(jcr->file_list);
      jcr->file_list = NULL;
   }
   return true;
}

static bool accurate_add_file(JCR *jcr, char *fname, char *lstat)
{
   bool ret = true;
   CurFile elt;
   struct stat statp;
   int LinkFIc;
   decode_stat(lstat, &statp, &LinkFIc); /* decode catalog stat */
   elt.ctime = statp.st_ctime;
   elt.mtime = statp.st_mtime;
   elt.seen = 0;

   CurFile *item;
   /* we store CurFile, fname and ctime/mtime in the same chunk */
   item = (CurFile *)jcr->file_list->hash_malloc(sizeof(CurFile)+strlen(fname)+1);
   memcpy(item, &elt, sizeof(CurFile));
   item->fname  = (char *)item+sizeof(CurFile);
   strcpy(item->fname, fname);
   jcr->file_list->insert(item->fname, item); 

   Dmsg2(dbglvl, "add fname=<%s> lstat=%s\n", fname, lstat);
   return ret;
}

/*
 * This function is called for each file seen in fileset.
 * We check in file_list hash if fname have been backuped
 * the last time. After we can compare Lstat field. 
 * Full Lstat usage have been removed on 6612 
 */
bool accurate_check_file(JCR *jcr, FF_PKT *ff_pkt)
{
   bool stat = false;
   char *fname;
   CurFile elt;

   if (!jcr->accurate || jcr->get_JobLevel() == L_FULL) {
      return true;
   }

   strip_path(ff_pkt);
 
   if (S_ISDIR(ff_pkt->statp.st_mode)) {
      fname = ff_pkt->link;
   } else {
      fname = ff_pkt->fname;
   } 

   if (!accurate_lookup(jcr, fname, &elt)) {
      Dmsg1(dbglvl, "accurate %s (not found)\n", fname);
      stat = true;
      goto bail_out;
   }

   if (elt.seen) { /* file has been seen ? */
      Dmsg1(dbglvl, "accurate %s (already seen)\n", fname);
      goto bail_out;
   }

   /*
    * We check only mtime/ctime like with the normal
    * incremental/differential mode
    */
   if (elt.mtime != ff_pkt->statp.st_mtime) {
//   Jmsg(jcr, M_SAVED, 0, _("%s      st_mtime differs\n"), fname);
      Dmsg3(dbglvl, "%s      st_mtime differs (%i!=%i)\n", 
            fname, elt.mtime, ff_pkt->statp.st_mtime);
     stat = true;
   } else if (!(ff_pkt->flags & FO_MTIMEONLY) 
              && (elt.ctime != ff_pkt->statp.st_ctime)) {
//   Jmsg(jcr, M_SAVED, 0, _("%s      st_ctime differs\n"), fname);
      Dmsg3(dbglvl, "%s      st_ctime differs\n", 
            fname, elt.ctime, ff_pkt->statp.st_ctime);
     stat = true;
   }

   accurate_mark_file_as_seen(jcr, &elt);
//   Dmsg2(dbglvl, "accurate %s = %i\n", fname, stat);

bail_out:
   unstrip_path(ff_pkt);
   return stat;
}

/* 
 * TODO: use big buffer from htable
 */
int accurate_cmd(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   int len;
   int32_t nb;

   if (!jcr->accurate || job_canceled(jcr) || jcr->get_JobLevel()==L_FULL) {
      return true;
   }

   if (sscanf(dir->msg, "accurate files=%ld", &nb) != 1) {
      dir->fsend(_("2991 Bad accurate command\n"));
      return false;
   }

   accurate_init(jcr, nb);

   /*
    * buffer = sizeof(CurFile) + dirmsg
    * dirmsg = fname + \0 + lstat
    */
   /* get current files */
   while (dir->recv() >= 0) {
      len = strlen(dir->msg) + 1;
      if (len < dir->msglen) {
         accurate_add_file(jcr, dir->msg, dir->msg + len);
      }
   }

#ifdef DEBUG
   extern void *start_heap;

   char b1[50], b2[50], b3[50], b4[50], b5[50];
   Dmsg5(dbglvl," Heap: heap=%s smbytes=%s max_bytes=%s bufs=%s max_bufs=%s\n",
         edit_uint64_with_commas((char *)sbrk(0)-(char *)start_heap, b1),
         edit_uint64_with_commas(sm_bytes, b2),
         edit_uint64_with_commas(sm_max_bytes, b3),
         edit_uint64_with_commas(sm_buffers, b4),
         edit_uint64_with_commas(sm_max_buffers, b5));
#endif

   return true;
}
