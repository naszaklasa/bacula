/*
 *
 *   Bacula Director -- fd_cmds.c -- send commands to File daemon
 *
 *     Kern Sibbald, October MM
 *
 *    This routine is run as a separate thread.  There may be more
 *    work to be done to make it totally reentrant!!!!
 *
 *  Utility functions for sending info to File Daemon.
 *   These functions are used by both backup and verify.
 *
 *   Version $Id: fd_cmds.c,v 1.78.2.1 2006/03/24 16:35:23 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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
#include "dird.h"

/* Commands sent to File daemon */
static char filesetcmd[]  = "fileset%s\n"; /* set full fileset */
static char jobcmd[]      = "JobId=%s Job=%s SDid=%u SDtime=%u Authorization=%s\n";
/* Note, mtime_only is not used here -- implemented as file option */
static char levelcmd[]    = "level = %s%s mtime_only=%d\n";
static char runbefore[]   = "RunBeforeJob %s\n";
static char runafter[]    = "RunAfterJob %s\n";


/* Responses received from File daemon */
static char OKinc[]       = "2000 OK include\n";
static char OKjob[]       = "2000 OK Job";
static char OKbootstrap[] = "2000 OK bootstrap\n";
static char OKlevel[]     = "2000 OK level\n";
static char OKRunBefore[] = "2000 OK RunBefore\n";
static char OKRunAfter[]  = "2000 OK RunAfter\n";

/* Forward referenced functions */

/* External functions */
extern int debug_level;
extern DIRRES *director;
extern int FDConnectTimeout;

#define INC_LIST 0
#define EXC_LIST 1

/*
 * Open connection with File daemon.
 * Try connecting every retry_interval (default 10 sec), and
 *   give up after max_retry_time (default 30 mins).
 */

int connect_to_file_daemon(JCR *jcr, int retry_interval, int max_retry_time,
                           int verbose)
{
   BSOCK   *fd;
   char ed1[30];

   if (!jcr->file_bsock) {
      fd = bnet_connect(jcr, retry_interval, max_retry_time,
           _("File daemon"), jcr->client->address,
           NULL, jcr->client->FDport, verbose);
      if (fd == NULL) {
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         return 0;
      }
      Dmsg0(10, "Opened connection with File daemon\n");
   } else {
      fd = jcr->file_bsock;           /* use existing connection */
   }
   fd->res = (RES *)jcr->client;      /* save resource in BSOCK */
   jcr->file_bsock = fd;
   set_jcr_job_status(jcr, JS_Running);

   if (!authenticate_file_daemon(jcr)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }

   /*
    * Now send JobId and authorization key
    */
   bnet_fsend(fd, jobcmd, edit_int64(jcr->JobId, ed1), jcr->Job, jcr->VolSessionId,
      jcr->VolSessionTime, jcr->sd_auth_key);
   if (strcmp(jcr->sd_auth_key, "dummy") != 0) {
      memset(jcr->sd_auth_key, 0, strlen(jcr->sd_auth_key));
   }
   Dmsg1(100, ">filed: %s", fd->msg);
   if (bget_dirmsg(fd) > 0) {
       Dmsg1(110, "<filed: %s", fd->msg);
       if (strncmp(fd->msg, OKjob, strlen(OKjob)) != 0) {
          Jmsg(jcr, M_FATAL, 0, _("File daemon \"%s\" rejected Job command: %s\n"),
             jcr->client->hdr.name, fd->msg);
          set_jcr_job_status(jcr, JS_ErrorTerminated);
          return 0;
       } else if (jcr->db) {
          CLIENT_DBR cr;
          memset(&cr, 0, sizeof(cr));
          bstrncpy(cr.Name, jcr->client->hdr.name, sizeof(cr.Name));
          cr.AutoPrune = jcr->client->AutoPrune;
          cr.FileRetention = jcr->client->FileRetention;
          cr.JobRetention = jcr->client->JobRetention;
          bstrncpy(cr.Uname, fd->msg+strlen(OKjob)+1, sizeof(cr.Uname));
          if (!db_update_client_record(jcr, jcr->db, &cr)) {
             Jmsg(jcr, M_WARNING, 0, _("Error updating Client record. ERR=%s\n"),
                db_strerror(jcr->db));
          }
       }
   } else {
      Jmsg(jcr, M_FATAL, 0, _("FD gave bad response to JobId command: %s\n"),
         bnet_strerror(fd));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }
   return 1;
}

