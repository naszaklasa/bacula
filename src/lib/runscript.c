/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2006-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

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
/*
 * Manipulation routines for RunScript list
 *
 *  Eric Bollengier, May 2006
 *
 *  Version $Id: runscript.c 5222 2007-07-22 12:21:06Z kerns $
 *
 */


#include "bacula.h"
#include "jcr.h"

#include "runscript.h"

RUNSCRIPT *new_runscript()
{
   Dmsg0(500, "runscript: creating new RUNSCRIPT object\n");
   RUNSCRIPT *cmd = (RUNSCRIPT *)malloc(sizeof(RUNSCRIPT));
   memset(cmd, 0, sizeof(RUNSCRIPT));
   cmd->reset_default();
   
   return cmd;
}

void RUNSCRIPT::reset_default(bool free_strings)
{
   if (free_strings && command) {
     free_pool_memory(command);
   }
   if (free_strings && target) {
     free_pool_memory(target);
   }
   
   target = NULL;
   command = NULL;
   on_success = true;
   on_failure = false;
   fail_on_error = true;
   when = SCRIPT_Never;
   old_proto = false;        /* TODO: drop this with bacula 1.42 */
}

RUNSCRIPT *copy_runscript(RUNSCRIPT *src)
{
   Dmsg0(500, "runscript: creating new RUNSCRIPT object from other\n");

   RUNSCRIPT *dst = (RUNSCRIPT *)malloc(sizeof(RUNSCRIPT));
   memcpy(dst, src, sizeof(RUNSCRIPT));

   dst->command = NULL;
   dst->target = NULL;

   dst->set_command(src->command);
   dst->set_target(src->target);

   return dst;   
}

void free_runscript(RUNSCRIPT *script)
{
   Dmsg0(500, "runscript: freeing RUNSCRIPT object\n");

   if (script->command) {
      free_pool_memory(script->command);
   }
   if (script->target) {
      free_pool_memory(script->target);
   }
   free(script);
}

int run_scripts(JCR *jcr, alist *runscripts, const char *label)
{
   Dmsg2(200, "runscript: running all RUNSCRIPT object (%s) JobStatus=%c\n", label, jcr->JobStatus);
   
   RUNSCRIPT *script;
   bool runit;

   int when;

   if (strstr(label, NT_("Before"))) {
      when = SCRIPT_Before;
   } else {
      when = SCRIPT_After;
   }

   if (runscripts == NULL) {
      Dmsg0(100, "runscript: WARNING RUNSCRIPTS list is NULL\n");
      return 0;
   }

   foreach_alist(script, runscripts) {
      Dmsg2(200, "runscript: try to run %s:%s\n", NPRT(script->target), NPRT(script->command));
      runit = false;

      if ((script->when & SCRIPT_Before) && (when & SCRIPT_Before)) {
         if ((script->on_success 
            && (jcr->JobStatus == JS_Running || jcr->JobStatus == JS_Created))
            || (script->on_failure && job_canceled(jcr))
            )
         {
            Dmsg4(200, "runscript: Run it because SCRIPT_Before (%s,%i,%i,%c)\n", 
                  script->command, script->on_success, script->on_failure,
                  jcr->JobStatus );
            runit = true;
         }
      }

      if ((script->when & SCRIPT_After) && (when & SCRIPT_After)) {
         if ((script->on_success && (jcr->JobStatus == JS_Terminated))
             || (script->on_failure && job_canceled(jcr))
            )
         {
            Dmsg4(200, "runscript: Run it because SCRIPT_After (%s,%i,%i,%c)\n", 
                  script->command, script->on_success, script->on_failure,
                  jcr->JobStatus );
            runit = true;
         }
      }

      if (!script->is_local()) {
         runit = false;
      }

      /* we execute it */
      if (runit) {
         script->run(jcr, label);
      }
   }
   return 1;
}

bool RUNSCRIPT::is_local()
{
   if (!target || (strcmp(target, "") == 0)) {
      return true;
   } else {
      return false;
   }
}

/* set this->command to cmd */
void RUNSCRIPT::set_command(const POOLMEM *cmd)
{
   Dmsg1(500, "runscript: setting command = %s\n", NPRT(cmd));

   if (!cmd) {
      return;
   }

   if (!command) {
      command = get_pool_memory(PM_FNAME);
   }

   pm_strcpy(command, cmd);
}

/* set this->target to client_name */
void RUNSCRIPT::set_target(const POOLMEM *client_name)
{
   Dmsg1(500, "runscript: setting target = %s\n", NPRT(client_name));

   if (!client_name) {
      return;
   }

   if (!target) {
      target = get_pool_memory(PM_FNAME);
   }

   pm_strcpy(target, client_name);
}

bool RUNSCRIPT::run(JCR *jcr, const char *name)
{
   Dmsg0(200, "runscript: running a RUNSCRIPT object\n");
   POOLMEM *ecmd = get_pool_memory(PM_FNAME);
   int status;
   BPIPE *bpipe;
   char line[MAXSTRING];

   ecmd = edit_job_codes(jcr, ecmd, this->command, "");
   Dmsg1(100, "runscript: running '%s'...\n", ecmd);
   Jmsg(jcr, M_INFO, 0, _("%s: run command \"%s\"\n"), name, ecmd);

   bpipe = open_bpipe(ecmd, 0, "r");
   free_pool_memory(ecmd);
   if (bpipe == NULL) {
      berrno be;
      Jmsg(jcr, M_ERROR, 0, _("Runscript: %s could not execute. ERR=%s\n"), name,
         be.bstrerror());
      goto bail_out;
   }
   while (fgets(line, sizeof(line), bpipe->rfd)) {
      int len = strlen(line);
      if (len > 0 && line[len-1] == '\n') {
         line[len-1] = 0;
      }
      Jmsg(jcr, M_INFO, 0, _("%s: %s\n"), name, line);
   }
   status = close_bpipe(bpipe);
   if (status != 0) {
      berrno be;
      Jmsg(jcr, M_ERROR, 0, _("Runscript: %s returned non-zero status=%d. ERR=%s\n"), name,
         be.code(status), be.bstrerror(status));
      goto bail_out;
   }
   Dmsg0(100, "runscript OK\n");
   return true;

bail_out:
   /* cancel running job properly */
   if (fail_on_error) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }
   Dmsg1(100, "runscript failed. fail_on_error=%d\n", fail_on_error);
   return false;
}

void free_runscripts(alist *runscripts)
{
   Dmsg0(500, "runscript: freeing all RUNSCRIPTS object\n");

   RUNSCRIPT *elt;
   foreach_alist(elt, runscripts) {
      free_runscript(elt);
   }
}

void RUNSCRIPT::debug()
{
   Dmsg0(200, "runscript: debug\n");
   Dmsg0(200,  _(" --> RunScript\n"));
   Dmsg1(200,  _("  --> Command=%s\n"), NPRT(command));
   Dmsg1(200,  _("  --> Target=%s\n"),  NPRT(target));
   Dmsg1(200,  _("  --> RunOnSuccess=%u\n"),  on_success);
   Dmsg1(200,  _("  --> RunOnFailure=%u\n"),  on_failure);
   Dmsg1(200,  _("  --> FailJobOnError=%u\n"),  fail_on_error);
   Dmsg1(200,  _("  --> RunWhen=%u\n"),  when);
}
