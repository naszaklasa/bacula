/*
 *
 *   Bacula Director -- verify.c -- responsible for running file verification
 *
 *     Kern Sibbald, October MM
 *
 *  Basic tasks done here:
 *     Open DB
 *     Open connection with File daemon and pass him commands
 *	 to do the verify.
 *     When the File daemon sends the attributes, compare them to
 *	 what is in the DB.
 *
 *   Version $Id: verify.c,v 1.59.2.1 2005/02/27 21:53:28 kerns Exp $
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
#include "dird.h"
#include "findlib/find.h"

/* Imported Global Variables */
extern int debug_level;

/* Commands sent to File daemon */
static char verifycmd[]    = "verify level=%s\n";
static char storaddr[]     = "storage address=%s port=%d ssl=0\n";
static char sessioncmd[]   = "session %s %ld %ld %ld %ld %ld %ld\n";  

/* Responses received from File daemon */
static char OKverify[]    = "2000 OK verify\n";
static char OKstore[]     = "2000 OK storage\n";
static char OKsession[]   = "2000 OK session\n";

/* Forward referenced functions */
static void verify_cleanup(JCR *jcr, int TermCode);
static void prt_fname(JCR *jcr);
static int missing_handler(void *ctx, int num_fields, char **row);

/* 
 * Do a verification of the specified files against the Catlaog
 *    
 *  Returns:  false on failure
 *	      true  on success
 */
