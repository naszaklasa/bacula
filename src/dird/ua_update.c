/*
 *
 *   Bacula Director -- Update command processing
 *     Split from ua_cmds.c March 2005
 *
 *     Kern Sibbald, September MM
 *
 *   Version $Id: ua_update.c,v 1.29 2006/12/23 16:33:52 kerns Exp $
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

#include "bacula.h"
#include "dird.h"

/* Forward referenced functions */
static int update_volume(UAContext *ua);
static bool update_pool(UAContext *ua);

/*
 * Update a Pool Record in the database.
 *  It is always updated from the Resource record.
 *
 *    update pool=<pool-name>
 *         updates pool from Pool resource
 *    update media pool=<pool-name> volume=<volume-name>
 *         changes pool info for volume
 *    update slots [scan=...]
 *         updates autochanger slots
 */
int update_cmd(UAContext *ua, const char *cmd)
{
   static const char *kw[] = {
      NT_("media"),  /* 0 */
      NT_("volume"), /* 1 */
      NT_("pool"),   /* 2 */
      NT_("slots"),  /* 3 */
      NULL};

   if (!open_client_db(ua)) {
      return 1;
   }

   switch (find_arg_keyword(ua, kw)) {
   case 0:
   case 1:
      update_volume(ua);
      return 1;
   case 2:
      update_pool(ua);
      return 1;
   case 3:
      update_slots(ua);
      return 1;
   default:
      break;
   }

   start_prompt(ua, _("Update choice:\n"));
   add_prompt(ua, _("Volume parameters"));
   add_prompt(ua, _("Pool from resource"));
   add_prompt(ua, _("Slots from autochanger"));
   switch (do_prompt(ua, _("item"), _("Choose catalog item to update"), NULL, 0)) {
   case 0:
      update_volume(ua);
      break;
   case 1:
      update_pool(ua);
      break;
   case 2:
      update_slots(ua);
      break;
   default:
      break;
   }
   return 1;
}

static void update_volstatus(UAContext *ua, const char *val, MEDIA_DBR *mr)
{
   POOL_MEM query(PM_MESSAGE);
   const char *kw[] = {
      NT_("Append"),
      NT_("Archive"),
      NT_("Disabled"),
      NT_("Full"),
      NT_("Used"),
      NT_("Cleaning"),
      NT_("Recycle"),
      NT_("Read-Only"),
      NT_("Error"),
      NULL};
   bool found = false;
   int i;

   for (i=0; kw[i]; i++) {
      if (strcasecmp(val, kw[i]) == 0) {
         found = true;
         break;
      }
   }
   if (!found) {
      bsendmsg(ua, _("Invalid VolStatus specified: %s\n"), val);
   } else {
      char ed1[50];
      bstrncpy(mr->VolStatus, kw[i], sizeof(mr->VolStatus));
      Mmsg(query, "UPDATE Media SET VolStatus='%s' WHERE MediaId=%s",
         mr->VolStatus, edit_int64(mr->MediaId,ed1));
      if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      } else {
         bsendmsg(ua, _("New Volume status is: %s\n"), mr->VolStatus);
      }
   }
}

static void update_volretention(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   char ed1[150], ed2[50];
   POOL_MEM query(PM_MESSAGE);
   if (!duration_to_utime(val, &mr->VolRetention)) {
      bsendmsg(ua, _("Invalid retention period specified: %s\n"), val);
      return;
   }
   Mmsg(query, "UPDATE Media SET VolRetention=%s WHERE MediaId=%s",
      edit_uint64(mr->VolRetention, ed1), edit_int64(mr->MediaId,ed2));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New retention period is: %s\n"),
         edit_utime(mr->VolRetention, ed1, sizeof(ed1)));
   }
}

