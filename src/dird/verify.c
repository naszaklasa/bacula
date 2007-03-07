/*
 *
 *   Bacula Director -- verify.c -- responsible for running file verification
 *
 *     Kern Sibbald, October MM
 *
 *  Basic tasks done here:
 *     Open DB
 *     Open connection with File daemon and pass him commands
 *       to do the verify.
 *     When the File daemon sends the attributes, compare them to
 *       what is in the DB.
 *
 *   Version $Id: verify.c 4183 2007-02-15 18:57:55Z kerns $
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


#include "bacula.h"
#include "dird.h"
#include "findlib/find.h"

/* Commands sent to File daemon */
static char verifycmd[]    = "verify level=%s\n";
static char storaddr[]     = "storage address=%s port=%d ssl=0\n";

/* Responses received from File daemon */
static char OKverify[]    = "2000 OK verify\n";
static char OKstore[]     = "2000 OK storage\n";
static char OKbootstrap[] = "2000 OK bootstrap\n";

/* Forward referenced functions */
static void prt_fname(JCR *jcr);
static int missing_handler(void *ctx, int num_fields, char **row);


/* 
 * Called here before the job is run to do the job
 *   specific setup.
 */
bool do_verify_init(JCR *jcr) 
{
   JOB_DBR jr;
   JobId_t verify_jobid = 0;
   const char *Name;

   free_wstorage(jcr);                   /* we don't write */

   memset(&jcr->previous_jr, 0, sizeof(jcr->previous_jr));

   Dmsg1(9, "bdird: created client %s record\n", jcr->client->hdr.name);

   /*
    * Find JobId of last job that ran.  E.g.
    *   for VERIFY_CATALOG we want the JobId of the last INIT.
    *   for VERIFY_VOLUME_TO_CATALOG, we want the JobId of the
    *       last backup Job.
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
         return false;
      }
      verify_jobid = jr.JobId;
      Dmsg1(100, "Last full jobid=%d\n", verify_jobid);
   }
   /*
    * Now get the job record for the previous backup that interests
    *   us. We use the verify_jobid that we found above.
    */
   if (jcr->JobLevel == L_VERIFY_CATALOG ||
       jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG ||
       jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG) {
      jcr->previous_jr.JobId = verify_jobid;
      if (!db_get_job_record(jcr, jcr->db, &jcr->previous_jr)) {
         Jmsg(jcr, M_FATAL, 0, _("Could not get job record for previous Job. ERR=%s"),
              db_strerror(jcr->db));
         return false;
      }
      if (jcr->previous_jr.JobStatus != 'T') {
         Jmsg(jcr, M_FATAL, 0, _("Last Job %d did not terminate normally. JobStatus=%c\n"),
            verify_jobid, jcr->previous_jr.JobStatus);
         return false;
      }
      Jmsg(jcr, M_INFO, 0, _("Verifying against JobId=%d Job=%s\n"),
         jcr->previous_jr.JobId, jcr->previous_jr.Job);
   }

   /*
    * If we are verifying a Volume, we need the Storage
    *   daemon, so open a connection, otherwise, just
    *   create a dummy authorization key (passed to
    *   File daemon but not used).
    */
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG) {
      if (!create_restore_bootstrap_file(jcr)) {
         return false;
      }
   } else {
      jcr->sd_auth_key = bstrdup("dummy");    /* dummy Storage daemon key */
   }

   if (jcr->JobLevel == L_VERIFY_DISK_TO_CATALOG && jcr->verify_job) {
      jcr->fileset = jcr->verify_job->fileset;
   }
   Dmsg2(100, "ClientId=%u JobLevel=%c\n", jcr->previous_jr.ClientId, jcr->JobLevel);
   return true;
}


/*
 * Do a verification of the specified files against the Catlaog
 *
 *  Returns:  false on failure
 *            true  on success
 */
