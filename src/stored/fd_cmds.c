/*
 * This file handles commands from the File daemon.
 *
 *  Kern Sibbald, MM
 *
 * We get here because the Director has initiated a Job with
 *  the Storage daemon, then done the same with the File daemon,
 *  then when the Storage daemon receives a proper connection from
 *  the File daemon, control is passed here to handle the 
 *  subsequent File daemon commands.
 *
 *   Version $Id: fd_cmds.c,v 1.25 2004/09/27 07:01:37 kerns Exp $
 * 
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

/* Imported variables */
extern STORES *me;

/* Static variables */
static char ferrmsg[]      = "3900 Invalid command\n";

/* Imported functions */
extern bool do_append_data(JCR *jcr);
extern bool do_read_data(JCR *jcr);

/* Forward referenced FD commands */
static bool append_open_session(JCR *jcr);
static bool append_close_session(JCR *jcr);
static bool append_data_cmd(JCR *jcr);
static bool append_end_session(JCR *jcr);
static bool read_open_session(JCR *jcr);
static bool read_data_cmd(JCR *jcr);
static bool read_close_session(JCR *jcr);

/* Exported function */
bool bootstrap_cmd(JCR *jcr);

struct s_cmds {
   const char *cmd;
   bool (*func)(JCR *jcr);
};

/*  
 * The following are the recognized commands from the File daemon
 */
static struct s_cmds fd_cmds[] = {
   {"append open",  append_open_session},
   {"append data",  append_data_cmd},
   {"append end",   append_end_session},
   {"append close", append_close_session},
   {"read open",    read_open_session},
   {"read data",    read_data_cmd},
   {"read close",   read_close_session},
   {"bootstrap",    bootstrap_cmd},
   {NULL,	    NULL}		   /* list terminator */
};

/* Commands from the File daemon that require additional scanning */
static char read_open[]       = "read open session = %127s %ld %ld %ld %ld %ld %ld\n";

/* Responses sent to the File daemon */
static char NO_open[]         = "3901 Error session already open\n";
static char NOT_opened[]      = "3902 Error session not opened\n";
static char OK_end[]          = "3000 OK end\n";
static char OK_close[]        = "3000 OK close Status = %d\n";
static char OK_open[]         = "3000 OK open ticket = %d\n";
static char OK_append[]       = "3000 OK append data\n";
static char ERROR_append[]    = "3903 Error append data\n";
static char OK_bootstrap[]    = "3000 OK bootstrap\n";
static char ERROR_bootstrap[] = "3904 Error bootstrap\n";

/* Information sent to the Director */
static char Job_start[] = "3010 Job %s start\n";
static char Job_end[]	= 
   "3099 Job %s end JobStatus=%d JobFiles=%d JobBytes=%s\n";

/*
 * Run a File daemon Job -- File daemon already authorized
 *
 * Basic task here is:
 * - Read a command from the File daemon
 * - Execute it
 *
 */
void run_job(JCR *jcr)
{
   int i;
   bool found, quit;
   BSOCK *fd = jcr->file_bsock;
   BSOCK *dir = jcr->dir_bsock;
   char ec1[30];


   fd->jcr = jcr;
   dir->jcr = jcr;
   Dmsg1(120, "Start run Job=%s\n", jcr->Job);
   bnet_fsend(dir, Job_start, jcr->Job); 
   jcr->start_time = time(NULL);
   jcr->run_time = jcr->start_time;
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);	      /* update director */
   for (quit=false; !quit;) {
      int stat;

      /* Read command coming from the File daemon */
      stat = bnet_recv(fd);
      if (is_bnet_stop(fd)) {	      /* hardeof or error */
	 break; 		      /* connection terminated */
      }
      if (stat <= 0) {
	 continue;		      /* ignore signals and zero length msgs */
      }
      Dmsg1(110, "<filed: %s", fd->msg);
      found = false;
      for (i=0; fd_cmds[i].cmd; i++) {
	 if (strncmp(fd_cmds[i].cmd, fd->msg, strlen(fd_cmds[i].cmd)) == 0) {
	    found = true;		/* indicate command found */
	    if (!fd_cmds[i].func(jcr)) {    /* do command */
	       set_jcr_job_status(jcr, JS_ErrorTerminated);
	       quit = true;
	    }
	    break;
	 }
      }
      if (!found) {		      /* command not found */
         Dmsg1(110, "<filed: Command not found: %s\n", fd->msg);
	 bnet_fsend(fd, ferrmsg);
	 break;
      }
   }
   bnet_sig(fd, BNET_TERMINATE);      /* signal to FD job is done */
   jcr->end_time = time(NULL);
   dequeue_messages(jcr);	      /* send any queued messages */
   set_jcr_job_status(jcr, JS_Terminated);
   bnet_fsend(dir, Job_end, jcr->Job, jcr->JobStatus, jcr->JobFiles,
      edit_uint64(jcr->JobBytes, ec1));
   bnet_sig(dir, BNET_EOD);	      /* send EOD to Director daemon */
   return;
}

	
/*
 *   Append Data command
 *     Open Data Channel and receive Data for archiving
 *     Write the Data to the archive device
 */
static bool append_data_cmd(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "Append data: %s", fd->msg);
   if (jcr->session_opened) {
      Dmsg1(110, "<bfiled: %s", fd->msg);
      jcr->JobType = JT_BACKUP;
      if (do_append_data(jcr)) {
	 return bnet_fsend(fd, OK_append);
      } else {
	 bnet_suppress_error_messages(fd, 1); /* ignore errors at this point */
	 bnet_fsend(fd, ERROR_append);
      }
   } else {
      bnet_fsend(fd, NOT_opened);
   }
   return false;
}

