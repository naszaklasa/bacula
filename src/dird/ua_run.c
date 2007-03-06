/*
 *
 *   Bacula Director -- Run Command
 *
 *     Kern Sibbald, December MMI
 *
 *   Version $Id: ua_run.c,v 1.58.2.1 2005/02/14 10:02:22 kerns Exp $
 */

/*
   Copyright (C) 2001-2004 Kern Sibbald

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

/* Imported subroutines */

/* Imported variables */
extern struct s_kw ReplaceOptions[];

/*
 * For Backup and Verify Jobs
 *     run [job=]<job-name> level=<level-name>
 *
 * For Restore Jobs
 *     run <job-name> jobid=nn
 *
 *  Returns: 0 on error
 *	     JobId if OK
 *
 *  Returns: 0 on error
 *	     JobId if OK
 *
 */
int run_cmd(UAContext *ua, const char *cmd)
{
   JCR *jcr;
   char *job_name, *level_name, *jid, *store_name, *pool_name;
   char *where, *fileset_name, *client_name, *bootstrap;
   const char *replace;
   char *when, *verify_job_name, *catalog_name;
   int Priority = 0;
   int i, j, opt, files = 0;
   bool kw_ok;
   JOB *job = NULL;
   JOB *verify_job = NULL;
   STORE *store = NULL;
   CLIENT *client = NULL;
   FILESET *fileset = NULL;
   POOL *pool = NULL;
   static const char *kw[] = {	      /* command line arguments */
      N_("job"),                      /*  Used in a switch() */
      N_("jobid"),                    /* 1 */
      N_("client"),                   /* 2 */
      N_("fd"), 
      N_("fileset"),                  /* 4 */
      N_("level"),                    /* 5 */
      N_("storage"),                  /* 6 */
      N_("sd"),                       /* 7 */
      N_("pool"),                     /* 8 */
      N_("where"),                    /* 9 */
      N_("bootstrap"),                /* 10 */
      N_("replace"),                  /* 11 */
      N_("when"),                     /* 12 */
      N_("priority"),                 /* 13 */
      N_("yes"),          /* 14 -- if you change this change YES_POS too */
      N_("verifyjob"),                /* 15 */
      N_("files"),                    /* 16 number of files to restore */
       N_("catalog"),                 /* 17 override catalog */
      NULL};

#define YES_POS 14

   if (!open_db(ua)) {
      return 1;
   }

   job_name = NULL;
   level_name = NULL;
   jid = NULL;
   store_name = NULL;
   pool_name = NULL;
   where = NULL;
   when = NULL;
   client_name = NULL;
   fileset_name = NULL;
   bootstrap = NULL;
   replace = NULL;
   verify_job_name = NULL;
   catalog_name = NULL;

   for (i=1; i<ua->argc; i++) {
      Dmsg2(800, "Doing arg %d = %s\n", i, ua->argk[i]);
      kw_ok = false;
      /* Keep looking until we find a good keyword */
      for (j=0; !kw_ok && kw[j]; j++) {
	 if (strcasecmp(ua->argk[i], _(kw[j])) == 0) {
	    /* Note, yes and run have no value, so do not err */
	    if (!ua->argv[i] && j != YES_POS /*yes*/) {  
               bsendmsg(ua, _("Value missing for keyword %s\n"), ua->argk[i]);
	       return 1;
	    }
            Dmsg1(800, "Got keyword=%s\n", kw[j]);
	    switch (j) {
	    case 0: /* job */
	       if (job_name) {
                  bsendmsg(ua, _("Job name specified twice.\n"));
		  return 0;
	       }
	       job_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 1: /* JobId */
	       if (jid) {
                  bsendmsg(ua, _("JobId specified twice.\n"));
		  return 0;
	       }
	       jid = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 2: /* client */
	    case 3: /* fd */
	       if (client_name) {
                  bsendmsg(ua, _("Client specified twice.\n"));
		  return 0;
	       }
	       client_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 4: /* fileset */
	       if (fileset_name) {
                  bsendmsg(ua, _("FileSet specified twice.\n"));
		  return 0;
	       }
	       fileset_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 5: /* level */
	       if (level_name) {
                  bsendmsg(ua, _("Level specified twice.\n"));
		  return 0;
	       }
	       level_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 6: /* storage */
	    case 7: /* sd */
	       if (store_name) {
                  bsendmsg(ua, _("Storage specified twice.\n"));
		  return 0;
	       }
	       store_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 8: /* pool */
	       if (pool_name) {
                  bsendmsg(ua, _("Pool specified twice.\n"));
		  return 0;
	       }
	       pool_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 9: /* where */
	       if (where) {
                  bsendmsg(ua, _("Where specified twice.\n"));
		  return 0;
	       }
	       where = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 10: /* bootstrap */
	       if (bootstrap) {
                  bsendmsg(ua, _("Bootstrap specified twice.\n"));
		  return 0;
	       }
	       bootstrap = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 11: /* replace */
	       if (replace) {
                  bsendmsg(ua, _("Replace specified twice.\n"));
		  return 0;
	       }
	       replace = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 12: /* When */
	       if (when) {
                  bsendmsg(ua, _("When specified twice.\n"));
		  return 0;
	       }
	       when = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 13:  /* Priority */
	       if (Priority) {
                  bsendmsg(ua, _("Priority specified twice.\n"));
		  return 0;
	       }
	       Priority = atoi(ua->argv[i]);
	       if (Priority <= 0) {
                  bsendmsg(ua, _("Priority must be positive nonzero setting it to 10.\n"));
		  Priority = 10;
	       }
	       kw_ok = true;
	       break;
	    case 14: /* yes */
	       kw_ok = true;
	       break;
	    case 15: /* Verify Job */
	       if (verify_job_name) {
                  bsendmsg(ua, _("Verify Job specified twice.\n"));
		  return 0;
	       }
	       verify_job_name = ua->argv[i];
	       kw_ok = true;
	       break;
	    case 16: /* files */
	       files = atoi(ua->argv[i]);
	       kw_ok = true;
	       break;

	    case 17: /* catalog */
	       catalog_name = ua->argv[i];
	       kw_ok = true;
	       break;

	    default:
	       break;
	    }
	 } /* end strcase compare */
      } /* end keyword loop */
      /*
       * End of keyword for loop -- if not found, we got a bogus keyword
       */
      if (!kw_ok) {
         Dmsg1(200, "%s not found\n", ua->argk[i]);
	 /*
	  * Special case for Job Name, it can be the first
	  * keyword that has no value.
	  */
	 if (!job_name && !ua->argv[i]) {
	    job_name = ua->argk[i];   /* use keyword as job name */
            Dmsg1(200, "Set jobname=%s\n", job_name);
	 } else {
            bsendmsg(ua, _("Invalid keyword: %s\n"), ua->argk[i]);
	    return 0;
	 }
      }
   } /* end argc loop */
	     
   Dmsg0(800, "Done scan.\n");

   CAT *catalog = NULL;
   if (catalog_name != NULL) {
       catalog = (CAT *)GetResWithName(R_CATALOG, catalog_name);
       if (catalog == NULL) {
            bsendmsg(ua, _("Catalog \"%s\" not found\n"), catalog_name);
	   return 0;
       }
   }
   Dmsg1(200, "Using catalog=%s\n", catalog_name);

   if (job_name) {
      /* Find Job */
      job = (JOB *)GetResWithName(R_JOB, job_name);
      if (!job) {
	 if (*job_name != 0) {
            bsendmsg(ua, _("Job \"%s\" not found\n"), job_name);
	 }
	 job = select_job_resource(ua);
      } else {
         Dmsg1(200, "Found job=%s\n", job_name);
      }
   } else {
      bsendmsg(ua, _("A job name must be specified.\n"));
      job = select_job_resource(ua);
   }
   if (!job) {
      return 0;
   } else if (!acl_access_ok(ua, Job_ACL, job->hdr.name)) {
      bsendmsg(ua, _("No authorization. Job \"%s\".\n"),
	 job->hdr.name);
      return 0;
   }

   if (store_name) {
      store = (STORE *)GetResWithName(R_STORAGE, store_name);
      if (!store) {
	 if (*store_name != 0) {
            bsendmsg(ua, _("Storage \"%s\" not found.\n"), store_name);
	 }
	 store = select_storage_resource(ua);
      }
   } else {
      store = (STORE *)job->storage[0]->first();	   /* use default */
   }
   if (!store) {
      return 1;
   } else if (!acl_access_ok(ua, Storage_ACL, store->hdr.name)) {
      bsendmsg(ua, _("No authorization. Storage \"%s\".\n"),
	       store->hdr.name);
      return 0;
   }
   Dmsg1(200, "Using storage=%s\n", store->hdr.name);

   if (pool_name) {
      pool = (POOL *)GetResWithName(R_POOL, pool_name);
      if (!pool) {
	 if (*pool_name != 0) {
            bsendmsg(ua, _("Pool \"%s\" not found.\n"), pool_name);
	 }
	 pool = select_pool_resource(ua);
      }
   } else {
      pool = job->pool; 	    /* use default */
   }
   if (!pool) {
      return 0;
   } else if (!acl_access_ok(ua, Pool_ACL, pool->hdr.name)) {
      bsendmsg(ua, _("No authorization. Pool \"%s\".\n"),
	       pool->hdr.name);
      return 0;
   }
   Dmsg1(200, "Using pool\n", pool->hdr.name);

   if (client_name) {
      client = (CLIENT *)GetResWithName(R_CLIENT, client_name);
      if (!client) {
	 if (*client_name != 0) {
            bsendmsg(ua, _("Client \"%s\" not found.\n"), client_name);
	 }
	 client = select_client_resource(ua);
      }
   } else {
      client = job->client;	      /* use default */
   }
   if (!client) {
      return 0;
   } else if (!acl_access_ok(ua, Client_ACL, client->hdr.name)) {
      bsendmsg(ua, _("No authorization. Client \"%s\".\n"),
	       client->hdr.name);
      return 0;
   }
   Dmsg1(200, "Using client=%s\n", client->hdr.name);

   if (fileset_name) {
      fileset = (FILESET *)GetResWithName(R_FILESET, fileset_name);
      if (!fileset) {
         bsendmsg(ua, _("FileSet \"%s\" not found.\n"), fileset_name);
	 fileset = select_fileset_resource(ua);
      }
   } else {
      fileset = job->fileset;		/* use default */
   }
   if (!fileset) {
      return 0;
   } else if (!acl_access_ok(ua, FileSet_ACL, fileset->hdr.name)) {
      bsendmsg(ua, _("No authorization. FileSet \"%s\".\n"),
	       fileset->hdr.name);
      return 0;
   }

   if (verify_job_name) {
      verify_job = (JOB *)GetResWithName(R_JOB, verify_job_name);
      if (!verify_job) {
         bsendmsg(ua, _("Verify Job \"%s\" not found.\n"), verify_job_name);
	 verify_job = select_job_resource(ua);
      }
   } else {
      verify_job = job->verify_job;
   }

   /*
    * Create JCR to run job.  NOTE!!! after this point, free_jcr()
    *  before returning.
    */
   jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   set_jcr_defaults(jcr, job);

   jcr->verify_job = verify_job;
   set_storage(jcr, store);
   jcr->client = client;
   jcr->fileset = fileset;
   jcr->pool = pool;
   jcr->ExpectedFiles = files;
   if (catalog != NULL) {
      jcr->catalog = catalog;
   }
   if (where) {
      if (jcr->where) {
	 free(jcr->where);
      }
      jcr->where = bstrdup(where);
   }

   if (when) {
      jcr->sched_time = str_to_utime(when);
      if (jcr->sched_time == 0) {
         bsendmsg(ua, _("Invalid time, using current time.\n"));
	 jcr->sched_time = time(NULL);
      }
   }
	 
   if (bootstrap) {
      if (jcr->RestoreBootstrap) {
	 free(jcr->RestoreBootstrap);
      }
      jcr->RestoreBootstrap = bstrdup(bootstrap);
   }

   if (replace) {
      jcr->replace = 0;
      for (i=0; ReplaceOptions[i].name; i++) {
	 if (strcasecmp(replace, ReplaceOptions[i].name) == 0) {
	    jcr->replace = ReplaceOptions[i].token;
	 }
      }
      if (!jcr->replace) {
         bsendmsg(ua, _("Invalid replace option: %s\n"), replace);
	 goto bail_out;
      }
   } else if (job->replace) {
      jcr->replace = job->replace;
   } else {
      jcr->replace = REPLACE_ALWAYS;
   }

   if (Priority) {
      jcr->JobPriority = Priority;
   }
      
   if (find_arg(ua, _("fdcalled")) > 0) {
      jcr->file_bsock = dup_bsock(ua->UA_sock);
      ua->quit = true;
   }

try_again:
   replace = ReplaceOptions[0].name;
   for (i=0; ReplaceOptions[i].name; i++) {
      if (ReplaceOptions[i].token == jcr->replace) {
	 replace = ReplaceOptions[i].name;
      }
   }
   if (level_name) {
      if (!get_level_from_name(jcr, level_name)) {
         bsendmsg(ua, _("Level %s not valid.\n"), level_name);
	 goto bail_out;
      }
   }
   if (jid) {
      jcr->RestoreJobId = atoi(jid);
   }

   /* Run without prompting? */
   if (ua->batch || find_arg(ua, _("yes")) > 0) {
      goto start_job;
   }

   /*  
    * Prompt User to see if all run job parameters are correct, and
    *	allow him to modify them.
    */
   Dmsg1(20, "JobType=%c\n", jcr->JobType);
   switch (jcr->JobType) {
      char ec1[30];
      char dt[MAX_TIME_LENGTH];
   case JT_ADMIN:
         bsendmsg(ua, _("Run %s job\n\
JobName:  %s\n\
FileSet:  %s\n\
Client:   %s\n\
Storage:  %s\n\
When:     %s\n\
Priority: %d\n"),
                 _("Admin"),
		 job->hdr.name,
		 jcr->fileset->hdr.name,
		 NPRT(jcr->client->hdr.name),
		 NPRT(jcr->store->hdr.name), 
		 bstrutime(dt, sizeof(dt), jcr->sched_time), 
		 jcr->JobPriority);
      jcr->JobLevel = L_FULL;
      break;
   case JT_BACKUP:
   case JT_VERIFY:
      if (jcr->JobType == JT_BACKUP) {
         bsendmsg(ua, _("Run %s job\n\
JobName:  %s\n\
FileSet:  %s\n\
Level:    %s\n\
Client:   %s\n\
Storage:  %s\n\
Pool:     %s\n\
When:     %s\n\
Priority: %d\n"),
                 _("Backup"),
		 job->hdr.name,
		 jcr->fileset->hdr.name,
		 level_to_str(jcr->JobLevel),
		 jcr->client->hdr.name,
		 jcr->store->hdr.name,
		 NPRT(jcr->pool->hdr.name), 
		 bstrutime(dt, sizeof(dt), jcr->sched_time),
		 jcr->JobPriority);
      } else {	/* JT_VERIFY */
	 const char *Name;
	 if (jcr->verify_job) {
	    Name = jcr->verify_job->hdr.name;
	 } else {
            Name = "";
	 }
         bsendmsg(ua, _("Run %s job\n\
JobName:     %s\n\
FileSet:     %s\n\
Level:       %s\n\
Client:      %s\n\
Storage:     %s\n\
Pool:        %s\n\
Verify Job:  %s\n\
When:        %s\n\
Priority:    %d\n"),
              _("Verify"),
	      job->hdr.name,
	      jcr->fileset->hdr.name,
	      level_to_str(jcr->JobLevel),
	      jcr->client->hdr.name,
	      jcr->store->hdr.name,
	      NPRT(jcr->pool->hdr.name), 
	      Name,	       
	      bstrutime(dt, sizeof(dt), jcr->sched_time),
	      jcr->JobPriority);
      }
      break;
   case JT_RESTORE:
      if (jcr->RestoreJobId == 0 && !jcr->RestoreBootstrap) {
	 if (jid) {
	    jcr->RestoreJobId = atoi(jid);
	 } else {
            if (!get_pint(ua, _("Please enter a JobId for restore: "))) {
	       goto bail_out;
	    }  
	    jcr->RestoreJobId = ua->pint32_val;
	 }
      }
      jcr->JobLevel = L_FULL;	   /* default level */
      Dmsg1(20, "JobId to restore=%d\n", jcr->RestoreJobId);
      if (jcr->RestoreJobId == 0) {
         bsendmsg(ua, _("Run Restore job\n"
                        "JobName:    %s\n"
                        "Bootstrap:  %s\n"
                        "Where:      %s\n"
                        "Replace:    %s\n"
                        "FileSet:    %s\n"
                        "Client:     %s\n"
                        "Storage:    %s\n"
                        "When:       %s\n"
                        "Catalog:    %s\n"
                        "Priority:   %d\n"),
	      job->hdr.name,
	      NPRT(jcr->RestoreBootstrap),
	      jcr->where?jcr->where:NPRT(job->RestoreWhere),
	      replace,
	      jcr->fileset->hdr.name,
	      jcr->client->hdr.name,
	      jcr->store->hdr.name, 
	      bstrutime(dt, sizeof(dt), jcr->sched_time),
	      jcr->catalog->hdr.name,
	      jcr->JobPriority);
      } else {
         bsendmsg(ua, _("Run Restore job\n"
                       "JobName:    %s\n"
                       "Bootstrap:  %s\n"
                       "Where:      %s\n"
                       "Replace:    %s\n"
                       "Client:     %s\n"
                       "Storage:    %s\n"
                       "JobId:      %s\n"
                       "When:       %s\n"
                       "Catalog:    %s\n"
                       "Priority:   %d\n"),
	      job->hdr.name,
	      NPRT(jcr->RestoreBootstrap),
	      jcr->where?jcr->where:NPRT(job->RestoreWhere),
	      replace,
	      jcr->client->hdr.name,
	      jcr->store->hdr.name, 
              jcr->RestoreJobId==0?"*None*":edit_uint64(jcr->RestoreJobId, ec1), 
	      bstrutime(dt, sizeof(dt), jcr->sched_time),
	      jcr->catalog->hdr.name,
	      jcr->JobPriority);
      }
      break;
   default:
      bsendmsg(ua, _("Unknown Job Type=%d\n"), jcr->JobType);
      goto bail_out;
   }


   if (!get_cmd(ua, _("OK to run? (yes/mod/no): "))) {
      goto bail_out;
   }
   /*
    * At user request modify parameters of job to be run.
    */
   if (ua->cmd[0] == 0) {
      goto bail_out;
   }
   if (strncasecmp(ua->cmd, _("mod"), strlen(ua->cmd)) == 0) {
      FILE *fd;

      start_prompt(ua, _("Parameters to modify:\n"));
      add_prompt(ua, _("Level"));            /* 0 */
      add_prompt(ua, _("Storage"));          /* 1 */
      add_prompt(ua, _("Job"));              /* 2 */
      add_prompt(ua, _("FileSet"));          /* 3 */
      add_prompt(ua, _("Client"));           /* 4 */
      add_prompt(ua, _("When"));             /* 5 */
      add_prompt(ua, _("Priority"));         /* 6 */
      if (jcr->JobType == JT_BACKUP ||
	  jcr->JobType == JT_VERIFY) {
         add_prompt(ua, _("Pool"));          /* 7 */
	 if (jcr->JobType == JT_VERIFY) {
            add_prompt(ua, _("Verify Job"));  /* 8 */
	 }
      } else if (jcr->JobType == JT_RESTORE) {
         add_prompt(ua, _("Bootstrap"));     /* 7 */
         add_prompt(ua, _("Where"));         /* 8 */
         add_prompt(ua, _("Replace"));       /* 9 */
         add_prompt(ua, _("JobId"));         /* 10 */
      }
      switch (do_prompt(ua, "", _("Select parameter to modify"), NULL, 0)) {
      case 0:
	 /* Level */
	 if (jcr->JobType == JT_BACKUP) {
            start_prompt(ua, _("Levels:\n"));
            add_prompt(ua, _("Base"));
            add_prompt(ua, _("Full"));
            add_prompt(ua, _("Incremental"));
            add_prompt(ua, _("Differential"));
            add_prompt(ua, _("Since"));
            switch (do_prompt(ua, "", _("Select level"), NULL, 0)) {
	    case 0:
	       jcr->JobLevel = L_BASE;
	       break;
	    case 1:
	       jcr->JobLevel = L_FULL;
	       break;
	    case 2:
	       jcr->JobLevel = L_INCREMENTAL;
	       break;
	    case 3:
	       jcr->JobLevel = L_DIFFERENTIAL;
	       break;
	    case 4:
	       jcr->JobLevel = L_SINCE;
	       break;
	    default:
	       break;
	    }
	    goto try_again;
	 } else if (jcr->JobType == JT_VERIFY) {
            start_prompt(ua, _("Levels:\n"));
            add_prompt(ua, _("Initialize Catalog"));
            add_prompt(ua, _("Verify Catalog"));
            add_prompt(ua, _("Verify Volume to Catalog"));
            add_prompt(ua, _("Verify Disk to Catalog"));
            add_prompt(ua, _("Verify Volume Data (not yet implemented)"));
            switch (do_prompt(ua, "",  _("Select level"), NULL, 0)) {
	    case 0:
	       jcr->JobLevel = L_VERIFY_INIT;
	       break;
	    case 1:
	       jcr->JobLevel = L_VERIFY_CATALOG;
	       break;
	    case 2:
	       jcr->JobLevel = L_VERIFY_VOLUME_TO_CATALOG;
	       break;
	    case 3:
	       jcr->JobLevel = L_VERIFY_DISK_TO_CATALOG;
	       break;
	    case 4:
	       jcr->JobLevel = L_VERIFY_DATA;
	       break;
	    default:
	       break;
	    }
	    goto try_again;
	 } else {
            bsendmsg(ua, _("Level not appropriate for this Job. Cannot be changed.\n"));
	 }
	 goto try_again;
      case 1:
	 /* Storage */
	 store = select_storage_resource(ua);
	 if (store) {
	    set_storage(jcr, store);
	    goto try_again;
	 }
	 break;
      case 2:
	 /* Job */
	 job = select_job_resource(ua);
	 if (job) {
	    jcr->job = job;
	    set_jcr_defaults(jcr, job);
	    goto try_again;
	 }
	 break;
      case 3:
	 /* FileSet */
	 fileset = select_fileset_resource(ua);
	 if (fileset) {
	    jcr->fileset = fileset;
	    goto try_again;
	 }	
	 break;
      case 4:
	 /* Client */
	 client = select_client_resource(ua);
	 if (client) {
	    jcr->client = client;
	    goto try_again;
	 }
	 break;
      case 5:
	 /* When */
         if (!get_cmd(ua, _("Please enter desired start time as YYYY-MM-DD HH:MM:SS (return for now): "))) {
	    break;
	 }
	 if (ua->cmd[0] == 0) {
	    jcr->sched_time = time(NULL);
	 } else {
	    jcr->sched_time = str_to_utime(ua->cmd);
	    if (jcr->sched_time == 0) {
               bsendmsg(ua, _("Invalid time, using current time.\n"));
	       jcr->sched_time = time(NULL);
	    }
	 }
	 goto try_again;
      case 6:
	 /* Priority */
         if (!get_pint(ua, _("Enter new Priority: "))) {
	    break;
	 }
	 if (ua->pint32_val == 0) {
            bsendmsg(ua, _("Priority must be a positive integer.\n"));
	 } else {
	    jcr->JobPriority = ua->pint32_val;
	 }
	 goto try_again;
      case 7:
	 /* Pool or Bootstrap depending on JobType */
	 if (jcr->JobType == JT_BACKUP ||
	     jcr->JobType == JT_VERIFY) {      /* Pool */
	    pool = select_pool_resource(ua);
	    if (pool) {
	       jcr->pool = pool;
	       goto try_again;
	    }
	    break;
	 }

	 /* Bootstrap */
         if (!get_cmd(ua, _("Please enter the Bootstrap file name: "))) {
	    break;
	 }
	 if (jcr->RestoreBootstrap) {
	    free(jcr->RestoreBootstrap);
	    jcr->RestoreBootstrap = NULL;
	 }
	 if (ua->cmd[0] != 0) {
	    jcr->RestoreBootstrap = bstrdup(ua->cmd);
            fd = fopen(jcr->RestoreBootstrap, "r");
	    if (!fd) {
               bsendmsg(ua, _("Warning cannot open %s: ERR=%s\n"),
		  jcr->RestoreBootstrap, strerror(errno));
	       free(jcr->RestoreBootstrap);
	       jcr->RestoreBootstrap = NULL;
	    } else {
	       fclose(fd);
	    }
	 }
	 goto try_again;
      case 8:
	 /* Verify Job */
	 if (jcr->JobType == JT_VERIFY) {
	    verify_job = select_job_resource(ua);
	    if (verify_job) {
	      jcr->verify_job = verify_job;
	    }
	    goto try_again;
	 }
	 /* Where */
         if (!get_cmd(ua, _("Please enter path prefix for restore (/ for none): "))) {
	    break;
	 }
	 if (jcr->where) {
	    free(jcr->where);
	    jcr->where = NULL;
	 }
         if (ua->cmd[0] == '/' && ua->cmd[1] == 0) {
	    ua->cmd[0] = 0;
	 }
	 jcr->where = bstrdup(ua->cmd);
	 goto try_again;
      case 9:
	 /* Replace */
         start_prompt(ua, _("Replace:\n"));
	 for (i=0; ReplaceOptions[i].name; i++) {
	    add_prompt(ua, ReplaceOptions[i].name);
	 }
         opt = do_prompt(ua, "", _("Select replace option"), NULL, 0);
	 if (opt >=  0) {
	    jcr->replace = ReplaceOptions[opt].token;
	 }
	 goto try_again;
      case 10:
	 /* JobId */
	 jid = NULL;		      /* force reprompt */
	 jcr->RestoreJobId = 0;
	 if (jcr->RestoreBootstrap) {
            bsendmsg(ua, _("You must set the bootstrap file to NULL to be able to specify a JobId.\n"));
	 }
	 goto try_again;
      default: 
	 goto try_again;
      }
      goto bail_out;
   }

   if (strncasecmp(ua->cmd, _("yes"), strlen(ua->cmd)) == 0) {
      JobId_t JobId;
      Dmsg1(200, "Calling run_job job=%x\n", jcr->job);
start_job:
      JobId = run_job(jcr);
      free_jcr(jcr);		      /* release jcr */
      if (JobId == 0) {
         bsendmsg(ua, _("Job failed.\n"));
      } else {
         bsendmsg(ua, _("Job started. JobId=%u\n"), JobId);
      }
      return JobId;
   }

bail_out:
   bsendmsg(ua, _("Job not run.\n"));
   free_jcr(jcr);
   return 0;			   /* do not run */
}
