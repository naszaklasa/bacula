/*
 *
 *  Routines for handling the autochanger.
 *
 *   Kern Sibbald, August MMII
 *			      
 *   Version $Id: autochanger.c,v 1.24 2004/11/22 09:25:19 kerns Exp $
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

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Forward referenced functions */
char *edit_device_codes(JCR *jcr, char *omsg, const char *imsg, const char *cmd);
static int get_autochanger_loaded_slot(JCR *jcr);


/*
 * Called here to do an autoload using the autochanger, if
 *  configured, and if a Slot has been defined for this Volume.
 *  On success this routine loads the indicated tape, but the
 *  label is not read, so it must be verified.
 *
 *  Note if dir is not NULL, it is the console requesting the 
 *   autoload for labeling, so we respond directly to the
 *   dir bsock.
 *
 *  Returns: 1 on success
 *	     0 on failure (no changer available) 
 *	    -1 on error on autochanger
 */
int autoload_device(DCR *dcr, int writing, BSOCK *dir)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   int slot;
   int drive = jcr->device->drive_index;
   int rtn_stat = -1;		      /* error status */
     
   slot = dcr->VolCatInfo.InChanger ? dcr->VolCatInfo.Slot : 0;
   /*
    * Handle autoloaders here.	If we cannot autoload it, we
    *  will return FALSE to ask the sysop.
    */
   if (writing && dev_cap(dev, CAP_AUTOCHANGER) && slot <= 0) {
      if (dir) {
	 return 0;		      /* For user, bail out right now */
      }
      if (dir_find_next_appendable_volume(dcr)) {
	 slot = dcr->VolCatInfo.InChanger ? dcr->VolCatInfo.Slot : 0;
      } else {
	 slot = 0;
      }
   }
   Dmsg1(400, "Want changer slot=%d\n", slot);

   if (slot > 0 && jcr->device->changer_name && jcr->device->changer_command) {
      uint32_t timeout = jcr->device->max_changer_wait;
      POOLMEM *changer;
      int loaded, status;     

      changer = get_pool_memory(PM_FNAME);

      loaded = get_autochanger_loaded_slot(jcr);

      /* If tape we want is not loaded, load it. */
      if (loaded != slot) { 
	 offline_or_rewind_dev(dev);
	 /* We are going to load a new tape, so close the device */
	 force_close_dev(dev);
	 if (loaded != 0 && loaded != -1) {	   /* must unload drive */
            Dmsg0(400, "Doing changer unload.\n");
	    Jmsg(jcr, M_INFO, 0, 
                 _("3303 Issuing autochanger \"unload slot %d, drive %d\" command.\n"),
		 loaded, drive);
	    dcr->VolCatInfo.Slot = loaded;   /* slot to be unloaded */
	    changer = edit_device_codes(jcr, changer, 
                        jcr->device->changer_command, "unload");
	    status = run_program(changer, timeout, NULL);
	    if (status != 0) {
	       berrno be;
	       be.set_errno(status);
               Jmsg(jcr, M_INFO, 0, _("3992 Bad autochanger \"unload slot %d, drive %d\": ERR=%s.\n"),
		    slot, drive, be.strerror());
	    }

            Dmsg1(400, "unload status=%d\n", status);
	 }
	 /*
	  * Load the desired cassette	 
	  */
         Dmsg1(400, "Doing changer load slot %d\n", slot);
	 Jmsg(jcr, M_INFO, 0, 
              _("3304 Issuing autochanger \"load slot %d, drive %d\" command.\n"), 
	      slot, drive);
	 dcr->VolCatInfo.Slot = slot;	 /* slot to be loaded */
	 changer = edit_device_codes(jcr, changer, 
                      jcr->device->changer_command, "load");
	 status = run_program(changer, timeout, NULL);
	 if (status == 0) {
            Jmsg(jcr, M_INFO, 0, _("3305 Autochanger \"load slot %d, drive %d\", status is OK.\n"),
		    slot, drive);
	 } else {
	   berrno be;
	   be.set_errno(status);
            Jmsg(jcr, M_INFO, 0, _("3992 Bad autochanger \"load slot %d, drive %d\": ERR=%s.\n"),
		    slot, drive, be.strerror());
	 }
         Dmsg2(400, "load slot %d status=%d\n", slot, status);
      } else { 
	 status = 0;		      /* we got what we want */
      }
      free_pool_memory(changer);
      Dmsg1(400, "After changer, status=%d\n", status);
      if (status == 0) {	      /* did we succeed? */
	 rtn_stat = 1;		      /* tape loaded by changer */
      }
   } else {
      rtn_stat = 0;		      /* no changer found */
   }
   return rtn_stat;
}