static void update_voluseduration(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   char ed1[150], ed2[50];
   POOL_MEM query(PM_MESSAGE);

   if (!duration_to_utime(val, &mr->VolUseDuration)) {
      bsendmsg(ua, _("Invalid use duration specified: %s\n"), val);
      return;
   }
   Mmsg(query, "UPDATE Media SET VolUseDuration=%s WHERE MediaId=%s",
      edit_uint64(mr->VolUseDuration, ed1), edit_int64(mr->MediaId,ed2));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New use duration is: %s\n"),
         edit_utime(mr->VolUseDuration, ed1, sizeof(ed1)));
   }
}

static void update_volmaxjobs(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   POOL_MEM query(PM_MESSAGE);
   char ed1[50];
   Mmsg(query, "UPDATE Media SET MaxVolJobs=%s WHERE MediaId=%s",
      val, edit_int64(mr->MediaId,ed1));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New max jobs is: %s\n"), val);
   }
}

static void update_volmaxfiles(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   POOL_MEM query(PM_MESSAGE);
   char ed1[50];
   Mmsg(query, "UPDATE Media SET MaxVolFiles=%s WHERE MediaId=%s",
      val, edit_int64(mr->MediaId, ed1));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New max files is: %s\n"), val);
   }
}

static void update_volmaxbytes(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   uint64_t maxbytes;
   char ed1[50], ed2[50];
   POOL_MEM query(PM_MESSAGE);

   if (!size_to_uint64(val, strlen(val), &maxbytes)) {
      bsendmsg(ua, _("Invalid max. bytes specification: %s\n"), val);
      return;
   }
   Mmsg(query, "UPDATE Media SET MaxVolBytes=%s WHERE MediaId=%s",
      edit_uint64(maxbytes, ed1), edit_int64(mr->MediaId, ed2));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New Max bytes is: %s\n"), edit_uint64(maxbytes, ed1));
   }
}

static void update_volrecycle(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   int recycle;
   char ed1[50];

   POOL_MEM query(PM_MESSAGE);
   if (!is_yesno(val, &recycle)) {
      bsendmsg(ua, _("Invalid value. It must be yes or no.\n"));
      return;
   }
   Mmsg(query, "UPDATE Media SET Recycle=%d WHERE MediaId=%s",
      recycle, edit_int64(mr->MediaId, ed1));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New Recycle flag is: %s\n"),
         mr->Recycle==1?_("yes"):_("no"));
   }
}

static void update_volinchanger(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   int InChanger;
   char ed1[50];

   POOL_MEM query(PM_MESSAGE);
   if (!is_yesno(val, &InChanger)) {
      bsendmsg(ua, _("Invalid value. It must be yes or no.\n"));
      return;
   }
   Mmsg(query, "UPDATE Media SET InChanger=%d WHERE MediaId=%s",
      InChanger, edit_int64(mr->MediaId, ed1));
   if (!db_sql_query(ua->db, query.c_str(), NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New InChanger flag is: %s\n"),
         mr->InChanger==1?_("yes"):_("no"));
   }
}


