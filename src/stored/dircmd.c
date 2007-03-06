/*
 *  This file handles accepting Director Commands
 *
 *    Most Director commands are handled here, with the
 *    exception of the Job command command and subsequent
 *    subcommands that are handled
 *    in job.c.
 *
 *    N.B. in this file, in general we must use P(dev->mutex) rather
 *      than lock_device(dev) so that we can examine the blocked
 *      state rather than blocking ourselves because a Job
 *      thread has the device blocked. In some "safe" cases,
 *      we can do things to a blocked device. CAREFUL!!!!
 *
 *    File daemon commands are handled in fdcmd.c
 *
 *     Kern Sibbald, May MMI
 *
 *   Version $Id: dircmd.c,v 1.116.2.7 2006/03/14 21:41:44 kerns Exp $
 *
 */
/*
   Copyright (C) 2001-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "stored.h"

/* Exported variables */

/* Imported variables */
extern BSOCK *filed_chan;
extern int r_first, r_last;
extern struct s_res resources[];
extern char my_name[];
extern time_t daemon_start_time;
extern struct s_last_job last_job;
extern bool init_done;

/* Static variables */
static char derrmsg[]     = "3900 Invalid command\n";
static char OKsetdebug[]  = "3000 OK setdebug=%d\n";
static char illegal_cmd[] = "3997 Illegal command for a Director with Monitor directive enabled\n";

/* Imported functions */
extern void terminate_child();
extern bool job_cmd(JCR *jcr);
extern bool use_cmd(JCR *jcr);
extern bool run_cmd(JCR *jcr);
extern bool status_cmd(JCR *sjcr);
extern bool qstatus_cmd(JCR *jcr);
//extern bool query_cmd(JCR *jcr);

/* Forward referenced functions */
static bool label_cmd(JCR *jcr);
static bool relabel_cmd(JCR *jcr);
static bool readlabel_cmd(JCR *jcr);
static bool release_cmd(JCR *jcr);
static bool setdebug_cmd(JCR *jcr);
static bool cancel_cmd(JCR *cjcr);
static bool mount_cmd(JCR *jcr);
static bool unmount_cmd(JCR *jcr);
static bool changer_cmd(JCR *sjcr);
static bool do_label(JCR *jcr, int relabel);
static DCR *find_device(JCR *jcr, POOL_MEM &dev_name, int drive);
static void read_volume_label(JCR *jcr, DEVICE *dev, int Slot);
static void label_volume_if_ok(DCR *dcr, char *oldname,
                               char *newname, char *poolname,
                               int Slot, int relabel);
static bool try_autoload_device(JCR *jcr, int slot, const char *VolName);
static void send_dir_busy_message(BSOCK *dir, DEVICE *dev);

struct s_cmds {
   const char *cmd;
   bool (*func)(JCR *jcr);
   int monitoraccess; /* specify if monitors have access to this function */
};

/*
 * The following are the recognized commands from the Director.
 */
static struct s_cmds cmds[] = {
   {"JobId=",      job_cmd,         0},     /* start Job */
   {"autochanger", changer_cmd,     0},
   {"bootstrap",   bootstrap_cmd,   0},
   {"cancel",      cancel_cmd,      0},
   {"label",       label_cmd,       0},     /* label a tape */
   {"mount",       mount_cmd,       0},
   {"readlabel",   readlabel_cmd,   0},
   {"release",     release_cmd,     0},
   {"relabel",     relabel_cmd,     0},     /* relabel a tape */
   {"setdebug=",   setdebug_cmd,    0},     /* set debug level */
   {"status",      status_cmd,      1},
   {".status",     qstatus_cmd,     1},
   {"unmount",     unmount_cmd,     0},
   {"use storage=", use_cmd,        0},
   {"run",         run_cmd,         0},
// {"query",       query_cmd,       0},
   {NULL,        NULL}                      /* list terminator */
};


/*
 * Connection request. We accept connections either from the
 *  Director or a Client (File daemon).
 *
 * Note, we are running as a seperate thread of the Storage daemon.
 *  and it is because a Director has made a connection with
 *  us on the "Message" channel.
 *
 * Basic tasks done here:
 *  - Create a JCR record
 *  - If it was from the FD, call handle_filed_connection()
 *  - Authenticate the Director
 *  - We wait for a command
 *  - We execute the command
 *  - We continue or exit depending on the return status
 */
