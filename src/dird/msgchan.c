/*
 *
 *   Bacula Director -- msgchan.c -- handles the message channel
 *    to the Storage daemon and the File daemon.
 *
 *     Kern Sibbald, August MM
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *    Open a message channel with the Storage daemon
 *	to authenticate ourself and to pass the JobId.
 *    Create a thread to interact with the Storage daemon
 *	who returns a job status and requests Catalog services, etc.
 *
 *   Version $Id: msgchan.c,v 1.32.4.1.2.1 2005/04/05 17:23:55 kerns Exp $
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald

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

/* Commands sent to Storage daemon */
static char jobcmd[]     = "JobId=%d job=%s job_name=%s client_name=%s \
type=%d level=%d FileSet=%s NoAttr=%d SpoolAttr=%d FileSetMD5=%s SpoolData=%d";
static char use_device[] = "use device=%s media_type=%s pool_name=%s pool_type=%s\n";

/* Response from Storage daemon */
static char OKjob[]      = "3000 OK Job SDid=%d SDtime=%d Authorization=%100s\n";
static char OK_device[]  = "3000 OK use device\n";

/* Storage Daemon requests */
static char Job_start[]  = "3010 Job %127s start\n";
static char Job_end[]	 = 
   "3099 Job %127s end JobStatus=%d JobFiles=%d JobBytes=%" lld "\n";
static char Job_status[] = "3012 Job %127s jobstatus %d\n";

/* Forward referenced functions */
extern "C" void *msg_thread(void *arg);

/*
 * Establish a message channel connection with the Storage daemon
 * and perform authentication. 
 */
bool connect_to_storage_daemon(JCR *jcr, int retry_interval,	
			      int max_retry_time, int verbose)
{
   BSOCK *sd;
   STORE *store;

   if (!jcr->storage[0]) {
      Jmsg(jcr, M_FATAL, 0, _("Attempt to contact the Storage daemon with no Storage resource.\n"));
      return false;
   }
   store = (STORE *)jcr->storage[0]->first();

   /*
    *  Open message channel with the Storage daemon   
    */
   Dmsg2(200, "bnet_connect to Storage daemon %s:%d\n", store->address,
      store->SDport);
   sd = bnet_connect(jcr, retry_interval, max_retry_time,
          _("Storage daemon"), store->address, 
	  NULL, store->SDport, verbose);
   if (sd == NULL) {
      return false;
   }
   sd->res = (RES *)store;	  /* save pointer to other end */
   jcr->store_bsock = sd;

   if (!authenticate_storage_daemon(jcr, store)) {
      return false;
   }
   return true;
}

/*
 * Start a job with the Storage daemon
 */
int start_storage_daemon_job(JCR *jcr)
{
   int status = 0;
   STORE *storage;
   BSOCK *sd;
   char auth_key[100];
   POOL_MEM device_name, pool_name, pool_type, media_type;
   int i;

   sd = jcr->store_bsock;
   /*
    * Now send JobId and permissions, and get back the authorization key.
    */
   bash_spaces(jcr->job->hdr.name);
   bash_spaces(jcr->client->hdr.name);
   bash_spaces(jcr->fileset->hdr.name);
   if (jcr->fileset->MD5[0] == 0) {
      strcpy(jcr->fileset->MD5, "**Dummy**");
   }
   bnet_fsend(sd, jobcmd, jcr->JobId, jcr->Job, jcr->job->hdr.name, 
	      jcr->client->hdr.name, jcr->JobType, jcr->JobLevel, 
	      jcr->fileset->hdr.name, !jcr->pool->catalog_files,
	      jcr->job->SpoolAttributes, jcr->fileset->MD5, jcr->spool_data);
   Dmsg1(200, "Jobcmd=%s\n", sd->msg);
   unbash_spaces(jcr->job->hdr.name);
   unbash_spaces(jcr->client->hdr.name);
   unbash_spaces(jcr->fileset->hdr.name);
   if (bget_dirmsg(sd) > 0) {
       Dmsg1(110, "<stored: %s", sd->msg);
       if (sscanf(sd->msg, OKjob, &jcr->VolSessionId, 
		  &jcr->VolSessionTime, &auth_key) != 3) {
          Dmsg1(100, "BadJob=%s\n", sd->msg);
          Jmsg(jcr, M_FATAL, 0, _("Storage daemon rejected Job command: %s\n"), sd->msg);
	  return 0;
       } else {
	  jcr->sd_auth_key = bstrdup(auth_key);
          Dmsg1(150, "sd_auth_key=%s\n", jcr->sd_auth_key);
       }
   } else {
      Jmsg(jcr, M_FATAL, 0, _("<stored: bad response to Job command: %s\n"),
	 bnet_strerror(sd));
      return 0;
   }

   /*
    * Send use device = xxx media = yyy pool = zzz
    */

   for (i=0; i < MAX_STORE; i++) {
      if (jcr->storage[i]) {
	 storage = (STORE *)jcr->storage[i]->first();
	 pm_strcpy(device_name, storage->dev_name);
	 pm_strcpy(media_type, storage->media_type);
	 pm_strcpy(pool_type, jcr->pool->pool_type);
	 pm_strcpy(pool_name, jcr->pool->hdr.name);
	 bash_spaces(device_name);
	 bash_spaces(media_type);
	 bash_spaces(pool_type);
	 bash_spaces(pool_name);
	 bnet_fsend(sd, use_device, device_name.c_str(), 
		    media_type.c_str(), pool_name.c_str(), pool_type.c_str());
         Dmsg1(110, ">stored: %s", sd->msg);
         status = response(jcr, sd, OK_device, "Use Device", NO_DISPLAY);
	 if (!status) {
	    pm_strcpy(pool_type, sd->msg); /* save message */
            Jmsg(jcr, M_FATAL, 0, _("\n"
               "     Storage daemon didn't accept Device \"%s\" because:\n     %s"),
	       device_name.c_str(), pool_type.c_str()/* sd->msg */);
	 }
      }
   }
   return status;
}

