/*

   Copyright (C) 2007-2008 Kern Sibbald

   You may freely use this code to create your own plugin provided
   it is to write a plugin for Bacula licensed under GPLv2
   (as Bacula is), and in that case, you may also remove
   the above Copyright and this notice as well as modify 
   the code in any way. 

*/
#include "bacula.h"
#include "fd_plugins.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_LICENSE      "GPL"
#define PLUGIN_AUTHOR       "Your name"
#define PLUGIN_DATE         "January 2008"
#define PLUGIN_VERSION      "1"
#define PLUGIN_DESCRIPTION  "Test File Daemon Plugin"

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

bRC loadPlugin(bInfo *lbinfo, bFuncs *lbfuncs, pInfo **pinfo, pFuncs **pfuncs)
{
   bfuncs = lbfuncs;                  /* set Bacula funct pointers */
   binfo  = lbinfo;
   printf("plugin: Loaded: size=%d version=%d\n", bfuncs->size, bfuncs->version);

   *pinfo  = &pluginInfo;             /* return pointer to our info */
   *pfuncs = &pluginFuncs;            /* return pointer to our functions */

   return bRC_OK;
}

bRC unloadPlugin() 
{
   printf("plugin: Unloaded\n");
   return bRC_OK;
}

static bRC newPlugin(bpContext *ctx)
{
   int JobId = 0;
   bfuncs->getBaculaValue(ctx, bVarJobId, (void *)&JobId);
// printf("plugin: newPlugin JobId=%d\n", JobId);
   bfuncs->registerBaculaEvents(ctx, 1, 2, 0);
   return bRC_OK;
}

static bRC freePlugin(bpContext *ctx)
{
   int JobId = 0;
   bfuncs->getBaculaValue(ctx, bVarJobId, (void *)&JobId);
// printf("plugin: freePlugin JobId=%d\n", JobId);
   return bRC_OK;
}

static bRC getPluginValue(bpContext *ctx, pVariable var, void *value) 
{
// printf("plugin: getPluginValue var=%d\n", var);
   return bRC_OK;
}

static bRC setPluginValue(bpContext *ctx, pVariable var, void *value) 
{
// printf("plugin: setPluginValue var=%d\n", var);
   return bRC_OK;
}

static bRC handlePluginEvent(bpContext *ctx, bEvent *event, void *value)
{
   char *name;

   switch (event->eventType) {
   case bEventJobStart:
      printf("plugin: JobStart=%s\n", NPRT((char *)value));
      break;
   case bEventJobEnd:
      printf("plugin: JobEnd\n");
      break;
   case bEventStartBackupJob:
      printf("plugin: BackupStart\n");
      break;
   case bEventEndBackupJob:
      printf("plugin: BackupEnd\n");
      break;
   case bEventLevel:
      printf("plugin: JobLevel=%c %d\n", (int64_t)value, (int64_t)value);
      break;
   case bEventSince:
      printf("plugin: since=%d\n", (int64_t)value);
      break;
   case bEventStartRestoreJob:
      printf("plugin: StartRestoreJob\n");
      break;
   case bEventEndRestoreJob:
      printf("plugin: EndRestoreJob\n");
      break;

   /* Plugin command e.g. plugin = <plugin-name>:<name-space>:command */
   case bEventRestoreCommand:
      printf("plugin: backup command=%s\n", NPRT((char *)value));
      break;

   case bEventBackupCommand:
      printf("plugin: backup command=%s\n", NPRT((char *)value));
      break;

   default:
      printf("plugin: unknown event=%d\n", event->eventType);
   }
   bfuncs->getBaculaValue(ctx, bVarFDName, (void *)&name);
// printf("FD Name=%s\n", name);
// bfuncs->JobMessage(ctx, __FILE__, __LINE__, 1, 0, "JobMesssage message");
// bfuncs->DebugMessage(ctx, __FILE__, __LINE__, 1, "DebugMesssage message");
   return bRC_OK;
}

static bRC startBackupFile(bpContext *ctx, struct save_pkt *sp)
{
   return bRC_OK;
}

static bRC endBackupFile(bpContext *ctx)
{ 
   return bRC_OK;
}

/*
 * Do actual I/O
 */
static bRC pluginIO(bpContext *ctx, struct io_pkt *io)
{
   io->status = 0;
   io->io_errno = 0;
   switch(io->func) {
   case IO_OPEN:
      printf("plugin: IO_OPEN\n");
      break;
   case IO_READ:
      printf("plugin: IO_READ buf=%p len=%d\n", io->buf, io->count);
      break;
   case IO_WRITE:
      printf("plugin: IO_WRITE buf=%p len=%d\n", io->buf, io->count);
      break;
   case IO_CLOSE:
      printf("plugin: IO_CLOSE\n");
      break;
   }
   return bRC_OK;
}

static bRC startRestoreFile(bpContext *ctx, const char *cmd)
{
   return bRC_OK;
}

static bRC endRestoreFile(bpContext *ctx)
{
   return bRC_OK;
}

static bRC createFile(bpContext *ctx, struct restore_pkt *rp)
{
   return bRC_OK;
}

static bRC setFileAttributes(bpContext *ctx, struct restore_pkt *rp)
{
   return bRC_OK;
}


#ifdef __cplusplus
}
#endif
