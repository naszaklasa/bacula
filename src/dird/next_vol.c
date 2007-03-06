/*
 *
 *   Bacula Director -- next_vol -- handles finding the next
 *    volume for append.  Split out of catreq.c August MMIII
 *    catalog request from the Storage daemon.

 *     Kern Sibbald, March MMI
 *
 *   Version $Id: next_vol.c,v 1.12 2004/09/19 18:56:24 kerns Exp $
 */
/*
   Copyright (C) 2001-2004 Kern Sibbald and John Walker

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

/*
 *  Items needed:
 *   jcr->PoolId
 *   jcr->store
 *   jcr->db
 *   jcr->pool
 *   MEDIA_DBR mr (zeroed out)
 *   create -- whether or not to create a new volume
 */
int find_next_volume_for_append(JCR *jcr, MEDIA_DBR *mr, bool create)
{
   int retry = 0;
   bool ok;
   bool InChanger;
   STORE *store = jcr->store;

   mr->PoolId = jcr->PoolId;
   bstrncpy(mr->MediaType, store->media_type, sizeof(mr->MediaType));
   Dmsg2(120, "CatReq FindMedia: Id=%d, MediaType=%s\n", mr->PoolId, mr->MediaType);
   /*
    * If we are using an Autochanger, restrict Volume 
    *	search to the Autochanger on the first pass 
    */
   InChanger = store->autochanger;
   /*
    * Find the Next Volume for Append
    */
   db_lock(jcr->db);
   for ( ;; ) {
      bstrncpy(mr->VolStatus, "Append", sizeof(mr->VolStatus));  /* want only appendable volumes */
      /*
       *  1. Look for volume with "Append" status.
       */
      ok = db_find_next_volume(jcr, jcr->db, 1, InChanger, mr);  
      Dmsg2(100, "catreq after find_next_vol ok=%d FW=%d\n", ok, mr->FirstWritten);
      if (!ok) {
	 /*
	  * 2. Try finding a recycled volume
	  */
	 ok = find_recycled_volume(jcr, InChanger, mr);
         Dmsg2(100, "find_recycled_volume %d FW=%d\n", ok, mr->FirstWritten);
	 if (!ok) {
	    /* 
	     * 3. Try recycling any purged volume
	     */
	    ok = recycle_oldest_purged_volume(jcr, InChanger, mr);
	    if (!ok) {
	       /*
		* 4. Try pruning Volumes
		*/
	       prune_volumes(jcr);  
	       ok = recycle_oldest_purged_volume(jcr, InChanger, mr);
	       if (InChanger) {
		  InChanger = false;
		  if (!ok) {
		     continue;		 /* retry again accepting any volume */
		  }
	       }
               Dmsg2(200, "find_recycled_volume2 %d FW=%d\n", ok, mr->FirstWritten);
	       if (!ok && create) {
		  /*
                   * 5. Try "creating" a new Volume
		   */
		  ok = newVolume(jcr, mr);
	       }
	    }
	 }

	 /* 
	  *  Look at more drastic ways to find an Appendable Volume
	  */ 
	 if (!ok && (jcr->pool->purge_oldest_volume ||
		     jcr->pool->recycle_oldest_volume)) {
            Dmsg2(200, "No next volume found. PurgeOldest=%d\n RecyleOldest=%d",
		jcr->pool->purge_oldest_volume, jcr->pool->recycle_oldest_volume);
	    /* Find oldest volume to recycle */
	    ok = db_find_next_volume(jcr, jcr->db, -1, InChanger, mr);
            Dmsg1(400, "Find oldest=%d\n", ok);
	    if (ok) {
	       UAContext *ua;
               Dmsg0(400, "Try purge.\n");
	       /* 
		* 5.  Try to purging oldest volume only if not UA calling us.
		*/
	       ua = new_ua_context(jcr);
	       if (jcr->pool->purge_oldest_volume && create) {
                  Jmsg(jcr, M_INFO, 0, _("Purging oldest volume \"%s\"\n"), mr->VolumeName);
		  ok = purge_jobs_from_volume(ua, mr);
	       /*
		* 5. or try recycling the oldest volume
		*/
	       } else if (jcr->pool->recycle_oldest_volume) {
                  Jmsg(jcr, M_INFO, 0, _("Pruning oldest volume \"%s\"\n"), mr->VolumeName);
		  ok = prune_volume(ua, mr);
	       }
	       free_ua_context(ua);
	       if (ok) {
		  ok = recycle_volume(jcr, mr);
                  Dmsg1(400, "Recycle after purge oldest=%d\n", ok);
	       }
	    }
	 }
      }
      Dmsg2(100, "VolJobs=%d FirstWritten=%d\n", mr->VolJobs, mr->FirstWritten);
      if (ok) {
	 /* If we can use the volume, check if it is expired */
	 if (has_volume_expired(jcr, mr)) {
	    if (retry++ < 200) {	    /* sanity check */
	       continue;		    /* try again from the top */
	    } else {
	       Jmsg(jcr, M_ERROR, 0, _(
"We seem to be looping trying to find the next volume. I give up.\n"));
	    }
	 }
      }
      break;
   } /* end for loop */
   db_unlock(jcr->db);
   return ok;
}

/*
 * Check if any time limits or use limits have expired
 *   if so, set the VolStatus appropriately.
 */
