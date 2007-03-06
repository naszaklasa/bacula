/*
 *
 *   Bacula Director -- Automatic Pruning 
 *	Applies retention periods
 *
 *     Kern Sibbald, May MMII
 *
 *   Version $Id: autoprune.c,v 1.12 2004/04/19 14:27:00 kerns Exp $
 */

/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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
#include "dird.h"
#include "ua.h"

/* Forward referenced functions */


/*
 * Auto Prune Jobs and Files. This is called at the end of every
 *   Job.  We do not prune volumes here.
 */
int do_autoprune(JCR *jcr)
{
   UAContext *ua;
   CLIENT *client;
   bool pruned;

   if (!jcr->client) {		      /* temp -- remove me */
      return 1;
   }

   ua = new_ua_context(jcr);

   client = jcr->client;

   if (jcr->job->PruneJobs || jcr->client->AutoPrune) {
      Jmsg(jcr, M_INFO, 0, _("Begin pruning Jobs.\n"));
      prune_jobs(ua, client, jcr->JobType);
      pruned = true;
   } else {
      pruned = false;
   }
  
   if (jcr->job->PruneFiles || jcr->client->AutoPrune) {
      Jmsg(jcr, M_INFO, 0, _("Begin pruning Files.\n"));
      prune_files(ua, client);
      pruned = true;
   }
   if (pruned) {
      Jmsg(jcr, M_INFO, 0, _("End auto prune.\n\n"));
   }

   free_ua_context(ua);
   return 1;	
}

/*
 * Prune all volumes in current Pool. This is called from
 *   catreq.c when the Storage daemon is asking for another
 *   volume and no appendable volumes are available.
 *
 *  Return 0: on error
 *	   number of Volumes Purged
 */
int prune_volumes(JCR *jcr)
{
   int stat = 0;
   int i;
   uint32_t *ids = NULL;
   int num_ids = 0;
   MEDIA_DBR mr;
   UAContext *ua;

   if (!jcr->job->PruneVolumes && !jcr->pool->AutoPrune) {
      Dmsg0(100, "AutoPrune not set in Pool.\n");
      return 0;
   }
   memset(&mr, 0, sizeof(mr));
   ua = new_ua_context(jcr);

   db_lock(jcr->db);

   /* Get the List of all media ids in the current Pool */
   if (!db_get_media_ids(jcr, jcr->db, jcr->PoolId, &num_ids, &ids)) {
      Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
      goto bail_out;
   }

   /* Visit each Volume and Prune it */
   for (i=0; i<num_ids; i++) {
      mr.MediaId = ids[i];
      if (!db_get_media_record(jcr, jcr->db, &mr)) {
         Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
	 continue;
      }
      /* Prune only Volumes from current Pool */
      if (jcr->PoolId != mr.PoolId) {
	 continue;
      }
      /* Prune only Volumes with status "Full", "Used", or "Append" */
      if (strcmp(mr.VolStatus, "Full")   == 0 || 
          strcmp(mr.VolStatus, "Append") == 0 ||
          strcmp(mr.VolStatus, "Used")   == 0) {
         Dmsg1(200, "Prune Volume %s\n", mr.VolumeName);
	 stat += prune_volume(ua, &mr); 
         Dmsg1(200, "Num pruned = %d\n", stat);
      }
   }   

bail_out:
   db_unlock(jcr->db);
   free_ua_context(ua);
   if (ids) {
      free(ids);
   }
   return stat;
}