/*
 * This subroutine edits the last job start time into a
 *   "since=date/time" buffer that is returned in the
 *   variable since.  This is used for display purposes in
 *   the job report.  The time in jcr->stime is later
 *   passed to tell the File daemon what to do.
 */
void get_level_since_time(JCR *jcr, char *since, int since_len)
{
   int JobLevel;

   since[0] = 0;
   if (jcr->cloned) {
      if (jcr->stime && jcr->stime[0]) {
         bstrncpy(since, _(", since="), since_len);
         bstrncat(since, jcr->stime, since_len);
      }
      return;
   }
   if (!jcr->stime) {
      jcr->stime = get_pool_memory(PM_MESSAGE);
   } 
   jcr->stime[0] = 0;
   /* Lookup the last FULL backup job to get the time/date for a
    * differential or incremental save.
    */
   switch (jcr->JobLevel) {
   case L_DIFFERENTIAL:
   case L_INCREMENTAL:
      /* Look up start time of last job */
      jcr->jr.JobId = 0;     /* flag for db_find_job_start time */
      if (!db_find_job_start_time(jcr, jcr->db, &jcr->jr, &jcr->stime)) {
         /* No job found, so upgrade this one to Full */
         Jmsg(jcr, M_INFO, 0, "%s", db_strerror(jcr->db));
         Jmsg(jcr, M_INFO, 0, _("No prior or suitable Full backup found. Doing FULL backup.\n"));
         bsnprintf(since, since_len, _(" (upgraded from %s)"),
            level_to_str(jcr->JobLevel));
         jcr->JobLevel = jcr->jr.JobLevel = L_FULL;
      } else {
         if (jcr->job->rerun_failed_levels) {
            if (db_find_failed_job_since(jcr, jcr->db, &jcr->jr, jcr->stime, JobLevel)) {
               Jmsg(jcr, M_INFO, 0, _("Prior failed job found. Upgrading to %s.\n"),
                  level_to_str(JobLevel));
               bsnprintf(since, since_len, _(" (upgraded from %s)"),
                  level_to_str(jcr->JobLevel));
               jcr->JobLevel = jcr->jr.JobLevel = JobLevel;
               jcr->jr.JobId = jcr->JobId;
               break;
            }
         }
         bstrncpy(since, _(", since="), since_len);
         bstrncat(since, jcr->stime, since_len);
      }
      jcr->jr.JobId = jcr->JobId;
      break;
   }
   Dmsg2(100, "Level=%c last start time=%s\n", jcr->JobLevel, jcr->stime);
}

static void send_since_time(JCR *jcr)
{
   BSOCK   *fd = jcr->file_bsock;
   utime_t stime;
   char ed1[50];

   stime = str_to_utime(jcr->stime);
   bnet_fsend(fd, levelcmd, _("since_utime "), edit_uint64(stime, ed1), 0);
   while (bget_dirmsg(fd) >= 0) {  /* allow him to poll us to sync clocks */
      Jmsg(jcr, M_INFO, 0, "%s\n", fd->msg);
   }
}


/*
 * Send level command to FD.
 * Used for backup jobs and estimate command.
 */
bool send_level_command(JCR *jcr)
{
   BSOCK   *fd = jcr->file_bsock;
   /*
    * Send Level command to File daemon
    */
   switch (jcr->JobLevel) {
   case L_BASE:
      bnet_fsend(fd, levelcmd, "base", " ", 0);
      break;
   /* L_NONE is the console, sending something off to the FD */
   case L_NONE:
   case L_FULL:
      bnet_fsend(fd, levelcmd, "full", " ", 0);
      break;
   case L_DIFFERENTIAL:
      bnet_fsend(fd, levelcmd, "differential", " ", 0);
      send_since_time(jcr);
      break;
   case L_INCREMENTAL:
      bnet_fsend(fd, levelcmd, "incremental", " ", 0);
      send_since_time(jcr);
      break;
   case L_SINCE:
   default:
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented backup level %d %c\n"),
         jcr->JobLevel, jcr->JobLevel);
      return 0;
   }
   Dmsg1(120, ">filed: %s", fd->msg);
   if (!response(jcr, fd, OKlevel, "Level", DISPLAY_ERROR)) {
      return 0;
   }
   return 1;
}