bool has_volume_expired(JCR *jcr, MEDIA_DBR *mr)
{
   bool expired = false;
   /*
    * Check limits and expirations if "Append" and it has been used
    * i.e. mr->VolJobs > 0
    *
    */
   if (strcmp(mr->VolStatus, "Append") == 0 && mr->VolJobs > 0) {
      /* First handle Max Volume Bytes */
      if ((mr->MaxVolBytes > 0 && mr->VolBytes >= mr->MaxVolBytes)) {
         Jmsg(jcr, M_INFO, 0, _("Max Volume bytes exceeded. "             
             "Marking Volume \"%s\" as Full.\n"), mr->VolumeName);
         bstrncpy(mr->VolStatus, "Full", sizeof(mr->VolStatus));
	 expired = true;

      /* Now see if Volume should only be used once */
      } else if (mr->VolBytes > 0 && jcr->pool->use_volume_once) {
         Jmsg(jcr, M_INFO, 0, _("Volume used once. "             
             "Marking Volume \"%s\" as Used.\n"), mr->VolumeName);
         bstrncpy(mr->VolStatus, "Used", sizeof(mr->VolStatus));
	 expired = true;

      /* Now see if Max Jobs written to volume */
      } else if (mr->MaxVolJobs > 0 && mr->MaxVolJobs <= mr->VolJobs) {
         Jmsg(jcr, M_INFO, 0, _("Max Volume jobs exceeded. "       
             "Marking Volume \"%s\" as Used.\n"), mr->VolumeName);
         bstrncpy(mr->VolStatus, "Used", sizeof(mr->VolStatus));
	 expired = true;

      /* Now see if Max Files written to volume */
      } else if (mr->MaxVolFiles > 0 && mr->MaxVolFiles <= mr->VolFiles) {
         Jmsg(jcr, M_INFO, 0, _("Max Volume files exceeded. "       
             "Marking Volume \"%s\" as Used.\n"), mr->VolumeName);
         bstrncpy(mr->VolStatus, "Used", sizeof(mr->VolStatus));
	 expired = true;

      /* Finally, check Use duration expiration */
      } else if (mr->VolUseDuration > 0) {
	 utime_t now = time(NULL);
	 /* See if Vol Use has expired */
	 if (mr->VolUseDuration <= (now - mr->FirstWritten)) {
            Jmsg(jcr, M_INFO, 0, _("Max configured use duration exceeded. "       
               "Marking Volume \"%s\" as Used.\n"), mr->VolumeName);
            bstrncpy(mr->VolStatus, "Used", sizeof(mr->VolStatus));
	    expired = true;
	 }
      }
   }
   if (expired) {
      /* Need to update media */
      if (!db_update_media_record(jcr, jcr->db, mr)) {
         Jmsg(jcr, M_ERROR, 0, _("Catalog error updating volume \"%s\". ERR=%s"),
	      mr->VolumeName, db_strerror(jcr->db));
      } 	  
   }  
   return expired;
}

/*
 * Try hard to recycle the current volume
 *
 *  Returns: on failure - reason = NULL
 *	     on success - reason - pointer to reason
 */
void check_if_volume_valid_or_recyclable(JCR *jcr, MEDIA_DBR *mr, const char **reason)
{
   int ok;

   *reason = NULL;

   /*  Check if a duration or limit has expired */
   if (has_volume_expired(jcr, mr)) {
      *reason = "volume has expired";
      /* Keep going because we may be able to recycle volume */
   }

   /*
    * Now see if we can use the volume as is
    */
   if (strcmp(mr->VolStatus, "Append") == 0 ||
       strcmp(mr->VolStatus, "Recycle") == 0) {
      *reason = NULL;
      return;
   }
											
   /*
    * Check if the Volume is already marked for recycling
    */
   if (strcmp(mr->VolStatus, "Purged") == 0) {
      if (recycle_volume(jcr, mr)) {
         Jmsg(jcr, M_INFO, 0, "Recycled current volume \"%s\"\n", mr->VolumeName);
	 *reason = NULL;
	 return;
      } else {
         /* In principle this shouldn't happen */
         *reason = "and recycling of current volume failed";
	 return;
      }
   }
											
   /* At this point, the volume is not valid for writing */
   *reason = "but should be Append, Purged or Recycle";
											
   /*
    * What we're trying to do here is see if the current volume is
    * "recyclable" - ie. if we prune all expired jobs off it, is
    * it now possible to reuse it for the job that it is currently
    * needed for?
    */
   if ((mr->LastWritten + mr->VolRetention) < (utime_t)time(NULL)
	 && mr->Recycle && jcr->pool->recycle_current_volume
         && (strcmp(mr->VolStatus, "Full") == 0 ||
            strcmp(mr->VolStatus, "Used") == 0)) {
      /*
       * Attempt prune of current volume to see if we can
       * recycle it for use.
       */
      UAContext *ua;
											
      ua = new_ua_context(jcr);
      ok = prune_volume(ua, mr);
      free_ua_context(ua);
											
      if (ok) {
	 /* If fully purged, recycle current volume */
	 if (recycle_volume(jcr, mr)) {
            Jmsg(jcr, M_INFO, 0, "Recycled current volume \"%s\"\n", mr->VolumeName);
	    *reason = NULL;
	 } else {
            *reason = "but should be Append, Purged or Recycle (recycling of the "
               "current volume failed)";
	 }
      } else {
         *reason = "but should be Append, Purged or Recycle (cannot automatically "
            "recycle current volume, as it still contains unpruned data)";
      }
   }
}
