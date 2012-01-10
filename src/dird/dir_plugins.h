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
 * Interface definition for Bacula Plugins
 *
 * Kern Sibbald, October 2007
 *
 */
 
#ifndef __FD_PLUGINS_H 
#define __FD_PLUGINS_H

#ifndef _BACULA_H
#ifdef __cplusplus
/* Workaround for SGI IRIX 6.5 */
#define _LANGUAGE_C_PLUS_PLUS 1
#endif
#define _REENTRANT    1
#define _THREAD_SAFE  1
#define _POSIX_PTHREAD_SEMANTICS 1
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGE_FILES 1
#endif

#include <sys/types.h>
#ifndef __CONFIG_H
#define __CONFIG_H
#include "config.h"
#endif
#include "bc_types.h"
#include "lib/plugins.h"

#ifdef __cplusplus
extern "C" {
#endif




/****************************************************************************
 *                                                                          *
 *                Bacula definitions                                        *
 *                                                                          *
 ****************************************************************************/

/* Bacula Variable Ids */       /* return value */
typedef enum {
  bVarJob       = 1,            // string
  bVarLevel     = 2,            // int   
  bVarType      = 3,            // int   
  bVarJobId     = 4,            // int   
  bVarClient    = 5,            // string
  bVarNumVols   = 6,            // int   
  bVarPool      = 7,            // string
  bVarStorage   = 8,            // string
  bVarWriteStorage = 9,         // string
  bVarReadStorage  = 10,        // string
  bVarCatalog   = 11,           // string
  bVarMediaType = 12,           // string
  bVarJobName   = 13,           // string
  bVarJobStatus = 14,           // int   
  bVarPriority  = 15,           // int   
  bVarVolumeName = 16,          // string
  bVarCatalogRes = 17,          // NYI      
  bVarJobErrors  = 18,          // int   
  bVarJobFiles   = 19,          // int   
  bVarSDJobFiles = 20,          // int   
  bVarSDErrors   = 21,          // int   
  bVarFDJobStatus = 22,         // int   
  bVarSDJobStatus = 23          // int   
} brVariable;

typedef enum {
  bwVarJobReport  = 1,
  bwVarVolumeName = 2,
  bwVarPriority   = 3,
  bwVarJobLevel   = 4
} bwVariable;


typedef enum {
  bEventJobStart      = 1,
  bEventJobEnd        = 2,
  bEventJobInit       = 3,
  bEventJobRun        = 4,
  bEventVolumePurged  = 5,
  bEventNewVolume     = 6,
  bEventNeedVolume    = 7,
  bEventVolumeFull    = 8,
  bEventRecyle        = 9,
  bEventGetScratch    = 10
} bEventType;

typedef struct s_bEvent {
   uint32_t eventType;
} bEvent;

typedef struct s_baculaInfo {
   uint32_t size;
   uint32_t version;  
} bInfo;

/* Bacula interface version and function pointers */
typedef struct s_baculaFuncs {  
   uint32_t size;
   uint32_t version;
   bRC (*registerBaculaEvents)(bpContext *ctx, ...);
   bRC (*getBaculaValue)(bpContext *ctx, brVariable var, void *value);
   bRC (*setBaculaValue)(bpContext *ctx, bwVariable var, void *value);
   bRC (*JobMessage)(bpContext *ctx, const char *file, int line, 
                     int type, utime_t mtime, const char *fmt, ...);     
   bRC (*DebugMessage)(bpContext *ctx, const char *file, int line,
                       int level, const char *fmt, ...);
} bFuncs;

/* Bacula Core Routines -- not used within a plugin */
#ifdef DIRECTOR_DAEMON
void load_dir_plugins(const char *plugin_dir);
void new_plugins(JCR *jcr);
void free_plugins(JCR *jcr);
void generate_plugin_event(JCR *jcr, bEventType event, void *value=NULL);
#endif


/****************************************************************************
 *                                                                          *
 *                Plugin definitions                                        *
 *                                                                          *
 ****************************************************************************/

typedef enum {
  pVarName = 1,
  pVarDescription = 2
} pVariable;


#define DIR_PLUGIN_MAGIC     "*DirPluginData*" 
#define DIR_PLUGIN_INTERFACE_VERSION  1

typedef struct s_pluginInfo {
   uint32_t size;
   uint32_t version;
   const char *plugin_magic;
   const char *plugin_license;
   const char *plugin_author;
   const char *plugin_date;
   const char *plugin_version;
   const char *plugin_description;
} pInfo;

typedef struct s_pluginFuncs {  
   uint32_t size;
   uint32_t version;
   bRC (*newPlugin)(bpContext *ctx);
   bRC (*freePlugin)(bpContext *ctx);
   bRC (*getPluginValue)(bpContext *ctx, pVariable var, void *value);
   bRC (*setPluginValue)(bpContext *ctx, pVariable var, void *value);
   bRC (*handlePluginEvent)(bpContext *ctx, bEvent *event, void *value);
} pFuncs;

#define plug_func(plugin) ((pFuncs *)(plugin->pfuncs))
#define plug_info(plugin) ((pInfo *)(plugin->pinfo))

#ifdef __cplusplus
}
#endif

#endif /* __FD_PLUGINS_H */
