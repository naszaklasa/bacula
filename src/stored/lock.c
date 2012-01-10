/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2010 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * Collection of Bacula Storage daemon locking software
 *
 *  Kern Sibbald, 2000-2007.  June 2007
 *
 */

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

#ifdef SD_DEBUG_LOCK
const int dbglvl = 0;
#else
const int dbglvl = 500;
#endif


/*
 *
 * The Storage daemon has three locking concepts that must be
 *   understood:
 *
 *  1. dblock    blocking the device, which means that the device
 *               is "marked" in use.  When setting and removing the
                 block, the device is locked, but after dblock is
                 called the device is unlocked.
 *  2. dlock()   simple mutex that locks the device structure. A dlock
 *               can be acquired while a device is blocked if it is not
 *               locked.      
 *  3. r_dlock(locked)  "recursive" dlock, when means that a dlock (mutex)
 *               will be acquired on the device if it is not blocked
 *               by some other thread. If the device was blocked by
 *               the current thread, it will acquire the lock.
 *               If some other thread has set a block on the device,
 *               this call will wait until the device is unblocked.
 *               Can be called with locked true, which means the
 *               dlock is already set
 *
 *  A lock is normally set when modifying the device structure.
 *  A r_lock is normally acquired when you want to block the device
 *    i.e. it will wait until the device is not blocked.
 *  A block is normally set during long operations like writing to
 *    the device.
 *  If you are writing the device, you will normally block and 
 *    lock it.
 *  A lock cannot be violated. No other thread can touch the
 *    device while a lock is set.  
 *  When a block is set, every thread accept the thread that set
 *    the block will block if r_dlock is called.
 *  A device can be blocked for multiple reasons, labeling, writing,
 *    acquiring (opening) the device, waiting for the operator, unmounted,
 *    ...
 *  Under certain conditions the block that is set on a device can be    
 *    stolen and the device can be used by another thread. For example,
 *    a device is blocked because it is waiting for the operator to  
 *    mount a tape.  The operator can then unmount the device, and label
 *    a tape, re-mount it, give back the block, and the job will continue.
 *
 *
 * Functions:
 *
 *   DEVICE::dlock()   does P(m_mutex)     (in dev.h)
 *   DEVICE::dunlock() does V(m_mutex)
 *
 *   DEVICE::r_dlock(locked) allows locking the device when this thread
 *                     already has the device blocked.
 *                    if (!locked)
 *                       dlock()
 *                    if blocked and not same thread that locked
 *                       pthread_cond_wait
 *                    leaves device locked 
 *
 *   DEVICE::r_dunlock() unlocks but does not unblock
 *                    same as dunlock();
 *
 *   DEVICE::dblock(why)  does 
 *                    r_dlock();         (recursive device lock)
 *                    block_device(this, why) 
 *                    r_dunlock()
 *
 *   DEVICE::dunblock does
 *                    dlock()
 *                    unblock_device()
 *                    dunlock()
 *
 *   block_device() does  (must be locked and not blocked at entry)  
 *                    set blocked status
 *                    set our pid
 *
 *   unblock_device() does (must be blocked at entry)
 *                        (locked on entry)
 *                        (locked on exit)
 *                    set unblocked status
 *                    clear pid
 *                    if waiting threads
 *                       pthread_cond_broadcast
 *
 *   steal_device_lock() does (must be locked and blocked at entry)
 *                    save status
 *                    set new blocked status
 *                    set new pid
 *                    unlock()
 *
 *   give_back_device_lock() does (must be blocked but not locked)
 *                    dlock()
 *                    reset blocked status
 *                    save previous blocked
 *                    reset pid
 *                    if waiting threads
 *                       pthread_cond_broadcast
 *
 */


void DEVICE::dblock(int why)
{
   r_dlock();              /* need recursive lock to block */
   block_device(this, why);
   r_dunlock();
}

void DEVICE::dunblock(bool locked)
{
   if (!locked) {
      dlock();
   }
   unblock_device(this);
   dunlock();
}


#ifdef SD_DEBUG_LOCK
void DCR::_dlock(const char *file, int line)
{
   dev->_dlock(file, line);
   m_dev_locked = true;
}
void DCR::_dunlock(const char *file, int line)
{
   m_dev_locked = false;
   dev->_dunlock(file, line);

}

void DEVICE::_dlock(const char *file, int line)
{
   Dmsg3(sd_dbglvl, "dlock from %s:%d precnt=%d\n", file, line, m_count); 
   /* Note, this *really* should be protected by a mutex, but
    *  since it is only debug code we don't worry too much.  
    */
   if (m_count > 0 && pthread_equal(m_pid, pthread_self())) {
      Dmsg4(sd_dbglvl, "Possible DEADLOCK!! lock held by JobId=%u from %s:%d m_count=%d\n", 
            get_jobid_from_tid(m_pid),
            file, line, m_count);
   }
   P(m_mutex);
   m_pid = pthread_self();
   m_count++; 
}

void DEVICE::_dunlock(const char *file, int line)
{
   m_count--; 
   Dmsg3(sd_dbglvl+1, "dunlock from %s:%d postcnt=%d\n", file, line, m_count); 
   V(m_mutex);   
}

void DEVICE::_r_dunlock(const char *file, int line)
{
   this->_dunlock(file, line);
}

#endif


/*
 * This is a recursive lock that checks if the device is blocked.
 *
 * When blocked is set, all threads EXCEPT thread with id no_wait_id
 * must wait. The no_wait_id thread is out obtaining a new volume
 * and preparing the label.
 */