void *handle_connection_request(void *arg)
{
   BSOCK *bs = (BSOCK *)arg;
   JCR *jcr;
   int i;
   bool found, quit;
   int bnet_stat = 0;
   char name[MAX_NAME_LENGTH];

   if (bnet_recv(bs) <= 0) {
      Emsg0(M_ERROR, 0, _("Connection request failed.\n"));
      bnet_close(bs);
      return NULL;
   }

   /*
    * Do a sanity check on the message received
    */
   if (bs->msglen < 25 || bs->msglen > (int)sizeof(name)-25) {
      Emsg1(M_ERROR, 0, _("Invalid connection. Len=%d\n"), bs->msglen);
      bnet_close(bs);
      return NULL;
   }
   /*
    * See if this is a File daemon connection. If so
    *   call FD handler.
    */
   Dmsg1(110, "Conn: %s", bs->msg);
   if (sscanf(bs->msg, "Hello Start Job %127s", name) == 1) {
      handle_filed_connection(bs, name);
      return NULL;
   }

   /* 
    * This is a connection from the Director, so setup a JCR 
    */
   Dmsg0(110, "Start Dir Job\n");
   jcr = new_jcr(sizeof(JCR), stored_free_jcr); /* create Job Control Record */
   jcr->dir_bsock = bs;               /* save Director bsock */
   jcr->dir_bsock->jcr = jcr;
   jcr->dcrs = New(alist(10, not_owned_by_alist));
   /* Initialize FD start condition variable */
   int errstat = pthread_cond_init(&jcr->job_start_wait, NULL);
   if (errstat != 0) {
      Jmsg1(jcr, M_FATAL, 0, _("Unable to init job cond variable: ERR=%s\n"), strerror(errstat));
      goto bail_out;
   }

   Dmsg0(1000, "stored in start_job\n");

   /*
    * Authenticate the Director
    */
   if (!authenticate_director(jcr)) {
      Jmsg(jcr, M_FATAL, 0, _("Unable to authenticate Director\n"));
      goto bail_out;
   }
   Dmsg0(90, "Message channel init completed.\n");

   for (quit=false; !quit;) {
      /* Read command */
      if ((bnet_stat = bnet_recv(bs)) <= 0) {
         break;               /* connection terminated */
      }
      Dmsg1(199, "<dird: %s", bs->msg);
      /* Ensure that device initialization is complete */
      while (!init_done) {
         bmicrosleep(1, 0);
      }
      found = false;
      for (i=0; cmds[i].cmd; i++) {
        if (strncmp(cmds[i].cmd, bs->msg, strlen(cmds[i].cmd)) == 0) {
           if ((!cmds[i].monitoraccess) && (jcr->director->monitor)) {
              Dmsg1(100, "Command %s illegal.\n", cmds[i].cmd);
              bnet_fsend(bs, illegal_cmd);
              bnet_sig(bs, BNET_EOD);
              break;
           }
           Dmsg1(200, "Do command: %s\n", cmds[i].cmd);
           if (!cmds[i].func(jcr)) { /* do command */
              quit = true; /* error, get out */
              Dmsg1(190, "Command %s requsts quit\n", cmds[i].cmd);
           }
           found = true;             /* indicate command found */
           break;
        }
      }
      if (!found) {                   /* command not found */
        bnet_fsend(bs, derrmsg);
        break;
      }
   }
bail_out:
   generate_daemon_event(jcr, "JobEnd");
   dequeue_messages(jcr);             /* send any queued messages */
   bnet_sig(bs, BNET_TERMINATE);
   free_jcr(jcr);
   return NULL;
}

/*
 * Set debug level as requested by the Director
 *
 */
static bool setdebug_cmd(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   int level, trace_flag;

   Dmsg1(10, "setdebug_cmd: %s", dir->msg);
   if (sscanf(dir->msg, "setdebug=%d trace=%d", &level, &trace_flag) != 2 || level < 0) {
      bnet_fsend(dir, _("3991 Bad setdebug command: %s\n"), dir->msg);
      return 0;
   }
   debug_level = level;
   set_trace(trace_flag);
   return bnet_fsend(dir, OKsetdebug, level);
}


/*
 * Cancel a Job
 */