static bool append_end_session(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "store<file: %s", fd->msg);
   if (!jcr->session_opened) {
      bnet_fsend(fd, NOT_opened);
      return false;
   }
   set_jcr_job_status(jcr, JS_Terminated);
   return bnet_fsend(fd, OK_end);
}


/* 
 * Append Open session command
 *
 */
static bool append_open_session(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "Append open session: %s", fd->msg);
   if (jcr->session_opened) {
      bnet_fsend(fd, NO_open);
      return false;
   }

   Dmsg1(110, "Append open session: %s\n", dev_name(jcr->device->dev));
   jcr->session_opened = TRUE;

   /* Send "Ticket" to File Daemon */
   bnet_fsend(fd, OK_open, jcr->VolSessionId);
   Dmsg1(110, ">filed: %s", fd->msg);

   return true;
}

/*
 *   Append Close session command
 *	Close the append session and send back Statistics     
 *	   (need to fix statistics)
 */
static bool append_close_session(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "<filed: %s", fd->msg);
   if (!jcr->session_opened) {
      bnet_fsend(fd, NOT_opened);
      return false;
   }
   /* Send final statistics to File daemon */
   bnet_fsend(fd, OK_close, jcr->JobStatus);
   Dmsg1(120, ">filed: %s", fd->msg);

   bnet_sig(fd, BNET_EOD);	      /* send EOD to File daemon */
       
   Dmsg1(110, "Append close session: %s\n", dev_name(jcr->device->dev));

   jcr->session_opened = false;
   return true;
}

/*
 *   Read Data command
 *     Open Data Channel, read the data from  
 *     the archive device and send to File
 *     daemon.
 */
static bool read_data_cmd(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "Read data: %s", fd->msg);
   if (jcr->session_opened) {
      Dmsg1(120, "<bfiled: %s", fd->msg);
      return do_read_data(jcr);
   } else {
      bnet_fsend(fd, NOT_opened);
      return false;
   }
}

/* 
 * Read Open session command
 *
 *  We need to scan for the parameters of the job
 *    to be restored.
 */
static bool read_open_session(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "%s\n", fd->msg);
   if (jcr->session_opened) {
      bnet_fsend(fd, NO_open);
      return false;
   }

   if (sscanf(fd->msg, read_open, jcr->dcr->VolumeName, &jcr->read_VolSessionId,
	 &jcr->read_VolSessionTime, &jcr->read_StartFile, &jcr->read_EndFile,
	 &jcr->read_StartBlock, &jcr->read_EndBlock) == 7) {
      if (jcr->session_opened) {
	 bnet_fsend(fd, NOT_opened);
	 return false;
      }
      Dmsg4(100, "read_open_session got: JobId=%d Vol=%s VolSessId=%ld VolSessT=%ld\n",
	 jcr->JobId, jcr->dcr->VolumeName, jcr->read_VolSessionId, 
	 jcr->read_VolSessionTime);
      Dmsg4(100, "  StartF=%ld EndF=%ld StartB=%ld EndB=%ld\n",
	 jcr->read_StartFile, jcr->read_EndFile, jcr->read_StartBlock,
	 jcr->read_EndBlock);
   }

   Dmsg1(110, "Read open session: %s\n", dev_name(jcr->device->dev));

   jcr->session_opened = TRUE;
   jcr->JobType = JT_RESTORE;

   /* Send "Ticket" to File Daemon */
   bnet_fsend(fd, OK_open, jcr->VolSessionId);
   Dmsg1(110, ">filed: %s", fd->msg);

   return true;
}

bool bootstrap_cmd(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   POOLMEM *fname = get_pool_memory(PM_FNAME);
   FILE *bs;
   bool ok = false;

   if (jcr->RestoreBootstrap) {
      unlink(jcr->RestoreBootstrap);
      free_pool_memory(jcr->RestoreBootstrap);
   }
   Mmsg(fname, "%s/%s.%s.bootstrap", me->working_directory, me->hdr.name,
      jcr->Job);
   Dmsg1(400, "bootstrap=%s\n", fname);
   jcr->RestoreBootstrap = fname;
   bs = fopen(fname, "a+");           /* create file */
   if (!bs) {
      Jmsg(jcr, M_FATAL, 0, _("Could not create bootstrap file %s: ERR=%s\n"),
	 jcr->RestoreBootstrap, strerror(errno));
      goto bail_out;
   }
   while (bnet_recv(fd) >= 0) {
       Dmsg1(400, "stored<filed: bootstrap file %s\n", fd->msg);
       fputs(fd->msg, bs);
   }
   fclose(bs);
   jcr->bsr = parse_bsr(jcr, jcr->RestoreBootstrap);
   if (!jcr->bsr) {
      Jmsg(jcr, M_FATAL, 0, _("Error parsing bootstrap file.\n"));
      goto bail_out;
   }
   if (debug_level > 20) {
      dump_bsr(jcr->bsr, true);
   }
   ok = true;

bail_out:
   unlink(jcr->RestoreBootstrap);
   free_pool_memory(jcr->RestoreBootstrap);
   jcr->RestoreBootstrap = NULL;
   if (!ok) {
      bnet_fsend(fd, ERROR_bootstrap);
      return false;
   }
   return bnet_fsend(fd, OK_bootstrap);
}


/*
 *   Read Close session command
 *	Close the read session
 */
static bool read_close_session(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;

   Dmsg1(120, "Read close session: %s\n", fd->msg);
   if (!jcr->session_opened) {
      bnet_fsend(fd, NOT_opened);
      return false;
   }
   /* Send final statistics to File daemon */
   bnet_fsend(fd, OK_close);
   Dmsg1(160, ">filed: %s\n", fd->msg);

   bnet_sig(fd, BNET_EOD);	    /* send EOD to File daemon */
       
   jcr->session_opened = FALSE;
   return true;
}