#ifdef SD_DEBUG_LOCK
void DEVICE::_r_dlock(const char *file, int line, bool locked)
{
   Dmsg3(sd_dbglvl+1, "r_dlock blked=%s from %s:%d\n", this->print_blocked(),
         file, line);
#else
void DEVICE::r_dlock(bool locked)
{
#endif
   int stat;
   if (!locked) {
      P(m_mutex); /*    this->dlock();   */
      m_count++;  /*    this->dlock() */
   }
   if (this->blocked() && !pthread_equal(this->no_wait_id, pthread_self())) {
      this->num_waiting++;             /* indicate that I am waiting */
      while (this->blocked()) {
#ifndef HAVE_WIN32
         /* thread id on Win32 may be a struct */
         Dmsg3(sd_dbglvl, "r_dlock blked=%s no_wait=%p me=%p\n", this->print_blocked(),
               this->no_wait_id, pthread_self());
#endif
         if ((stat = pthread_cond_wait(&this->wait, &m_mutex)) != 0) {
            berrno be;
            this->dunlock();
            Emsg1(M_ABORT, 0, _("pthread_cond_wait failure. ERR=%s\n"),
               be.bstrerror(stat));
         }
      }
      this->num_waiting--;             /* no longer waiting */
   }
}

/*
 * Block all other threads from using the device
 *  Device must already be locked.  After this call,
 *  the device is blocked to any thread calling dev->r_lock(),
 *  but the device is not locked (i.e. no P on device).  Also,
 *  the current thread can do slip through the dev->r_lock()
 *  calls without blocking.
 */
void _block_device(const char *file, int line, DEVICE *dev, int state)
{
// ASSERT(lmgr_mutex_is_locked(&dev->m_mutex) == 1);
   ASSERT(dev->blocked() == BST_NOT_BLOCKED);
   dev->set_blocked(state);           /* make other threads wait */
   dev->no_wait_id = pthread_self();  /* allow us to continue */
   Dmsg3(sd_dbglvl, "set blocked=%s from %s:%d\n", dev->print_blocked(), file, line);
}

/*
 * Unblock the device, and wake up anyone who went to sleep.
 * Enter: device locked
 * Exit:  device locked
 */
void _unblock_device(const char *file, int line, DEVICE *dev)
{
   Dmsg3(sd_dbglvl, "unblock %s from %s:%d\n", dev->print_blocked(), file, line);
// ASSERT(lmgr_mutex_is_locked(&dev->m_mutex) == 1);
   ASSERT(dev->blocked());
   dev->set_blocked(BST_NOT_BLOCKED);
   clear_thread_id(dev->no_wait_id);
   if (dev->num_waiting > 0) {
      pthread_cond_broadcast(&dev->wait); /* wake them up */
   }
}

/*
 * Enter with device locked and blocked
 * Exit with device unlocked and blocked by us.
 */
void _steal_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold, int state)
{

   Dmsg3(sd_dbglvl, "steal lock. old=%s from %s:%d\n", dev->print_blocked(),
      file, line);
   hold->dev_blocked = dev->blocked();
   hold->dev_prev_blocked = dev->dev_prev_blocked;
   hold->no_wait_id = dev->no_wait_id;
   dev->set_blocked(state);
   Dmsg1(sd_dbglvl, "steal lock. new=%s\n", dev->print_blocked());
   dev->no_wait_id = pthread_self();
   dev->dunlock();
}

/*
 * Enter with device blocked by us but not locked
 * Exit with device locked, and blocked by previous owner
 */
void _give_back_device_lock(const char *file, int line, DEVICE *dev, bsteal_lock_t *hold)
{
   Dmsg3(sd_dbglvl, "return lock. old=%s from %s:%d\n",
      dev->print_blocked(), file, line);
   dev->dlock();
   dev->set_blocked(hold->dev_blocked);
   dev->dev_prev_blocked = hold->dev_prev_blocked;
   dev->no_wait_id = hold->no_wait_id;
   Dmsg1(sd_dbglvl, "return lock. new=%s\n", dev->print_blocked());
   if (dev->num_waiting > 0) {
      pthread_cond_broadcast(&dev->wait); /* wake them up */
   }
}

const char *DEVICE::print_blocked() const 
{
   switch (m_blocked) {
   case BST_NOT_BLOCKED:
      return "BST_NOT_BLOCKED";
   case BST_UNMOUNTED:
      return "BST_UNMOUNTED";
   case BST_WAITING_FOR_SYSOP:
      return "BST_WAITING_FOR_SYSOP";
   case BST_DOING_ACQUIRE:
      return "BST_DOING_ACQUIRE";
   case BST_WRITING_LABEL:
      return "BST_WRITING_LABEL";
   case BST_UNMOUNTED_WAITING_FOR_SYSOP:
      return "BST_UNMOUNTED_WAITING_FOR_SYSOP";
   case BST_MOUNT:
      return "BST_MOUNT";
   case BST_DESPOOLING:
      return "BST_DESPOOLING";
   case BST_RELEASING:
      return "BST_RELEASING";
   default:
      return _("unknown blocked code");
   }
}


/*
 * Check if the device is blocked or not
 */
bool DEVICE::is_device_unmounted()
{
   bool stat;
   int blk = blocked();
   stat = (blk == BST_UNMOUNTED) ||
          (blk == BST_UNMOUNTED_WAITING_FOR_SYSOP);
   return stat;
}