static bool cancel_cmd(JCR *cjcr)
{
   BSOCK *dir = cjcr->dir_bsock;
   int oldStatus;
   char Job[MAX_NAME_LENGTH];
   JCR *jcr;

   if (sscanf(dir->msg, "cancel Job=%127s", Job) == 1) {
      if (!(jcr=get_jcr_by_full_name(Job))) {
         bnet_fsend(dir, _("3904 Job %s not found.\n"), Job);
      } else {
         jcr->lock();
         oldStatus = jcr->JobStatus;
         set_jcr_job_status(jcr, JS_Canceled);
         if (!jcr->authenticated && oldStatus == JS_WaitFD) {
            pthread_cond_signal(&jcr->job_start_wait); /* wake waiting thread */
         }
         jcr->unlock();
         if (jcr->file_bsock) {
            bnet_sig(jcr->file_bsock, BNET_TERMINATE);
         }
         /* If thread waiting on mount, wake him */
         if (jcr->dcr && jcr->dcr->dev && jcr->dcr->dev->waiting_for_mount()) {
            pthread_cond_broadcast(&jcr->dcr->dev->wait_next_vol);
            pthread_cond_broadcast(&wait_device_release);
         }
         if (jcr->read_dcr && jcr->read_dcr->dev && jcr->read_dcr->dev->waiting_for_mount()) {
            pthread_cond_broadcast(&jcr->read_dcr->dev->wait_next_vol);
            pthread_cond_broadcast(&wait_device_release);
         }
         bnet_fsend(dir, _("3000 Job %s marked to be canceled.\n"), jcr->Job);
         free_jcr(jcr);
      }
   } else {
      bnet_fsend(dir, _("3903 Error scanning cancel command.\n"));
   }
   bnet_sig(dir, BNET_EOD);
   return 1;
}

/*
 * Label a Volume
 *
 */
static bool label_cmd(JCR *jcr)
{
   return do_label(jcr, 0);
}

static bool relabel_cmd(JCR *jcr)
{
   return do_label(jcr, 1);
}

static bool do_label(JCR *jcr, int relabel)
{
   POOLMEM *newname, *oldname, *poolname, *mtype;
   POOL_MEM dev_name;
   BSOCK *dir = jcr->dir_bsock;
   DCR *dcr;
   DEVICE *dev;
   bool ok = false;
   int slot;
   int drive;

   newname = get_memory(dir->msglen+1);
   oldname = get_memory(dir->msglen+1);
   poolname = get_memory(dir->msglen+1);
   mtype = get_memory(dir->msglen+1);
   if (relabel) {
      if (sscanf(dir->msg, "relabel %127s OldName=%127s NewName=%127s PoolName=%127s "
                 "MediaType=%127s Slot=%d drive=%d",
                  dev_name.c_str(), oldname, newname, poolname, mtype, 
                  &slot, &drive) == 7) {
         ok = true;
      }
   } else {
      *oldname = 0;
      if (sscanf(dir->msg, "label %127s VolumeName=%127s PoolName=%127s "
                 "MediaType=%127s Slot=%d drive=%d", 
          dev_name.c_str(), newname, poolname, mtype, &slot, &drive) == 6) {
         ok = true;
      }
   }
   if (ok) {
      unbash_spaces(newname);
      unbash_spaces(oldname);
      unbash_spaces(poolname);
      unbash_spaces(mtype);
      dcr = find_device(jcr, dev_name, drive);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         if (!dev->is_open()) {
            Dmsg0(400, "Can relabel. Device is not open\n");
            label_volume_if_ok(dcr, oldname, newname, poolname, slot, relabel);
            force_close_device(dev);
         /* Under certain "safe" conditions, we can steal the lock */
         } else if (dev->can_steal_lock()) {
            Dmsg0(400, "Can relabel. can_steal_lock\n");
            label_volume_if_ok(dcr, oldname, newname, poolname, slot, relabel);
         } else if (dev->is_busy() || dev->is_blocked()) {
            send_dir_busy_message(dir, dev);
         } else {                     /* device not being used */
            Dmsg0(400, "Can relabel. device not used\n");
            label_volume_if_ok(dcr, oldname, newname, poolname, slot, relabel);
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), dev_name.c_str());
      }
   } else {
      /* NB dir->msg gets clobbered in bnet_fsend, so save command */
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3903 Error scanning label command: %s\n"), jcr->errmsg);
   }
   free_memory(oldname);
   free_memory(newname);
   free_memory(poolname);
   free_memory(mtype);
   bnet_sig(dir, BNET_EOD);
   return true;
}