static int get_autochanger_loaded_slot(JCR *jcr)
{
   POOLMEM *changer, *results;
   int status, loaded;
   uint32_t timeout = jcr->device->max_changer_wait;
   int drive = jcr->device->drive_index;

   results = get_pool_memory(PM_MESSAGE);
   changer = get_pool_memory(PM_FNAME);

   /* Find out what is loaded, zero means device is unloaded */
   Jmsg(jcr, M_INFO, 0, _("3301 Issuing autochanger \"loaded drive %d\" command.\n"),
	drive);
   changer = edit_device_codes(jcr, changer, jcr->device->changer_command, 
                "loaded");
   status = run_program(changer, timeout, results);
   Dmsg3(100, "run_prog: %s stat=%d result=%s\n", changer, status, results);
   if (status == 0) {
      loaded = atoi(results);
      if (loaded > 0) {
         Jmsg(jcr, M_INFO, 0, _("3302 Autochanger \"loaded drive %d\", result is Slot %d.\n"),
	      drive, loaded);
      } else {
         Jmsg(jcr, M_INFO, 0, _("3302 Autochanger \"loaded drive %d\", result: nothing loaded.\n"),
	      drive);
      }
   } else {
      berrno be;
      be.set_errno(status);
      Jmsg(jcr, M_INFO, 0, _("3991 Bad autochanger \"loaded drive %d\" command: ERR=%s.\n"), 
	   drive, be.strerror());
      loaded = -1;		/* force unload */
   }
   free_pool_memory(changer);
   free_pool_memory(results);
   return loaded;
}

/*
 * The Volume is not in the correct slot, so mark this 
 *   Volume as not being in the Changer.
 */
void invalid_slot_in_catalog(DCR *dcr)
{
   JCR *jcr = dcr->jcr;
   DEVICE *dev = dcr->dev;
   Jmsg(jcr, M_ERROR, 0, _("Autochanger Volume \"%s\" not found in slot %d.\n"
"    Setting slot to zero in catalog.\n"),
	dcr->VolCatInfo.VolCatName, dcr->VolCatInfo.Slot);
   dcr->VolCatInfo.InChanger = false;
   dev->VolCatInfo.InChanger = false;
   Dmsg0(100, "update vol info in mount\n");
   dir_update_volume_info(dcr, true);  /* set new status */
}

/*
 * List the Volumes that are in the autoloader possibly
 *   with their barcodes.
 *   We assume that it is always the Console that is calling us.
 */