static void update_volslot(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   POOL_DBR pr;

   memset(&pr, 0, sizeof(POOL_DBR));
   pr.PoolId = mr->PoolId;
   if (!db_get_pool_record(ua->jcr, ua->db, &pr)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      return;
   }
   mr->Slot = atoi(val);
   if (pr.MaxVols > 0 && mr->Slot > (int)pr.MaxVols) {
      bsendmsg(ua, _("Invalid slot, it must be between 0 and MaxVols=%d\n"),
         pr.MaxVols);
      return;
   }
   /*
    * Make sure to use db_update... rather than doing this directly,
    *   so that any Slot is handled correctly.
    */
   if (!db_update_media_record(ua->jcr, ua->db, mr)) {
      bsendmsg(ua, _("Error updating media record Slot: ERR=%s"), db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New Slot is: %d\n"), mr->Slot);
   }
}

/* Modify the Pool in which this Volume is located */
static void update_vol_pool(UAContext *ua, char *val, MEDIA_DBR *mr, POOL_DBR *opr)
{
   POOL_DBR pr;
   POOLMEM *query;
   char ed1[50], ed2[50];

   memset(&pr, 0, sizeof(pr));
   bstrncpy(pr.Name, val, sizeof(pr.Name));
   if (!get_pool_dbr(ua, &pr)) {
      return;
   }
   mr->PoolId = pr.PoolId;            /* set new PoolId */
   /*
    */
   query = get_pool_memory(PM_MESSAGE);
   db_lock(ua->db);
   Mmsg(query, "UPDATE Media SET PoolId=%s WHERE MediaId=%s",
      edit_int64(mr->PoolId, ed1),
      edit_int64(mr->MediaId, ed2));
   if (!db_sql_query(ua->db, query, NULL, NULL)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New Pool is: %s\n"), pr.Name);
      opr->NumVols--;
      if (!db_update_pool_record(ua->jcr, ua->db, opr)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      pr.NumVols++;
      if (!db_update_pool_record(ua->jcr, ua->db, &pr)) {
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
   }
   db_unlock(ua->db);
   free_pool_memory(query);
}

/*
 * Refresh the Volume information from the Pool record
 */
static void update_vol_from_pool(UAContext *ua, MEDIA_DBR *mr)
{
   POOL_DBR pr;

   memset(&pr, 0, sizeof(pr));
   pr.PoolId = mr->PoolId;
   if (!db_get_pool_record(ua->jcr, ua->db, &pr) ||
       !acl_access_ok(ua, Pool_ACL, pr.Name)) {
      return;
   }
   set_pool_dbr_defaults_in_media_dbr(mr, &pr);
   if (!db_update_media_defaults(ua->jcr, ua->db, mr)) {
      bsendmsg(ua, _("Error updating Volume record: ERR=%s"), db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("Volume defaults updated from \"%s\" Pool record.\n"),
         pr.Name);
   }
}

/*
 * Refresh the Volume information from the Pool record
 *   for all Volumes
 */
static void update_all_vols_from_pool(UAContext *ua)
{
   POOL_DBR pr;
   MEDIA_DBR mr;

   memset(&pr, 0, sizeof(pr));
   memset(&mr, 0, sizeof(mr));
   if (!get_pool_dbr(ua, &pr)) {
      return;
   }
   set_pool_dbr_defaults_in_media_dbr(&mr, &pr);
   mr.PoolId = pr.PoolId;
   if (!db_update_media_defaults(ua->jcr, ua->db, &mr)) {
      bsendmsg(ua, _("Error updating Volume records: ERR=%s"), db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("All Volume defaults updated from Pool record.\n"));
   }
}

static void update_volenabled(UAContext *ua, char *val, MEDIA_DBR *mr)
{
   mr->Enabled = get_enabled(ua, val);
   if (mr->Enabled < 0) {
      return;
   }
   if (!db_update_media_record(ua->jcr, ua->db, mr)) {
      bsendmsg(ua, _("Error updating media record Enabled: ERR=%s"), db_strerror(ua->db));
   } else {
      bsendmsg(ua, _("New Enabled is: %d\n"), mr->Enabled);
   }
}



/*
 * Update a media record -- allows you to change the
 *  Volume status. E.g. if you want Bacula to stop
 *  writing on the volume, set it to anything other
 *  than Append.
 */
static int update_volume(UAContext *ua)
{
   MEDIA_DBR mr;
   POOL_DBR pr;
   POOLMEM *query;
   char ed1[130];
   bool done = false;
   int i;
   const char *kw[] = {
      _("VolStatus"),                /* 0 */
      _("VolRetention"),             /* 1 */
      _("VolUse"),                   /* 2 */
      _("MaxVolJobs"),               /* 3 */
      _("MaxVolFiles"),              /* 4 */
      _("MaxVolBytes"),              /* 5 */
      _("Recycle"),                  /* 6 */
      _("InChanger"),                /* 7 */
      _("Slot"),                     /* 8 */
      _("Pool"),                     /* 9 */
      _("FromPool"),                 /* 10 */
      _("AllFromPool"),              /* 11 !!! see below !!! */
      _("Enabled"),                  /* 12 */
      NULL };

#define AllFromPool 11               /* keep this updated with above */

   for (i=0; kw[i]; i++) {
      int j;
      POOL_DBR pr;
      if ((j=find_arg_with_value(ua, kw[i])) > 0) {
         /* If all from pool don't select a media record */
         if (i != AllFromPool && !select_media_dbr(ua, &mr)) {
            return 0;
         }
         switch (i) {
         case 0:
            update_volstatus(ua, ua->argv[j], &mr);
            break;
         case 1:
            update_volretention(ua, ua->argv[j], &mr);
            break;
         case 2:
            update_voluseduration(ua, ua->argv[j], &mr);
            break;
         case 3:
            update_volmaxjobs(ua, ua->argv[j], &mr);
            break;
         case 4:
            update_volmaxfiles(ua, ua->argv[j], &mr);
            break;
         case 5:
            update_volmaxbytes(ua, ua->argv[j], &mr);
            break;
         case 6:
            update_volrecycle(ua, ua->argv[j], &mr);
            break;
         case 7:
            update_volinchanger(ua, ua->argv[j], &mr);
            break;
         case 8:
            update_volslot(ua, ua->argv[j], &mr);
            break;
         case 9:
            memset(&pr, 0, sizeof(POOL_DBR));
            pr.PoolId = mr.PoolId;
            if (!db_get_pool_record(ua->jcr, ua->db, &pr)) {
               bsendmsg(ua, "%s", db_strerror(ua->db));
               break;
            }
            update_vol_pool(ua, ua->argv[j], &mr, &pr);
            break;
         case 10:
            update_vol_from_pool(ua, &mr);
            return 1;
         case 11:
            update_all_vols_from_pool(ua);
            return 1;
         case 12:
            update_volenabled(ua, ua->argv[j], &mr);
            break;
         }
         done = true;
      }
   }

   for ( ; !done; ) {
      start_prompt(ua, _("Parameters to modify:\n"));
      add_prompt(ua, _("Volume Status"));              /* 0 */
      add_prompt(ua, _("Volume Retention Period"));    /* 1 */
      add_prompt(ua, _("Volume Use Duration"));        /* 2 */
      add_prompt(ua, _("Maximum Volume Jobs"));        /* 3 */
      add_prompt(ua, _("Maximum Volume Files"));       /* 4 */
      add_prompt(ua, _("Maximum Volume Bytes"));       /* 5 */
      add_prompt(ua, _("Recycle Flag"));               /* 6 */
      add_prompt(ua, _("Slot"));                       /* 7 */
      add_prompt(ua, _("InChanger Flag"));             /* 8 */
      add_prompt(ua, _("Volume Files"));               /* 9 */
      add_prompt(ua, _("Pool"));                       /* 10 */
      add_prompt(ua, _("Volume from Pool"));           /* 11 */
      add_prompt(ua, _("All Volumes from Pool"));      /* 12 */
      add_prompt(ua, _("Enabled")),                    /* 13 */
      add_prompt(ua, _("Done"));                       /* 14 */
      i = do_prompt(ua, "", _("Select parameter to modify"), NULL, 0);  

      /* For All Volumes from Pool and Done, we don't need a Volume record */
      if (i != 12 && i != 14) {
         if (!select_media_dbr(ua, &mr)) {  /* Get Volume record */
            return 0;
         }
         bsendmsg(ua, _("Updating Volume \"%s\"\n"), mr.VolumeName);
      }
      switch (i) {
      case 0:                         /* Volume Status */
         /* Modify Volume Status */
         bsendmsg(ua, _("Current Volume status is: %s\n"), mr.VolStatus);
         start_prompt(ua, _("Possible Values are:\n"));
         add_prompt(ua, NT_("Append")); 
         add_prompt(ua, NT_("Archive"));
         add_prompt(ua, NT_("Disabled"));
         add_prompt(ua, NT_("Full"));
         add_prompt(ua, NT_("Used"));
         add_prompt(ua, NT_("Cleaning"));
         if (strcmp(mr.VolStatus, NT_("Purged")) == 0) {
            add_prompt(ua, NT_("Recycle"));
         }
         add_prompt(ua, NT_("Read-Only"));
         if (do_prompt(ua, "", _("Choose new Volume Status"), ua->cmd, sizeof(mr.VolStatus)) < 0) {
            return 1;
         }
         update_volstatus(ua, ua->cmd, &mr);
         break;
      case 1:                         /* Retention */
         bsendmsg(ua, _("Current retention period is: %s\n"),
            edit_utime(mr.VolRetention, ed1, sizeof(ed1)));
         if (!get_cmd(ua, _("Enter Volume Retention period: "))) {
            return 0;
         }
         update_volretention(ua, ua->cmd, &mr);
         break;

      case 2:                         /* Use Duration */
         bsendmsg(ua, _("Current use duration is: %s\n"),
            edit_utime(mr.VolUseDuration, ed1, sizeof(ed1)));
         if (!get_cmd(ua, _("Enter Volume Use Duration: "))) {
            return 0;
         }
         update_voluseduration(ua, ua->cmd, &mr);
         break;

      case 3:                         /* Max Jobs */
         bsendmsg(ua, _("Current max jobs is: %u\n"), mr.MaxVolJobs);
         if (!get_pint(ua, _("Enter new Maximum Jobs: "))) {
            return 0;
         }
         update_volmaxjobs(ua, ua->cmd, &mr);
         break;

      case 4:                         /* Max Files */
         bsendmsg(ua, _("Current max files is: %u\n"), mr.MaxVolFiles);
         if (!get_pint(ua, _("Enter new Maximum Files: "))) {
            return 0;
         }
         update_volmaxfiles(ua, ua->cmd, &mr);
         break;

      case 5:                         /* Max Bytes */
         bsendmsg(ua, _("Current value is: %s\n"), edit_uint64(mr.MaxVolBytes, ed1));
         if (!get_cmd(ua, _("Enter new Maximum Bytes: "))) {
            return 0;
         }
         update_volmaxbytes(ua, ua->cmd, &mr);
         break;


      case 6:                         /* Recycle */
         bsendmsg(ua, _("Current recycle flag is: %s\n"),
            mr.Recycle==1?_("yes"):_("no"));
         if (!get_yesno(ua, _("Enter new Recycle status: "))) {
            return 0;
         }
         update_volrecycle(ua, ua->cmd, &mr);
         break;

      case 7:                         /* Slot */
         bsendmsg(ua, _("Current Slot is: %d\n"), mr.Slot);
         if (!get_pint(ua, _("Enter new Slot: "))) {
            return 0;
         }
         update_volslot(ua, ua->cmd, &mr);
         break;
         
      case 8:                         /* InChanger */
         bsendmsg(ua, _("Current InChanger flag is: %d\n"), mr.InChanger);
         if (!get_yesno(ua, _("Set InChanger flag? yes/no: "))) {
            return 0;
         }
         mr.InChanger = ua->pint32_val;
         /*
          * Make sure to use db_update... rather than doing this directly,
          *   so that any Slot is handled correctly.
          */
         if (!db_update_media_record(ua->jcr, ua->db, &mr)) {
            bsendmsg(ua, _("Error updating media record Slot: ERR=%s"), db_strerror(ua->db));
         } else {
            bsendmsg(ua, _("New InChanger flag is: %d\n"), mr.InChanger);
         }
         break;


      case 9:                         /* Volume Files */
         int32_t VolFiles;
         bsendmsg(ua, _("Warning changing Volume Files can result\n"
                        "in loss of data on your Volume\n\n"));
         bsendmsg(ua, _("Current Volume Files is: %u\n"), mr.VolFiles);
         if (!get_pint(ua, _("Enter new number of Files for Volume: "))) {
            return 0;
         }
         VolFiles = ua->pint32_val;
         if (VolFiles != (int)(mr.VolFiles + 1)) {
            bsendmsg(ua, _("Normally, you should only increase Volume Files by one!\n"));
            if (!get_yesno(ua, _("Continue? (yes/no): ")) || ua->pint32_val == 0) {
               break;
            }
         }
         query = get_pool_memory(PM_MESSAGE);
         Mmsg(query, "UPDATE Media SET VolFiles=%u WHERE MediaId=%s",
            VolFiles, edit_int64(mr.MediaId, ed1));
         if (!db_sql_query(ua->db, query, NULL, NULL)) {
            bsendmsg(ua, "%s", db_strerror(ua->db));
         } else {
            bsendmsg(ua, _("New Volume Files is: %u\n"), VolFiles);
         }
         free_pool_memory(query);
         break;

      case 10:                        /* Volume's Pool */
         memset(&pr, 0, sizeof(POOL_DBR));
         pr.PoolId = mr.PoolId;
         if (!db_get_pool_record(ua->jcr, ua->db, &pr)) {
            bsendmsg(ua, "%s", db_strerror(ua->db));
            return 0;
         }
         bsendmsg(ua, _("Current Pool is: %s\n"), pr.Name);
         if (!get_cmd(ua, _("Enter new Pool name: "))) {
            return 0;
         }
         update_vol_pool(ua, ua->cmd, &mr, &pr);
         return 1;

      case 11:
         update_vol_from_pool(ua, &mr);
         return 1;
      case 12:
         update_all_vols_from_pool(ua);
         return 1;

      case 13:
         bsendmsg(ua, _("Current Enabled is: %d\n"), mr.Enabled);
         if (!get_cmd(ua, _("Enter new Enabled: "))) {
            return 0;
         }
         if (strcasecmp(ua->cmd, "yes") == 0 || strcasecmp(ua->cmd, "true") == 0) {
            mr.Enabled = 1;
         } else if (strcasecmp(ua->cmd, "no") == 0 || strcasecmp(ua->cmd, "false") == 0) {
            mr.Enabled = 0;
         } else if (strcasecmp(ua->cmd, "archived") == 0) { 
            mr.Enabled = 2;
         } else {
            mr.Enabled = atoi(ua->cmd);
         }
         update_volenabled(ua, ua->cmd, &mr);
         break;

      default:                        /* Done or error */
         bsendmsg(ua, _("Selection terminated.\n"));
         return 1;
      }
   }
   return 1;
}

/*
 * Update pool record -- pull info from current POOL resource
 */
static bool update_pool(UAContext *ua)
{
   POOL_DBR  pr;
   int id;
   POOL *pool;
   POOLMEM *query;
   char ed1[50];

   pool = get_pool_resource(ua);
   if (!pool) {
      return false;
   }

   memset(&pr, 0, sizeof(pr));
   bstrncpy(pr.Name, pool->hdr.name, sizeof(pr.Name));
   if (!get_pool_dbr(ua, &pr)) {
      return false;
   }

   set_pooldbr_from_poolres(&pr, pool, POOL_OP_UPDATE); /* update */

   id = db_update_pool_record(ua->jcr, ua->db, &pr);
   if (id <= 0) {
      bsendmsg(ua, _("db_update_pool_record returned %d. ERR=%s\n"),
         id, db_strerror(ua->db));
   }
   query = get_pool_memory(PM_MESSAGE);
   Mmsg(query, list_pool, edit_int64(pr.PoolId, ed1));
   db_list_sql_query(ua->jcr, ua->db, query, prtit, ua, 1, HORZ_LIST);
   free_pool_memory(query);
   bsendmsg(ua, _("Pool DB record updated from resource.\n"));
   return true;
}