/*
 * Send either an Included or an Excluded list to FD
 */
static int send_fileset(JCR *jcr)
{
   FILESET *fileset = jcr->fileset;
   BSOCK   *fd = jcr->file_bsock;
   int num;
   bool include = true;

   for ( ;; ) {
      if (include) {
         num = fileset->num_includes;
      } else {
         num = fileset->num_excludes;
      }
      for (int i=0; i<num; i++) {
         BPIPE *bpipe;
         FILE *ffd;
         char buf[2000];
         char *p;
         int optlen, stat;
         INCEXE *ie;
         int j, k;

         if (include) {
            ie = fileset->include_items[i];
            bnet_fsend(fd, "I\n");
         } else {
            ie = fileset->exclude_items[i];
            bnet_fsend(fd, "E\n");
         }
         for (j=0; j<ie->num_opts; j++) {
            FOPTS *fo = ie->opts_list[j];
            bnet_fsend(fd, "O %s\n", fo->opts);
            for (k=0; k<fo->regex.size(); k++) {
               bnet_fsend(fd, "R %s\n", fo->regex.get(k));
            }
            for (k=0; k<fo->regexdir.size(); k++) {
               bnet_fsend(fd, "RD %s\n", fo->regexdir.get(k));
            }
            for (k=0; k<fo->regexfile.size(); k++) {
               bnet_fsend(fd, "RF %s\n", fo->regexfile.get(k));
            }
            for (k=0; k<fo->wild.size(); k++) {
               bnet_fsend(fd, "W %s\n", fo->wild.get(k));
            }
            for (k=0; k<fo->wilddir.size(); k++) {
               bnet_fsend(fd, "WD %s\n", fo->wilddir.get(k));
            }
            for (k=0; k<fo->wildfile.size(); k++) {
               bnet_fsend(fd, "WF %s\n", fo->wildfile.get(k));
            }
            for (k=0; k<fo->base.size(); k++) {
               bnet_fsend(fd, "B %s\n", fo->base.get(k));
            }
            for (k=0; k<fo->fstype.size(); k++) {
               bnet_fsend(fd, "X %s\n", fo->fstype.get(k));
            }
            if (fo->reader) {
               bnet_fsend(fd, "D %s\n", fo->reader);
            }
            if (fo->writer) {
               bnet_fsend(fd, "T %s\n", fo->writer);
            }
            bnet_fsend(fd, "N\n");
         }

         for (j=0; j<ie->name_list.size(); j++) {
            p = (char *)ie->name_list.get(j);
            switch (*p) {
            case '|':
               p++;                      /* skip over the | */
               fd->msg = edit_job_codes(jcr, fd->msg, p, "");
               bpipe = open_bpipe(fd->msg, 0, "r");
               if (!bpipe) {
                  berrno be;
                  Jmsg(jcr, M_FATAL, 0, _("Cannot run program: %s. ERR=%s\n"),
                     p, be.strerror());
                  goto bail_out;
               }
               bstrncpy(buf, "F ", sizeof(buf));
               Dmsg1(500, "Opts=%s\n", buf);
               optlen = strlen(buf);
               while (fgets(buf+optlen, sizeof(buf)-optlen, bpipe->rfd)) {
                  fd->msglen = Mmsg(fd->msg, "%s", buf);
                  Dmsg2(500, "Inc/exc len=%d: %s", fd->msglen, fd->msg);
                  if (!bnet_send(fd)) {
                     Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
                     goto bail_out;
                  }
               }
               if ((stat=close_bpipe(bpipe)) != 0) {
                  berrno be;
                  Jmsg(jcr, M_FATAL, 0, _("Error running program: %s. ERR=%s\n"),
                     p, be.strerror(stat));
                  goto bail_out;
               }
               break;
            case '<':
               p++;                      /* skip over < */
               if ((ffd = fopen(p, "r")) == NULL) {
                  berrno be;
                  Jmsg(jcr, M_FATAL, 0, _("Cannot open included file: %s. ERR=%s\n"),
                     p, be.strerror());
                  goto bail_out;
               }
               bstrncpy(buf, "F ", sizeof(buf));
               Dmsg1(500, "Opts=%s\n", buf);
               optlen = strlen(buf);
               while (fgets(buf+optlen, sizeof(buf)-optlen, ffd)) {
                  fd->msglen = Mmsg(fd->msg, "%s", buf);
                  if (!bnet_send(fd)) {
                     Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
                     goto bail_out;
                  }
               }
               fclose(ffd);
               break;
            case '\\':
               p++;                      /* skip over \ */
               /* Note, fall through wanted */
            default:
               pm_strcpy(fd->msg, "F ");
               fd->msglen = pm_strcat(fd->msg, p);
               Dmsg1(500, "Inc/Exc name=%s\n", fd->msg);
               if (!bnet_send(fd)) {
                  Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
                  goto bail_out;
               }
               break;
            }
         }
         bnet_fsend(fd, "N\n");
      }
      if (!include) {                 /* If we just did excludes */
         break;                       /*   all done */
      }
      include = false;                /* Now do excludes */
   }

   bnet_sig(fd, BNET_EOD);            /* end of data */
   if (!response(jcr, fd, OKinc, "Include", DISPLAY_ERROR)) {
      goto bail_out;
   }
   return 1;

bail_out:
   set_jcr_job_status(jcr, JS_ErrorTerminated);
   return 0;

}