bool do_verify(JCR *jcr) 
{
   JOB_DBR jr;
   const char *level, *Name;
   BSOCK   *fd;
   JobId_t verify_jobid = 0;
   int stat;

   memset(&jcr->verify_jr, 0, sizeof(jcr->verify_jr));

   if (!get_or_create_client_record(jcr)) {
      goto bail_out;
   }

   Dmsg1(9, "bdird: created client %s record\n", jcr->client->hdr.name);

   /*
    * Find JobId of last job that ran.	E.g. 
    *	for VERIFY_CATALOG we want the JobId of the last INIT.
    *	for VERIFY_VOLUME_TO_CATALOG, we want the JobId of the 
    *	    last backup Job.
    */
   if (jcr->JobLevel == L_VERIFY_CATALOG || 
       jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG ||
       jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG) {
      memcpy(&jr, &jcr->jr, sizeof(jr));
      if (jcr->verify_job &&
	  (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG ||
	   jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG)) {
	 Name = jcr->verify_job->hdr.name;
      } else {
	 Name = NULL;
      }
      Dmsg1(100, "find last jobid for: %s\n", NPRT(Name));
      if (!db_find_last_jobid(jcr, jcr->db, Name, &jr)) {
	 if (jcr->JobLevel == L_VERIFY_CATALOG) {
	    Jmsg(jcr, M_FATAL, 0, _(
                 "Unable to find JobId of previous InitCatalog Job.\n"
                 "Please run a Verify with Level=InitCatalog before\n"
                 "running the current Job.\n"));
	  } else {
	    Jmsg(jcr, M_FATAL, 0, _(
                 "Unable to find JobId of previous Job for this client.\n"));
	 }   
	 goto bail_out;
      }
      verify_jobid = jr.JobId;
      Dmsg1(100, "Last full jobid=%d\n", verify_jobid);
   } 

   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      goto bail_out;
   }

   if (!jcr->fname) {
      jcr->fname = get_pool_memory(PM_FNAME);
   }

   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Verify JobId=%d Level=%s Job=%s\n"),
      jcr->JobId, level_to_str(jcr->JobLevel), jcr->Job);

   /*
    * Now get the job record for the previous backup that interests
    *	us. We use the verify_jobid that we found above.
    */
   if (jcr->JobLevel == L_VERIFY_CATALOG || 
       jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG ||
       jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG) {
      jcr->verify_jr.JobId = verify_jobid;
      if (!db_get_job_record(jcr, jcr->db, &jcr->verify_jr)) {
         Jmsg(jcr, M_FATAL, 0, _("Could not get job record for previous Job. ERR=%s"), 
	      db_strerror(jcr->db));
	 goto bail_out;
      }
      if (jcr->verify_jr.JobStatus != 'T') {
         Jmsg(jcr, M_FATAL, 0, _("Last Job %d did not terminate normally. JobStatus=%c\n"),
	    verify_jobid, jcr->verify_jr.JobStatus);
	 goto bail_out;
      }
      Jmsg(jcr, M_INFO, 0, _("Verifying against JobId=%d Job=%s\n"),
	 jcr->verify_jr.JobId, jcr->verify_jr.Job); 
   }

   /* 
    * If we are verifying a Volume, we need the Storage
    *	daemon, so open a connection, otherwise, just
    *	create a dummy authorization key (passed to
    *	File daemon but not used).
    */
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG) {
      RBSR *bsr = new_bsr();
      UAContext *ua;
      bsr->JobId = jcr->verify_jr.JobId;
      ua = new_ua_context(jcr);
      complete_bsr(ua, bsr);
      bsr->fi = new_findex();
      bsr->fi->findex = 1;
      bsr->fi->findex2 = jcr->verify_jr.JobFiles;
      jcr->ExpectedFiles = write_bsr_file(ua, bsr);
      if (jcr->ExpectedFiles == 0) {
	 free_ua_context(ua);
	 free_bsr(bsr);
	 goto bail_out;
      }
      free_ua_context(ua);
      free_bsr(bsr);
      if (jcr->RestoreBootstrap) {
	 free(jcr->RestoreBootstrap);
      }
      POOLMEM *fname = get_pool_memory(PM_MESSAGE);
      Mmsg(fname, "%s/restore.bsr", working_directory);
      jcr->RestoreBootstrap = bstrdup(fname);
      free_pool_memory(fname);

      /*
       * Start conversation with Storage daemon  
       */
      set_jcr_job_status(jcr, JS_Blocked);
      if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
	 goto bail_out;
      }
      /*
       * Now start a job with the Storage daemon
       */
      if (!start_storage_daemon_job(jcr)) {
	 goto bail_out;
      }
      /*
       * Now start a Storage daemon message thread
       */
      if (!start_storage_daemon_message_thread(jcr)) {
	 goto bail_out;
      }
      Dmsg0(50, "Storage daemon connection OK\n");
   } else {
      jcr->sd_auth_key = bstrdup("dummy");    /* dummy Storage daemon key */
   }

   if (jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG && jcr->verify_job) {
      jcr->fileset = jcr->verify_job->fileset;
   }
   Dmsg2(100, "ClientId=%u JobLevel=%c\n", jcr->verify_jr.ClientId, jcr->JobLevel);

   /*
    * OK, now connect to the File daemon
    *  and ask him for the files.
    */
   set_jcr_job_status(jcr, JS_Blocked);
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      goto bail_out;
   }

   set_jcr_job_status(jcr, JS_Running);
   fd = jcr->file_bsock;


   Dmsg0(30, ">filed: Send include list\n");
   if (!send_include_list(jcr)) {
      goto bail_out;
   }

   Dmsg0(30, ">filed: Send exclude list\n");
   if (!send_exclude_list(jcr)) {
      goto bail_out;
   }

   /* 
    * Send Level command to File daemon, as well
    *	as the Storage address if appropriate.
    */
   switch (jcr->JobLevel) {
   case L_VERIFY_INIT:
      level = "init";
      break;
   case L_VERIFY_CATALOG:
      level = "catalog";
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      /* 
       * send Storage daemon address to the File daemon
       */
      if (jcr->store->SDDport == 0) {
	 jcr->store->SDDport = jcr->store->SDport;
      }
      bnet_fsend(fd, storaddr, jcr->store->address, jcr->store->SDDport);
      if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
	 goto bail_out;
      }

      /* 
       * Send the bootstrap file -- what Volumes/files to restore
       */
      if (!send_bootstrap_file(jcr)) {
	 goto bail_out;
      }

      /* 
       * The following code is deprecated   
       */
      if (!jcr->RestoreBootstrap) {
	 /*
	  * Pass the VolSessionId, VolSessionTime, Start and
	  * end File and Blocks on the session command.
	  */
	 bnet_fsend(fd, sessioncmd, 
		   jcr->VolumeName,
		   jr.VolSessionId, jr.VolSessionTime, 
		   jr.StartFile, jr.EndFile, jr.StartBlock, 
		   jr.EndBlock);
         if (!response(jcr, fd, OKsession, "Session", DISPLAY_ERROR)) {
	    goto bail_out;
	 }
      }
      level = "volume";
      break;
   case L_VERIFY_DATA:
      level = "data";
      break;
   case L_VERIFY_DISK_TO_CATALOG:
      level="disk_to_catalog";
      break;
   default:
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented save level %d(%c)\n"), jcr->JobLevel,
	 jcr->JobLevel);
      goto bail_out;
   }

   if (!send_run_before_and_after_commands(jcr)) {
      goto bail_out;
   }

   /* 
    * Send verify command/level to File daemon
    */
   bnet_fsend(fd, verifycmd, level);
   if (!response(jcr, fd, OKverify, "Verify", DISPLAY_ERROR)) {
      goto bail_out;
   }

   /*
    * Now get data back from File daemon and
    *  compare it to the catalog or store it in the
    *  catalog depending on the run type.
    */
   /* Compare to catalog */
   switch (jcr->JobLevel) { 
   case L_VERIFY_CATALOG:
      Dmsg0(10, "Verify level=catalog\n");
      jcr->sd_msg_thread_done = true;	/* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, verify_jobid);
      break;

   case L_VERIFY_VOLUME_TO_CATALOG:
      Dmsg0(10, "Verify level=volume\n");
      get_attributes_and_compare_to_catalog(jcr, verify_jobid);
      break;

   case L_VERIFY_DISK_TO_CATALOG:
      Dmsg0(10, "Verify level=disk_to_catalog\n");
      jcr->sd_msg_thread_done = true;	/* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, verify_jobid);
      break;

   case L_VERIFY_INIT:
      /* Build catalog */
      Dmsg0(10, "Verify level=init\n");
      jcr->sd_msg_thread_done = true;	/* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_put_in_catalog(jcr);
      break;

   default:
      Jmsg1(jcr, M_FATAL, 0, _("Unimplemented verify level %d\n"), jcr->JobLevel);
      goto bail_out;
   }

   stat = wait_for_job_termination(jcr);
   verify_cleanup(jcr, stat);
   return true;

bail_out:
   verify_cleanup(jcr, JS_ErrorTerminated);
   return false;
}

/*
 * Release resources allocated during backup.
 *
 */
static void verify_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50];
   char ec1[30], ec2[30];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   JobId_t JobId;
   const char *Name;

