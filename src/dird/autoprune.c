/*
 *
 *   Bacula Director -- Automatic Pruning
 *      Applies retention periods
 *
 *     Kern Sibbald, May MMII
 *
 *   Version $Id: autoprune.c,v 1.19 2006/11/21 13:20:08 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2006 Free Software Foundation Europe e.V.

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

#include "bacula.h"
#include "dird.h"
#include "ua.h"

/* Forward referenced functions */


/*
 * Auto Prune Jobs and Files. This is called at the end of every
 *   Job.  We do not prune volumes here.
 */
void do_autoprune(JCR *jcr)
{
   UAContext *ua;
   CLIENT *client;
   bool pruned;

   if (!jcr->client) {                /* temp -- remove me */
      return;
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
   return;
}

/*
 * Prune all volumes in current Pool. This is called from
 *   catreq.c when the Storage daemon is asking for another
 *   volume and no appendable volumes are available.
 *
 *  Return 0: on error
 *         number of Volumes Purged
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
   if (!db_get_media_ids(jcr, jcr->db, jcr->jr.PoolId, &num_ids, &ids)) {
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
      if (jcr->jr.PoolId != mr.PoolId) {
         continue;
      }
      /* Don't prune archived volumes */
      if (mr.Enabled == 2) {
         continue;
      }
      /* Prune only Volumes with status "Full", or "Used" */
      if (strcmp(mr.VolStatus, "Full")   == 0 ||
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
