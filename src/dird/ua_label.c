/*
 *
 *   Bacula Director -- Tape labeling commands
 *
 *     Kern Sibbald, April MMIII
 *
 *   Version $Id: ua_label.c,v 1.81 2006/12/23 16:33:52 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2006 Free Software Foundation Europe e.V.

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

/* Slot list definition */
typedef struct s_vol_list {
   struct s_vol_list *next;
   char *VolName;
   int Slot;
} vol_list_t;


/* Forward referenced functions */
static int do_label(UAContext *ua, const char *cmd, int relabel);
static void label_from_barcodes(UAContext *ua, int drive);
static bool send_label_request(UAContext *ua, MEDIA_DBR *mr, MEDIA_DBR *omr,
               POOL_DBR *pr, int relabel, bool media_record_exits, int drive);
static vol_list_t *get_vol_list_from_SD(UAContext *ua, bool scan);
static void free_vol_list(vol_list_t *vol_list);
static bool is_cleaning_tape(UAContext *ua, MEDIA_DBR *mr, POOL_DBR *pr);
static BSOCK *open_sd_bsock(UAContext *ua);
static void close_sd_bsock(UAContext *ua);
static char *get_volume_name_from_SD(UAContext *ua, int Slot, int drive);
static int get_num_slots_from_SD(UAContext *ua);


/*
 * Label a tape
 *
 *   label storage=xxx volume=vvv
 */
int label_cmd(UAContext *ua, const char *cmd)
{
   return do_label(ua, cmd, 0);       /* standard label */
}

int relabel_cmd(UAContext *ua, const char *cmd)
{
   return do_label(ua, cmd, 1);      /* relabel tape */
}

static bool get_user_slot_list(UAContext *ua, char *slot_list, int num_slots)
{
   int i;
   const char *msg;

   /* slots are numbered 1 to num_slots */
   for (int i=0; i <= num_slots; i++) {
      slot_list[i] = 0;
   }
   i = find_arg_with_value(ua, "slots");
   if (i > 0) {
      /* scan slot list in ua->argv[i] */
      char *p, *e, *h;
      int beg, end;

      strip_trailing_junk(ua->argv[i]);
      for (p=ua->argv[i]; p && *p; p=e) {
         /* Check for list */
         e = strchr(p, ',');
         if (e) {
            *e++ = 0;
         }
         /* Check for range */
         h = strchr(p, '-');             /* range? */
         if (h == p) {
            msg = _("Negative numbers not permitted\n");
            goto bail_out;
         }
         if (h) {
            *h++ = 0;
            if (!is_an_integer(h)) {
               msg = _("Range end is not integer.\n");
               goto bail_out;
            }
            skip_spaces(&p);
            if (!is_an_integer(p)) {
               msg = _("Range start is not an integer.\n");
               goto bail_out;
            }
            beg = atoi(p);
            end = atoi(h);
            if (end < beg) {
               msg = _("Range end not bigger than start.\n");
               goto bail_out;
            }
         } else {
            skip_spaces(&p);
            if (!is_an_integer(p)) {
               msg = _("Input value is not an integer.\n");
               goto bail_out;
            }
            beg = end = atoi(p);
         }
         if (beg <= 0 || end <= 0) {
            msg = _("Values must be be greater than zero.\n");
            goto bail_out;
         }
         if (end > num_slots) {
            msg = _("Slot too large.\n");
            goto bail_out;
         }
         for (i=beg; i<=end; i++) {
            slot_list[i] = 1;         /* Turn on specified range */
         }
      }
   } else {
      /* Turn everything on */
      for (i=1; i <= num_slots; i++) {
         slot_list[i] = 1;
      }
   }
   Dmsg0(100, "Slots turned on:\n");
   for (i=1; i <= num_slots; i++) {
      if (slot_list[i]) {
         Dmsg1(100, "%d\n", i);
      }
   }
   return true;

bail_out:
   return false;
}

/*
 * Update Slots corresponding to Volumes in autochanger
 */
