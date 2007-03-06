/*
 *
 *   Bacula Director -- User Agent Commands
 *     These are "dot" commands, i.e. commands preceded
 *        by a period. These commands are meant to be used
 *        by a program, so there is no prompting, and the
 *        returned results are (supposed to be) predictable.
 *
 *     Kern Sibbald, April MMII
 *
 *   Version $Id: ua_dotcmds.c,v 1.45 2006/12/23 16:33:52 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2006 Free Software Foundation Europe e.V.

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

/* Imported variables */
extern int r_first;
extern int r_last;
extern struct s_res resources[];

/* Imported functions */
extern void do_messages(UAContext *ua, const char *cmd);
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
static int getmsgscmd(UAContext *ua, const char *cmd);

struct cmdstruct { const char *key; int (*func)(UAContext *ua, const char *cmd); const char *help; };
static struct cmdstruct commands[] = {
 { NT_(".backups"),    backupscmd,   NULL},
 { NT_(".clients"),    clientscmd,   NULL},
 { NT_(".defaults"),   defaultscmd,  NULL},
 { NT_(".die"),        diecmd,       NULL},
 { NT_(".exit"),       quit_cmd,     NULL},
 { NT_(".filesets"),   filesetscmd,  NULL},
 { NT_(".help"),       qhelp_cmd,    NULL},
 { NT_(".jobs"),       jobscmd,      NULL},
 { NT_(".levels"),     levelscmd,    NULL},
 { NT_(".messages"),   getmsgscmd,   NULL},
 { NT_(".msgs"),       msgscmd,      NULL},
 { NT_(".pools"),      poolscmd,     NULL},
 { NT_(".quit"),       quit_cmd,     NULL},
 { NT_(".status"),     qstatus_cmd,  NULL},
 { NT_(".storage"),    storagecmd,   NULL},
 { NT_(".types"),      typescmd,     NULL} 
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

   Dmsg1(1400, "Dot command: %s\n", ua->UA_sock->msg);
   if (ua->argc == 0) {
      return 1;
   }

   len = strlen(ua->argk[0]);
   if (len == 1) {
      return 1;                       /* no op */
   }
   for (i=0; i<(int)comsize; i++) {     /* search for command */
      if (strncasecmp(ua->argk[0],  _(commands[i].key), len) == 0) {
         bool gui = ua->gui;
         ua->gui = true;
         stat = (*commands[i].func)(ua, cmd);   /* go execute command */
         ua->gui = gui;
         found = true;
         break;
      }
   }
   if (!found) {
      pm_strcat(ua->UA_sock->msg, _(": is an invalid command\n"));
      ua->UA_sock->msglen = strlen(ua->UA_sock->msg);
      bnet_send(ua->UA_sock);
   }
   return stat;
}

static int getmsgscmd(UAContext *ua, const char *cmd)
{
   if (console_msg_pending) {
      do_messages(ua, cmd);
   }
   return 1;
}

/*
 * Create segmentation fault
 */
static int diecmd(UAContext *ua, const char *cmd)
{
   JCR *jcr = NULL;
   int a;

   bsendmsg(ua, _("The Director will segment fault.\n"));
   a = jcr->JobId; /* ref NULL pointer */
   jcr->JobId = 1000; /* another ref NULL pointer */
   return 0;
}

static int jobscmd(UAContext *ua, const char *cmd)
{
   JOB *job = NULL;
   LockRes();
   while ( (job = (JOB *)GetNextRes(R_JOB, (RES *)job)) ) {
      if (acl_access_ok(ua, Job_ACL, job->name())) {
         bsendmsg(ua, "%s\n", job->name());
      }
   }
   UnlockRes();
   return 1;
}

static int filesetscmd(UAContext *ua, const char *cmd)
{
   FILESET *fs = NULL;
   LockRes();
   while ( (fs = (FILESET *)GetNextRes(R_FILESET, (RES *)fs)) ) {
      if (acl_access_ok(ua, FileSet_ACL, fs->name())) {
         bsendmsg(ua, "%s\n", fs->name());
      }
   }
   UnlockRes();
   return 1;
}

static int clientscmd(UAContext *ua, const char *cmd)
{
   CLIENT *client = NULL;
   LockRes();
   while ( (client = (CLIENT *)GetNextRes(R_CLIENT, (RES *)client)) ) {
      if (acl_access_ok(ua, Client_ACL, client->name())) {
         bsendmsg(ua, "%s\n", client->name());
      }
   }
   UnlockRes();
   return 1;
}

static int msgscmd(UAContext *ua, const char *cmd)
{
   MSGS *msgs = NULL;
   LockRes();
   while ( (msgs = (MSGS *)GetNextRes(R_MSGS, (RES *)msgs)) ) {
      bsendmsg(ua, "%s\n", msgs->name());
   }
   UnlockRes();
   return 1;
}

static int poolscmd(UAContext *ua, const char *cmd)
{
   POOL *pool = NULL;
   LockRes();
   while ( (pool = (POOL *)GetNextRes(R_POOL, (RES *)pool)) ) {
      if (acl_access_ok(ua, Pool_ACL, pool->name())) {
         bsendmsg(ua, "%s\n", pool->name());
      }
   }
   UnlockRes();
   return 1;
}

