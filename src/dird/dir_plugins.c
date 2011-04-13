/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2009 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation, which is 
   listed in the file LICENSE.

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
 * Main program to test loading and running Bacula plugins.
 *   Destined to become Bacula pluginloader, ...
 *
 * Kern Sibbald, October 2007
 */
#include "bacula.h"
#include "dird.h"

const int dbglvl = 0;
const char *plugin_type = "-dir.so";


/* Forward referenced functions */
static bRC baculaGetValue(bpContext *ctx, brVariable var, void *value);
static bRC baculaSetValue(bpContext *ctx, bwVariable var, void *value);
static bRC baculaRegisterEvents(bpContext *ctx, ...);
static bRC baculaJobMsg(bpContext *ctx, const char *file, int line,
  int type, utime_t mtime, const char *msg);
static bRC baculaDebugMsg(bpContext *ctx, const char *file, int line,
  int level, const char *msg);


/* Bacula info */
static bInfo binfo = {
   sizeof(bFuncs),
   DIR_PLUGIN_INTERFACE_VERSION,
};

/* Bacula entry points */
static bFuncs bfuncs = {
   sizeof(bFuncs),
   DIR_PLUGIN_INTERFACE_VERSION,
   baculaRegisterEvents,
   baculaGetValue,
   baculaSetValue,
   baculaJobMsg,
   baculaDebugMsg
};

/*
 * Create a plugin event 
 */
