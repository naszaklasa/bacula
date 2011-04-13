/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2007 Free Software Foundation Europe e.V.

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
 * Sample Plugin program
 *
 *  Kern Sibbald, October 2007
 *
 */
#include <stdio.h>
#include "plugin-sd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_LICENSE      "GPL"
#define PLUGIN_AUTHOR       "Kern Sibbald"
#define PLUGIN_DATE         "November 2007"
#define PLUGIN_VERSION      "1"
#define PLUGIN_DESCRIPTION  "Test Storage Daemon Plugin"

/* Forward referenced functions */
static bpError newPlugin(bpContext *ctx);
static bpError freePlugin(bpContext *ctx);
static bpError getPluginValue(bpContext *ctx, pVariable var, void *value);
static bpError setPluginValue(bpContext *ctx, pVariable var, void *value);
static bpError handlePluginEvent(bpContext *ctx, bEvent *event);


/* Pointers to Bacula functions */
static bFuncs *bfuncs = NULL;

static pFuncs pluginFuncs = {
   sizeof(pluginFuncs),
   PLUGIN_INTERFACE,
   PLUGIN_MAGIC,
   PLUGIN_LICENSE,
   PLUGIN_AUTHOR,
   PLUGIN_DATE,
   PLUGIN_VERSION,
   PLUGIN_DESCRIPTION,

   /* Entry points into plugin */
   newPlugin,                         /* new plugin instance */
   freePlugin,                        /* free plugin instance */
   getPluginValue,
   setPluginValue,
   handlePluginEvent
};

bpError loadPlugin(bFuncs *lbfuncs, pFuncs **pfuncs) 
{
   bfuncs = lbfuncs;                  /* set Bacula funct pointers */
   printf("plugin: Loaded: size=%d version=%d\n", bfuncs->size, bfuncs->interface);

   *pfuncs = &pluginFuncs;            /* return pointer to our functions */

   return 0;
}

bpError unloadPlugin() 
{
   printf("plugin: Unloaded\n");
   return 0;
}

static bpError newPlugin(bpContext *ctx)
{
   int JobId = 0;
   bfuncs->getBaculaValue(ctx, bVarJobId, (void *)&JobId);
   printf("plugin: newPlugin JobId=%d\n", JobId);
   return 0;
}

static bpError freePlugin(bpContext *ctx)
{
   int JobId = 0;
   bfuncs->getBaculaValue(ctx, bVarJobId, (void *)&JobId);
   printf("plugin: freePlugin JobId=%d\n", JobId);
   return 0;
}

static bpError getPluginValue(bpContext *ctx, pVariable var, void *value) 
{
   printf("plugin: getPluginValue var=%d\n", var);
   return 0;
}

static bpError setPluginValue(bpContext *ctx, pVariable var, void *value) 
{
   printf("plugin: setPluginValue var=%d\n", var);
   return 0;
}

static bpError handlePluginEvent(bpContext *ctx, bEvent *event) 
{
   printf("plugin: HandleEvent Event=%d\n", event->eventType);
   return 0;
}

#ifdef __cplusplus
}
#endif
