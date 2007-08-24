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
 *   Version $Id: fd_cmds.c 4992 2007-06-07 14:46:43Z kerns $
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

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
static bool bootstrap_cmd(JCR *jcr);

/* Exported function */
bool get_bootstrap_file(JCR *jcr, BSOCK *bs);

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
   {NULL,           NULL}                  /* list terminator */
};

/* Commands from the File daemon that require additional scanning */
static char read_open[]       = "read open session = %127s %ld %ld %ld %ld %ld %ld\n";

/* Responses sent to the File daemon */
static char NO_open[]         = "3901 Error session already open\n";
static char NOT_opened[]      = "3902 Error session not opened\n";
static char OK_end[]          = "3000 OK end\n";
static char OK_close[]        = "3000 OK close Status = %d\n";
static char OK_open[]         = "3000 OK open ticket = %d\n";
static char ERROR_append[]    = "3903 Error append data\n";
static char OK_bootstrap[]    = "3000 OK bootstrap\n";
static char ERROR_bootstrap[] = "3904 Error bootstrap\n";

/* Information sent to the Director */
static char Job_start[] = "3010 Job %s start\n";
char Job_end[]   =
   "3099 Job %s end JobStatus=%d JobFiles=%d JobBytes=%s\n";

/*
 * Run a File daemon Job -- File daemon already authorized
 *  Director sends us this command.
 *
 * Basic task here is:
 * - Read a command from the File daemon
 * - Execute it
 *
 */
void run_job(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   char ec1[30];

   dir->set_jcr(jcr);
   Dmsg1(120, "Start run Job=%s\n", jcr->Job);
   bnet_fsend(dir, Job_start, jcr->Job);
   jcr->start_time = time(NULL);
   jcr->run_time = jcr->start_time;
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);          /* update director */
   do_fd_commands(jcr);
   jcr->end_time = time(NULL);
   dequeue_messages(jcr);             /* send any queued messages */
   set_jcr_job_status(jcr, JS_Terminated);
   generate_daemon_event(jcr, "JobEnd");
   bnet_fsend(dir, Job_end, jcr->Job, jcr->JobStatus, jcr->JobFiles,
      edit_uint64(jcr->JobBytes, ec1));
   bnet_sig(dir, BNET_EOD);           /* send EOD to Director daemon */
   return;
}

/*
 * Now talk to the FD and do what he says
 */
void do_fd_commands(JCR *jcr)
{
   int i;
   bool found, quit;
   BSOCK *fd = jcr->file_bsock;

   fd->set_jcr(jcr);
   for (quit=false; !quit;) {
      int stat;

      /* Read command coming from the File daemon */
      stat = bnet_recv(fd);
      if (is_bnet_stop(fd)) {         /* hardeof or error */
         break;                       /* connection terminated */
      }
      if (stat <= 0) {
         continue;                    /* ignore signals and zero length msgs */
      }
      Dmsg1(110, "<filed: %s", fd->msg);
      found = false;
      for (i=0; fd_cmds[i].cmd; i++) {
         if (strncmp(fd_cmds[i].cmd, fd->msg, strlen(fd_cmds[i].cmd)) == 0) {
            found = true;               /* indicate command found */
            if (!fd_cmds[i].func(jcr) || job_canceled(jcr)) {    /* do command */
               set_jcr_job_status(jcr, JS_ErrorTerminated);
               quit = true;
            }
            break;
         }
      }
      if (!found) {                   /* command not found */
         Dmsg1(110, "<filed: Command not found: %s\n", fd->msg);
         bnet_fsend(fd, ferrmsg);
         break;
      }
   }
   bnet_sig(fd, BNET_TERMINATE);      /* signal to FD job is done */
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
         return true;
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

   jcr->session_opened = true;

   /* Send "Ticket" to File Daemon */
   bnet_fsend(fd, OK_open, jcr->VolSessionId);
   Dmsg1(110, ">filed: %s", fd->msg);

   return true;
}