/*
 * Read the tape label and determine if we can safely
 * label the tape (not a Bacula volume), then label it.
 *
 *  Enter with the mutex set
 */
static void label_volume_if_ok(DCR *dcr, char *oldname,
                               char *newname, char *poolname,
                               int slot, int relabel)
{
   BSOCK *dir = dcr->jcr->dir_bsock;
   bsteal_lock_t hold;
   DEVICE *dev = dcr->dev;
   int label_status;

   steal_device_lock(dev, &hold, BST_WRITING_LABEL);
   Dmsg1(100, "Stole device %s lock, writing label.\n", dev->print_name());

   /* Note, try_autoload_device() opens the device */
   if (!try_autoload_device(dcr->jcr, slot, newname)) {
      goto bail_out;                  /* error */
   }

   /* See what we have for a Volume */
   label_status = read_dev_volume_label(dcr);
   
   switch(label_status) {
   case VOL_NAME_ERROR:
   case VOL_VERSION_ERROR:
   case VOL_LABEL_ERROR:
   case VOL_OK:
      if (!relabel) {
         bnet_fsend(dir, _(
            "3920 Cannot label Volume because it is already labeled: \"%s\"\n"),
             dev->VolHdr.VolumeName);
         break;
      }

      /* Relabel request. If oldname matches, continue */
      if (strcmp(oldname, dev->VolHdr.VolumeName) != 0) {
         bnet_fsend(dir, _("3921 Wrong volume mounted.\n"));
         break;
      }
      if (dev->label_type != B_BACULA_LABEL) {
         bnet_fsend(dir, _("3922 Cannot relabel an ANSI/IBM labeled Volume.\n"));
         break;
      }
      free_volume(dev);               /* release old volume name */
      /* Fall through wanted! */
   case VOL_IO_ERROR:
   case VOL_NO_LABEL:
      if (!write_new_volume_label_to_dev(dcr, newname, poolname)) {
         bnet_fsend(dir, _("3912 Failed to label Volume: ERR=%s\n"), strerror_dev(dev));
         break;
      }
      bstrncpy(dcr->VolumeName, newname, sizeof(dcr->VolumeName));
      /* The following 3000 OK label. string is scanned in ua_label.c */
      bnet_fsend(dir, "3000 OK label. Volume=%s Device=%s\n",
         newname, dev->print_name());
      break;
   case VOL_NO_MEDIA:
      bnet_fsend(dir, _("3912 Failed to label Volume: ERR=%s\n"), strerror_dev(dev));
      break;
   default:
      bnet_fsend(dir, _("3913 Cannot label Volume. "
"Unknown status %d from read_volume_label()\n"), label_status);
      break;
   }

bail_out:
   if (!dev->is_open()) {
      free_volume(dev);
   }
   give_back_device_lock(dev, &hold);
   return;
}


/*
 * Read the tape label
 *
 *  Enter with the mutex set
 */
static bool read_label(DCR *dcr)
{
   int ok;
   JCR *jcr = dcr->jcr;
   BSOCK *dir = jcr->dir_bsock;
   bsteal_lock_t hold;
   DEVICE *dev = dcr->dev;

   steal_device_lock(dev, &hold, BST_DOING_ACQUIRE);

   dcr->VolumeName[0] = 0;
   dev->clear_labeled();              /* force read of label */
   switch (read_dev_volume_label(dcr)) {
   case VOL_OK:
      bnet_fsend(dir, _("3001 Mounted Volume: %s\n"), dev->VolHdr.VolumeName);
      ok = true;
      break;
   default:
      bnet_fsend(dir, _("3902 Cannot mount Volume on Storage Device %s because:\n%s"),
         dev->print_name(), jcr->errmsg);
      ok = false;
      break;
   }
   give_back_device_lock(dev, &hold);
   return ok;
}

/* 
 * Searches for device by name, and if found, creates a dcr and
 *  returns it.
 */
