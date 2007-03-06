/*
 * Configuration file parser for Bacula Storage daemon
 *
 *     Kern Sibbald, March MM
 *
 *   Version $Id: stored_conf.c,v 1.55.4.1 2005/02/25 09:47:07 kerns Exp $
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
#include "stored.h"

extern int debug_level;


/* First and last resource ids */
int r_first = R_FIRST;
int r_last  = R_LAST;
static RES *sres_head[R_LAST - R_FIRST + 1];
RES **res_head = sres_head;


/* Forward referenced subroutines */

/* We build the current resource here statically,
 * then move it to dynamic memory */
URES res_all;
int res_all_size = sizeof(res_all);

/* Definition of records permitted within each
 * resource with the routine to process the record 
 * information.
 */ 

/* Globals for the Storage daemon. */
static RES_ITEM store_items[] = {
   {"name",                  store_name, ITEM(res_store.hdr.name),   0, ITEM_REQUIRED, 0},
   {"description",           store_str,  ITEM(res_dir.hdr.desc),     0, 0, 0},
   {"sdaddress",             store_addresses_address,  ITEM(res_store.sdaddrs),     0, ITEM_DEFAULT, 9103},
   {"sdaddresses",           store_addresses,  ITEM(res_store.sdaddrs), 0, ITEM_DEFAULT, 9103},
   {"messages",              store_res,  ITEM(res_store.messages),   0, R_MSGS, 0},
   {"sdport",                store_addresses_port,  ITEM(res_store.sdaddrs),     0, ITEM_DEFAULT, 9103},
   {"workingdirectory",      store_dir,  ITEM(res_store.working_directory), 0, ITEM_REQUIRED, 0},
   {"piddirectory",          store_dir,  ITEM(res_store.pid_directory), 0, ITEM_REQUIRED, 0},
   {"subsysdirectory",       store_dir,  ITEM(res_store.subsys_directory), 0, 0, 0},
   {"requiressl",            store_yesno,ITEM(res_store.require_ssl), 1, ITEM_DEFAULT, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_store.max_concurrent_jobs), 0, ITEM_DEFAULT, 10},
   {"heartbeatinterval",     store_time, ITEM(res_store.heartbeat_interval), 0, ITEM_DEFAULT, 0},
   {NULL, NULL, 0, 0, 0, 0} 
};


/* Directors that can speak to the Storage daemon */
static RES_ITEM dir_items[] = {
   {"name",        store_name,     ITEM(res_dir.hdr.name),   0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(res_dir.hdr.desc),   0, 0, 0},
   {"password",    store_password, ITEM(res_dir.password),   0, ITEM_REQUIRED, 0},
   {"enablessl",   store_yesno,    ITEM(res_dir.enable_ssl), 1, ITEM_DEFAULT, 0},
   {"monitor",     store_yesno,    ITEM(res_dir.monitor),   1, ITEM_DEFAULT, 0},
   {NULL, NULL, 0, 0, 0, 0} 
};