void update_slots(UAContext *ua)
{
   USTORE store;
   vol_list_t *vl, *vol_list = NULL;
   MEDIA_DBR mr;
   char *slot_list;
   bool scan;
   int max_slots;
   int drive;
   int Enabled = 1;
   bool have_enabled;
   int i;


   if (!open_client_db(ua)) {
      return;
   }
   store.store = get_storage_resource(ua, true/*arg is storage*/);
   if (!store.store) {
      return;
   }
   pm_strcpy(store.store_source, _("command line"));
   set_wstorage(ua->jcr, &store);
   drive = get_storage_drive(ua, store.store);

   scan = find_arg(ua, NT_("scan")) >= 0;
   if ((i=find_arg_with_value(ua, NT_("Enabled"))) >= 0) {
      Enabled = get_enabled(ua, ua->argv[i]);
      if (Enabled < 0) {
         return;
      }
      have_enabled = true;
   } else {
      have_enabled = false;
   }

   max_slots = get_num_slots_from_SD(ua);
   Dmsg1(100, "max_slots=%d\n", max_slots);
   if (max_slots <= 0) {
      bsendmsg(ua, _("No slots in changer to scan.\n"));
      return;
   }
   slot_list = (char *)malloc(max_slots+1);
   if (!get_user_slot_list(ua, slot_list, max_slots)) {
      free(slot_list);
      return;
   }

   vol_list = get_vol_list_from_SD(ua, scan);

   if (!vol_list) {
      bsendmsg(ua, _("No Volumes found to label, or no barcodes.\n"));
      goto bail_out;
   }

   /* First zap out any InChanger with StorageId=0 */
   db_sql_query(ua->db, "UPDATE Media SET InChanger=0 WHERE StorageId=0", NULL, NULL);

   /* Walk through the list updating the media records */
   for (vl=vol_list; vl; vl=vl->next) {
      if (vl->Slot > max_slots) {
         bsendmsg(ua, _("Slot %d greater than max %d ignored.\n"),
            vl->Slot, max_slots);
         continue;
      }
      /* Check if user wants us to look at this slot */
      if (!slot_list[vl->Slot]) {
         Dmsg1(100, "Skipping slot=%d\n", vl->Slot);
         continue;
      }
      /* If scanning, we read the label rather than the barcode */
      if (scan) {
         if (vl->VolName) {
            free(vl->VolName);
            vl->VolName = NULL;
         }
         vl->VolName = get_volume_name_from_SD(ua, vl->Slot, drive);
         Dmsg2(100, "Got Vol=%s from SD for Slot=%d\n", vl->VolName, vl->Slot);
      }
      slot_list[vl->Slot] = 0;        /* clear Slot */
      memset(&mr, 0, sizeof(mr));
      mr.Slot = vl->Slot;
      mr.InChanger = 1;
      mr.StorageId = store.store->StorageId;
      /* Set InChanger to zero for this Slot */
      db_lock(ua->db);
      db_make_inchanger_unique(ua->jcr, ua->db, &mr);
      db_unlock(ua->db);
      if (!vl->VolName) {
         Dmsg1(000, "No VolName for Slot=%d setting InChanger to zero.\n", vl->Slot);
         bsendmsg(ua, _("No VolName for Slot=%d InChanger set to zero.\n"), vl->Slot);
         continue;
      }
      memset(&mr, 0, sizeof(mr));
      bstrncpy(mr.VolumeName, vl->VolName, sizeof(mr.VolumeName));
      db_lock(ua->db);
      if (db_get_media_record(ua->jcr, ua->db, &mr)) {
         if (mr.Slot != vl->Slot || !mr.InChanger || mr.StorageId != store.store->StorageId) {
            mr.Slot = vl->Slot;
            mr.InChanger = 1;
            mr.StorageId = store.store->StorageId;
            if (have_enabled) {
               mr.Enabled = Enabled;
            }
            if (!db_update_media_record(ua->jcr, ua->db, &mr)) {
               bsendmsg(ua, "%s", db_strerror(ua->db));
            } else {
               bsendmsg(ua, _(
                 "Catalog record for Volume \"%s\" updated to reference slot %d.\n"),
                 mr.VolumeName, mr.Slot);
            }
         } else {
            bsendmsg(ua, _("Catalog record for Volume \"%s\" is up to date.\n"),
               mr.VolumeName);
         }
         db_unlock(ua->db);
         continue;
      } else {
         bsendmsg(ua, _("Volume \"%s\" not found in catalog. Slot=%d InChanger set to zero.\n"),
             mr.VolumeName, vl->Slot);
      }
      db_unlock(ua->db);
   }
   memset(&mr, 0, sizeof(mr));
   mr.InChanger = 1;
   mr.StorageId = store.store->StorageId;
   db_lock(ua->db);
   for (int i=1; i <= max_slots; i++) {
      if (slot_list[i]) {
         mr.Slot = i;
         /* Set InChanger to zero for this Slot */
         db_make_inchanger_unique(ua->jcr, ua->db, &mr);
      }
   }
   db_unlock(ua->db);

bail_out:

   free_vol_list(vol_list);
   free(slot_list);
   close_sd_bsock(ua);

   return;
}


