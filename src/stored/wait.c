/*
 *  Subroutines to handle waiting for operator intervention
 *   or waiting for a Device to be released
 *
 *  Code for wait_for_sysop() pulled from askdir.c
 *
 *   Kern Sibbald, March 2005
 *
 *   Version $Id: wait.c 3805 2006-12-16 11:10:17Z kerns $
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


#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

//static bool double_jcr_wait_time(JCR *jcr);


/*
 * Wait for SysOp to mount a tape on a specific device
 *
 *   Returns: W_ERROR, W_TIMEOUT, W_POLL, W_MOUNT, or W_WAKE 
 */
int wait_for_sysop(DCR *dcr)
{
   struct timeval tv;
   struct timezone tz;
   struct timespec timeout;
   time_t last_heartbeat = 0;
   time_t first_start = time(NULL);
   int stat = 0;
   int add_wait;
   bool unmounted;
   DEVICE *dev = dcr->dev;
   JCR *jcr = dcr->jcr;

   P(dev->mutex);
   Dmsg1(100, "Enter blocked=%s\n", dev->print_blocked());
   unmounted = is_device_unmounted(dev);

   dev->poll = false;
   /*
    * Wait requested time (dev->rem_wait_sec).  However, we also wake up every
    *    HB_TIME seconds and send a heartbeat to the FD and the Director
    *    to keep stateful firewalls from closing them down while waiting
    *    for the operator.
    */
   add_wait = dev->rem_wait_sec;
   if (me->heartbeat_interval && add_wait > me->heartbeat_interval) {
      add_wait = me->heartbeat_interval;
   }
   /* If the user did not unmount the tape and we are polling, ensure
    *  that we poll at the correct interval.
    */
   if (!unmounted && dev->vol_poll_interval && add_wait > dev->vol_poll_interval) {
      add_wait = dev->vol_poll_interval;
   }

   if (!unmounted) {
      Dmsg1(400, "blocked=%s\n", dev->print_blocked());
      dev->dev_prev_blocked = dev->dev_blocked;
      dev->set_blocked(BST_WAITING_FOR_SYSOP); /* indicate waiting for mount */
   }

   for ( ; !job_canceled(jcr); ) {
      time_t now, start;

      gettimeofday(&tv, &tz);
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec + add_wait;

      Dmsg4(400, "I'm going to sleep on device %s. HB=%d wait=%d add_wait=%d\n", 
         dev->print_name(), (int)me->heartbeat_interval, dev->wait_sec, add_wait);
      start = time(NULL);
      /* Wait required time */
      stat = pthread_cond_timedwait(&dev->wait_next_vol, &dev->mutex, &timeout);
      Dmsg2(400, "Wokeup from sleep on device stat=%d blocked=%s\n", stat,
         dev->print_blocked());

      now = time(NULL);
      dev->rem_wait_sec -= (now - start);

      /* Note, this always triggers the first time. We want that. */
      if (me->heartbeat_interval) {
         if (now - last_heartbeat >= me->heartbeat_interval) {
            /* send heartbeats */
            if (jcr->file_bsock) {
               bnet_sig(jcr->file_bsock, BNET_HEARTBEAT);
               Dmsg0(400, "Send heartbeat to FD.\n");
            }
            if (jcr->dir_bsock) {
               bnet_sig(jcr->dir_bsock, BNET_HEARTBEAT);
            }
            last_heartbeat = now;
         }
      }

      if (stat == EINVAL) {
         berrno be;
         Jmsg1(jcr, M_FATAL, 0, _("pthread timedwait error. ERR=%s\n"), be.strerror(stat));
         stat = W_ERROR;               /* error */
         break;
      }


      if (dev->rem_wait_sec <= 0) {  /* on exceeding wait time return */
         Dmsg0(400, "Exceed wait time.\n");
         stat = W_TIMEOUT;
         break;
      }

      /*
       * Check if user unmounted the device while we were waiting
       */
      unmounted = is_device_unmounted(dev);

      if (!unmounted && dev->vol_poll_interval &&
          (now - first_start >= dev->vol_poll_interval)) {
         Dmsg1(400, "In wait blocked=%s\n", dev->print_blocked());
         dev->poll = true;            /* returning a poll event */
         stat = W_POLL;
         break;
      }
      /*
       * Check if user mounted the device while we were waiting
       */
      if (dev->get_blocked() == BST_MOUNT) {   /* mount request ? */
         stat = W_MOUNT;
         break;
      }

      /*
       * If we did not timeout, then some event happened, so
       *   return to check if state changed.   
       */
      if (stat != 0) {
         stat = W_WAKE;          /* someone woke us */
         break;
      }

      /* 
       * At this point, we know we woke up because of a timeout,
       *   that was due to a heartbeat, so we just update
       *   the wait counters and continue.
       */
      add_wait = dev->wait_sec - (now - start);
      if (add_wait < 0) {
         add_wait = 0;
      }
      if (me->heartbeat_interval && add_wait > me->heartbeat_interval) {
         add_wait = me->heartbeat_interval;
      }
   }

   if (!unmounted) {
      dev->set_blocked(dev->dev_prev_blocked);    /* restore entry state */
      Dmsg1(400, "set %s\n", dev->print_blocked());
   }
   Dmsg1(400, "Exit blocked=%s\n", dev->print_blocked());
   V(dev->mutex);
   return stat;
}


/*
 * Wait for any device to be released, then we return, so 
 *  higher level code can rescan possible devices.
 * 
 * Returns: true  if a device has changed state
 *          false if the total wait time has expired.
 */
bool wait_for_device(JCR *jcr, bool first)
{
   struct timeval tv;
   struct timezone tz;
   struct timespec timeout;
   int stat = 0;
   bool ok = true;
   const int wait_time = 5 * 60;       /* wait 5 minutes */

   Dmsg0(100, "Enter wait_for_device\n");
   P(device_release_mutex);

   if (first) {
      Jmsg(jcr, M_MOUNT, 0, _("Job %s waiting to reserve a device.\n"), jcr->Job);
   }

   gettimeofday(&tv, &tz);
   timeout.tv_nsec = tv.tv_usec * 1000;
   timeout.tv_sec = tv.tv_sec + wait_time;

   Dmsg0(100, "I'm going to wait for a device.\n");

   /* Wait required time */
   stat = pthread_cond_timedwait(&wait_device_release, &device_release_mutex, &timeout);
   Dmsg1(100, "Wokeup from sleep on device stat=%d\n", stat);


   V(device_release_mutex);
   Dmsg1(100, "Return from wait_device ok=%d\n", ok);
   return ok;
}

#ifdef xxx
/*
 * The jcr timers are used for waiting on any device *
 * Returns: true if time doubled
 *          false if max time expired
 */
static bool double_jcr_wait_time(JCR *jcr)
{
   jcr->wait_sec *= 2;               /* double wait time */
   if (jcr->wait_sec > jcr->max_wait) {   /* but not longer than maxtime */
      jcr->wait_sec = jcr->max_wait;
   }
   jcr->num_wait++;
   jcr->rem_wait_sec = jcr->wait_sec;
   if (jcr->num_wait >= jcr->max_num_wait) {
      return false;
   }
   return true;
}
#endif