/*
 * Send include list to File daemon
 */
bool send_include_list(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   if (jcr->fileset->new_include) {
      bnet_fsend(fd, filesetcmd, jcr->fileset->enable_vss ? " vss=1" : "");
      return send_fileset(jcr);
   }
   return true;
}


/*
 * Send exclude list to File daemon
 *   Under the new scheme, the Exclude list
 *   is part of the FileSet sent with the
 *   "include_list" above.
 */
bool send_exclude_list(JCR *jcr)
{
   return true;
}


/*
 * Send bootstrap file if any to the File daemon.
 *  This is used for restore and verify VolumeToCatalog
 */
bool send_bootstrap_file(JCR *jcr)
{
   FILE *bs;
   char buf[1000];
   BSOCK *fd = jcr->file_bsock;
   const char *bootstrap = "bootstrap\n";

   Dmsg1(400, "send_bootstrap_file: %s\n", jcr->RestoreBootstrap);
   if (!jcr->RestoreBootstrap) {
      return 1;
   }
   bs = fopen(jcr->RestoreBootstrap, "r");
   if (!bs) {
      berrno be;
      Jmsg(jcr, M_FATAL, 0, _("Could not open bootstrap file %s: ERR=%s\n"),
         jcr->RestoreBootstrap, be.strerror());
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }
   bnet_fsend(fd, bootstrap);
   while (fgets(buf, sizeof(buf), bs)) {
      bnet_fsend(fd, "%s", buf);
   }
   bnet_sig(fd, BNET_EOD);
   fclose(bs);
   if (jcr->unlink_bsr) {
      unlink(jcr->RestoreBootstrap);
      jcr->unlink_bsr = false;
   }                         
   if (!response(jcr, fd, OKbootstrap, "Bootstrap", DISPLAY_ERROR)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }
   return 1;
}

/*
 * Send ClientRunBeforeJob and ClientRunAfterJob to File daemon
 */
int send_run_before_and_after_commands(JCR *jcr)
{
   POOLMEM *msg = get_pool_memory(PM_FNAME);
   BSOCK *fd = jcr->file_bsock;
   if (jcr->job->ClientRunBeforeJob) {
      pm_strcpy(msg, jcr->job->ClientRunBeforeJob);
      bash_spaces(msg);
      bnet_fsend(fd, runbefore, msg);
      if (!response(jcr, fd, OKRunBefore, "ClientRunBeforeJob", DISPLAY_ERROR)) {
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         free_pool_memory(msg);
         return 0;
      }
   }
   if (jcr->job->ClientRunAfterJob) {
      fd->msglen = pm_strcpy(msg, jcr->job->ClientRunAfterJob);
      bash_spaces(msg);
      bnet_fsend(fd, runafter, msg);
      if (!response(jcr, fd, OKRunAfter, "ClientRunAfterJob", DISPLAY_ERROR)) {
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         free_pool_memory(msg);
         return 0;
      }
   }
   free_pool_memory(msg);
   return 1;
}