void generate_plugin_event(JCR *jcr, bEventType eventType, void *value)
{
   bEvent event;
   Plugin *plugin;
   int i = 0;

   if (!plugin_list || !jcr || !jcr->plugin_ctx_list) {
      return;
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   jcr->eventType = event.eventType = eventType;

   Dmsg2(dbglvl, "plugin_ctx_list=%p JobId=%d\n", jcr->plugin_ctx_list, jcr->JobId);

   foreach_alist(plugin, plugin_list) {
      bRC rc;
      rc = plug_func(plugin)->handlePluginEvent(&plugin_ctx_list[i++], &event, value);
      if (rc != bRC_OK) {
         break;
      }
   }

   return;
}

static void dump_dir_plugin(Plugin *plugin, FILE *fp)
{
   if (!plugin) {
      return ;
   }
   pInfo *info = (pInfo *) plugin->pinfo;
   fprintf(fp, "\tversion=%d\n", info->version);
   fprintf(fp, "\tdate=%s\n", NPRTB(info->plugin_date));
   fprintf(fp, "\tmagic=%s\n", NPRTB(info->plugin_magic));
   fprintf(fp, "\tauthor=%s\n", NPRTB(info->plugin_author));
   fprintf(fp, "\tlicence=%s\n", NPRTB(info->plugin_license));
   fprintf(fp, "\tversion=%s\n", NPRTB(info->plugin_version));
   fprintf(fp, "\tdescription=%s\n", NPRTB(info->plugin_description));
}

void load_dir_plugins(const char *plugin_dir)
{
   if (!plugin_dir) {
      return;
   }

   plugin_list = New(alist(10, not_owned_by_alist));
   load_plugins((void *)&binfo, (void *)&bfuncs, plugin_dir, plugin_type, NULL);
   dbg_plugin_add_hook(dump_dir_plugin);
}

/*
 * Create a new instance of each plugin for this Job
 */
void new_plugins(JCR *jcr)
{
   Plugin *plugin;
   int i = 0;

   if (!plugin_list) {
      return;
   }

   int num = plugin_list->size();

   if (num == 0) {
      return;
   }

   jcr->plugin_ctx_list = (bpContext *)malloc(sizeof(bpContext) * num);

   bpContext *plugin_ctx_list = jcr->plugin_ctx_list;
   Dmsg2(dbglvl, "Instantiate plugin_ctx_list=%p JobId=%d\n", jcr->plugin_ctx_list, jcr->JobId);
   foreach_alist(plugin, plugin_list) {
      /* Start a new instance of each plugin */
      plugin_ctx_list[i].bContext = (void *)jcr;
      plugin_ctx_list[i].pContext = NULL;
      plug_func(plugin)->newPlugin(&plugin_ctx_list[i++]);
   }
}

/*
 * Free the plugin instances for this Job
 */
void free_plugins(JCR *jcr)
{
   Plugin *plugin;
   int i = 0;

   if (!plugin_list || !jcr->plugin_ctx_list) {
      return;
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   Dmsg2(dbglvl, "Free instance plugin_ctx_list=%p JobId=%d\n", jcr->plugin_ctx_list, jcr->JobId);
   foreach_alist(plugin, plugin_list) {
      /* Free the plugin instance */
      plug_func(plugin)->freePlugin(&plugin_ctx_list[i]);
      plugin_ctx_list[i].bContext = NULL;
      plugin_ctx_list[i].pContext = NULL;
      i++;
   }
   free(plugin_ctx_list);
   jcr->plugin_ctx_list = NULL;
}

/* ==============================================================
 *
 * Callbacks from the plugin
 *
 * ==============================================================
 */
static bRC baculaGetValue(bpContext *ctx, brVariable var, void *value)
{
   bRC ret=bRC_OK;

   if (!ctx) {
      return bRC_Error;
   }
   JCR *jcr = (JCR *)(ctx->bContext);
// Dmsg1(dbglvl, "bacula: baculaGetValue var=%d\n", var);
   if (!jcr || !value) {
      return bRC_Error;
   }
// Dmsg1(dbglvl, "Bacula: jcr=%p\n", jcr); 
   switch (var) {
   case bVarJobId:
      *((int *)value) = jcr->JobId;
      Dmsg1(dbglvl, "Bacula: return bVarJobId=%d\n", jcr->JobId);
      break;
   case bVarJobName:
      *((char **)value) = jcr->Job;
      Dmsg1(dbglvl, "Bacula: return bVarJobName=%s\n", jcr->Job);
      break;
   case bVarJob:
      *((char **)value) = jcr->job->hdr.name;
      Dmsg1(dbglvl, "Bacula: return bVarJob=%s\n", jcr->job->hdr.name);
      break;
   case bVarLevel:
      *((int *)value) = jcr->getJobLevel();
      Dmsg1(dbglvl, "Bacula: return bVarLevel=%c\n", jcr->getJobLevel());
      break;
   case bVarType:
      *((int *)value) = jcr->getJobType();
      Dmsg1(dbglvl, "Bacula: return bVarType=%c\n", jcr->getJobType());
      break;
   case bVarClient:
      *((char **)value) = jcr->client->hdr.name;
      Dmsg1(dbglvl, "Bacula: return bVarClient=%s\n", jcr->client->hdr.name);
      break;
   case bVarNumVols:
      POOL_DBR pr;
      memset(&pr, 0, sizeof(pr));
      bstrncpy(pr.Name, jcr->pool->hdr.name, sizeof(pr.Name));
      if (!db_get_pool_record(jcr, jcr->db, &pr)) {
         ret=bRC_Error;
      }
      *((int *)value) = pr.NumVols;
      Dmsg1(dbglvl, "Bacula: return bVarNumVols=%d\n", pr.NumVols);
      break;
   case bVarPool:
      *((char **)value) = jcr->pool->hdr.name;
      Dmsg1(dbglvl, "Bacula: return bVarPool=%s\n", jcr->pool->hdr.name);
      break;
   case bVarStorage:
      if (jcr->wstore) {
         *((char **)value) = jcr->wstore->hdr.name;
      } else if (jcr->rstore) {
         *((char **)value) = jcr->rstore->hdr.name;
      } else {
         *((char **)value) = NULL;
         ret=bRC_Error;
      }
      Dmsg1(dbglvl, "Bacula: return bVarStorage=%s\n", NPRT(*((char **)value)));
      break;
   case bVarWriteStorage:
      if (jcr->wstore) {
         *((char **)value) = jcr->wstore->hdr.name;
      } else {
         *((char **)value) = NULL;
         ret=bRC_Error;
      }
      Dmsg1(dbglvl, "Bacula: return bVarWriteStorage=%s\n", NPRT(*((char **)value)));
      break;
   case bVarReadStorage:
      if (jcr->rstore) {
         *((char **)value) = jcr->rstore->hdr.name;
      } else {
         *((char **)value) = NULL;
         ret=bRC_Error;
      }
      Dmsg1(dbglvl, "Bacula: return bVarReadStorage=%s\n", NPRT(*((char **)value)));
      break;
   case bVarCatalog:
      *((char **)value) = jcr->catalog->hdr.name;
      Dmsg1(dbglvl, "Bacula: return bVarCatalog=%s\n", jcr->catalog->hdr.name);
      break;
   case bVarMediaType:
      if (jcr->wstore) {
         *((char **)value) = jcr->wstore->media_type;
      } else if (jcr->rstore) {
         *((char **)value) = jcr->rstore->media_type;
      } else {
         *((char **)value) = NULL;
         ret=bRC_Error;
      }
      Dmsg1(dbglvl, "Bacula: return bVarMediaType=%s\n", NPRT(*((char **)value)));
      break;
   case bVarJobStatus:
      *((int *)value) = jcr->JobStatus;
      Dmsg1(dbglvl, "Bacula: return bVarJobStatus=%c\n", jcr->JobStatus);
      break;
   case bVarPriority:
      *((int *)value) = jcr->JobPriority;
      Dmsg1(dbglvl, "Bacula: return bVarPriority=%d\n", jcr->JobPriority);
      break;
   case bVarVolumeName:
      *((char **)value) = jcr->VolumeName;
      Dmsg1(dbglvl, "Bacula: return bVarVolumeName=%s\n", jcr->VolumeName);
      break;
   case bVarCatalogRes:
      ret = bRC_Error;
      break;
   case bVarJobErrors:
      *((int *)value) = jcr->JobErrors;
      Dmsg1(dbglvl, "Bacula: return bVarErrors=%d\n", jcr->JobErrors);
      break;
   case bVarJobFiles:
      *((int *)value) = jcr->JobFiles;
      Dmsg1(dbglvl, "Bacula: return bVarFiles=%d\n", jcr->JobFiles);
      break;
   case bVarSDJobFiles:
      *((int *)value) = jcr->SDJobFiles;
      Dmsg1(dbglvl, "Bacula: return bVarSDFiles=%d\n", jcr->SDJobFiles);
      break;
   case bVarSDErrors:
      *((int *)value) = jcr->SDErrors;
      Dmsg1(dbglvl, "Bacula: return bVarSDErrors=%d\n", jcr->SDErrors);
      break;
   case bVarFDJobStatus:
      *((int *)value) = jcr->FDJobStatus;
      Dmsg1(dbglvl, "Bacula: return bVarFDJobStatus=%c\n", jcr->FDJobStatus);
      break;      
   case bVarSDJobStatus:
      *((int *)value) = jcr->SDJobStatus;
      Dmsg1(dbglvl, "Bacula: return bVarSDJobStatus=%c\n", jcr->SDJobStatus);
      break;      
   default:
      ret = bRC_Error;
      break;
   }
   return ret;
}

extern struct s_jl joblevels[];

static bRC baculaSetValue(bpContext *ctx, bwVariable var, void *value)
{
   bRC ret=bRC_OK;

   if (!ctx || !var || !value) {
      return bRC_Error;
   }
   
   JCR *jcr = (JCR *)ctx->bContext;
   int intval = *(int*)value;
   char *strval = (char *)value;
   bool ok;

   switch (var) {
   case bwVarJobReport:
      Jmsg(jcr, M_INFO, 0, "%s", (char *)value);
      break;
   
   case bwVarVolumeName:
      /* Make sure VolumeName is valid and we are in VolumeName event */
      if (jcr->eventType == bEventNewVolume &&
          is_volume_name_legal(NULL, strval))
      {
         pm_strcpy(jcr->VolumeName, strval);
         Dmsg1(100, "Set Vol=%s\n", strval);
      } else {
         jcr->VolumeName[0] = 0;
         ret = bRC_Error;
      }
      break;

   case bwVarPriority:
      Dmsg1(000, "Set priority=%d\n", intval);
      if (intval >= 1 && intval <= 100) {
         jcr->JobPriority = intval;
      } else {
         ret = bRC_Error;
      }
      break;

   case bwVarJobLevel:
      ok=true;
      if (jcr->eventType == bEventJobInit) {
         for (int i=0; ok && joblevels[i].level_name; i++) {
            if (strcasecmp(strval, joblevels[i].level_name) == 0) {
               if (joblevels[i].job_type == jcr->getJobType()) {
                  jcr->set_JobLevel(joblevels[i].level);
                  jcr->jr.JobLevel = jcr->getJobLevel();
                  ok = false;
               }
            }
         }
      } else {
         ret = bRC_Error;
      }
      break;
   default:
      ret = bRC_Error;
      break;
   }
   Dmsg1(dbglvl, "bacula: baculaSetValue var=%d\n", var);
   return ret;
}

static bRC baculaRegisterEvents(bpContext *ctx, ...)
{
   va_list args;
   uint32_t event;

   va_start(args, ctx);
   while ((event = va_arg(args, uint32_t))) {
      Dmsg1(dbglvl, "Plugin wants event=%u\n", event);
   }
   va_end(args);
   return bRC_OK;
}

static bRC baculaJobMsg(bpContext *ctx, const char *file, int line,
  int type, utime_t mtime, const char *msg)
{
   Dmsg5(dbglvl, "Job message: %s:%d type=%d time=%lld msg=%s\n",
      file, line, type, mtime, msg);
   return bRC_OK;
}

static bRC baculaDebugMsg(bpContext *ctx, const char *file, int line,
  int level, const char *msg)
{
   Dmsg4(dbglvl, "Debug message: %s:%d level=%d msg=%s\n",
      file, line, level, msg);
   return bRC_OK;
}

#ifdef TEST_PROGRAM


int main(int argc, char *argv[])
{
   char plugin_dir[1000];
   JCR mjcr1, mjcr2;
   JCR *jcr1 = &mjcr1;
   JCR *jcr2 = &mjcr2;

   strcpy(my_name, "test-dir");
    
   getcwd(plugin_dir, sizeof(plugin_dir)-1);
   load_dir_plugins(plugin_dir);

   jcr1->JobId = 111;
   new_plugins(jcr1);

   jcr2->JobId = 222;
   new_plugins(jcr2);

   generate_plugin_event(jcr1, bEventJobStart, (void *)"Start Job 1");
   generate_plugin_event(jcr1, bEventJobEnd);
   generate_plugin_event(jcr2, bEventJobInit, (void *)"Start Job 1");
   generate_plugin_event(jcr2, bEventJobStart, (void *)"Start Job 1");
   free_plugins(jcr1);
   generate_plugin_event(jcr2, bEventJobEnd);
   free_plugins(jcr2);

   unload_plugins();

   Dmsg0(dbglvl, "bacula: OK ...\n");
   close_memory_pool();
   sm_dump(false);
   return 0;
}

#endif /* TEST_PROGRAM */
