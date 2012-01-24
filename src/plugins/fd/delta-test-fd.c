/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2011 Free Software Foundation Europe e.V.

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
 * A simple delta plugin for the Bacula File Daemon
 *
 *
 */
#include "bacula.h"
#include "fd_plugins.h"
#include "fd_common.h"

#undef malloc
#undef free
#undef strdup

#ifdef __cplusplus
extern "C" {
#endif

static const int dbglvl = 0;

#define PLUGIN_LICENSE      "Bacula AGPLv3"
#define PLUGIN_AUTHOR       "Eric Bollengier"
#define PLUGIN_DATE         "November 2010"
#define PLUGIN_VERSION      "1"
#define PLUGIN_DESCRIPTION  "Bacula Delta Test Plugin"

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
   setFileAttributes,
   NULL                         /* no checkFile */
};

#define get_self(x) ((delta_test*)((x)->pContext))
#define FO_DELTA        (1<<28)       /* Do delta on file */
#define FO_OFFSETS      (1<<30)       /* Keep block offsets */

class delta_test
{
private:
   bpContext *ctx;

public:
   POOLMEM *fname;              /* Filename to save */
   int32_t delta;
   FILE *fd;
   bool done;
   int level;

   delta_test(bpContext *bpc) { 
      fd = NULL;
      ctx = bpc;
      done = false;
      level = 0;
      delta = 0;
      fname = get_pool_memory(PM_FNAME);
   }
   ~delta_test() {
      free_and_null_pool_memory(fname);
   }
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

   /* Activate this plugin only in developer mode */
#ifdef DEVELOPER
   return bRC_OK;
#else
   return bRC_Error;
#endif
}

/*
 * External entry point to unload the plugin 
 */
bRC unloadPlugin() 
{
// Dmsg(NULL, dbglvl, "delta-test-fd: Unloaded\n");
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
   delta_test *self = new delta_test(ctx);
   if (!self) {
      return bRC_Error;
   }
   ctx->pContext = (void *)self;        /* set our context pointer */
   return bRC_OK;
}

/*
 * Free a plugin instance, i.e. release our private storage
 */
static bRC freePlugin(bpContext *ctx)
{
   delta_test *self = get_self(ctx);
   if (!self) {
      return bRC_Error;
   }
   delete self;
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
   delta_test *self = get_self(ctx);
   int accurate=0;

   if (!self) {
      return bRC_Error;
   }

// char *name;

   /*
    * Most events don't interest us so we ignore them.
    *   the printfs are so that plugin writers can enable them to see
    *   what is really going on.
    */
   switch (event->eventType) {
   case bEventPluginCommand:
//    Dmsg(ctx, dbglvl, 
//         "delta-test-fd: PluginCommand=%s\n", (char *)value);
      break;
   case bEventJobStart:
//    Dmsg(ctx, dbglvl, "delta-test-fd: JobStart=%s\n", (char *)value);
      break;
   case bEventJobEnd:
//    Dmsg(ctx, dbglvl, "delta-test-fd: JobEnd\n");
      break;
   case bEventStartBackupJob:
//    Dmsg(ctx, dbglvl, "delta-test-fd: StartBackupJob\n");
      break;
   case bEventEndBackupJob:
//    Dmsg(ctx, dbglvl, "delta-test-fd: EndBackupJob\n");
      break;
   case bEventLevel:
//    Dmsg(ctx, dbglvl, "delta-test-fd: JobLevel=%c %d\n", (int)value, (int)value);
      self->level = (int)(intptr_t)value;
      break;
   case bEventSince:
//    Dmsg(ctx, dbglvl, "delta-test-fd: since=%d\n", (int)value);
      break;

   case bEventStartRestoreJob:
//    Dmsg(ctx, dbglvl, "delta-test-fd: StartRestoreJob\n");
      break;

   case bEventEndRestoreJob:
//    Dmsg(ctx, dbglvl, "delta-test-fd: EndRestoreJob\n");
      break;

   /* Plugin command e.g. plugin = <plugin-name>:<name-space>:read command:write command */
   case bEventRestoreCommand:
//    Dmsg(ctx, dbglvl, "delta-test-fd: EventRestoreCommand cmd=%s\n", (char *)value);
      /* Fall-through wanted */
      break;
   case bEventBackupCommand:
      Dmsg(ctx, dbglvl, "delta-test-fd: pluginEvent cmd=%s\n", (char *)value);
      if (self->level == 'I' || self->level == 'D') {
         bfuncs->getBaculaValue(ctx, bVarAccurate, (void *)&accurate);
         if (!accurate) {       /* can be changed to FATAL */
            Jmsg(ctx, M_FATAL, 
                 "Accurate mode should be turned on when using the "
                 "delta-test plugin\n");
            return bRC_Error;
         }
      }
      break;

   default:
//    Dmsg(ctx, dbglvl, "delta-test-fd: unknown event=%d\n", event->eventType);
      break;
   }
   return bRC_OK;
}

static const char *files[] = {
   "/etc/passwd",
   "/etc/group",
   "/etc/hosts",
   "/etc/services"
};
static int nb_files = 4;

/* 
 * Start the backup of a specific file
 */