static DCR *find_device(JCR *jcr, POOL_MEM &devname, int drive)
{
   DEVRES *device;
   AUTOCHANGER *changer;
   bool found = false;
   DCR *dcr = NULL;

   unbash_spaces(devname);
   foreach_res(device, R_DEVICE) {
      /* Find resource, and make sure we were able to open it */
      if (fnmatch(device->hdr.name, devname.c_str(), 0) == 0) {
         if (!device->dev) {
            device->dev = init_dev(jcr, device);
         }
         if (!device->dev) {
            Jmsg(jcr, M_WARNING, 0, _("\n"
               "     Device \"%s\" requested by DIR could not be opened or does not exist.\n"),
                 devname.c_str());
            continue;
         }
         Dmsg1(20, "Found device %s\n", device->hdr.name);
         found = true;
         break;
      }
   }
   foreach_res(changer, R_AUTOCHANGER) {
      /* Find resource, and make sure we were able to open it */
      if (fnmatch(devname.c_str(), changer->hdr.name, 0) == 0) {
         /* Try each device in this AutoChanger */
         foreach_alist(device, changer->device) {
            Dmsg1(100, "Try changer device %s\n", device->hdr.name);
            if (!device->dev) {
               device->dev = init_dev(jcr, device);
            }
            if (!device->dev) {
               Dmsg1(100, "Device %s could not be opened. Skipped\n", devname.c_str());
               Jmsg(jcr, M_WARNING, 0, _("\n"
                  "     Device \"%s\" in changer \"%s\" requested by DIR could not be opened or does not exist.\n"),
                    device->hdr.name, devname.c_str());
               continue;
            }
            if (!device->dev->autoselect) {
               Dmsg1(100, "Device %s not autoselect skipped.\n", devname.c_str());
               continue;              /* device is not available */
            }
            if (drive < 0 || drive == (int)device->dev->drive_index) {
               Dmsg1(20, "Found changer device %s\n", device->hdr.name);
               found = true;
               break;
            }
            Dmsg3(100, "Device %s drive wrong: want=%d got=%d skipping\n",
               devname.c_str(), drive, (int)device->dev->drive_index);
         }
         break;                    /* we found it but could not open a device */
      }
   }

   if (found) {
      Dmsg1(100, "Found changer device %s\n", device->hdr.name);
      dcr = new_dcr(jcr, device->dev);
      dcr->device = device;
      jcr->dcr = dcr;
   }
   return dcr;
}


/*
 * Mount command from Director
 */
