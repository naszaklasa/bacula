/*
 *
 *   Bacula Director -- admin.c -- responsible for doing admin jobs
 *
 *     Kern Sibbald, May MMIII
 *
 *  Basic tasks done here:
 *     Display the job report.
 *
 *   Version $Id: admin.c,v 1.7.2.1 2005/02/14 10:02:19 kerns Exp $
 */

/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "dird.h"
#include "ua.h"


/* Forward referenced functions */
static void admin_cleanup(JCR *jcr, int TermCode);

/* External functions */

/*
 *  Returns:  0 on failure
 *	      1 on success
 */
int do_admin(JCR *jcr)
{

   jcr->jr.JobId = jcr->JobId;

   jcr->fname = (char *)get_pool_memory(PM_FNAME);

   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Admin JobId %d, Job=%s\n"),
	jcr->JobId, jcr->Job);

   set_jcr_job_status(jcr, JS_Running);
   admin_cleanup(jcr, JS_Terminated);
   return 0;
}

/*
 * Release resources allocated during backup.
 */
static void admin_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50];
   char term_code[100];
   const char *term_msg;
   int msg_type;
   MEDIA_DBR mr;

   Dmsg0(100, "Enter backup_cleanup()\n");
   memset(&mr, 0, sizeof(mr));
   set_jcr_job_status(jcr, TermCode);

   update_job_end_record(jcr);	      /* update database */

   if (!db_get_job_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error getting job record for stats: %s"),
	 db_strerror(jcr->db));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
   }

   msg_type = M_INFO;		      /* by default INFO message */
   switch (jcr->JobStatus) {
   case JS_Terminated:
      term_msg = _("Admin OK");
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** Admin Error ***");
      msg_type = M_ERROR;	   /* Generate error message */
      break;
   case JS_Canceled:
      term_msg = _("Admin Canceled");
      break;
   default:
      term_msg = term_code;
      sprintf(term_code, _("Inappropriate term code: %c\n"), jcr->JobStatus);
      break;
   }
   bstrftime(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftime(edt, sizeof(edt), jcr->jr.EndTime);

   Jmsg(jcr, msg_type, 0, _("Bacula " VERSION " (" LSMDATE "): %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Termination:            %s\n\n"),
	edt,
	jcr->jr.JobId,
	jcr->jr.Job,
	sdt,
	edt,
	term_msg);

   Dmsg0(100, "Leave admin_cleanup()\n");
}