/*
 *   Append Close session command
 *      Close the append session and send back Statistics
 *         (need to fix statistics)
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

   bnet_sig(fd, BNET_EOD);            /* send EOD to File daemon */

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

   if (sscanf(fd->msg, read_open, jcr->read_dcr->VolumeName, &jcr->read_VolSessionId,
         &jcr->read_VolSessionTime, &jcr->read_StartFile, &jcr->read_EndFile,
         &jcr->read_StartBlock, &jcr->read_EndBlock) == 7) {
      if (jcr->session_opened) {
         bnet_fsend(fd, NOT_opened);
         return false;
      }
      Dmsg4(100, "read_open_session got: JobId=%d Vol=%s VolSessId=%ld VolSessT=%ld\n",
         jcr->JobId, jcr->read_dcr->VolumeName, jcr->read_VolSessionId,
         jcr->read_VolSessionTime);
      Dmsg4(100, "  StartF=%ld EndF=%ld StartB=%ld EndB=%ld\n",
         jcr->read_StartFile, jcr->read_EndFile, jcr->read_StartBlock,
         jcr->read_EndBlock);
   }

   jcr->session_opened = true;
   jcr->JobType = JT_RESTORE;

   /* Send "Ticket" to File Daemon */
   bnet_fsend(fd, OK_open, jcr->VolSessionId);
   Dmsg1(110, ">filed: %s", fd->msg);

   return true;
}

static bool bootstrap_cmd(JCR *jcr)
{
   return get_bootstrap_file(jcr, jcr->file_bsock);
}

static pthread_mutex_t bsr_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t bsr_uniq = 0;

bool get_bootstrap_file(JCR *jcr, BSOCK *sock)
{
   POOLMEM *fname = get_pool_memory(PM_FNAME);
   FILE *bs;
   bool ok = false;

   if (jcr->RestoreBootstrap) {
      unlink(jcr->RestoreBootstrap);
      free_pool_memory(jcr->RestoreBootstrap);
   }
   P(bsr_mutex);
   bsr_uniq++;
   Mmsg(fname, "%s/%s.%s.%d.bootstrap", me->working_directory, me->hdr.name,
      jcr->Job, bsr_uniq);
   V(bsr_mutex);
   Dmsg1(400, "bootstrap=%s\n", fname);
   jcr->RestoreBootstrap = fname;
   bs = fopen(fname, "a+b");           /* create file */
   if (!bs) {
      Jmsg(jcr, M_FATAL, 0, _("Could not create bootstrap file %s: ERR=%s\n"),
         jcr->RestoreBootstrap, strerror(errno));
      goto bail_out;
   }
   Dmsg0(10, "=== Bootstrap file ===\n");
   while (bnet_recv(sock) >= 0) {
       Dmsg1(10, "%s", sock->msg);
       fputs(sock->msg, bs);
   }
   fclose(bs);
   Dmsg0(10, "=== end bootstrap file ===\n");
   jcr->bsr = parse_bsr(jcr, jcr->RestoreBootstrap);
   if (!jcr->bsr) {
      Jmsg(jcr, M_FATAL, 0, _("Error parsing bootstrap file.\n"));
      goto bail_out;
   }
   if (debug_level >= 10) {
      dump_bsr(jcr->bsr, true);
   }
   ok = true;

bail_out:
   unlink(jcr->RestoreBootstrap);
   free_pool_memory(jcr->RestoreBootstrap);
   jcr->RestoreBootstrap = NULL;
   if (!ok) {
      bnet_fsend(sock, ERROR_bootstrap);
      return false;
   }
   return bnet_fsend(sock, OK_bootstrap);
}


/*
 *   Read Close session command
 *      Close the read session
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

   bnet_sig(fd, BNET_EOD);          /* send EOD to File daemon */

   jcr->session_opened = false;
   return true;
}