static bRC startBackupFile(bpContext *ctx, struct save_pkt *sp)
{
   delta_test *self = get_self(ctx);
   if (!self) {
      return bRC_Error;
   }
   time_t now = time(NULL);
   sp->fname = (char *)"/delta.txt";
   sp->type = FT_REG;
   sp->statp.st_mode = 0700 | S_IFREG;
   sp->statp.st_ctime = now;
   sp->statp.st_mtime = now;
   sp->statp.st_atime = now;
   sp->statp.st_size = -1;
   sp->statp.st_blksize = 4096;
   sp->statp.st_blocks = 1;
   if (self->level == 'I' || self->level == 'D') {
      bRC state = bfuncs->checkChanges(ctx, sp);
      /* Should always be bRC_OK */
      sp->type = (state == bRC_Seen)? FT_NOCHG : FT_REG;
      sp->flags |= (FO_DELTA|FO_OFFSETS);
      self->delta = sp->delta_seq + 1;
   }
   pm_strcpy(self->fname, files[self->delta % nb_files]);
   Dmsg(ctx, dbglvl, "delta-test-fd: delta_seq=%i delta=%i fname=%s\n", 
        sp->delta_seq, self->delta, self->fname);
// Dmsg(ctx, dbglvl, "delta-test-fd: startBackupFile\n");
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
   delta_test *self = get_self(ctx);
   struct stat statp;
   if (!self) {
      return bRC_Error;
   }
    
   io->status = 0;
   io->io_errno = 0;
   switch(io->func) {
   case IO_OPEN:
      Dmsg(ctx, dbglvl, "delta-test-fd: IO_OPEN\n");
      if (io->flags & (O_CREAT | O_WRONLY)) {
         /* TODO: if the file already exists, the result is undefined */
         if (stat(io->fname, &statp) == 0) { /* file exists */
            self->fd = fopen(io->fname, "r+");
         } else {
            self->fd = fopen(io->fname, "w"); /* file doesn't exist,create it */
         }
         if (!self->fd) {
            io->io_errno = errno;
            Jmsg(ctx, M_FATAL, 
                 "Open failed: ERR=%s\n", strerror(errno));
            return bRC_Error;
         }

      } else {
         self->fd = fopen(self->fname, "r");
         if (!self->fd) {
            io->io_errno = errno;
            Jmsg(ctx, M_FATAL, 
               "Open failed: ERR=%s\n", strerror(errno));
            return bRC_Error;
         }
      }
      break;

   case IO_READ:
      if (!self->fd) {
         Jmsg(ctx, M_FATAL, "Logic error: NULL read FD\n");
         return bRC_Error;
      }
      if (self->done) {
         io->status = 0;
      } else {
         /* first time, read 300, then replace 50-250 by other data */
         if (self->delta == 0) {
            io->status = fread(io->buf, 1, 400, self->fd);            
         } else {
            io->offset = self->delta * 100 / 2; /* chunks are melted */
            io->status = fread(io->buf, 1, 100, self->fd);
         }
         Dmsg(ctx, dbglvl, "delta-test-fd: READ offset=%lld\n", (int64_t)io->offset);
         self->done = true;
      }
      if (io->status == 0 && ferror(self->fd)) {
         Jmsg(ctx, M_FATAL, 
            "Pipe read error: ERR=%s\n", strerror(errno));
         Dmsg(ctx, dbglvl, 
            "Pipe read error: ERR=%s\n", strerror(errno));
         return bRC_Error;
      }
      Dmsg(ctx, dbglvl, "offset=%d\n", io->offset);
      break;

   case IO_WRITE:
      if (!self->fd) {
         Jmsg(ctx, M_FATAL, "Logic error: NULL write FD\n");
         return bRC_Error;
      }
      Dmsg(ctx, dbglvl, "delta-test-fd: WRITE count=%lld\n", (int64_t)io->count);
      io->status = fwrite(io->buf, 1, io->count, self->fd);
      if (io->status == 0 && ferror(self->fd)) {
         Jmsg(ctx, M_FATAL, 
            "Pipe write error\n");
         Dmsg(ctx, dbglvl, 
            "Pipe read error: ERR=%s\n", strerror(errno));
         return bRC_Error;
      }
      break;

   case IO_CLOSE:
      if (!self->fd) {
         Jmsg(ctx, M_FATAL, "Logic error: NULL FD on delta close\n");
         return bRC_Error;
      }
      io->status = fclose(self->fd);
      break;

   case IO_SEEK:
      if (!self->fd) {
         Jmsg(ctx, M_FATAL, "Logic error: NULL FD on delta close\n");
         return bRC_Error;
      }
      Dmsg(ctx, dbglvl, "delta-test-fd: SEEK offset=%lld\n", (int64_t)io->offset);
      io->status = fseek(self->fd, io->offset, io->whence);
      Dmsg(ctx, dbglvl, "after SEEK=%lld\n", (int64_t)ftell(self->fd));
      break;
   }
   return bRC_OK;
}

/*
 * Bacula is notifying us that a plugin name string was found, and
 *   passing us the plugin command, so we can prepare for a restore.
 */
static bRC startRestoreFile(bpContext *ctx, const char *cmd)
{
// Dmsg(ctx, dbglvl, "delta-test-fd: startRestoreFile cmd=%s\n", cmd);
   return bRC_OK;
}

/*
 * Bacula is notifying us that the plugin data has terminated, so
 *  the restore for this particular file is done.
 */
static bRC endRestoreFile(bpContext *ctx)
{
// Dmsg(ctx, dbglvl, "delta-test-fd: endRestoreFile\n");
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
   delta_test *self = get_self(ctx);
   pm_strcpy(self->fname, rp->ofname);
   rp->create_status = CF_EXTRACT;
   return bRC_OK;
}

/*
 * We will get here if the File is a directory after everything
 * is written in the directory.
 */
static bRC setFileAttributes(bpContext *ctx, struct restore_pkt *rp)
{
// Dmsg(ctx, dbglvl, "delta-test-fd: setFileAttributes\n");
   return bRC_OK;
}

#ifdef __cplusplus
}
#endif