// Dmsg1(100, "Enter verify_cleanup() TermCod=%d\n", TermCode);
   dequeue_messages(jcr);	      /* display any queued messages */

   Dmsg3(900, "JobLevel=%c Expected=%u JobFiles=%u\n", jcr->JobLevel,
      jcr->ExpectedFiles, jcr->JobFiles);
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG &&
       jcr->ExpectedFiles != jcr->JobFiles) {
      TermCode = JS_ErrorTerminated;
   }

   JobId = jcr->jr.JobId;
   set_jcr_job_status(jcr, TermCode);

   update_job_end_record(jcr);

   msg_type = M_INFO;		      /* by default INFO message */
   switch (TermCode) {
   case JS_Terminated:
      term_msg = _("Verify OK");
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** Verify Error ***"); 
      msg_type = M_ERROR;	   /* Generate error message */
      break;
   case JS_Error:
      term_msg = _("Verify warnings");
      break;
   case JS_Canceled:
      term_msg = _("Verify Canceled");
      break;
   case JS_Differences:
      term_msg = _("Verify Differences");
      break;
   default:
      term_msg = term_code;
      bsnprintf(term_code, sizeof(term_code), 
                _("Inappropriate term code: %d %c\n"), TermCode, TermCode);
      break;
   }
   bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);
   if (jcr->verify_job) {
      Name = jcr->verify_job->hdr.name;
   } else {
      Name = "";
   }

   jobstatus_to_ascii(jcr->FDJobStatus, fd_term_msg, sizeof(fd_term_msg));
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG) {
      jobstatus_to_ascii(jcr->SDJobStatus, sd_term_msg, sizeof(sd_term_msg));
      Jmsg(jcr, msg_type, 0, _("Bacula " VERSION " (" LSMDATE "): %s\n\
  JobId:                  %d\n\
  Job:                    %s\n\
  FileSet:                %s\n\
  Verify Level:           %s\n\
  Client:                 %s\n\
  Verify JobId:           %d\n\
  Verify Job:             %s\n\
  Start time:             %s\n\
  End time:               %s\n\
  Files Expected:         %s\n\
  Files Examined:         %s\n\
  Non-fatal FD errors:    %d\n\
  FD termination status:  %s\n\
  SD termination status:  %s\n\
  Termination:            %s\n\n"),
	 edt,
	 jcr->jr.JobId,
	 jcr->jr.Job,
	 jcr->fileset->hdr.name,
	 level_to_str(jcr->JobLevel),
	 jcr->client->hdr.name,
	 jcr->verify_jr.JobId,
	 Name,
	 sdt,
	 edt,
	 edit_uint64_with_commas(jcr->ExpectedFiles, ec1),
	 edit_uint64_with_commas(jcr->JobFiles, ec2),
	 jcr->Errors,
	 fd_term_msg,
	 sd_term_msg,
	 term_msg);
   } else {
      Jmsg(jcr, msg_type, 0, _("Bacula " VERSION " (" LSMDATE "): %s\n\
  JobId:                  %d\n\
  Job:                    %s\n\
  FileSet:                %s\n\
  Verify Level:           %s\n\
  Client:                 %s\n\
  Verify JobId:           %d\n\
  Verify Job:             %s\n\
  Start time:             %s\n\
  End time:               %s\n\
  Files Examined:         %s\n\
  Non-fatal FD errors:    %d\n\
  FD termination status:  %s\n\
  Termination:            %s\n\n"),
	 edt,
	 jcr->jr.JobId,
	 jcr->jr.Job,
	 jcr->fileset->hdr.name,
	 level_to_str(jcr->JobLevel),
	 jcr->client->hdr.name,
	 jcr->verify_jr.JobId,
	 Name,
	 sdt,
	 edt,
	 edit_uint64_with_commas(jcr->JobFiles, ec1),
	 jcr->Errors,
	 fd_term_msg,
	 term_msg);
   }
   Dmsg0(100, "Leave verify_cleanup()\n");
   if (jcr->fname) {
      free_memory(jcr->fname);
      jcr->fname = NULL;
   }
}

