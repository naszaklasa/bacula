/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2007 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula Director -- Automatic Pruning
 *      Applies retention periods
 *
 *     Kern Sibbald, May MMII
 *
 *   Version $Id: autoprune.c 5144 2007-07-12 07:49:21Z kerns $
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
 * Prune at least one Volume in current Pool. This is called from
 *   catreq.c => next_vol.c when the Storage daemon is asking for another
 *   volume and no appendable volumes are available.
 *
 *  Return: false if nothing pruned
 *          true if pruned, and mr is set to pruned volume
 */
bool prune_volumes(JCR *jcr, bool InChanger, MEDIA_DBR *mr) 
{
   int count;
   int i;
   dbid_list ids;
   struct del_ctx prune_list;
   POOL_MEM query(PM_MESSAGE);
   UAContext *ua;
   bool ok = false;
   char ed1[50], ed2[100], ed3[50];
   POOL_DBR spr;

   Dmsg1(050, "Prune volumes PoolId=%d\n", jcr->jr.PoolId);
   if (!jcr->job->PruneVolumes && !jcr->pool->AutoPrune) {
      Dmsg0(100, "AutoPrune not set in Pool.\n");
      return 0;
   }

   memset(&prune_list, 0, sizeof(prune_list));
   prune_list.max_ids = 10000;
   prune_list.JobId = (JobId_t *)malloc(sizeof(JobId_t) * prune_list.max_ids);

   ua = new_ua_context(jcr);

   db_lock(jcr->db);

   /* Edit PoolId */
   edit_int64(mr->PoolId, ed1);
   /*
    * Get Pool record for Scratch Pool
    */
   memset(&spr, 0, sizeof(spr));
   bstrncpy(spr.Name, "Scratch", sizeof(spr.Name));
   if (db_get_pool_record(jcr, jcr->db, &spr)) {
      edit_int64(spr.PoolId, ed2);
      bstrncat(ed2, ",", sizeof(ed2));
   } else {
      ed2[0] = 0;
   }
   Dmsg1(050, "Scratch pool=%s\n", ed2);
   /*
    * ed2 ends up with scratch poolid and current poolid or
    *   just current poolid if there is no scratch pool 
    */
   bstrncat(ed2, ed1, sizeof(ed2));

   /*
    * Get the List of all media ids in the current Pool or whose
    *  RecyclePoolId is the current pool or the scratch pool
    */
   const char *select = "SELECT DISTINCT MediaId,LastWritten FROM Media WHERE "
        "(PoolId=%s OR RecyclePoolId IN (%s)) AND MediaType='%s' %s"
        "ORDER BY LastWritten ASC,MediaId";

   if (InChanger) {
      char changer[100];
      /* Ensure it is in this autochanger */
      bsnprintf(changer, sizeof(changer), "AND InChanger=1 AND StorageId=%s ",
         edit_int64(mr->StorageId, ed3));
      Mmsg(query, select, ed1, ed2, mr->MediaType, changer);
   } else {
      Mmsg(query, select, ed1, ed2, mr->MediaType, "");
   }

   Dmsg1(050, "query=%s\n", query.c_str());
   if (!db_get_query_dbids(ua->jcr, ua->db, query, ids)) {
      Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
      goto bail_out;
   }

   Dmsg1(050, "num_ids=%d\n", ids.num_ids);

   /* Visit each Volume and Prune it until we find one that is purged */
   for (i=0; i<ids.num_ids; i++) {
      MEDIA_DBR lmr;
      memset(&lmr, 0, sizeof(lmr));
      lmr.MediaId = ids.DBId[i];
      Dmsg1(050, "Get record MediaId=%d\n", (int)lmr.MediaId);
      if (!db_get_media_record(jcr, jcr->db, &lmr)) {
         Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
         continue;
      }
      /* Don't prune archived volumes */
      if (lmr.Enabled == 2) {
         continue;
      }
      /* Prune only Volumes with status "Full", or "Used" */
      if (strcmp(lmr.VolStatus, "Full")   == 0 ||
          strcmp(lmr.VolStatus, "Used")   == 0) {
         Dmsg2(050, "Add prune list MediaId=%d Volume %s\n", (int)lmr.MediaId, lmr.VolumeName);
         count = get_prune_list_for_volume(ua, &lmr, &prune_list);
         Dmsg1(050, "Num pruned = %d\n", count);
         if (count != 0) {
            purge_job_list_from_catalog(ua, prune_list);
            prune_list.num_ids = 0;             /* reset count */
         }
         ok = is_volume_purged(ua, &lmr);
         /*
          * If purged and not moved to another Pool, 
          *   then we stop pruning and take this volume.
          */
         if (ok && lmr.PoolId == mr->PoolId) {
            Dmsg2(050, "Vol=%s MediaId=%d purged.\n", lmr.VolumeName, (int)lmr.MediaId);
            mr = &lmr;                    /* struct copy */
            break;                        /* got a volume */
         }
         /*
          * We purged something but did not get a volume in the current pool.
          *  It must be a scratch volume, so try to get it.
          */
         if (ok && get_scratch_volume(jcr, InChanger, mr)) {
            break;                       /* got a volume */
         }
         ok = false;                     /* clear OK, in case we fall out */
      } else {
         Dmsg2(050, "Nothing pruned MediaId=%d Volume=%s\n", (int)lmr.MediaId, lmr.VolumeName);
      }
   }

bail_out:
   db_unlock(jcr->db);
   free_ua_context(ua);
   if (prune_list.JobId) {
      free(prune_list.JobId);
   }
   return ok;
}
