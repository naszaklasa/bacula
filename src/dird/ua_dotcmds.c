/*
 *
 *   Bacula Director -- User Agent Commands
 *     These are "dot" commands, i.e. commands preceded
 *	  by a period. These commands are meant to be used
 *	  by a program, so there is no prompting, and the
 *	  returned results are (supposed to be) predictable.
 *
 *     Kern Sibbald, April MMII
 *
 *   Version $Id: ua_dotcmds.c,v 1.18 2004/09/19 18:56:24 kerns Exp $
 */

/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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

/* Imported variables */
extern int r_first;
extern int r_last;
extern struct s_res resources[];
extern char my_name[];
extern const char *client_backups;

/* Imported functions */
extern int qmessagescmd(UAContext *ua, const char *cmd);
extern int quit_cmd(UAContext *ua, const char *cmd);
extern int qhelp_cmd(UAContext *ua, const char *cmd);
extern int qstatus_cmd(UAContext *ua, const char *cmd);

/* Forward referenced functions */
static int diecmd(UAContext *ua, const char *cmd);
static int jobscmd(UAContext *ua, const char *cmd);
static int filesetscmd(UAContext *ua, const char *cmd);
static int clientscmd(UAContext *ua, const char *cmd);
static int msgscmd(UAContext *ua, const char *cmd);
static int poolscmd(UAContext *ua, const char *cmd);
static int storagecmd(UAContext *ua, const char *cmd);
static int defaultscmd(UAContext *ua, const char *cmd);
static int typescmd(UAContext *ua, const char *cmd);
static int backupscmd(UAContext *ua, const char *cmd);
static int levelscmd(UAContext *ua, const char *cmd);

struct cmdstruct { const char *key; int (*func)(UAContext *ua, const char *cmd); const char *help; }; 
static struct cmdstruct commands[] = {
 { N_(".die"),        diecmd,       NULL},
 { N_(".jobs"),       jobscmd,      NULL},
 { N_(".filesets"),   filesetscmd,  NULL},
 { N_(".clients"),    clientscmd,   NULL},
 { N_(".msgs"),       msgscmd,      NULL},
 { N_(".pools"),      poolscmd,     NULL},
 { N_(".types"),      typescmd,     NULL},
 { N_(".backups"),    backupscmd,   NULL},
 { N_(".levels"),     levelscmd,    NULL},
 { N_(".status"),     qstatus_cmd,  NULL},
 { N_(".storage"),    storagecmd,   NULL},
 { N_(".defaults"),   defaultscmd,  NULL},
 { N_(".messages"),   qmessagescmd, NULL},
 { N_(".help"),       qhelp_cmd,    NULL},
 { N_(".quit"),       quit_cmd,     NULL},
 { N_(".exit"),       quit_cmd,     NULL} 
	     };
#define comsize (sizeof(commands)/sizeof(struct cmdstruct))

/*
 * Execute a command from the UA
 */
int do_a_dot_command(UAContext *ua, const char *cmd)
{
   int i;
   int len, stat;  
   bool found = false;

   stat = 1;

   Dmsg1(400, "Dot command: %s\n", ua->UA_sock->msg);
   if (ua->argc == 0) {
      return 1;
   }

   len = strlen(ua->argk[0]);
   if (len == 1) {
      return 1; 		      /* no op */
   }
   for (i=0; i<(int)comsize; i++) {	/* search for command */
      if (strncasecmp(ua->argk[0],  _(commands[i].key), len) == 0) {
	 stat = (*commands[i].func)(ua, cmd);	/* go execute command */
	 found = true;
	 break;
      }
   }
   if (!found) {
      pm_strcat(ua->UA_sock->msg, _(": is an illegal command\n"));
      ua->UA_sock->msglen = strlen(ua->UA_sock->msg);
      bnet_send(ua->UA_sock);
   }
   return stat;
}

/*
 * Create segmentation fault 
 */
static int diecmd(UAContext *ua, const char *cmd)
{
   JCR *jcr = NULL;
   int a;
   
   bsendmsg(ua, "The Director will segment fault.\n");
   a = jcr->JobId; /* ref NULL pointer */
   jcr->JobId = 1000; /* another ref NULL pointer */
   return 0;
}

static int jobscmd(UAContext *ua, const char *cmd)
{
   JOB *job = NULL;
   LockRes();
   while ( (job = (JOB *)GetNextRes(R_JOB, (RES *)job)) ) {
      bsendmsg(ua, "%s\n", job->hdr.name);
   }
   UnlockRes();
   return 1;
}