bool do_verify(JCR *jcr)
{
   const char *level;
   BSOCK   *fd;
   int stat;
   char ed1[100];

   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      return false;
   }

   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Verify JobId=%s Level=%s Job=%s\n"),
      edit_uint64(jcr->JobId, ed1), level_to_str(jcr->JobLevel), jcr->Job);

   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG) {
      /*
       * Start conversation with Storage daemon
       */
      set_jcr_job_status(jcr, JS_Blocked);
      if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
         return false;
      }
      /*
       * Now start a job with the Storage daemon
       */
      if (!start_storage_daemon_job(jcr, jcr->rstorage, NULL)) {
         return false;
      }
      if (!bnet_fsend(jcr->store_bsock, "run")) {
         return false;
      }
      /*
       * Now start a Storage daemon message thread
       */
      if (!start_storage_daemon_message_thread(jcr)) {
         return false;
      }
      Dmsg0(50, "Storage daemon connection OK\n");

   }
   /*
    * OK, now connect to the File daemon
    *  and ask him for the files.
    */
   set_jcr_job_status(jcr, JS_Blocked);
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      return false;
   }

   set_jcr_job_status(jcr, JS_Running);
   fd = jcr->file_bsock;


   Dmsg0(30, ">filed: Send include list\n");
   if (!send_include_list(jcr)) {
      return false;
   }

   Dmsg0(30, ">filed: Send exclude list\n");
   if (!send_exclude_list(jcr)) {
      return false;
   }

   /*
    * Send Level command to File daemon, as well
    *   as the Storage address if appropriate.
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
      if (jcr->rstore->SDDport == 0) {
         jcr->rstore->SDDport = jcr->rstore->SDport;
      }
      bnet_fsend(fd, storaddr, jcr->rstore->address, jcr->rstore->SDDport);
      if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
         return false;
      }

      /*
       * Send the bootstrap file -- what Volumes/files to restore
       */
      if (!send_bootstrap_file(jcr, fd) ||
          !response(jcr, fd, OKbootstrap, "Bootstrap", DISPLAY_ERROR)) {
         return false;
      }

      if (!jcr->RestoreBootstrap) {
         Jmsg0(jcr, M_FATAL, 0, _("Deprecated feature ... use bootstrap.\n"));
         return false;
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
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented Verify level %d(%c)\n"), jcr->JobLevel,
         jcr->JobLevel);
      return false;
   }

   if (!send_runscripts_commands(jcr)) {
      return false;
   }

   /*
    * Send verify command/level to File daemon
    */
   bnet_fsend(fd, verifycmd, level);
   if (!response(jcr, fd, OKverify, "Verify", DISPLAY_ERROR)) {
      return false;
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
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_VOLUME_TO_CATALOG:
      Dmsg0(10, "Verify level=volume\n");
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_DISK_TO_CATALOG:
      Dmsg0(10, "Verify level=disk_to_catalog\n");
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_INIT:
      /* Build catalog */
      Dmsg0(10, "Verify level=init\n");
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_put_in_catalog(jcr);
      break;

   default:
      Jmsg1(jcr, M_FATAL, 0, _("Unimplemented verify level %d\n"), jcr->JobLevel);
      return false;
   }

   stat = wait_for_job_termination(jcr);
   if (stat == JS_Terminated) {
      verify_cleanup(jcr, stat);
      return true;
   }
   return false;
}


/*
 * Release resources allocated during backup.
 *
 */
void verify_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50];
   char ec1[30], ec2[30];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   JobId_t JobId;
   const char *Name;