/*
 * Read the attributes from the File daemon for
 * a Verify job and store them in the catalog.
 */
int get_attributes_and_put_in_catalog(JCR *jcr)
{
   BSOCK   *fd;
   int n = 0;
   ATTR_DBR ar;

   fd = jcr->file_bsock;
   jcr->jr.FirstIndex = 1;
   memset(&ar, 0, sizeof(ar));
   jcr->FileIndex = 0;

   Dmsg0(120, "bdird: waiting to receive file attributes\n");
   /* Pickup file attributes and signature */
   while (!fd->errors && (n = bget_dirmsg(fd)) > 0) {

   /*****FIXME****** improve error handling to stop only on
    * really fatal problems, or the number of errors is too
    * large.
    */
      long file_index;
      int stream, len;
      char *attr, *p, *fn;
      char Opts_SIG[MAXSTRING];      /* either Verify opts or MD5/SHA1 signature */
      char SIG[MAXSTRING];

      jcr->fname = check_pool_memory_size(jcr->fname, fd->msglen);
      if ((len = sscanf(fd->msg, "%ld %d %s", &file_index, &stream, Opts_SIG)) != 3) {
         Jmsg(jcr, M_FATAL, 0, _("<filed: bad attributes, expected 3 fields got %d\n"
"msglen=%d msg=%s\n"), len, fd->msglen, fd->msg);
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         return 0;
      }
      p = fd->msg;
      skip_nonspaces(&p);             /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Stream */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Opts_SHA1 */
      p++;                            /* skip space */
      fn = jcr->fname;
      while (*p != 0) {
         *fn++ = *p++;                /* copy filename */
      }
      *fn = *p++;                     /* term filename and point to attribs */
      attr = p;

      if (stream == STREAM_UNIX_ATTRIBUTES || stream == STREAM_UNIX_ATTRIBUTES_EX) {
         jcr->JobFiles++;
         jcr->FileIndex = file_index;
         ar.attr = attr;
         ar.fname = jcr->fname;
         ar.FileIndex = file_index;
         ar.Stream = stream;
         ar.link = NULL;
         ar.JobId = jcr->JobId;
         ar.ClientId = jcr->ClientId;
         ar.PathId = 0;
         ar.FilenameId = 0;
         ar.Sig = NULL;
         ar.SigType = 0;

         Dmsg2(111, "dird<filed: stream=%d %s\n", stream, jcr->fname);
         Dmsg1(120, "dird<filed: attr=%s\n", attr);

         if (!db_create_file_attributes_record(jcr, jcr->db, &ar)) {
            Jmsg1(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
            set_jcr_job_status(jcr, JS_Error);
            continue;
         }
         jcr->FileId = ar.FileId;
      } else if (stream == STREAM_MD5_SIGNATURE || stream == STREAM_SHA1_SIGNATURE) {
         if (jcr->FileIndex != (uint32_t)file_index) {
            Jmsg2(jcr, M_ERROR, 0, _("MD5/SHA1 index %d not same as attributes %d\n"),
               file_index, jcr->FileIndex);
            set_jcr_job_status(jcr, JS_Error);
            continue;
         }
         db_escape_string(SIG, Opts_SIG, strlen(Opts_SIG));
         Dmsg2(120, "SIGlen=%d SIG=%s\n", strlen(SIG), SIG);
         if (!db_add_SIG_to_file_record(jcr, jcr->db, jcr->FileId, SIG,
                   stream==STREAM_MD5_SIGNATURE?MD5_SIG:SHA1_SIG)) {
            Jmsg1(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
            set_jcr_job_status(jcr, JS_Error);
         }
      }
      jcr->jr.JobFiles = jcr->JobFiles = file_index;
      jcr->jr.LastIndex = file_index;
   }
   if (is_bnet_error(fd)) {
      Jmsg1(jcr, M_FATAL, 0, _("<filed: Network error getting attributes. ERR=%s\n"),
                        bnet_strerror(fd));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }

   set_jcr_job_status(jcr, JS_Terminated);
   return 1;
}