static int storagecmd(UAContext *ua, const char *cmd)
{
   STORE *store = NULL;
   LockRes();
   while ( (store = (STORE *)GetNextRes(R_STORAGE, (RES *)store)) ) {
      if (acl_access_ok(ua, Storage_ACL, store->name())) {
         bsendmsg(ua, "%s\n", store->name());
      }
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
   bsendmsg(ua, "Migrate\n");
   return 1;
}

static int client_backups_handler(void *ctx, int num_field, char **row)
{
   UAContext *ua = (UAContext *)ctx;
   bsendmsg(ua, "| %s | %s | %s | %s | %s | %s | %s | %s |\n",
      row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8]);
   return 0;
}

static int backupscmd(UAContext *ua, const char *cmd)
{
   if (!open_client_db(ua)) {
      return 1;
   }
   if (ua->argc != 3 || strcmp(ua->argk[1], "client") != 0 || strcmp(ua->argk[2], "fileset") != 0) {
      return 1;
   }
   if (!acl_access_ok(ua, Client_ACL, ua->argv[1]) ||
       !acl_access_ok(ua, FileSet_ACL, ua->argv[2])) {
      return 1;
   }
   Mmsg(ua->cmd, client_backups, ua->argv[1], ua->argv[2]);
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
   CLIENT *client;
   STORE *storage;
   POOL *pool;

   if (ua->argc != 2 || !ua->argv[1]) {
      return 1;
   }

   /* Job defaults */   
   if (strcmp(ua->argk[1], "job") == 0) {
      if (!acl_access_ok(ua, Job_ACL, ua->argv[1])) {
         return 1;
      }
      job = (JOB *)GetResWithName(R_JOB, ua->argv[1]);
      if (job) {
         USTORE store;
         bsendmsg(ua, "job=%s", job->name());
         bsendmsg(ua, "pool=%s", job->pool->name());
         bsendmsg(ua, "messages=%s", job->messages->name());
         bsendmsg(ua, "client=%s", job->client->name());
         get_job_storage(&store, job, NULL);
         bsendmsg(ua, "storage=%s", store.store->name());
         bsendmsg(ua, "where=%s", job->RestoreWhere?job->RestoreWhere:"");
         bsendmsg(ua, "level=%s", level_to_str(job->JobLevel));
         bsendmsg(ua, "type=%s", job_type_to_str(job->JobType));
         bsendmsg(ua, "fileset=%s", job->fileset->name());
         bsendmsg(ua, "enabled=%d", job->enabled);
         bsendmsg(ua, "catalog=%s", job->client->catalog->name());
      }
   } 
   /* Client defaults */
   else if (strcmp(ua->argk[1], "client") == 0) {
      if (!acl_access_ok(ua, Client_ACL, ua->argv[1])) {
         return 1;   
      }
      client = (CLIENT *)GetResWithName(R_CLIENT, ua->argv[1]);
      if (client) {
         bsendmsg(ua, "client=%s", client->name());
         bsendmsg(ua, "address=%s", client->address);
         bsendmsg(ua, "fdport=%d", client->FDport);
         bsendmsg(ua, "file_retention=%d", client->FileRetention);
         bsendmsg(ua, "job_retention=%d", client->JobRetention);
         bsendmsg(ua, "autoprune=%d", client->AutoPrune);
         bsendmsg(ua, "catalog=%s", client->catalog->name());
      }
   }
   /* Storage defaults */
   else if (strcmp(ua->argk[1], "storage") == 0) {
      if (!acl_access_ok(ua, Storage_ACL, ua->argv[1])) {
         return 1;
      }
      storage = (STORE *)GetResWithName(R_STORAGE, ua->argv[1]);
      DEVICE *device;
      if (storage) {
         bsendmsg(ua, "storage=%s", storage->name());
         bsendmsg(ua, "address=%s", storage->address);
         bsendmsg(ua, "enabled=%d", storage->enabled);
         bsendmsg(ua, "media_type=%s", storage->media_type);
         bsendmsg(ua, "sdport=%d", storage->SDport);
         device = (DEVICE *)storage->device->first();
         bsendmsg(ua, "device=%s", device->name());
         if (storage->device->size() > 1) {
            while ((device = (DEVICE *)storage->device->next())) {
               bsendmsg(ua, ",%s", device->name());
            }
         }
      }
   }
   /* Pool defaults */
   else if (strcmp(ua->argk[1], "pool") == 0) {
      if (!acl_access_ok(ua, Pool_ACL, ua->argv[1])) {
         return 1;
      }
      pool = (POOL *)GetResWithName(R_POOL, ua->argv[1]);
      if (pool) {
         bsendmsg(ua, "pool=%s", pool->name());
         bsendmsg(ua, "pool_type=%s", pool->pool_type);
         bsendmsg(ua, "label_format=%s", pool->label_format?pool->label_format:"");
         bsendmsg(ua, "use_volume_once=%d", pool->use_volume_once);
         bsendmsg(ua, "purge_oldest_volume=%d", pool->purge_oldest_volume);
         bsendmsg(ua, "recycle_oldest_volume=%d", pool->recycle_oldest_volume);
         bsendmsg(ua, "recycle_current_volume=%d", pool->recycle_current_volume);
         bsendmsg(ua, "max_volumes=%d", pool->max_volumes);
         bsendmsg(ua, "vol_retention=%d", pool->VolRetention);
         bsendmsg(ua, "vol_use_duration=%d", pool->VolUseDuration);
         bsendmsg(ua, "max_vol_jobs=%d", pool->MaxVolJobs);
         bsendmsg(ua, "max_vol_files=%d", pool->MaxVolFiles);
         bsendmsg(ua, "max_vol_bytes=%d", pool->MaxVolBytes);
         bsendmsg(ua, "auto_prune=%d", pool->AutoPrune);
         bsendmsg(ua, "recycle=%d", pool->Recycle);
      }
   }
   return 1;
}