// Dmsg1(100, "Enter verify_cleanup() TermCod=%d\n", TermCode);

   Dmsg3(900, "JobLevel=%c Expected=%u JobFiles=%u\n", jcr->JobLevel,
      jcr->ExpectedFiles, jcr->JobFiles);
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG &&
       jcr->ExpectedFiles != jcr->JobFiles) {
      TermCode = JS_ErrorTerminated;
   }

   /* If no files were expected, there can be no error */
   if (jcr->JobLevel == L_VERIFY_VOLUME_TO_CATALOG &&
       jcr->ExpectedFiles == 0) {
      TermCode = JS_Terminated;
   }

   JobId = jcr->jr.JobId;

   update_job_end(jcr, TermCode);

   if (jcr->unlink_bsr && jcr->RestoreBootstrap) {
      unlink(jcr->RestoreBootstrap);
      jcr->unlink_bsr = false;
   }

   msg_type = M_INFO;                 /* by default INFO message */
   switch (TermCode) {
   case JS_Terminated:
      term_msg = _("Verify OK");
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** Verify Error ***");
      msg_type = M_ERROR;          /* Generate error message */
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
      Jmsg(jcr, msg_type, 0, _("Bacula %s (%s): %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  FileSet:                %s\n"
"  Verify Level:           %s\n"
"  Client:                 %s\n"
"  Verify JobId:           %d\n"
"  Verify Job:             %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Files Expected:         %s\n"
"  Files Examined:         %s\n"
"  Non-fatal FD errors:    %d\n"
"  FD termination status:  %s\n"
"  SD termination status:  %s\n"
"  Termination:            %s\n\n"),
         VERSION,
         LSMDATE,
         edt,
         jcr->jr.JobId,
         jcr->jr.Job,
         jcr->fileset->hdr.name,
         level_to_str(jcr->JobLevel),
         jcr->client->hdr.name,
         jcr->previous_jr.JobId,
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
      Jmsg(jcr, msg_type, 0, _("Bacula %s (%s): %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  FileSet:                %s\n"
"  Verify Level:           %s\n"
"  Client:                 %s\n"
"  Verify JobId:           %d\n"
"  Verify Job:             %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Files Examined:         %s\n"
"  Non-fatal FD errors:    %d\n"
"  FD termination status:  %s\n"
"  Termination:            %s\n\n"),
         VERSION,
         LSMDATE,
         edt,
         jcr->jr.JobId,
         jcr->jr.Job,
         jcr->fileset->hdr.name,
         level_to_str(jcr->JobLevel),
         jcr->client->hdr.name,
         jcr->previous_jr.JobId,
         Name,
         sdt,
         edt,
         edit_uint64_with_commas(jcr->JobFiles, ec1),
         jcr->Errors,
         fd_term_msg,
         term_msg);
   }
   Dmsg0(100, "Leave verify_cleanup()\n");
}

/*
 * This routine is called only during a Verify
 */