static bool mount_cmd(JCR *jcr)
{
   POOL_MEM devname;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev;
   DCR *dcr;
   int drive;

   if (sscanf(dir->msg, "mount %127s drive=%d", devname.c_str(), &drive) == 2) {
      dcr = find_device(jcr, devname, drive);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         Dmsg1(100, "mount cmd blocked=%d\n", dev->dev_blocked);
         switch (dev->dev_blocked) {         /* device blocked? */
         case BST_WAITING_FOR_SYSOP:
            /* Someone is waiting, wake him */
            Dmsg0(100, "Waiting for mount. Attempting to wake thread\n");
            dev->dev_blocked = BST_MOUNT;
            bnet_fsend(dir, "3001 OK mount. Device=%s\n", 
               dev->print_name());
            pthread_cond_broadcast(&dev->wait_next_vol);
            pthread_cond_broadcast(&wait_device_release);
            break;

         /* In both of these two cases, we (the user) unmounted the Volume */
         case BST_UNMOUNTED_WAITING_FOR_SYSOP:
         case BST_UNMOUNTED:
            /* We freed the device, so reopen it and wake any waiting threads */
            if (dev->open(dcr, OPEN_READ_ONLY) < 0) {
               bnet_fsend(dir, _("3901 open device failed: ERR=%s\n"),
                  strerror_dev(dev));
               if (dev->dev_blocked == BST_UNMOUNTED) {
                  /* We blocked the device, so unblock it */
                  Dmsg0(100, "Unmounted. Unblocking device\n");
                  unblock_device(dev);
               }
               break;
            }
            read_dev_volume_label(dcr);
            if (dev->dev_blocked == BST_UNMOUNTED) {
               /* We blocked the device, so unblock it */
               Dmsg0(100, "Unmounted. Unblocking device\n");
               read_label(dcr);       /* this should not be necessary */
               unblock_device(dev);
            } else {
               Dmsg0(100, "Unmounted waiting for mount. Attempting to wake thread\n");
               dev->dev_blocked = BST_MOUNT;
            }
            if (dev->is_labeled()) {
               bnet_fsend(dir, _("3001 Device %s is mounted with Volume \"%s\"\n"),
                  dev->print_name(), dev->VolHdr.VolumeName);
            } else {
               bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"
                                 "If this is not a blank tape, try unmounting and remounting the Volume.\n"),
                          dev->print_name());
            }
            pthread_cond_broadcast(&dev->wait_next_vol);
            pthread_cond_broadcast(&wait_device_release);
            break;

         case BST_DOING_ACQUIRE:
            bnet_fsend(dir, _("3001 Device %s is doing acquire.\n"),
                       dev->print_name());
            break;

         case BST_WRITING_LABEL:
            bnet_fsend(dir, _("3903 Device %s is being labeled.\n"), 
               dev->print_name());
            break;

         case BST_NOT_BLOCKED:
            if (dev->is_open()) {
               if (dev->is_labeled()) {
                  bnet_fsend(dir, _("3001 Device %s is mounted with Volume \"%s\"\n"),
                     dev->print_name(), dev->VolHdr.VolumeName);
               } else {
                  bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"
                                 "If this is not a blank tape, try unmounting and remounting the Volume.\n"),
                             dev->print_name());
               }
            } else if (dev->is_tape()) {
               if (dev->open(dcr, OPEN_READ_ONLY) < 0) {
                  bnet_fsend(dir, _("3901 open device failed: ERR=%s\n"),
                     strerror_dev(dev));
                  break;
               }
               read_label(dcr);
               if (dev->is_labeled()) {
                  bnet_fsend(dir, _("3001 Device %s is already mounted with Volume \"%s\"\n"),
                     dev->print_name(), dev->VolHdr.VolumeName);
               } else {
                  bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"
                                    "If this is not a blank tape, try unmounting and remounting the Volume.\n"),
                             dev->print_name());
               }
            } else if (dev->is_dvd()) {
               if (mount_dev(dev, 1)) {
                  bnet_fsend(dir, _("3002 Device %s is mounted.\n"), 
                     dev->print_name());
               } else {
                  bnet_fsend(dir, _("3907 %s"), strerror_dev(dev));
               } 
            } else { /* must be file */
               bnet_fsend(dir, _("3906 File device %s is always mounted.\n"),
                  dev->print_name());
            }
            break;

         default:
            bnet_fsend(dir, _("3905 Bizarre wait state %d\n"), dev->dev_blocked);
            break;
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), devname.c_str());
      }
   } else {
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3909 Error scanning mount command: %s\n"), jcr->errmsg);
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}

/*
 * unmount command from Director
 */
