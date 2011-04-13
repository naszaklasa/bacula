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
 * Common plugin definitions
 *
 * Kern Sibbald, October 2007
 */
#ifndef __PLUGINS_H
#define __PLUGINS_H

/****************************************************************************
 *                                                                          *
 *                Common definitions for all plugins                        *
 *                                                                          *
 ****************************************************************************/

#ifndef BUILD_PLUGIN
extern DLL_IMP_EXP alist *plugin_list;
#endif

/* Universal return codes from all plugin functions */
typedef enum {
  bRC_OK     = 0,                        /* OK */
  bRC_Stop   = 1,                        /* Stop calling other plugins */
  bRC_Error  = 2,                        /* Some kind of error */
  bRC_More   = 3,                        /* More files to backup */
  bRC_Term   = 4,                        /* Unload me */
  bRC_Seen   = 5,                        /* Return code from checkFiles */

  bRC_Max    = 9999                      /* Max code Bacula can use */
} bRC;



/* Context packet as first argument of all functions */
struct bpContext {
  void *bContext;                        /* Bacula private context */
  void *pContext;                        /* Plugin private context */
};

extern "C" {
typedef bRC (*t_loadPlugin)(void *binfo, void *bfuncs, void **pinfo, void **pfuncs);
typedef bRC (*t_unloadPlugin)(void);
}

class Plugin {
public:
   char *file;
   t_unloadPlugin unloadPlugin;
   void *pinfo;
   void *pfuncs;
   void *pHandle;
   bool disabled;
};

/* Functions */
extern Plugin *new_plugin();
extern bool load_plugins(void *binfo, void *bfuncs, const char *plugin_dir, 
        const char *type, bool is_plugin_compatible(Plugin *plugin));
extern void unload_plugins();

/* Each daemon can register a debug hook that will be called
 * after a fatal signal
 */
typedef void (dbg_plugin_hook_t)(Plugin *plug, FILE *fp);
extern void dbg_plugin_add_hook(dbg_plugin_hook_t *fct);

#endif /* __PLUGINS_H */
