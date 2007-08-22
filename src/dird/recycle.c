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
 *   Bacula Director -- Automatic Recycling of Volumes
 *      Recycles Volumes that have been purged
 *
 *     Kern Sibbald, May MMII
 *
 *   Version $Id: recycle.c 5124 2007-07-06 20:03:36Z kerns $
 */


#include "bacula.h"
#include "dird.h"
#include "ua.h"

struct s_oldest_ctx {
   uint32_t MediaId;
   char LastWritten[30];
};

static int oldest_handler(void *ctx, int num_fields, char **row)
{
   struct s_oldest_ctx *oldest = (struct s_oldest_ctx *)ctx;

   if (row[0]) {
      oldest->MediaId = str_to_int64(row[0]);
      bstrncpy(oldest->LastWritten, row[1]?row[1]:"", sizeof(oldest->LastWritten));
      Dmsg1(100, "New oldest %s\n", row[1]?row[1]:"");
   }
   return 1;
}

/* Forward referenced functions */

bool find_recycled_volume(JCR *jcr, bool InChanger, MEDIA_DBR *mr)
{
   bstrncpy(mr->VolStatus, "Recycle", sizeof(mr->VolStatus));
   if (db_find_next_volume(jcr, jcr->db, 1, InChanger, mr)) {
      jcr->MediaId = mr->MediaId;
      Dmsg1(20, "Find_next_vol MediaId=%u\n", jcr->MediaId);
      pm_strcpy(jcr->VolumeName, mr->VolumeName);
      return true;
   }
   return false;
}


/*
 *   Look for oldest Purged volume
 */
bool recycle_oldest_purged_volume(JCR *jcr, bool InChanger, MEDIA_DBR *mr)
{
   struct s_oldest_ctx oldest;
   char ed1[50];
   POOLMEM *query = get_pool_memory(PM_EMSG);
   const char *select =
          "SELECT MediaId,LastWritten FROM Media "
          "WHERE PoolId=%s AND Recycle=1 AND VolStatus='Purged' "
          "AND Enabled=1 AND MediaType='%s' %s"
          "ORDER BY LastWritten ASC,MediaId LIMIT 1";

   Dmsg0(100, "Enter recycle_oldest_purged_volume\n");
   oldest.MediaId = 0;
   if (InChanger) {
      char changer[100];
      bsnprintf(changer, sizeof(changer), "AND InChanger=1 AND StorageId=%s ",
         edit_int64(mr->StorageId, ed1));
      Mmsg(query, select, edit_int64(mr->PoolId, ed1), mr->MediaType, changer);
   } else {
      Mmsg(query, select, edit_int64(mr->PoolId, ed1), mr->MediaType, "");
   }

   if (!db_sql_query(jcr->db, query, oldest_handler, (void *)&oldest)) {
      Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
      Dmsg0(100, "return 0  recycle_oldest_purged_volume query\n");
      free_pool_memory(query);
      return false;
   }
   free_pool_memory(query);
   Dmsg1(100, "Oldest mediaid=%d\n", oldest.MediaId);
   if (oldest.MediaId != 0) {
      mr->MediaId = oldest.MediaId;
      if (db_get_media_record(jcr, jcr->db, mr)) {
         if (recycle_volume(jcr, mr)) {
            Jmsg(jcr, M_INFO, 0, _("Recycled volume \"%s\"\n"), mr->VolumeName);
            Dmsg1(100, "return 1  recycle_oldest_purged_volume Vol=%s\n", mr->VolumeName);
            return true;
         }
      }
      Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
   }
   Dmsg0(100, "return 0  recycle_oldest_purged_volume end\n");
   return false;
}

/*
 * Recycle the specified volume
 */
int recycle_volume(JCR *jcr, MEDIA_DBR *mr)
{
   bstrncpy(mr->VolStatus, "Recycle", sizeof(mr->VolStatus));
   mr->VolJobs = mr->VolFiles = mr->VolBlocks = mr->VolErrors = 0;
   mr->VolBytes = 1;
   mr->FirstWritten = mr->LastWritten = 0;
   mr->RecycleCount++;
   mr->set_first_written = true;
   return db_update_media_record(jcr, jcr->db, mr);
}