static bool unmount_cmd(JCR *jcr)
{
   POOL_MEM devname;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev;
   DCR *dcr;
   int drive;

   if (sscanf(dir->msg, "unmount %127s drive=%d", devname.c_str(), &drive) == 2) {
      dcr = find_device(jcr, devname, drive);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         if (!dev->is_open()) {
            if (!dev->is_busy()) {
               unload_autochanger(jcr->dcr, -1);          
            }
            Dmsg0(90, "Device already unmounted\n");
            bnet_fsend(dir, _("3901 Device %s is already unmounted.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_WAITING_FOR_SYSOP) {
            Dmsg2(90, "%d waiter dev_block=%d. doing unmount\n", dev->num_waiting,
               dev->dev_blocked);
            if (!unload_autochanger(jcr->dcr, -1)) {
               offline_or_rewind_dev(dev);
               force_close_device(dev);
            }
            dev->dev_blocked = BST_UNMOUNTED_WAITING_FOR_SYSOP;
            bnet_fsend(dir, _("3001 Device %s unmounted.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_DOING_ACQUIRE) {
            bnet_fsend(dir, _("3902 Device %s is busy in acquire.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_WRITING_LABEL) {
            bnet_fsend(dir, _("3903 Device %s is being labeled.\n"), 
               dev->print_name());

         } else if (dev->is_busy()) {
            send_dir_busy_message(dir, dev);
         } else {                     /* device not being used */
            Dmsg0(90, "Device not in use, unmounting\n");
            /* On FreeBSD, I am having ASSERT() failures in block_device()
             * and I can only imagine that the thread id that we are
             * leaving in no_wait_id is being re-used. So here,
             * we simply do it by hand.  Gross, but a solution.
             */
            /*  block_device(dev, BST_UNMOUNTED); replace with 2 lines below */
            dev->dev_blocked = BST_UNMOUNTED;
            dev->no_wait_id = 0;
            if (!unload_autochanger(jcr->dcr, -1)) {
               offline_or_rewind_dev(dev);
               force_close_device(dev);
            }
            bnet_fsend(dir, _("3002 Device %s unmounted.\n"), 
               dev->print_name());
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), devname.c_str());
      }
   } else {
      /* NB dir->msg gets clobbered in bnet_fsend, so save command */
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3907 Error scanning unmount command: %s\n"), jcr->errmsg);
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}

/*
 * Release command from Director. This rewinds the device and if
 *   configured does a offline and ensures that Bacula will
 *   re-read the label of the tape before continuing. This gives
 *   the operator the chance to change the tape anytime before the
 *   next job starts.
 */
static bool release_cmd(JCR *jcr)
{
   POOL_MEM devname;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev;
   DCR *dcr;
   int drive;

   if (sscanf(dir->msg, "release %127s drive=%d", devname.c_str(), &drive) == 2) {
      dcr = find_device(jcr, devname, drive);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         if (!dev->is_open()) {
            Dmsg0(90, "Device already released\n");
            bnet_fsend(dir, _("3921 Device %s already released.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_WAITING_FOR_SYSOP ||
                    dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP) {
            Dmsg2(90, "%d waiter dev_block=%d. doing unmount\n", dev->num_waiting,
               dev->dev_blocked);
            bnet_fsend(dir, _("3922 Device %s waiting for mount.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_DOING_ACQUIRE) {
            bnet_fsend(dir, _("3923 Device %s is busy in acquire.\n"), 
               dev->print_name());

         } else if (dev->dev_blocked == BST_WRITING_LABEL) {
            bnet_fsend(dir, _("3914 Device %s is being labeled.\n"), 
               dev->print_name());

         } else if (dev->is_busy()) {
            send_dir_busy_message(dir, dev);
         } else {                     /* device not being used */
            Dmsg0(90, "Device not in use, unmounting\n");
            release_volume(jcr->dcr);
            bnet_fsend(dir, _("3022 Device %s released.\n"), 
               dev->print_name());
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), devname.c_str());
      }
   } else {
      /* NB dir->msg gets clobbered in bnet_fsend, so save command */
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3927 Error scanning release command: %s\n"), jcr->errmsg);
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}



/*
 * Autochanger command from Director
 */
static bool changer_cmd(JCR *jcr)
{
   POOL_MEM devname;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev;
   DCR *dcr;
   const char *cmd = NULL;
   bool ok = false;

   if (sscanf(dir->msg, "autochanger list %127s", devname.c_str()) == 1) {
      cmd = "list";
      ok = true;
   } else if (sscanf(dir->msg, "autochanger slots %127s", devname.c_str()) == 1) {
      cmd = "slots";
      ok = true;
   } else if (sscanf(dir->msg, "autochanger drives %127s", devname.c_str()) == 1) {
      cmd = "drives";
      ok = true;
   }
   if (ok) {
      dcr = find_device(jcr, devname, -1);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         if (!dev->device->changer_res) {
            bnet_fsend(dir, _("3995 Device %s is not an autochanger.\n"), 
               dev->print_name());
         /* Under certain "safe" conditions, we can steal the lock */
         } else if (!dev->is_open() || dev->can_steal_lock()) {
            autochanger_cmd(dcr, dir, cmd);
         } else if (dev->is_busy() || dev->is_blocked()) {
            send_dir_busy_message(dir, dev);
         } else {                     /* device not being used */
            autochanger_cmd(dcr, dir, cmd);
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), devname.c_str());
      }
   } else {  /* error on scanf */
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3908 Error scanning autocharger drives/list/slots command: %s\n"),
         jcr->errmsg);
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}

/*
 * Read and return the Volume label
 */
static bool readlabel_cmd(JCR *jcr)
{
   POOL_MEM devname;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev;
   DCR *dcr;
   int Slot;
   int drive;

   if (sscanf(dir->msg, "readlabel %127s Slot=%d drive=%d", devname.c_str(), 
       &Slot, &drive) == 3) {
      dcr = find_device(jcr, devname, drive);
      if (dcr) {
         dev = dcr->dev;
         P(dev->mutex);               /* Use P to avoid indefinite block */
         if (!dev->is_open()) {
            read_volume_label(jcr, dev, Slot);
            force_close_device(dev);
         /* Under certain "safe" conditions, we can steal the lock */
         } else if (dev->can_steal_lock()) {
            read_volume_label(jcr, dev, Slot);
         } else if (dev->is_busy() || dev->is_blocked()) {
            send_dir_busy_message(dir, dev);
         } else {                     /* device not being used */
            read_volume_label(jcr, dev, Slot);
         }
         V(dev->mutex);
         free_dcr(dcr);
         jcr->dcr = NULL;
      } else {
         bnet_fsend(dir, _("3999 Device \"%s\" not found or could not be opened.\n"), devname.c_str());
      }
   } else {
      pm_strcpy(jcr->errmsg, dir->msg);
      bnet_fsend(dir, _("3909 Error scanning readlabel command: %s\n"), jcr->errmsg);
   }
   bnet_sig(dir, BNET_EOD);
   return true;
}

/*
 * Read the tape label
 *
 *  Enter with the mutex set
 */
static void read_volume_label(JCR *jcr, DEVICE *dev, int Slot)
{
   BSOCK *dir = jcr->dir_bsock;
   bsteal_lock_t hold;
   DCR *dcr = jcr->dcr;

   dcr->dev = dev;
   steal_device_lock(dev, &hold, BST_WRITING_LABEL);

   if (!try_autoload_device(jcr, Slot, "")) {
      goto bail_out;                  /* error */
   }

   dev->clear_labeled();              /* force read of label */
   switch (read_dev_volume_label(dcr)) {
   case VOL_OK:
      /* DO NOT add quotes around the Volume name. It is scanned in the DIR */
      bnet_fsend(dir, _("3001 Volume=%s Slot=%d\n"), dev->VolHdr.VolumeName, Slot);
      Dmsg1(100, "Volume: %s\n", dev->VolHdr.VolumeName);
      break;
   default:
      bnet_fsend(dir, _("3902 Cannot mount Volume on Storage Device %s because:\n%s"),
                 dev->print_name(), jcr->errmsg);
      break;
   }

bail_out:
   give_back_device_lock(dev, &hold);
   return;
}

static bool try_autoload_device(JCR *jcr, int slot, const char *VolName)
{
   DCR *dcr = jcr->dcr;
   BSOCK *dir = jcr->dir_bsock;
   DEVICE *dev = dcr->dev;

   bstrncpy(dcr->VolumeName, VolName, sizeof(dcr->VolumeName));
   dcr->VolCatInfo.Slot = slot;
   dcr->VolCatInfo.InChanger = slot > 0;
   if (autoload_device(dcr, 0, dir) < 0) {    /* autoload if possible */
      return false;
   }

   /* Ensure that the device is open -- autoload_device() closes it */
   if (dev->open(dcr, OPEN_READ_WRITE) < 0) {
      bnet_fsend(dir, _("3910 Unable to open device %s: ERR=%s\n"),
         dev->print_name(), dev->strerror());
      return false;
   }
   return true;
}

static void send_dir_busy_message(BSOCK *dir, DEVICE *dev)
{
   if (dev->is_blocked()) {
      switch (dev->dev_blocked) {
      case BST_UNMOUNTED:
         bnet_fsend(dir, _("3931 Device %s is BLOCKED. user unmounted.\n"),
            dev->print_name());
         break;
      case BST_UNMOUNTED_WAITING_FOR_SYSOP:
         bnet_fsend(dir, _("3932 Device %s is BLOCKED. user unmounted during wait for media/mount.\n"),
             dev->print_name());
         break;
      case BST_WAITING_FOR_SYSOP:
         bnet_fsend(dir, _("3933 Device %s is BLOCKED waiting for media.\n"),
            dev->print_name());
         break;
      case BST_DOING_ACQUIRE:
         bnet_fsend(dir, _("3934 Device %s is being initialized.\n"),
            dev->print_name());
         break;
      case BST_WRITING_LABEL:
         bnet_fsend(dir, _("3935 Device %s is blocked labeling a Volume.\n"),
            dev->print_name());
         break;
      default:
         bnet_fsend(dir, _("3935 Device %s is blocked for unknown reason.\n"),
            dev->print_name());
         break;
      }
   } else if (dev->can_read()) {
       bnet_fsend(dir, _("3936 Device %s is busy reading.\n"),
                   dev->print_name());;
   } else {
       bnet_fsend(dir, _("3937 Device %s is busy with %d writer(s).\n"),
          dev->print_name(), dev->num_writers);
   }
}