/* Device definition */
static RES_ITEM dev_items[] = {
   {"name",                  store_name,   ITEM(res_dev.hdr.name),        0, ITEM_REQUIRED, 0},
   {"description",           store_str,    ITEM(res_dir.hdr.desc),        0, 0, 0},
   {"mediatype",             store_strname,ITEM(res_dev.media_type),      0, ITEM_REQUIRED, 0},
   {"archivedevice",         store_strname,ITEM(res_dev.device_name),     0, ITEM_REQUIRED, 0},
   {"hardwareendoffile",     store_yesno,  ITEM(res_dev.cap_bits), CAP_EOF,  ITEM_DEFAULT, 1},
   {"hardwareendofmedium",   store_yesno,  ITEM(res_dev.cap_bits), CAP_EOM,  ITEM_DEFAULT, 1},
   {"backwardspacerecord",   store_yesno,  ITEM(res_dev.cap_bits), CAP_BSR,  ITEM_DEFAULT, 1},
   {"backwardspacefile",     store_yesno,  ITEM(res_dev.cap_bits), CAP_BSF,  ITEM_DEFAULT, 1},
   {"bsfateom",              store_yesno,  ITEM(res_dev.cap_bits), CAP_BSFATEOM, ITEM_DEFAULT, 0},
   {"twoeof",                store_yesno,  ITEM(res_dev.cap_bits), CAP_TWOEOF, ITEM_DEFAULT, 0},
   {"forwardspacerecord",    store_yesno,  ITEM(res_dev.cap_bits), CAP_FSR,  ITEM_DEFAULT, 1},
   {"forwardspacefile",      store_yesno,  ITEM(res_dev.cap_bits), CAP_FSF,  ITEM_DEFAULT, 1},
   {"fastforwardspacefile",  store_yesno,  ITEM(res_dev.cap_bits), CAP_FASTFSF, ITEM_DEFAULT, 1},
   {"removablemedia",        store_yesno,  ITEM(res_dev.cap_bits), CAP_REM,  ITEM_DEFAULT, 1},
   {"randomaccess",          store_yesno,  ITEM(res_dev.cap_bits), CAP_RACCESS, 0, 0},
   {"automaticmount",        store_yesno,  ITEM(res_dev.cap_bits), CAP_AUTOMOUNT,  ITEM_DEFAULT, 0},
   {"labelmedia",            store_yesno,  ITEM(res_dev.cap_bits), CAP_LABEL,      ITEM_DEFAULT, 0},
   {"alwaysopen",            store_yesno,  ITEM(res_dev.cap_bits), CAP_ALWAYSOPEN, ITEM_DEFAULT, 1},
   {"autochanger",           store_yesno,  ITEM(res_dev.cap_bits), CAP_AUTOCHANGER, ITEM_DEFAULT, 0},
   {"closeonpoll",           store_yesno,  ITEM(res_dev.cap_bits), CAP_CLOSEONPOLL, ITEM_DEFAULT, 0},
   {"blockpositioning",      store_yesno,  ITEM(res_dev.cap_bits), CAP_POSITIONBLOCKS, ITEM_DEFAULT, 1},
   {"changerdevice",         store_strname,ITEM(res_dev.changer_name), 0, 0, 0},
   {"changercommand",        store_strname,ITEM(res_dev.changer_command), 0, 0, 0},
   {"alertcommand",          store_strname,ITEM(res_dev.alert_command), 0, 0, 0},
   {"maximumchangerwait",    store_pint,   ITEM(res_dev.max_changer_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"maximumopenwait",       store_pint,   ITEM(res_dev.max_open_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"maximumopenvolumes",    store_pint,   ITEM(res_dev.max_open_vols), 0, ITEM_DEFAULT, 1},
   {"maximumnetworkbuffersize", store_pint, ITEM(res_dev.max_network_buffer_size), 0, 0, 0},
   {"volumepollinterval",    store_time,   ITEM(res_dev.vol_poll_interval), 0, 0, 0},
   {"offlineonunmount",      store_yesno,  ITEM(res_dev.cap_bits), CAP_OFFLINEUNMOUNT, ITEM_DEFAULT, 0},
   {"maximumrewindwait",     store_pint,   ITEM(res_dev.max_rewind_wait), 0, ITEM_DEFAULT, 5 * 60},
   {"minimumblocksize",      store_pint,   ITEM(res_dev.min_block_size), 0, 0, 0},
   {"maximumblocksize",      store_pint,   ITEM(res_dev.max_block_size), 0, 0, 0},
   {"maximumvolumesize",     store_size,   ITEM(res_dev.max_volume_size), 0, 0, 0},
   {"maximumfilesize",       store_size,   ITEM(res_dev.max_file_size), 0, ITEM_DEFAULT, 1000000000},
   {"volumecapacity",        store_size,   ITEM(res_dev.volume_capacity), 0, 0, 0},
   {"spooldirectory",        store_dir,    ITEM(res_dev.spool_directory), 0, 0, 0},
   {"maximumspoolsize",      store_size,   ITEM(res_dev.max_spool_size), 0, 0, 0},
   {"maximumjobspoolsize",   store_size,   ITEM(res_dev.max_job_spool_size), 0, 0, 0},
   {"driveindex",            store_pint,   ITEM(res_dev.drive_index), 0, 0, 0},
   {NULL, NULL, 0, 0, 0, 0} 
};

// {"mountanonymousvolumes", store_yesno,  ITEM(res_dev.cap_bits), CAP_ANONVOLS,   ITEM_DEFAULT, 0},


/* Message resource */
extern RES_ITEM msgs_items[];


/* This is the master resource definition */
RES_TABLE resources[] = {
   {"director",      dir_items,   R_DIRECTOR},
   {"storage",       store_items, R_STORAGE},
   {"device",        dev_items,   R_DEVICE},
   {"messages",      msgs_items,  R_MSGS},
   {NULL,	     NULL,	  0}
};




/* Dump contents of resource */
void dump_resource(int type, RES *reshdr, void sendit(void *sock, const char *fmt, ...), void *sock)
{
   URES *res = (URES *)reshdr;
   char buf[MAXSTRING];
   int recurse = 1;
   IPADDR *p;
   if (res == NULL) {
      sendit(sock, _("Warning: no \"%s\" resource (%d) defined.\n"), res_to_str(type), type);
      return;
   }
   sendit(sock, "dump_resource type=%d\n", type);
   if (type < 0) {		      /* no recursion */
      type = - type;
      recurse = 0;
   }
   switch (type) {
   case R_DIRECTOR:
      sendit(sock, "Director: name=%s\n", res->res_dir.hdr.name);
      break;
   case R_STORAGE:
      sendit(sock, "Storage: name=%s SDaddr=%s SDport=%d SDDport=%d HB=%s\n",
	     res->res_store.hdr.name, 
	     NPRT(get_first_address(res->res_store.sdaddrs, buf, sizeof(buf))),
	     get_first_port_host_order(res->res_store.sdaddrs), 
	     get_first_port_host_order(res->res_store.sddaddrs),
	     edit_utime(res->res_store.heartbeat_interval, buf, sizeof(buf)));
      if (res->res_store.sdaddrs) {
	 foreach_dlist(p, res->res_store.sdaddrs) {
            sendit(sock, "        SDaddr=%s SDport=%d\n", 
		   p->get_address(buf, sizeof(buf)), p->get_port_host_order());
	 }
      }
      if (res->res_store.sddaddrs) {
	 foreach_dlist(p, res->res_store.sddaddrs) {
            sendit(sock, "        SDDaddr=%s SDDport=%d\n", 
		   p->get_address(buf, sizeof(buf)), p->get_port_host_order());
	 }
      }
      break;
   case R_DEVICE:
      sendit(sock, "Device: name=%s MediaType=%s Device=%s\n",
	 res->res_dev.hdr.name,
	 res->res_dev.media_type, res->res_dev.device_name);
      sendit(sock, "        rew_wait=%d min_bs=%d max_bs=%d\n",
	 res->res_dev.max_rewind_wait, res->res_dev.min_block_size, 
	 res->res_dev.max_block_size);
      sendit(sock, "        max_jobs=%d max_files=%" lld " max_size=%" lld "\n",
	 res->res_dev.max_volume_jobs, res->res_dev.max_volume_files,
	 res->res_dev.max_volume_size);
      sendit(sock, "        max_file_size=%" lld " capacity=%" lld "\n",
	 res->res_dev.max_file_size, res->res_dev.volume_capacity);
      sendit(sock, "         spool_directory=%s\n", NPRT(res->res_dev.spool_directory));
      sendit(sock, "         max_spool_size=%" lld " max_job_spool_size=%" lld "\n",
	 res->res_dev.max_spool_size, res->res_dev.max_job_spool_size);
      strcpy(buf, "        ");
      if (res->res_dev.cap_bits & CAP_EOF) {
         bstrncat(buf, "CAP_EOF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_BSR) {
         bstrncat(buf, "CAP_BSR ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_BSF) {
         bstrncat(buf, "CAP_BSF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_FSR) {
         bstrncat(buf, "CAP_FSR ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_FSF) {
         bstrncat(buf, "CAP_FSF ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_EOM) {
         bstrncat(buf, "CAP_EOM ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_REM) {
         bstrncat(buf, "CAP_REM ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_RACCESS) {
         bstrncat(buf, "CAP_RACCESS ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_AUTOMOUNT) {
         bstrncat(buf, "CAP_AUTOMOUNT ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_LABEL) {
         bstrncat(buf, "CAP_LABEL ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_ANONVOLS) {
         bstrncat(buf, "CAP_ANONVOLS ", sizeof(buf));
      }
      if (res->res_dev.cap_bits & CAP_ALWAYSOPEN) {
         bstrncat(buf, "CAP_ALWAYSOPEN ", sizeof(buf));
      }
      bstrncat(buf, "\n", sizeof(buf));
      sendit(sock, buf);
      break;
   case R_MSGS:
      sendit(sock, "Messages: name=%s\n", res->res_msgs.hdr.name);
      if (res->res_msgs.mail_cmd) 
         sendit(sock, "      mailcmd=%s\n", res->res_msgs.mail_cmd);
      if (res->res_msgs.operator_cmd) 
         sendit(sock, "      opcmd=%s\n", res->res_msgs.operator_cmd);
      break;
   default:
      sendit(sock, _("Warning: unknown resource type %d\n"), type);
      break;
   }
   if (recurse && res->res_dir.hdr.next)
      dump_resource(type, (RES *)res->res_dir.hdr.next, sendit, sock);
}

/* 
 * Free memory of resource.  
 * NB, we don't need to worry about freeing any references
 * to other resources as they will be freed when that 
 * resource chain is traversed.  Mainly we worry about freeing
 * allocated strings (names).
 */
void free_resource(RES *sres, int type)
{
   RES *nres;
   URES *res = (URES *)sres;

   if (res == NULL)
      return;

   /* common stuff -- free the resource name */
   nres = (RES *)res->res_dir.hdr.next;
   if (res->res_dir.hdr.name) {
      free(res->res_dir.hdr.name);
   }
   if (res->res_dir.hdr.desc) {
      free(res->res_dir.hdr.desc);
   }


   switch (type) {
      case R_DIRECTOR:
	 if (res->res_dir.password) {
	    free(res->res_dir.password);
	 }
	 if (res->res_dir.address) {
	    free(res->res_dir.address);
	 }
	 break;
      case R_STORAGE:
	 if (res->res_store.sdaddrs) {
	    free_addresses(res->res_store.sdaddrs);
	 }
	 if (res->res_store.sddaddrs) {
	    free_addresses(res->res_store.sddaddrs);
	 }
	 if (res->res_store.working_directory) {
	    free(res->res_store.working_directory);
	 }
	 if (res->res_store.pid_directory) {
	    free(res->res_store.pid_directory);
	 }
	 if (res->res_store.subsys_directory) {
	    free(res->res_store.subsys_directory);
	 }
	 break;
      case R_DEVICE:
	 if (res->res_dev.media_type) {
	    free(res->res_dev.media_type);
	 }
	 if (res->res_dev.device_name) {
	    free(res->res_dev.device_name);
	 }
	 if (res->res_dev.changer_name) {
	    free(res->res_dev.changer_name);
	 }
	 if (res->res_dev.changer_command) {
	    free(res->res_dev.changer_command);
	 }
	 if (res->res_dev.alert_command) {
	    free(res->res_dev.alert_command);
	 }
	 if (res->res_dev.spool_directory) {
	    free(res->res_dev.spool_directory);
	 }
	 break;
      case R_MSGS:
	 if (res->res_msgs.mail_cmd) {
	    free(res->res_msgs.mail_cmd);
	 }
	 if (res->res_msgs.operator_cmd) {
	    free(res->res_msgs.operator_cmd);
	 }
	 free_msgs_res((MSGS *)res);  /* free message resource */
	 res = NULL;
	 break;
      default:
         Dmsg1(0, "Unknown resource type %d\n", type);
	 break;
   }
   /* Common stuff again -- free the resource, recurse to next one */
   if (res) {
      free(res);
   }
   if (nres) {
      free_resource(nres, type);
   }
}

/* Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * pointers (currently only in the Job resource).
 */
void save_resource(int type, RES_ITEM *items, int pass)
{
   URES *res;
   int rindex = type - r_first;
   int i, size;
   int error = 0;

   /* 
    * Ensure that all required items are present
    */
   for (i=0; items[i].name; i++) {
      if (items[i].flags & ITEM_REQUIRED) {
	 if (!bit_is_set(i, res_all.res_dir.hdr.item_present)) {  
            Emsg2(M_ERROR_TERM, 0, _("\"%s\" item is required in \"%s\" resource, but not found.\n"),
	      items[i].name, resources[rindex]);
	  }
      }
      /* If this triggers, take a look at lib/parse_conf.h */
      if (i >= MAX_RES_ITEMS) {
         Emsg1(M_ERROR_TERM, 0, _("Too many items in \"%s\" resource\n"), resources[rindex]);
      }
   }

   /* During pass 2, we looked up pointers to all the resources
    * referrenced in the current resource, , now we
    * must copy their address from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      switch (type) {
	 /* Resources not containing a resource */
	 case R_DIRECTOR:
	 case R_DEVICE:
	 case R_MSGS:
	    break;

	 /* Resources containing a resource */
	 case R_STORAGE:
	    if ((res = (URES *)GetResWithName(R_STORAGE, res_all.res_dir.hdr.name)) == NULL) {
               Emsg1(M_ERROR_TERM, 0, "Cannot find Storage resource \"%s\"\n", res_all.res_dir.hdr.name);
	    }
	    res->res_store.messages = res_all.res_store.messages;
	    break;
	 default:
            printf("Unknown resource type %d\n", type);
	    error = 1;
	    break;
      }


      if (res_all.res_dir.hdr.name) {
	 free(res_all.res_dir.hdr.name);
	 res_all.res_dir.hdr.name = NULL;
      }
      if (res_all.res_dir.hdr.desc) {
	 free(res_all.res_dir.hdr.desc);
	 res_all.res_dir.hdr.desc = NULL;
      }
      return;
   }

   /* The following code is only executed on pass 1 */
   switch (type) {
      case R_DIRECTOR:
	 size = sizeof(DIRRES);
	 break;
      case R_STORAGE:
	 size = sizeof(STORES);
	 break;
      case R_DEVICE:
	 size = sizeof(DEVRES);
	 break;
      case R_MSGS:
	 size = sizeof(MSGS);	
	 break;
      default:
         printf("Unknown resource type %d\n", type);
	 error = 1;
	 size = 1;
	 break;
   }
   /* Common */
   if (!error) {
      res = (URES *)malloc(size);
      memcpy(res, &res_all, size);
      if (!res_head[rindex]) {
	 res_head[rindex] = (RES *)res; /* store first entry */
      } else {
	 RES *next;
	 /* Add new res to end of chain */
	 for (next=res_head[rindex]; next->next; next=next->next) {
	    if (strcmp(next->name, res->res_dir.hdr.name) == 0) {
	       Emsg2(M_ERROR_TERM, 0,
                  _("Attempt to define second \"%s\" resource named \"%s\" is not permitted.\n"),
		  resources[rindex].name, res->res_dir.hdr.name);
	    }
	 }
	 next->next = (RES *)res;
         Dmsg2(90, "Inserting %s res: %s\n", res_to_str(type),
	       res->res_dir.hdr.name);
      }
   }
}
