/*
 *  Bacula File Daemon estimate.c
 *   Make and estimate of the number of files and size to be saved.
 *
 *    Kern Sibbald, September MMI
 *
 *   Version $Id: estimate.c,v 1.12.8.1 2005/02/14 10:02:23 kerns Exp $
 *
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

#include "bacula.h"
#include "filed.h"

static int tally_file(FF_PKT *ff_pkt, void *pkt);

/*
 * Find all the requested files and count them.
 */
int make_estimate(JCR *jcr)
{
   int stat;

   jcr->JobStatus = JS_Running;

   set_find_options((FF_PKT *)jcr->ff, jcr->incremental, jcr->mtime);
   stat = find_files(jcr, (FF_PKT *)jcr->ff, tally_file, (void *)jcr);

   return stat;
}

/*
 * Called here by find() for each file included.
 *
 */
static int tally_file(FF_PKT *ff_pkt, void *ijcr)
{
   JCR *jcr = (JCR *)ijcr;
   ATTR attr;

   if (job_canceled(jcr)) {
      return 0;
   }
   switch (ff_pkt->type) {
   case FT_LNKSAVED:		      /* Hard linked, file already saved */
   case FT_REGE:
   case FT_REG:
   case FT_LNK:
   case FT_NORECURSE:
   case FT_NOFSCHG:
   case FT_INVALIDFS:
   case FT_DIREND:
   case FT_SPEC:
   case FT_RAW:
   case FT_FIFO:
      break;
   case FT_DIRBEGIN:
   case FT_NOACCESS:
   case FT_NOFOLLOW:
   case FT_NOSTAT:
   case FT_DIRNOCHG:
   case FT_NOCHG:
   case FT_ISARCH:
   case FT_NOOPEN:
   default:
      return 1;
   }

   if (ff_pkt->type != FT_LNKSAVED && S_ISREG(ff_pkt->statp.st_mode)) {
      if (ff_pkt->statp.st_size > 0) {
	 jcr->JobBytes += ff_pkt->statp.st_size;
      }
#ifdef HAVE_DARWIN_OS
      if (ff_pkt->flags & FO_HFSPLUS) {
	 if (ff_pkt->hfsinfo.rsrclength > 0) {
	    jcr->JobBytes += ff_pkt->hfsinfo.rsrclength;
	 }
	 jcr->JobBytes += 32;    /* Finder info */
      }
#endif
   }
   jcr->num_files_examined++;
   jcr->JobFiles++;		     /* increment number of files seen */
   if (jcr->listing) {
      memcpy(&attr.statp, &ff_pkt->statp, sizeof(struct stat));
      attr.type = ff_pkt->type;
      attr.ofname = (POOLMEM *)ff_pkt->fname;
      attr.olname = (POOLMEM *)ff_pkt->link;
      print_ls_output(jcr, &attr);
   }
   return 1;
}