int get_attributes_and_compare_to_catalog(JCR *jcr, JobId_t JobId)
{
   BSOCK   *fd;
   int n, len;
   FILE_DBR fdbr;
   struct stat statf;                 /* file stat */
   struct stat statc;                 /* catalog stat */
   int stat = JS_Terminated;
   char buf[MAXSTRING];
   POOLMEM *fname = get_pool_memory(PM_MESSAGE);
   int do_Digest = CRYPTO_DIGEST_NONE;
   int32_t file_index = 0;

   memset(&fdbr, 0, sizeof(FILE_DBR));
   fd = jcr->file_bsock;
   fdbr.JobId = JobId;
   jcr->FileIndex = 0;

   Dmsg0(20, "bdird: waiting to receive file attributes\n");
   /*
    * Get Attributes and Signature from File daemon
    * We expect:
    *   FileIndex
    *   Stream
    *   Options or Digest (MD5/SHA1)
    *   Filename
    *   Attributes
    *   Link name  ???
    */
   while ((n=bget_dirmsg(fd)) >= 0 && !job_canceled(jcr)) {
      int stream;
      char *attr, *p, *fn;
      char Opts_Digest[MAXSTRING];        /* Verify Opts or MD5/SHA1 digest */

      if (job_canceled(jcr)) {
         return false;
      }
      fname = check_pool_memory_size(fname, fd->msglen);
      jcr->fname = check_pool_memory_size(jcr->fname, fd->msglen);
      Dmsg1(200, "Atts+Digest=%s\n", fd->msg);
      if ((len = sscanf(fd->msg, "%ld %d %100s", &file_index, &stream,
            fname)) != 3) {
         Jmsg3(jcr, M_FATAL, 0, _("bird<filed: bad attributes, expected 3 fields got %d\n"
" mslen=%d msg=%s\n"), len, fd->msglen, fd->msg);
         return false;
      }
      /*
       * We read the Options or Signature into fname
       *  to prevent overrun, now copy it to proper location.
       */
      bstrncpy(Opts_Digest, fname, sizeof(Opts_Digest));
      p = fd->msg;
      skip_nonspaces(&p);             /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Stream */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Opts_Digest */
      p++;                            /* skip space */
      fn = fname;
      while (*p != 0) {
         *fn++ = *p++;                /* copy filename */
      }
      *fn = *p++;                     /* term filename and point to attribs */
      attr = p;
      /*
       * Got attributes stream, decode it
       */
      if (stream == STREAM_UNIX_ATTRIBUTES || stream == STREAM_UNIX_ATTRIBUTES_EX) {
         int32_t LinkFIf, LinkFIc;
         Dmsg2(400, "file_index=%d attr=%s\n", file_index, attr);
         jcr->JobFiles++;
         jcr->FileIndex = file_index;    /* remember attribute file_index */
         decode_stat(attr, &statf, &LinkFIf);  /* decode file stat packet */
         do_Digest = CRYPTO_DIGEST_NONE;
         jcr->fn_printed = false;
         pm_strcpy(jcr->fname, fname);  /* move filename into JCR */

         Dmsg2(040, "dird<filed: stream=%d %s\n", stream, jcr->fname);
         Dmsg1(020, "dird<filed: attr=%s\n", attr);

         /*
          * Find equivalent record in the database
          */
         fdbr.FileId = 0;
         if (!db_get_file_attributes_record(jcr, jcr->db, jcr->fname,
              &jcr->previous_jr, &fdbr)) {
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
            file_index, Opts_Digest);
         decode_stat(fdbr.LStat, &statc, &LinkFIc); /* decode catalog stat */
         /*
          * Loop over options supplied by user and verify the
          * fields he requests.
          */
         for (p=Opts_Digest; *p; p++) {
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
               do_Digest = CRYPTO_DIGEST_MD5;
               break;
            case '1':                 /* compare SHA1 */
               do_Digest = CRYPTO_DIGEST_SHA1;
               break;
            case ':':
            case 'V':
            default:
               break;
            }
         }
      /*
       * Got Digest Signature from Storage daemon
       *  It came across in the Opts_Digest field.
       */
      } else if (crypto_digest_stream_type(stream) != CRYPTO_DIGEST_NONE) {
         Dmsg2(400, "stream=Digest inx=%d Digest=%s\n", file_index, Opts_Digest);
         /*
          * When ever we get a digest is MUST have been
          * preceded by an attributes record, which sets attr_file_index
          */
         if (jcr->FileIndex != (uint32_t)file_index) {
            Jmsg2(jcr, M_FATAL, 0, _("MD5/SHA1 index %d not same as attributes %d\n"),
               file_index, jcr->FileIndex);
            return false;
         }
         if (do_Digest != CRYPTO_DIGEST_NONE) {
            db_escape_string(buf, Opts_Digest, strlen(Opts_Digest));
            if (strcmp(buf, fdbr.Digest) != 0) {
               prt_fname(jcr);
               if (debug_level >= 10) {
                  Jmsg(jcr, M_INFO, 0, _("      %s not same. File=%s Cat=%s\n"),
                       stream_to_ascii(stream), buf, fdbr.Digest);
               } else {
                  Jmsg(jcr, M_INFO, 0, _("      %s differs.\n"),
                       stream_to_ascii(stream));
               }
               stat = JS_Differences;
            }
            do_Digest = CRYPTO_DIGEST_NONE;
         }
      }
      jcr->JobFiles = file_index;
   }
   if (is_bnet_error(fd)) {
      berrno be;
      Jmsg2(jcr, M_FATAL, 0, _("bdird<filed: bad attributes from filed n=%d : %s\n"),
                        n, be.strerror());
      return false;
   }

   /* Now find all the files that are missing -- i.e. all files in
    *  the database where the MarkId != current JobId
    */
   jcr->fn_printed = false;
   bsnprintf(buf, sizeof(buf),
      "SELECT Path.Path,Filename.Name FROM File,Path,Filename "
      "WHERE File.JobId=%d "
      "AND File.MarkId!=%d AND File.PathId=Path.PathId "
      "AND File.FilenameId=Filename.FilenameId",
         JobId, jcr->JobId);
   /* missing_handler is called for each file found */
   db_sql_query(jcr->db, buf, missing_handler, (void *)jcr);
   if (jcr->fn_printed) {
      stat = JS_Differences;
   }
   free_pool_memory(fname);
   set_jcr_job_status(jcr, stat);
   return stat == JS_Terminated;
}

/*
 * We are called here for each record that matches the above
 *  SQL query -- that is for each file contained in the Catalog
 *  that was not marked earlier. This means that the file in
 *  question is a missing file (in the Catalog but not on Disk).
 */
static int missing_handler(void *ctx, int num_fields, char **row)
{
   JCR *jcr = (JCR *)ctx;

   if (job_canceled(jcr)) {
      return 1;
   }
   if (!jcr->fn_printed) {
      Jmsg(jcr, M_INFO, 0, "\n");
      Jmsg(jcr, M_INFO, 0, _("The following files are in the Catalog but not on disk:\n"));
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