bool autochanger_list(DCR *dcr, BSOCK *dir)
{
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;
   uint32_t timeout = jcr->device->max_changer_wait;
   POOLMEM *changer;
   BPIPE *bpipe;
   int slot, loaded;
   int len = sizeof_pool_memory(dir->msg) - 1;

   if (!dev_cap(dev, CAP_AUTOCHANGER) || !jcr->device->changer_name ||
       !jcr->device->changer_command) {
      bnet_fsend(dir, _("3993 Not a autochanger device.\n"));
      return false;
   }

   changer = get_pool_memory(PM_FNAME);
   offline_or_rewind_dev(dev);
   /* We are going to load a new tape, so close the device */
   force_close_dev(dev);

   /* First unload any tape */
   loaded = get_autochanger_loaded_slot(jcr);
   if (loaded > 0) {
      bnet_fsend(dir, _("3305 Issuing autochanger \"unload slot %d\" command.\n"), loaded);
      slot = dcr->VolCatInfo.Slot; 
      dcr->VolCatInfo.Slot = loaded;
      changer = edit_device_codes(jcr, changer, jcr->device->changer_command, "unload");
      int stat = run_program(changer, timeout, NULL);
      if (stat != 0) {
	 berrno be;
	 be.set_errno(stat);
         Jmsg(jcr, M_INFO, 0, _("3995 Bad autochanger \"unload slot %d\" command: ERR=%s.\n"), 
	      loaded, be.strerror());
      }
      dcr->VolCatInfo.Slot = slot;
   }

   /* Now list slots occupied */
   changer = edit_device_codes(jcr, changer, jcr->device->changer_command, "list");
   bnet_fsend(dir, _("3306 Issuing autochanger \"list\" command.\n"));
   bpipe = open_bpipe(changer, timeout, "r");
   if (!bpipe) {
      bnet_fsend(dir, _("3993 Open bpipe failed.\n"));
      free_pool_memory(changer);
      return false;
   }
   /* Get output from changer */
   while (fgets(dir->msg, len, bpipe->rfd)) { 
      dir->msglen = strlen(dir->msg);
      bnet_send(dir);
   }
   int stat = close_bpipe(bpipe);
   if (stat != 0) {
      berrno be;
      be.set_errno(stat);
      bnet_fsend(dir, "Autochanger error: ERR=%s\n", be.strerror());
   }
   bnet_sig(dir, BNET_EOD);

   free_pool_memory(changer);
   return true;
}


/*
 * Edit codes into ChangerCommand
 *  %% = %
 *  %a = archive device name
 *  %c = changer device name
 *  %d = changer drive index	    
 *  %f = Client's name
 *  %j = Job name
 *  %o = command
 *  %s = Slot base 0
 *  %S = Slot base 1
 *  %v = Volume name
 *
 *
 *  omsg = edited output message
 *  imsg = input string containing edit codes (%x)
 *  cmd = command string (load, unload, ...) 
 *
 */
char *edit_device_codes(JCR *jcr, char *omsg, const char *imsg, const char *cmd) 
{
   const char *p;
   const char *str;
   char add[20];

   *omsg = 0;
   Dmsg1(400, "edit_device_codes: %s\n", imsg);
   for (p=imsg; *p; p++) {
      if (*p == '%') {
	 switch (*++p) {
         case '%':
            str = "%";
	    break;
         case 'a':
	    str = dev_name(jcr->device->dev);
	    break;
         case 'c':
	    str = NPRT(jcr->device->changer_name);
	    break;
         case 'd':
            sprintf(add, "%d", jcr->device->dev->drive_index);
	    str = add;
	    break;
         case 'o':
	    str = NPRT(cmd);
	    break;
         case 's':
            sprintf(add, "%d", jcr->dcr->VolCatInfo.Slot - 1);
	    str = add;
	    break;
         case 'S':
            sprintf(add, "%d", jcr->dcr->VolCatInfo.Slot);
	    str = add;
	    break;
         case 'j':                    /* Job name */
	    str = jcr->Job;
	    break;
         case 'v':
	    str = NPRT(jcr->dcr->VolumeName);
	    break;
         case 'f':
	    str = NPRT(jcr->client_name);
	    break;

	 default:
            add[0] = '%';
	    add[1] = *p;
	    add[2] = 0;
	    str = add;
	    break;
	 }
      } else {
	 add[0] = *p;
	 add[1] = 0;
	 str = add;
      }
      Dmsg1(400, "add_str %s\n", str);
      pm_strcat(&omsg, (char *)str);
      Dmsg1(400, "omsg=%s\n", omsg);
   }
   return omsg;
}
