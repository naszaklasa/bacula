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
 
#ifndef __SD_PLUGINS_H 
#define __SD_PLUGINS_H

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

/* Bacula Variable Ids */
typedef enum {
  bVarJob       = 1,
  bVarLevel     = 2,
  bVarType      = 3,
  bVarJobId     = 4,
  bVarClient    = 5,
  bVarNumVols   = 6,
  bVarPool      = 7,
  bVarStorage   = 8,
  bVarCatalog   = 9,
  bVarMediaType = 10,
  bVarJobName   = 11,
  bVarJobStatus = 12,
  bVarPriority  = 13,
  bVarVolumeName = 14,
  bVarCatalogRes = 15,
  bVarJobErrors  = 16,
  bVarJobFiles   = 17,
  bVarSDJobFiles = 18,
  bVarSDErrors   = 19,
  bVarFDJobStatus = 20,
  bVarSDJobStatus = 21
} brVariable;

typedef enum {
  bwVarJobReport  = 1,
  bwVarVolumeName = 2,
  bwVarPriority   = 3,
  bwVarJobLevel   = 4
} bwVariable;


typedef enum {
  bEventJobStart      = 1,
  bEventJobEnd        = 2
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
       int type, utime_t mtime, const char *msg);     
   bRC (*DebugMessage)(bpContext *ctx, const char *file, int line,
       int level, const char *msg);
} bFuncs;

/* Bacula Subroutines */
void load_sd_plugins(const char *plugin_dir);
void new_plugins(JCR *jcr);
void free_plugins(JCR *jcr);
void generate_plugin_event(JCR *jcr, bEventType event, void *value=NULL);



/****************************************************************************
 *                                                                          *
 *                Plugin definitions                                        *
 *                                                                          *
 ****************************************************************************/

typedef enum {
  pVarName = 1,
  pVarDescription = 2
} pVariable;


#define SD_PLUGIN_MAGIC     "*DirPluginData*" 
#define SD_PLUGIN_INTERFACE_VERSION  1

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

#endif /* __SD_PLUGINS_H */