static int filesetscmd(UAContext *ua, const char *cmd)
{
   FILESET *fs = NULL;
   LockRes();
   while ( (fs = (FILESET *)GetNextRes(R_FILESET, (RES *)fs)) ) {
      bsendmsg(ua, "%s\n", fs->hdr.name);
   }
   UnlockRes();
   return 1;
}

static int clientscmd(UAContext *ua, const char *cmd)
{
   CLIENT *client = NULL;
   LockRes();
   while ( (client = (CLIENT *)GetNextRes(R_CLIENT, (RES *)client)) ) {
      bsendmsg(ua, "%s\n", client->hdr.name);
   }
   UnlockRes();
   return 1;
}

static int msgscmd(UAContext *ua, const char *cmd)
{
   MSGS *msgs = NULL;
   LockRes();
   while ( (msgs = (MSGS *)GetNextRes(R_MSGS, (RES *)msgs)) ) {
      bsendmsg(ua, "%s\n", msgs->hdr.name);
   }
   UnlockRes();
   return 1;
}

static int poolscmd(UAContext *ua, const char *cmd)
{
   POOL *pool = NULL;
   LockRes();
   while ( (pool = (POOL *)GetNextRes(R_POOL, (RES *)pool)) ) {
      bsendmsg(ua, "%s\n", pool->hdr.name);
   }
   UnlockRes();
   return 1;
}

static int storagecmd(UAContext *ua, const char *cmd)
{
   STORE *store = NULL;
   LockRes();
   while ( (store = (STORE *)GetNextRes(R_STORAGE, (RES *)store)) ) {
      bsendmsg(ua, "%s\n", store->hdr.name);
   }
   UnlockRes();
   return 1;
}


static int typescmd(UAContext *ua, const char *cmd)
{
   bsendmsg(ua, "Backup\n");
   bsendmsg(ua, "Restore\n");
   bsendmsg(ua, "Admin\n");
   bsendmsg(ua, "Verify\n");
   return 1;
}

static int client_backups_handler(void *ctx, int num_field, char **row)
{
   UAContext *ua = (UAContext *)ctx;
   bsendmsg(ua, "| %s | %s | %s | %s | %s | %s | %s |\n",
      row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7]);
   return 0;
}

static int backupscmd(UAContext *ua, const char *cmd)
{
   if (!open_db(ua)) {
      return 1;
   }
   if (ua->argc == 2 && strcmp(ua->argk[1], "client") != 0) {
      return 1;
   }
   Mmsg(ua->cmd, client_backups, ua->argv[1]);
   if (!db_sql_query(ua->db, ua->cmd, client_backups_handler, (void *)ua)) {
      bsendmsg(ua, _("Query failed: %s. ERR=%s\n"), ua->cmd, db_strerror(ua->db));
      return 1;
   }
   return 1;
}


static int levelscmd(UAContext *ua, const char *cmd)
{
   bsendmsg(ua, "Incremental\n");
   bsendmsg(ua, "Full\n");
   bsendmsg(ua, "Differential\n");
   bsendmsg(ua, "Catalog\n");
   bsendmsg(ua, "InitCatalog\n");
   bsendmsg(ua, "VolumeToCatalog\n");
   return 1;
}



/*
 * Return default values for a job
 */
static int defaultscmd(UAContext *ua, const char *cmd)
{
   JOB *job;	   
   if (ua->argc == 2 && strcmp(ua->argk[1], "job") == 0) {
      job = (JOB *)GetResWithName(R_JOB, ua->argv[1]);
      if (job) {
	 STORE *store;
         bsendmsg(ua, "job=%s", job->hdr.name);
         bsendmsg(ua, "pool=%s", job->pool->hdr.name);
         bsendmsg(ua, "messages=%s", job->messages->hdr.name);
         bsendmsg(ua, "client=%s", job->client->hdr.name);
	 store = (STORE *)job->storage[0]->first();
         bsendmsg(ua, "storage=%s", store->hdr.name);
         bsendmsg(ua, "where=%s", job->RestoreWhere?job->RestoreWhere:"");
         bsendmsg(ua, "level=%s", level_to_str(job->JobLevel));
         bsendmsg(ua, "type=%s", job_type_to_str(job->JobType));
         bsendmsg(ua, "fileset=%s", job->fileset->hdr.name);
      }
   }
   return 1;
}