/*
 * Common routine for both label and relabel
 */
static int do_label(UAContext *ua, const char *cmd, int relabel)
{
   USTORE store;
   BSOCK *sd;
   char dev_name[MAX_NAME_LENGTH];
   MEDIA_DBR mr, omr;
   POOL_DBR pr;
   bool print_reminder = true;
   bool label_barcodes = false;
   int ok = FALSE;
   int i, j;
   int drive;
   bool media_record_exists = false;
   static const char *barcode_keyword[] = {
      "barcode",
      "barcodes",
      NULL};


   memset(&pr, 0, sizeof(pr));
   if (!open_client_db(ua)) {
      return 1;
   }

   /* Look for one of the barcode keywords */
   if (!relabel && (i=find_arg_keyword(ua, barcode_keyword)) >= 0) {
      /* Now find the keyword in the list */
      if ((j = find_arg(ua, barcode_keyword[i])) > 0) {
         *ua->argk[j] = 0;      /* zap barcode keyword */
      }
      label_barcodes = true;
   }

   store.store = get_storage_resource(ua, true/*use default*/);
   if (!store.store) {
      return 1;
   }
   pm_strcpy(store.store_source, _("command line"));
   set_wstorage(ua->jcr, &store);
   drive = get_storage_drive(ua, store.store);

   if (label_barcodes) {
      label_from_barcodes(ua, drive);
      return 1;
   }

   /* If relabel get name of Volume to relabel */
   if (relabel) {
      /* Check for oldvolume=name */
      i = find_arg_with_value(ua, "oldvolume");
      if (i >= 0) {
         memset(&omr, 0, sizeof(omr));
         bstrncpy(omr.VolumeName, ua->argv[i], sizeof(omr.VolumeName));
         if (db_get_media_record(ua->jcr, ua->db, &omr)) {
            goto checkVol;
         }
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      /* No keyword or Vol not found, ask user to select */
      if (!select_media_dbr(ua, &omr)) {
         return 1;
      }

      /* Require Volume to be Purged or Recycled */
checkVol:
      if (strcmp(omr.VolStatus, "Purged") != 0 && strcmp(omr.VolStatus, "Recycle") != 0) {
         bsendmsg(ua, _("Volume \"%s\" has VolStatus %s. It must be Purged or Recycled before relabeling.\n"),
            omr.VolumeName, omr.VolStatus);
         return 1;
      }
   }

   /* Check for volume=NewVolume */
   i = find_arg_with_value(ua, "volume");
   if (i >= 0) {
      pm_strcpy(ua->cmd, ua->argv[i]);
      goto checkName;
   }

   /* Get a new Volume name */
   for ( ;; ) {
      media_record_exists = false;
      if (!get_cmd(ua, _("Enter new Volume name: "))) {
         return 1;
      }
checkName:
      if (!is_volume_name_legal(ua, ua->cmd)) {
         continue;
      }

      memset(&mr, 0, sizeof(mr));
      bstrncpy(mr.VolumeName, ua->cmd, sizeof(mr.VolumeName));
      /* If VolBytes are zero the Volume is not labeled */
      if (db_get_media_record(ua->jcr, ua->db, &mr)) {
         if (mr.VolBytes != 0) {
             bsendmsg(ua, _("Media record for new Volume \"%s\" already exists.\n"),
                mr.VolumeName);
             continue;
          }
          media_record_exists = true;
      }
      break;                          /* Got it */
   }

   /* If autochanger, request slot */
   i = find_arg_with_value(ua, "slot");
   if (i >= 0) {
      mr.Slot = atoi(ua->argv[i]);
      mr.InChanger = 1;               /* assumed if we are labeling it */
   } else if (store.store->autochanger) {
      if (!get_pint(ua, _("Enter slot (0 or Enter for none): "))) {
         return 1;
      }
      mr.Slot = ua->pint32_val;
      mr.InChanger = 1;               /* assumed if we are labeling it */
   }
   mr.StorageId = store.store->StorageId;

   bstrncpy(mr.MediaType, store.store->media_type, sizeof(mr.MediaType));

   /* Must select Pool if not already done */
   if (pr.PoolId == 0) {
      memset(&pr, 0, sizeof(pr));
      if (!select_pool_dbr(ua, &pr)) {
         return 1;
      }
   }

   ok = send_label_request(ua, &mr, &omr, &pr, relabel, media_record_exists, drive);

   if (ok) {
      sd = ua->jcr->store_bsock;
      if (relabel) {
         /* Delete the old media record */
         if (!db_delete_media_record(ua->jcr, ua->db, &omr)) {
            bsendmsg(ua, _("Delete of Volume \"%s\" failed. ERR=%s"),
               omr.VolumeName, db_strerror(ua->db));
         } else {
            bsendmsg(ua, _("Old volume \"%s\" deleted from catalog.\n"),
               omr.VolumeName);
            /* Update the number of Volumes in the pool */
            pr.NumVols--;
            if (!db_update_pool_record(ua->jcr, ua->db, &pr)) {
               bsendmsg(ua, "%s", db_strerror(ua->db));
            }
         }
      }
      if (ua->automount) {
         bstrncpy(dev_name, store.store->dev_name(), sizeof(dev_name));
         bsendmsg(ua, _("Requesting to mount %s ...\n"), dev_name);
         bash_spaces(dev_name);
         bnet_fsend(sd, "mount %s drive=%d", dev_name, drive);
         unbash_spaces(dev_name);
         while (bnet_recv(sd) >= 0) {
            bsendmsg(ua, "%s", sd->msg);
            /* Here we can get
             *  3001 OK mount. Device=xxx      or
             *  3001 Mounted Volume vvvv
             *  3002 Device "DVD-Writer" (/dev/hdc) is mounted.
             *  3906 is cannot mount non-tape
             * So for those, no need to print a reminder
             */
            if (strncmp(sd->msg, "3001 ", 5) == 0 ||
                strncmp(sd->msg, "3002 ", 5) == 0 ||
                strncmp(sd->msg, "3906 ", 5) == 0) {
               print_reminder = false;
            }
         }
      }
   }
   if (print_reminder) {
      bsendmsg(ua, _("Do not forget to mount the drive!!!\n"));
   }
   close_sd_bsock(ua);

   return 1;
}

/*
 * Request SD to send us the slot:barcodes, then wiffle
 *  through them all labeling them.
 */
static void label_from_barcodes(UAContext *ua, int drive)
{
   STORE *store = ua->jcr->wstore;
   POOL_DBR pr;
   MEDIA_DBR mr, omr;
   vol_list_t *vl, *vol_list = NULL;
   bool media_record_exists;
   char *slot_list;
   int max_slots;

  
   max_slots = get_num_slots_from_SD(ua);
   if (max_slots <= 0) {
      bsendmsg(ua, _("No slots in changer to scan.\n"));
      return;
   }
   slot_list = (char *)malloc(max_slots+1);
   if (!get_user_slot_list(ua, slot_list, max_slots)) {
      goto bail_out;
   }

   vol_list = get_vol_list_from_SD(ua, false /*no scan*/);

   if (!vol_list) {
      bsendmsg(ua, _("No Volumes found to label, or no barcodes.\n"));
      goto bail_out;
   }

   /* Display list of Volumes and ask if he really wants to proceed */
   bsendmsg(ua, _("The following Volumes will be labeled:\n"
                  "Slot  Volume\n"
                  "==============\n"));
   for (vl=vol_list; vl; vl=vl->next) {
      if (!vl->VolName || !slot_list[vl->Slot]) {
         continue;
      }
      bsendmsg(ua, "%4d  %s\n", vl->Slot, vl->VolName);
   }
   if (!get_yesno(ua, _("Do you want to continue? (yes|no): ")) ||
       (ua->pint32_val == 0)) {
      goto bail_out;
   }
   /* Select a pool */
   memset(&pr, 0, sizeof(pr));
   if (!select_pool_dbr(ua, &pr)) {
      goto bail_out;
   }
   memset(&omr, 0, sizeof(omr));

   /* Fire off the label requests */
   for (vl=vol_list; vl; vl=vl->next) {
      if (!vl->VolName || !slot_list[vl->Slot]) {
         continue;
      }
      memset(&mr, 0, sizeof(mr));
      bstrncpy(mr.VolumeName, vl->VolName, sizeof(mr.VolumeName));
      media_record_exists = false;
      if (db_get_media_record(ua->jcr, ua->db, &mr)) {
          if (mr.VolBytes != 0) {
             bsendmsg(ua, _("Media record for Slot %d Volume \"%s\" already exists.\n"),
                vl->Slot, mr.VolumeName);
             mr.Slot = vl->Slot;
             mr.InChanger = 1;
             mr.StorageId = store->StorageId;
             if (!db_update_media_record(ua->jcr, ua->db, &mr)) {
                bsendmsg(ua, _("Error setting InChanger: ERR=%s"), db_strerror(ua->db));
             }
             continue;
          }
          media_record_exists = true;
      }
      mr.InChanger = 1;
      mr.StorageId = store->StorageId;
      /*
       * Deal with creating cleaning tape here. Normal tapes created in
       *  send_label_request() below
       */
      if (is_cleaning_tape(ua, &mr, &pr)) {
         if (media_record_exists) {      /* we update it */
            mr.VolBytes = 1;             /* any bytes to indicate it exists */
            bstrncpy(mr.VolStatus, "Cleaning", sizeof(mr.VolStatus));
            mr.MediaType[0] = 0;
            mr.StorageId = store->StorageId;
            if (!db_update_media_record(ua->jcr, ua->db, &mr)) {
                bsendmsg(ua, "%s", db_strerror(ua->db));
            }
         } else {                        /* create the media record */
            if (pr.MaxVols > 0 && pr.NumVols >= pr.MaxVols) {
               bsendmsg(ua, _("Maximum pool Volumes=%d reached.\n"), pr.MaxVols);
               goto bail_out;
            }
            set_pool_dbr_defaults_in_media_dbr(&mr, &pr);
            bstrncpy(mr.VolStatus, "Cleaning", sizeof(mr.VolStatus));
            mr.MediaType[0] = 0;
            if (db_create_media_record(ua->jcr, ua->db, &mr)) {
               bsendmsg(ua, _("Catalog record for cleaning tape \"%s\" successfully created.\n"),
                  mr.VolumeName);
               pr.NumVols++;          /* this is a bit suspect */
               if (!db_update_pool_record(ua->jcr, ua->db, &pr)) {
                  bsendmsg(ua, "%s", db_strerror(ua->db));
               }
            } else {
               bsendmsg(ua, _("Catalog error on cleaning tape: %s"), db_strerror(ua->db));
            }
         }
         continue;                    /* done, go handle next volume */
      }
      bstrncpy(mr.MediaType, store->media_type, sizeof(mr.MediaType));

      mr.Slot = vl->Slot;
      send_label_request(ua, &mr, &omr, &pr, 0, media_record_exists, drive);
   }


bail_out:
   free(slot_list);
   free_vol_list(vol_list);
   close_sd_bsock(ua);

   return;
}

/*
 * Check if the Volume name has legal characters
 * If ua is non-NULL send the message
 */
bool is_volume_name_legal(UAContext *ua, const char *name)
{
   int len;
   const char *p;
   const char *accept = ":.-_";

   /* Restrict the characters permitted in the Volume name */
   for (p=name; *p; p++) {
      if (B_ISALPHA(*p) || B_ISDIGIT(*p) || strchr(accept, (int)(*p))) {
         continue;
      }
      if (ua) {
         bsendmsg(ua, _("Illegal character \"%c\" in a volume name.\n"), *p);
      }
      return 0;
   }
   len = strlen(name);
   if (len >= MAX_NAME_LENGTH) {
      if (ua) {
         bsendmsg(ua, _("Volume name too long.\n"));
      }
      return 0;
   }
   if (len == 0) {
      if (ua) {
         bsendmsg(ua, _("Volume name must be at least one character long.\n"));
      }
      return 0;
   }
   return 1;
}

/*
 * NOTE! This routine opens the SD socket but leaves it open
 */
static bool send_label_request(UAContext *ua, MEDIA_DBR *mr, MEDIA_DBR *omr,
                               POOL_DBR *pr, int relabel, bool media_record_exists,
                               int drive)
{
   BSOCK *sd;
   char dev_name[MAX_NAME_LENGTH];
   bool ok = false;
   bool is_dvd = false;
   uint64_t VolBytes = 0;

   if (!(sd=open_sd_bsock(ua))) {
      return false;
   }
   bstrncpy(dev_name, ua->jcr->wstore->dev_name(), sizeof(dev_name));
   bash_spaces(dev_name);
   bash_spaces(mr->VolumeName);
   bash_spaces(mr->MediaType);
   bash_spaces(pr->Name);
   if (relabel) {
      bash_spaces(omr->VolumeName);
      bnet_fsend(sd, "relabel %s OldName=%s NewName=%s PoolName=%s "
                     "MediaType=%s Slot=%d drive=%d",
                 dev_name, omr->VolumeName, mr->VolumeName, pr->Name, 
                 mr->MediaType, mr->Slot, drive);
      bsendmsg(ua, _("Sending relabel command from \"%s\" to \"%s\" ...\n"),
         omr->VolumeName, mr->VolumeName);
   } else {
      bnet_fsend(sd, "label %s VolumeName=%s PoolName=%s MediaType=%s "
                     "Slot=%d drive=%d",
                 dev_name, mr->VolumeName, pr->Name, mr->MediaType, 
                 mr->Slot, drive);
      bsendmsg(ua, _("Sending label command for Volume \"%s\" Slot %d ...\n"),
         mr->VolumeName, mr->Slot);
      Dmsg6(100, "label %s VolumeName=%s PoolName=%s MediaType=%s Slot=%d drive=%d\n",
         dev_name, mr->VolumeName, pr->Name, mr->MediaType, mr->Slot, drive);
   }

   while (bnet_recv(sd) >= 0) {
      int dvd;
      bsendmsg(ua, "%s", sd->msg);
      if (sscanf(sd->msg, "3000 OK label. VolBytes=%llu DVD=%d ", &VolBytes,
                 &dvd) == 2) {
         is_dvd = dvd;
         ok = true;
      }
   }
   unbash_spaces(mr->VolumeName);
   unbash_spaces(mr->MediaType);
   unbash_spaces(pr->Name);
   mr->LabelDate = time(NULL);
   mr->set_label_date = true;
   if (is_dvd) {
      /* We know that a freshly labelled DVD has 1 VolParts */
      /* This does not apply to auto-labelled DVDs. */
      mr->VolParts = 1;
   }
   if (ok) {
      if (media_record_exists) {      /* we update it */
         mr->VolBytes = VolBytes;
         mr->InChanger = 1;
         mr->StorageId = ua->jcr->wstore->StorageId;
         if (!db_update_media_record(ua->jcr, ua->db, mr)) {
             bsendmsg(ua, "%s", db_strerror(ua->db));
             ok = false;
         }
      } else {                        /* create the media record */
         set_pool_dbr_defaults_in_media_dbr(mr, pr);
         mr->VolBytes = VolBytes;
         mr->InChanger = 1;
         mr->StorageId = ua->jcr->wstore->StorageId;
         mr->Enabled = 1;
         if (db_create_media_record(ua->jcr, ua->db, mr)) {
            bsendmsg(ua, _("Catalog record for Volume \"%s\", Slot %d  successfully created.\n"),
            mr->VolumeName, mr->Slot);
            /* Update number of volumes in pool */
            pr->NumVols++;
            if (!db_update_pool_record(ua->jcr, ua->db, pr)) {
               bsendmsg(ua, "%s", db_strerror(ua->db));
            }
         } else {
            bsendmsg(ua, "%s", db_strerror(ua->db));
            ok = false;
         }
      }
   } else {
      bsendmsg(ua, _("Label command failed for Volume %s.\n"), mr->VolumeName);
   }
   return ok;
}

static BSOCK *open_sd_bsock(UAContext *ua)
{
   STORE *store = ua->jcr->wstore;

   if (!ua->jcr->store_bsock) {
      bsendmsg(ua, _("Connecting to Storage daemon %s at %s:%d ...\n"),
         store->name(), store->address, store->SDport);
      if (!connect_to_storage_daemon(ua->jcr, 10, SDConnectTimeout, 1)) {
         bsendmsg(ua, _("Failed to connect to Storage daemon.\n"));
         return NULL;
      }
   }
   return ua->jcr->store_bsock;
}

static void close_sd_bsock(UAContext *ua)
{
   if (ua->jcr->store_bsock) {
      bnet_sig(ua->jcr->store_bsock, BNET_TERMINATE);
      bnet_close(ua->jcr->store_bsock);
      ua->jcr->store_bsock = NULL;
   }
}

static char *get_volume_name_from_SD(UAContext *ua, int Slot, int drive)
{
   STORE *store = ua->jcr->wstore;
   BSOCK *sd;
   char dev_name[MAX_NAME_LENGTH];
   char *VolName = NULL;
   int rtn_slot;

   if (!(sd=open_sd_bsock(ua))) {
      bsendmsg(ua, _("Could not open SD socket.\n"));
      return NULL;
   }
   bstrncpy(dev_name, store->dev_name(), sizeof(dev_name));
   bash_spaces(dev_name);
   /* Ask for autochanger list of volumes */
   bnet_fsend(sd, NT_("readlabel %s Slot=%d drive=%d\n"), dev_name, Slot, drive);
   Dmsg1(100, "Sent: %s", sd->msg);

   /* Get Volume name in this Slot */
   while (bnet_recv(sd) >= 0) {
      bsendmsg(ua, "%s", sd->msg);
      Dmsg1(100, "Got: %s", sd->msg);
      if (strncmp(sd->msg, NT_("3001 Volume="), 12) == 0) {
         VolName = (char *)malloc(sd->msglen);
         if (sscanf(sd->msg, NT_("3001 Volume=%s Slot=%d"), VolName, &rtn_slot) == 2) {
            break;
         }
         free(VolName);
         VolName = NULL;
      }
   }
   close_sd_bsock(ua);
   Dmsg1(100, "get_vol_name=%s\n", NPRT(VolName));
   return VolName;
}

/*
 * We get the slot list from the Storage daemon.
 *  If scan is set, we return all slots found,
 *  otherwise, we return only slots with valid barcodes (Volume names)
 */
static vol_list_t *get_vol_list_from_SD(UAContext *ua, bool scan)
{
   STORE *store = ua->jcr->wstore;
   char dev_name[MAX_NAME_LENGTH];
   BSOCK *sd;
   vol_list_t *vl;
   vol_list_t *vol_list = NULL;


   if (!(sd=open_sd_bsock(ua))) {
      return NULL;
   }

   bstrncpy(dev_name, store->dev_name(), sizeof(dev_name));
   bash_spaces(dev_name);
   /* Ask for autochanger list of volumes */
   bnet_fsend(sd, NT_("autochanger list %s \n"), dev_name);

   /* Read and organize list of Volumes */
   while (bnet_recv(sd) >= 0) {
      char *p;
      int Slot;
      strip_trailing_junk(sd->msg);

      /* Check for returned SD messages */
      if (sd->msg[0] == '3'     && B_ISDIGIT(sd->msg[1]) &&
          B_ISDIGIT(sd->msg[2]) && B_ISDIGIT(sd->msg[3]) &&
          sd->msg[4] == ' ') {
         bsendmsg(ua, "%s\n", sd->msg);   /* pass them on to user */
         continue;
      }

      /* Validate Slot: if scanning, otherwise  Slot:Barcode */
      p = strchr(sd->msg, ':');
      if (scan && p) {
         /* Scanning -- require only valid slot */
         Slot = atoi(sd->msg);
         if (Slot <= 0) {
            p--;
            *p = ':';
            bsendmsg(ua, _("Invalid Slot number: %s\n"), sd->msg);
            continue;
         }
      } else {
         /* Not scanning */
         if (p && strlen(p) > 1) {
            *p++ = 0;
            if (!is_an_integer(sd->msg) || (Slot=atoi(sd->msg)) <= 0) {
               p--;
               *p = ':';
               bsendmsg(ua, _("Invalid Slot number: %s\n"), sd->msg);
               continue;
            }
         } else {
            continue;
         }
         if (!is_volume_name_legal(ua, p)) {
            p--;
            *p = ':';
            bsendmsg(ua, _("Invalid Volume name: %s\n"), sd->msg);
            continue;
         }
      }

      /* Add Slot and VolumeName to list */
      vl = (vol_list_t *)malloc(sizeof(vol_list_t));
      vl->Slot = Slot;
      if (p) {
         if (*p == ':') {
            p++;                      /* skip separator */
         }
         vl->VolName = bstrdup(p);
      } else {
         vl->VolName = NULL;
      }
      Dmsg2(100, "Add slot=%d Vol=%s to SD list.\n", vl->Slot, NPRT(vl->VolName));
      if (!vol_list) {
         vl->next = vol_list;
         vol_list = vl;
      } else {
         /* Add new entry to end of list */
         for (vol_list_t *tvl=vol_list; tvl; tvl=tvl->next) {
            if (!tvl->next) {
               tvl->next = vl;
               vl->next = NULL;
               break;
            }
         }
      }
   }
   close_sd_bsock(ua);
   return vol_list;
}

static void free_vol_list(vol_list_t *vol_list)
{
   vol_list_t *vl;

   /* Free list */
   for (vl=vol_list; vl; ) {
      vol_list_t *ovl;
      if (vl->VolName) {
         free(vl->VolName);
      }
      ovl = vl;
      vl = vl->next;
      free(ovl);
   }
}

/*
 * We get the number of slots in the changer from the SD
 */
static int get_num_slots_from_SD(UAContext *ua)
{
   STORE *store = ua->jcr->wstore;
   char dev_name[MAX_NAME_LENGTH];
   BSOCK *sd;
   int slots = 0;


   if (!(sd=open_sd_bsock(ua))) {
      return 0;
   }

   bstrncpy(dev_name, store->dev_name(), sizeof(dev_name));
   bash_spaces(dev_name);
   /* Ask for autochanger number of slots */
   bnet_fsend(sd, NT_("autochanger slots %s\n"), dev_name);

   while (bnet_recv(sd) >= 0) {
      if (sscanf(sd->msg, "slots=%d\n", &slots) == 1) {
         break;
      } else {
         bsendmsg(ua, "%s", sd->msg);
      }
   }
   close_sd_bsock(ua);
   bsendmsg(ua, _("Device \"%s\" has %d slots.\n"), store->dev_name(), slots);
   return slots;
}

/*
 * We get the number of drives in the changer from the SD
 */
int get_num_drives_from_SD(UAContext *ua)
{
   STORE *store = ua->jcr->wstore;
   char dev_name[MAX_NAME_LENGTH];
   BSOCK *sd;
   int drives = 0;


   if (!(sd=open_sd_bsock(ua))) {
      return 0;
   }

   bstrncpy(dev_name, store->dev_name(), sizeof(dev_name));
   bash_spaces(dev_name);
   /* Ask for autochanger number of slots */
   bnet_fsend(sd, NT_("autochanger drives %s\n"), dev_name);

   while (bnet_recv(sd) >= 0) {
      if (sscanf(sd->msg, NT_("drives=%d\n"), &drives) == 1) {
         break;
      } else {
         bsendmsg(ua, "%s", sd->msg);
      }
   }
   close_sd_bsock(ua);
//   bsendmsg(ua, _("Device \"%s\" has %d drives.\n"), store->dev_name(), drives);
   return drives;
}




/*
 * Check if this is a cleaning tape by comparing the Volume name
 *  with the Cleaning Prefix. If they match, this is a cleaning
 *  tape.
 */
static bool is_cleaning_tape(UAContext *ua, MEDIA_DBR *mr, POOL_DBR *pr)
{
   /* Find Pool resource */
   ua->jcr->pool = (POOL *)GetResWithName(R_POOL, pr->Name);
   if (!ua->jcr->pool) {
      bsendmsg(ua, _("Pool \"%s\" resource not found for volume \"%s\"!\n"),
         pr->Name, mr->VolumeName);
      return false;
   }
   if (ua->jcr->pool->cleaning_prefix == NULL) {
      return false;
   }
   Dmsg4(100, "CLNprefix=%s: Vol=%s: len=%d strncmp=%d\n",
      ua->jcr->pool->cleaning_prefix, mr->VolumeName,
      strlen(ua->jcr->pool->cleaning_prefix),
      strncmp(mr->VolumeName, ua->jcr->pool->cleaning_prefix,
                  strlen(ua->jcr->pool->cleaning_prefix)));
   return strncmp(mr->VolumeName, ua->jcr->pool->cleaning_prefix,
                  strlen(ua->jcr->pool->cleaning_prefix)) == 0;
}
