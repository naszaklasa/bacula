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
#include "stored.h"
#include "sd_plugins.h"

const int dbglvl = 0;
const char *plugin_type = "-sd.so";


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
   SD_PLUGIN_INTERFACE_VERSION,
};

/* Bacula entry points */
static bFuncs bfuncs = {
   sizeof(bFuncs),
   SD_PLUGIN_INTERFACE_VERSION,
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

   if (!plugin_list) {
      return;
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   event.eventType = eventType;

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

static void dump_sd_plugin(Plugin *plugin, FILE *fp)
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

void load_sd_plugins(const char *plugin_dir)
{
   if (!plugin_dir) {
      return;
   }

   plugin_list = New(alist(10, not_owned_by_alist));
   load_plugins((void *)&binfo, (void *)&bfuncs, plugin_dir, plugin_type, NULL);
   dbg_plugin_add_hook(dump_sd_plugin);
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

   if (!plugin_list) {
      return;
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   Dmsg2(dbglvl, "Free instance plugin_ctx_list=%p JobId=%d\n", jcr->plugin_ctx_list, jcr->JobId);
   foreach_alist(plugin, plugin_list) {
      /* Free the plugin instance */
      plug_func(plugin)->freePlugin(&plugin_ctx_list[i++]);
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
   JCR *jcr = (JCR *)(ctx->bContext);
// Dmsg1(dbglvl, "bacula: baculaGetValue var=%d\n", var);
   if (!value) {
      return bRC_Error;
   }
// Dmsg1(dbglvl, "Bacula: jcr=%p\n", jcr); 
   switch (var) {
   case bVarJobId:
      *((int *)value) = jcr->JobId;
      Dmsg1(dbglvl, "Bacula: return bVarJobId=%d\n", jcr->JobId);
      break;
   default:
      break;
   }
   return bRC_OK;
}

static bRC baculaSetValue(bpContext *ctx, bwVariable var, void *value)
{
   Dmsg1(dbglvl, "bacula: baculaSetValue var=%d\n", var);
   return bRC_OK;
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
   load_sd_plugins(plugin_dir);

   jcr1->JobId = 111;
   new_plugins(jcr1);

   jcr2->JobId = 222;
   new_plugins(jcr2);

   generate_plugin_event(jcr1, bEventJobStart, (void *)"Start Job 1");
   generate_plugin_event(jcr1, bEventJobEnd);
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