/*
 * This routine is called only during a Verify
 */
int get_attributes_and_compare_to_catalog(JCR *jcr, JobId_t JobId)
{
   BSOCK   *fd;
   int n, len;
   FILE_DBR fdbr;
   struct stat statf;		      /* file stat */
   struct stat statc;		      /* catalog stat */
   int stat = JS_Terminated;
   char buf[MAXSTRING];
   POOLMEM *fname = get_pool_memory(PM_MESSAGE);
   int do_SIG = NO_SIG;
   long file_index = 0;

   memset(&fdbr, 0, sizeof(FILE_DBR));
   fd = jcr->file_bsock;
   fdbr.JobId = JobId;
   jcr->FileIndex = 0;
   
   Dmsg0(20, "bdird: waiting to receive file attributes\n");
   /*
    * Get Attributes and Signature from File daemon
    * We expect:
    *	FileIndex
    *	Stream
    *	Options or SIG (MD5/SHA1)
    *	Filename
    *	Attributes
    *	Link name  ???
    */
   while ((n=bget_dirmsg(fd)) >= 0 && !job_canceled(jcr)) {
      int stream;
      char *attr, *p, *fn;
      char Opts_SIG[MAXSTRING];        /* Verify Opts or MD5/SHA1 signature */

      fname = check_pool_memory_size(fname, fd->msglen);
      jcr->fname = check_pool_memory_size(jcr->fname, fd->msglen);
      Dmsg1(200, "Atts+SIG=%s\n", fd->msg);
      if ((len = sscanf(fd->msg, "%ld %d %100s", &file_index, &stream, 
	    fname)) != 3) {
         Jmsg3(jcr, M_FATAL, 0, _("bird<filed: bad attributes, expected 3 fields got %d\n\
 mslen=%d msg=%s\n"), len, fd->msglen, fd->msg);
	 goto bail_out;
      }
      /*
       * We read the Options or Signature into fname				    
       *  to prevent overrun, now copy it to proper location.
       */
      bstrncpy(Opts_SIG, fname, sizeof(Opts_SIG));
      p = fd->msg;
      skip_nonspaces(&p);	      /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);	      /* skip Stream */
      skip_spaces(&p);
      skip_nonspaces(&p);	      /* skip Opts_SIG */   
      p++;			      /* skip space */
      fn = fname;
      while (*p != 0) {
	 *fn++ = *p++;		      /* copy filename */
      }
      *fn = *p++;		      /* term filename and point to attribs */
      attr = p;
      /*
       * Got attributes stream, decode it
       */
      if (stream == STREAM_UNIX_ATTRIBUTES || stream == STREAM_UNIX_ATTRIBUTES_EX) {
	 int32_t LinkFIf, LinkFIc;
         Dmsg2(400, "file_index=%d attr=%s\n", file_index, attr);
	 jcr->JobFiles++;
	 jcr->FileIndex = file_index;	 /* remember attribute file_index */
	 decode_stat(attr, &statf, &LinkFIf);  /* decode file stat packet */
	 do_SIG = NO_SIG;
	 jcr->fn_printed = false;
	 pm_strcpy(jcr->fname, fname);	/* move filename into JCR */

         Dmsg2(040, "dird<filed: stream=%d %s\n", stream, jcr->fname);
         Dmsg1(020, "dird<filed: attr=%s\n", attr);

	 /* 
	  * Find equivalent record in the database 
	  */
	 fdbr.FileId = 0;
	 if (!db_get_file_attributes_record(jcr, jcr->db, jcr->fname, 
	      &jcr->verify_jr, &fdbr)) {
            Jmsg(jcr, M_INFO, 0, _("New file: %s\n"), jcr->fname);
            Dmsg1(020, _("File not in catalog: %s\n"), jcr->fname);
	    stat = JS_Differences;
	    continue;
	 } else {
	    /* 
	     * mark file record as visited by stuffing the
	     * current JobId, which is unique, into the MarkId field.
	     */
	    db_mark_file_record(jcr, jcr->db, fdbr.FileId, jcr->JobId);
	 }

         Dmsg3(400, "Found %s in catalog. inx=%d Opts=%s\n", jcr->fname, 
	    file_index, Opts_SIG);
	 decode_stat(fdbr.LStat, &statc, &LinkFIc); /* decode catalog stat */
	 /*
	  * Loop over options supplied by user and verify the
	  * fields he requests.
	  */
	 for (p=Opts_SIG; *p; p++) {
	    char ed1[30], ed2[30];
	    switch (*p) {
            case 'i':                /* compare INODEs */
	       if (statc.st_ino != statf.st_ino) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_ino   differ. Cat: %s File: %s\n"), 
		     edit_uint64((uint64_t)statc.st_ino, ed1),
		     edit_uint64((uint64_t)statf.st_ino, ed2));
		  stat = JS_Differences;
	       }
	       break;
            case 'p':                /* permissions bits */
	       if (statc.st_mode != statf.st_mode) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_mode  differ. Cat: %x File: %x\n"), 
		     (uint32_t)statc.st_mode, (uint32_t)statf.st_mode);
		  stat = JS_Differences;
	       }
	       break;
            case 'n':                /* number of links */
	       if (statc.st_nlink != statf.st_nlink) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_nlink differ. Cat: %d File: %d\n"), 
		     (uint32_t)statc.st_nlink, (uint32_t)statf.st_nlink);
		  stat = JS_Differences;
	       }
	       break;
            case 'u':                /* user id */
	       if (statc.st_uid != statf.st_uid) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_uid   differ. Cat: %u File: %u\n"), 
		     (uint32_t)statc.st_uid, (uint32_t)statf.st_uid);
		  stat = JS_Differences;
	       }
	       break;
            case 'g':                /* group id */
	       if (statc.st_gid != statf.st_gid) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_gid   differ. Cat: %u File: %u\n"), 
		     (uint32_t)statc.st_gid, (uint32_t)statf.st_gid);
		  stat = JS_Differences;
	       }
	       break;
            case 's':                /* size */
	       if (statc.st_size != statf.st_size) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_size  differ. Cat: %s File: %s\n"), 
		     edit_uint64((uint64_t)statc.st_size, ed1),
		     edit_uint64((uint64_t)statf.st_size, ed2));
		  stat = JS_Differences;
	       }
	       break;
            case 'a':                /* access time */
	       if (statc.st_atime != statf.st_atime) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_atime differs\n"));
		  stat = JS_Differences;
	       }
	       break;
            case 'm':
	       if (statc.st_mtime != statf.st_mtime) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_mtime differs\n"));
		  stat = JS_Differences;
	       }
	       break;
            case 'c':                /* ctime */
	       if (statc.st_ctime != statf.st_ctime) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_ctime differs\n"));
		  stat = JS_Differences;
	       }
	       break;
            case 'd':                /* file size decrease */
	       if (statc.st_size > statf.st_size) {
		  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_size  decrease. Cat: %s File: %s\n"), 
		     edit_uint64((uint64_t)statc.st_size, ed1),
		     edit_uint64((uint64_t)statf.st_size, ed2));
		  stat = JS_Differences;
	       }
	       break;
            case '5':                /* compare MD5 */
               Dmsg1(500, "set Do_MD5 for %s\n", jcr->fname);
	       do_SIG = MD5_SIG;
	       break;
            case '1':                 /* compare SHA1 */
	       do_SIG = SHA1_SIG;
	       break;
            case ':':
            case 'V':
	    default:
	       break;
	    }
	 }
      /*
       * Got SIG Signature from Storage daemon
       *  It came across in the Opts_SIG field.
       */
      } else if (stream == STREAM_MD5_SIGNATURE || stream == STREAM_SHA1_SIGNATURE) {
         Dmsg2(400, "stream=SIG inx=%d SIG=%s\n", file_index, Opts_SIG);
	 /* 
	  * When ever we get a signature is MUST have been
	  * preceded by an attributes record, which sets attr_file_index
	  */
	 if (jcr->FileIndex != (uint32_t)file_index) {
            Jmsg2(jcr, M_FATAL, 0, _("MD5/SHA1 index %d not same as attributes %d\n"),
	       file_index, jcr->FileIndex);
	    goto bail_out;
	 } 
	 if (do_SIG) {
	    db_escape_string(buf, Opts_SIG, strlen(Opts_SIG));
	    if (strcmp(buf, fdbr.SIG) != 0) {
	       prt_fname(jcr);
	       if (debug_level >= 10) {
                  Jmsg(jcr, M_INFO, 0, _("      %s not same. File=%s Cat=%s\n"), 
                       stream==STREAM_MD5_SIGNATURE?"MD5":"SHA1", buf, fdbr.SIG);
	       } else {
                  Jmsg(jcr, M_INFO, 0, _("      %s differs.\n"), 
                       stream==STREAM_MD5_SIGNATURE?"MD5":"SHA1");
	       }
	       stat = JS_Differences;
	    }
	    do_SIG = FALSE;
	 }
      }
      jcr->JobFiles = file_index;
   } 
   if (is_bnet_error(fd)) {
      Jmsg2(jcr, M_FATAL, 0, _("bdird<filed: bad attributes from filed n=%d : %s\n"),
			n, strerror(errno));
      goto bail_out;
   }

   /* Now find all the files that are missing -- i.e. all files in
    *  the database where the MarkedId != current JobId
    */
   jcr->fn_printed = false;
   sprintf(buf, 
"SELECT Path.Path,Filename.Name FROM File,Path,Filename "
"WHERE File.JobId=%d "
"AND File.MarkedId!=%d AND File.PathId=Path.PathId "
"AND File.FilenameId=Filename.FilenameId", 
      JobId, jcr->JobId);
   /* missing_handler is called for each file found */
   db_sql_query(jcr->db, buf, missing_handler, (void *)jcr);
   if (jcr->fn_printed) {
      stat = JS_Differences;
   }
   free_pool_memory(fname);
   set_jcr_job_status(jcr, stat);
   return 1;
    
bail_out:
   free_pool_memory(fname);
   set_jcr_job_status(jcr, JS_ErrorTerminated);
   return 0;
}

/*
 * We are called here for each record that matches the above
 *  SQL query -- that is for each file contained in the Catalog
 *  that was not marked earlier. This means that the file in
 *  question is a missing file (in the Catalog but on on Disk).
 */
static int missing_handler(void *ctx, int num_fields, char **row)
{
   JCR *jcr = (JCR *)ctx;

   if (!jcr->fn_printed) {
      Jmsg(jcr, M_INFO, 0, "\n");
      Jmsg(jcr, M_INFO, 0, _("The following files are missing:\n"));
      jcr->fn_printed = true;
   }
   Jmsg(jcr, M_INFO, 0, "      %s%s\n", row[0]?row[0]:"", row[1]?row[1]:"");
   return 0;
}


/* 
 * Print filename for verify
 */
static void prt_fname(JCR *jcr)
{
   if (!jcr->fn_printed) {
      Jmsg(jcr, M_INFO, 0, _("File: %s\n"), jcr->fname);
      jcr->fn_printed = TRUE;
   }
}