/* 
 * Start a thread to handle Storage daemon messages and
 *  Catalog requests.
 */
int start_storage_daemon_message_thread(JCR *jcr)
{
   int status;
   pthread_t thid;

   P(jcr->mutex);
   jcr->use_count++;		      /* mark in use by msg thread */
   jcr->sd_msg_thread_done = false;
   jcr->SD_msg_chan = 0;
   V(jcr->mutex);
   Dmsg0(100, "Start SD msg_thread.\n");
   if ((status=pthread_create(&thid, NULL, msg_thread, (void *)jcr)) != 0) {
      Jmsg1(jcr, M_ABORT, 0, _("Cannot create message thread: %s\n"), strerror(status));
   }	     
   Dmsg0(100, "SD msg_thread started.\n");
   /* Wait for thread to start */
   while (jcr->SD_msg_chan == 0) {
      bmicrosleep(0, 50);
   }  
   return 1;
}

extern "C" void msg_thread_cleanup(void *arg)
{
   JCR *jcr = (JCR *)arg;
   Dmsg0(200, "End msg_thread\n");
   db_end_transaction(jcr, jcr->db);	   /* terminate any open transaction */
   P(jcr->mutex);
   jcr->sd_msg_thread_done = true;
   pthread_cond_broadcast(&jcr->term_wait); /* wakeup any waiting threads */
   jcr->SD_msg_chan = 0;
   V(jcr->mutex);
   free_jcr(jcr);		      /* release jcr */
}

/*
 * Handle the message channel (i.e. requests from the
 *  Storage daemon).
 * Note, we are running in a separate thread.
 */
extern "C"
void *msg_thread(void *arg)
{
   JCR *jcr = (JCR *)arg;
   BSOCK *sd;
   int JobStatus;
   char Job[MAX_NAME_LENGTH];
   uint32_t JobFiles;
   uint64_t JobBytes;
   int stat;

   pthread_detach(pthread_self());
   jcr->SD_msg_chan = pthread_self();
   pthread_cleanup_push(msg_thread_cleanup, arg);
   sd = jcr->store_bsock;

   /* Read the Storage daemon's output.
    */
   Dmsg0(100, "Start msg_thread loop\n");
   while ((stat=bget_dirmsg(sd)) >= 0) {
      Dmsg1(200, "<stored: %s", sd->msg);
      if (sscanf(sd->msg, Job_start, &Job) == 1) {
	 continue;
      }
      if (sscanf(sd->msg, Job_end, &Job, &JobStatus, &JobFiles,
		 &JobBytes) == 4) {
	 jcr->SDJobStatus = JobStatus; /* termination status */
	 jcr->SDJobFiles = JobFiles;
	 jcr->SDJobBytes = JobBytes;
	 break;
      }     
      if (sscanf(sd->msg, Job_status, &Job, &JobStatus) == 2) {
	 jcr->SDJobStatus = JobStatus; /* current status */
	 continue;
      }
   }
   if (is_bnet_error(sd)) {		      
      jcr->SDJobStatus = JS_ErrorTerminated;
   }
   pthread_cleanup_pop(1);
   return NULL;
}

void wait_for_storage_daemon_termination(JCR *jcr)
{
   int cancel_count = 0;
   /* Now wait for Storage daemon to terminate our message thread */
   set_jcr_job_status(jcr, JS_WaitSD);
   P(jcr->mutex);
   while (!jcr->sd_msg_thread_done) {
      struct timeval tv;
      struct timezone tz;
      struct timespec timeout;

      gettimeofday(&tv, &tz);
      timeout.tv_nsec = 0;
      timeout.tv_sec = tv.tv_sec + 10; /* wait 10 seconds */
      Dmsg0(300, "I'm waiting for message thread termination.\n");
      pthread_cond_timedwait(&jcr->term_wait, &jcr->mutex, &timeout);
      if (job_canceled(jcr)) {
	 cancel_count++;
      }
      /* Give SD 30 seconds to clean up after cancel */
      if (cancel_count == 3) {
	 break;
      }
   }
   V(jcr->mutex);
   set_jcr_job_status(jcr, JS_Terminated);
}
