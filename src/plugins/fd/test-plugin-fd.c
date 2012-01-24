/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2010 Free Software Foundation Europe e.V.

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
 * A simple test plugin for the Bacula File Daemon derived from
 *   the bpipe plugin, but used for testing new features.
 *
 *  Kern Sibbald, October 2007
 *
 */
#include "bacula.h"
#include "fd_plugins.h"
#include "lib/ini.h"
#include <wchar.h>

#undef malloc
#undef free
#undef strdup

#define fi __FILE__
#define li __LINE__

#ifdef __cplusplus
extern "C" {
#endif

static const int dbglvl = 000;

#define PLUGIN_LICENSE      "Bacula AGPLv3"
#define PLUGIN_AUTHOR       "Kern Sibbald"
#define PLUGIN_DATE         "May 2011"
#define PLUGIN_VERSION      "3"
#define PLUGIN_DESCRIPTION  "Bacula Test File Daemon Plugin"

/* Forward referenced functions */
static bRC newPlugin(bpContext *ctx);
static bRC freePlugin(bpContext *ctx);
static bRC getPluginValue(bpContext *ctx, pVariable var, void *value);
static bRC setPluginValue(bpContext *ctx, pVariable var, void *value);
static bRC handlePluginEvent(bpContext *ctx, bEvent *event, void *value);
static bRC startBackupFile(bpContext *ctx, struct save_pkt *sp);
static bRC endBackupFile(bpContext *ctx);
static bRC pluginIO(bpContext *ctx, struct io_pkt *io);
static bRC startRestoreFile(bpContext *ctx, const char *cmd);
static bRC endRestoreFile(bpContext *ctx);
static bRC createFile(bpContext *ctx, struct restore_pkt *rp);
static bRC setFileAttributes(bpContext *ctx, struct restore_pkt *rp);

/* Pointers to Bacula functions */
static bFuncs *bfuncs = NULL;
static bInfo  *binfo = NULL;

/* Plugin Information block */
static pInfo pluginInfo = {
   sizeof(pluginInfo),
   FD_PLUGIN_INTERFACE_VERSION,
   FD_PLUGIN_MAGIC,
   PLUGIN_LICENSE,
   PLUGIN_AUTHOR,
   PLUGIN_DATE,
   PLUGIN_VERSION,
   PLUGIN_DESCRIPTION,
};

/* Plugin entry points for Bacula */
static pFuncs pluginFuncs = {
   sizeof(pluginFuncs),
   FD_PLUGIN_INTERFACE_VERSION,

   /* Entry points into plugin */
   newPlugin,                         /* new plugin instance */
   freePlugin,                        /* free plugin instance */
   getPluginValue,
   setPluginValue,
   handlePluginEvent,
   startBackupFile,
   endBackupFile,
   startRestoreFile,
   endRestoreFile,
   pluginIO,
   createFile,
   setFileAttributes
};

static struct ini_items test_items[] = {
   // name       handler         comment            required
   { "string1",  ini_store_str,  "Special String",    1},
   { "string2",  ini_store_str,  "2nd String",        0},
   { "ok",       ini_store_bool, "boolean",           0},

// We can also use the ITEMS_DEFAULT  
// { "ok",       ini_store_bool, "boolean",           0, ITEMS_DEFAULT},
   { NULL,       NULL,           NULL,                0}
};

/*
 * Plugin private context
 */
struct plugin_ctx {
   boffset_t offset;
   FILE *fd;                          /* pipe file descriptor */
   char *cmd;                         /* plugin command line */
   char *fname;                       /* filename to "backup/restore" */
   char *reader;                      /* reader program for backup */
   char *writer;                      /* writer program for backup */

   char where[512];
   int replace;

   int nb_obj;                        /* Number of objects created */
   POOLMEM *buf;                      /* store ConfigFile */
};

/*
 * loadPlugin() and unloadPlugin() are entry points that are
 *  exported, so Bacula can directly call these two entry points
 *  they are common to all Bacula plugins.
 */
/*
 * External entry point called by Bacula to "load the plugin
 */
bRC loadPlugin(bInfo *lbinfo, bFuncs *lbfuncs, pInfo **pinfo, pFuncs **pfuncs)
{
   bfuncs = lbfuncs;                  /* set Bacula funct pointers */
   binfo  = lbinfo;
   *pinfo  = &pluginInfo;             /* return pointer to our info */
   *pfuncs = &pluginFuncs;            /* return pointer to our functions */

   return bRC_OK;
}

/*
 * External entry point to unload the plugin 
 */
bRC unloadPlugin() 
{
// printf("test-plugin-fd: Unloaded\n");
   return bRC_OK;
}

/*
 * The following entry points are accessed through the function 
 *   pointers we supplied to Bacula. Each plugin type (dir, fd, sd)
 *   has its own set of entry points that the plugin must define.
 */
/*
 * Create a new instance of the plugin i.e. allocate our private storage
 */
static bRC newPlugin(bpContext *ctx)
{
   struct plugin_ctx *p_ctx = (struct plugin_ctx *)malloc(sizeof(struct plugin_ctx));
   if (!p_ctx) {
      return bRC_Error;
   }
   memset(p_ctx, 0, sizeof(struct plugin_ctx));
   ctx->pContext = (void *)p_ctx;        /* set our context pointer */
   return bRC_OK;
}

/*
 * Free a plugin instance, i.e. release our private storage
 */
static bRC freePlugin(bpContext *ctx)
{
   struct plugin_ctx *p_ctx = (struct plugin_ctx *)ctx->pContext;
   if (!p_ctx) {
      return bRC_Error;
   }
   if (p_ctx->buf) {
      free_pool_memory(p_ctx->buf);
   }
   if (p_ctx->cmd) {
      free(p_ctx->cmd);                  /* free any allocated command string */
   }
   free(p_ctx);                          /* free our private context */
   ctx->pContext = NULL;
   return bRC_OK;
}

/*
 * Return some plugin value (none defined)
 */
static bRC getPluginValue(bpContext *ctx, pVariable var, void *value) 
{
   return bRC_OK;
}

/*
 * Set a plugin value (none defined)
 */
static bRC setPluginValue(bpContext *ctx, pVariable var, void *value) 
{
   return bRC_OK;
}

/*
 * Handle an event that was generated in Bacula
 */
static bRC handlePluginEvent(bpContext *ctx, bEvent *event, void *value)
{
   struct plugin_ctx *p_ctx = (struct plugin_ctx *)ctx->pContext;
   restore_object_pkt *rop;
   if (!p_ctx) {
      return bRC_Error;
   }

// char *name;

   /*
    * Most events don't interest us so we ignore them.
    *   the printfs are so that plugin writers can enable them to see
    *   what is really going on.
    */
   switch (event->eventType) {
   case bEventJobStart:
      bfuncs->DebugMessage(ctx, fi, li, dbglvl, "test-plugin-fd: JobStart=%s\n", (char *)value);
      break;
   case bEventJobEnd:
   case bEventEndBackupJob:
   case bEventLevel:
   case bEventSince:
   case bEventStartRestoreJob:
   case bEventEndRestoreJob:
      break;
   /* End of Dir FileSet commands, now we can add excludes */
   case bEventEndFileSet:
      bfuncs->NewOptions(ctx);
      bfuncs->AddWild(ctx, "*.c", ' ');
      bfuncs->AddWild(ctx, "*.cpp", ' ');
      bfuncs->AddOptions(ctx, "ei");         /* exclude, ignore case */
      bfuncs->AddExclude(ctx, "/home/kern/bacula/regress/README");
      break;
   case bEventStartBackupJob:
      break;
   case bEventRestoreObject:
      printf("Plugin RestoreObject\n");
      if (!value) {
         bfuncs->DebugMessage(ctx, fi, li, dbglvl, "test-plugin-fd: End restore objects\n");
         break;
      }
      rop = (restore_object_pkt *)value;
      bfuncs->DebugMessage(ctx, fi, li, dbglvl, 
                           "Get RestoreObject len=%d JobId=%d oname=%s type=%d data=%s\n",
                           rop->object_len, rop->JobId, rop->object_name, rop->object_type,
                           rop->object);

      if (!strcmp(rop->object_name, INI_RESTORE_OBJECT_NAME)) {
         ConfigFile ini;
         if (ini.dump_string(rop->object, rop->object_len)) {
            break;
         }
         ini.register_items(test_items, sizeof(struct ini_items));
         if (ini.parse(ini.out_fname)) {
            bfuncs->JobMessage(ctx, fi, li, M_INFO, 0, "string1 = %s\n", 
                               ini.items[0].val.strval);
         } else {
            bfuncs->JobMessage(ctx, fi, li, M_ERROR, 0, "Can't parse config\n");
         }
      }

      break;
   /* Plugin command e.g. plugin = <plugin-name>:<name-space>:read command:write command */
   case bEventRestoreCommand:
      /* Fall-through wanted */
   case bEventEstimateCommand:
      /* Fall-through wanted */
   case bEventBackupCommand:
      char *p;
      bfuncs->DebugMessage(ctx, fi, li, dbglvl, "test-plugin-fd: pluginEvent cmd=%s\n", (char *)value);
      p_ctx->cmd = strdup((char *)value);
      p = strchr(p_ctx->cmd, ':');
      if (!p) {
         bfuncs->JobMessage(ctx, fi, li, M_FATAL, 0, "Plugin terminator not found: %s\n", (char *)value);
         return bRC_Error;
      }
      *p++ = 0;           /* terminate plugin */
      p_ctx->fname = p;
      p = strchr(p, ':');
      if (!p) {
         bfuncs->JobMessage(ctx, fi, li, M_FATAL, 0, "File terminator not found: %s\n", (char *)value);
         return bRC_Error;
      }
      *p++ = 0;           /* terminate file */
      p_ctx->reader = p;
      p = strchr(p, ':');
      if (!p) {
         bfuncs->JobMessage(ctx, fi, li, M_FATAL, 0, "Reader terminator not found: %s\n", (char *)value);
         return bRC_Error;
      }
      *p++ = 0;           /* terminate reader string */
      p_ctx->writer = p;
      printf("test-plugin-fd: plugin=%s fname=%s reader=%s writer=%s\n", 
          p_ctx->cmd, p_ctx->fname, p_ctx->reader, p_ctx->writer);
      break;
   case bEventPluginCommand:
      break;
   case bEventVssBeforeCloseRestore:
      break;
   default:
      printf("test-plugin-fd: unknown event=%d\n", event->eventType);
      break;
   }
   return bRC_OK;
}

/* 
 * Start the backup of a specific file
 */
static bRC startBackupFile(bpContext *ctx, struct save_pkt *sp)
{
   struct plugin_ctx *p_ctx = (struct plugin_ctx *)ctx->pContext;
   if (!p_ctx) {
      return bRC_Error;
   }

   if (p_ctx->nb_obj == 0) {
      sp->object_name = (char *)"james.xml";
      sp->object = (char *)"This is test data for the restore object. "
  "garbage=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa."
  "\0secret";
      sp->object_len = strlen(sp->object)+1+6+1; /* str + 0 + secret + 0 */
      sp->type = FT_RESTORE_FIRST;
   
   } else if (p_ctx->nb_obj == 1) {
      ConfigFile ini;
      p_ctx->buf = get_pool_memory(PM_BSOCK);
      ini.register_items(test_items, sizeof(struct ini_items));

      sp->object_name = (char*)INI_RESTORE_OBJECT_NAME;
      sp->object_len = ini.serialize(&p_ctx->buf);
      sp->object = p_ctx->buf;
      sp->type = FT_PLUGIN_CONFIG;

      Dmsg1(0, "RestoreOptions=<%s>\n", p_ctx->buf);
   } 

   time_t now = time(NULL);
   sp->index = ++p_ctx->nb_obj;
   sp->statp.st_mode = 0700 | S_IFREG;
   sp->statp.st_ctime = now;
   sp->statp.st_mtime = now;
   sp->statp.st_atime = now;
   sp->statp.st_size = sp->object_len;
   sp->statp.st_blksize = 4096;
   sp->statp.st_blocks = 1;
   bfuncs->DebugMessage(ctx, fi, li, dbglvl,
                        "Creating RestoreObject len=%d oname=%s data=%s\n", 
                        sp->object_len, sp->object_name, sp->object);

   printf("test-plugin-fd: startBackupFile\n");
   return bRC_OK;
}

/*
 * Done with backup of this file
 */
static bRC endBackupFile(bpContext *ctx)
{
   /*
    * We would return bRC_More if we wanted startBackupFile to be
    * called again to backup another file
    */
   return bRC_OK;
}


/*
 * Bacula is calling us to do the actual I/O
 */
static bRC pluginIO(bpContext *ctx, struct io_pkt *io)
{
   struct plugin_ctx *p_ctx = (struct plugin_ctx *)ctx->pContext;
   if (!p_ctx) {
      return bRC_Error;
   }
    
   io->status = 0;
   io->io_errno = 0;
   return bRC_OK;
}

/*
 * Bacula is notifying us that a plugin name string was found, and
 *   passing us the plugin command, so we can prepare for a restore.
 */
static bRC startRestoreFile(bpContext *ctx, const char *cmd)
{
   printf("test-plugin-fd: startRestoreFile cmd=%s\n", cmd);
   return bRC_OK;
}

/*
 * Bacula is notifying us that the plugin data has terminated, so
 *  the restore for this particular file is done.
 */
static bRC endRestoreFile(bpContext *ctx)
{
   printf("test-plugin-fd: endRestoreFile\n");
   return bRC_OK;
}

/*
 * This is called during restore to create the file (if necessary)
 * We must return in rp->create_status:
 *   
 *  CF_ERROR    -- error
 *  CF_SKIP     -- skip processing this file
 *  CF_EXTRACT  -- extract the file (i.e.call i/o routines)
 *  CF_CREATED  -- created, but no content to extract (typically directories)
 *
 */
static bRC createFile(bpContext *ctx, struct restore_pkt *rp)
{
   printf("test-plugin-fd: createFile\n");
   if (strlen(rp->where) > 512) {
      printf("Restore target dir too long. Restricting to first 512 bytes.\n");
   }
   strncpy(((struct plugin_ctx *)ctx->pContext)->where, rp->where, 513);
   ((struct plugin_ctx *)ctx->pContext)->replace = rp->replace;
   rp->create_status = CF_EXTRACT;
   return bRC_OK;
}

/*
 * We will get here if the File is a directory after everything
 * is written in the directory.
 */
static bRC setFileAttributes(bpContext *ctx, struct restore_pkt *rp)
{
   printf("test-plugin-fd: setFileAttributes\n");
   return bRC_OK;
}


#ifdef __cplusplus
}
#endif
