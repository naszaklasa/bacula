/*
 *
 *   Bacula authentication. Provides authentication with
 *     File and Storage daemons.
 *
 *     Nicolas Boichat, August MMIV
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *
 */
/*
   Copyright (C) 2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.
 */

#include "bacula.h"
#include "tray-monitor.h"
#include "jcr.h"

void senditf(const char *fmt, ...);
void sendit(const char *buf);

/* Commands sent to Director */
static char DIRhello[]    = "Hello %s calling\n";

/* Response from Director */
static char DIROKhello[]   = "1000 OK:";

/* Commands sent to Storage daemon and File daemon and received
 *  from the User Agent */
static char SDFDhello[]    = "Hello Director %s calling\n";

/* Response from SD */
static char SDOKhello[]   = "3000 OK Hello\n";
/* Response from FD */
static char FDOKhello[] = "2000 OK Hello\n";

/* Forward referenced functions */

/*
 * Authenticate Director
 */
int authenticate_director(JCR *jcr, MONITOR *mon, DIRRES *director)
{
   BSOCK *dir = jcr->dir_bsock;
   int ssl_need = BNET_SSL_NONE;
   char bashed_name[MAX_NAME_LENGTH];
   char *password;

   bstrncpy(bashed_name, mon->hdr.name, sizeof(bashed_name));
   bash_spaces(bashed_name);
   password = mon->password;

   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(dir, 60 * 5);
   bnet_fsend(dir, DIRhello, bashed_name);

   if (!cram_md5_get_auth(dir, password, ssl_need) ||
       !cram_md5_auth(dir, password, ssl_need)) {
      stop_bsock_timer(tid);
      Jmsg0(jcr, M_FATAL, 0, _("Director authorization problem.\n"
	    "Most likely the passwords do not agree.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"));
      return 0;
   }

   Dmsg1(6, ">dird: %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      stop_bsock_timer(tid);
      Jmsg1(jcr, M_FATAL, 0, _("Bad response to Hello command: ERR=%s\n"),
	 bnet_strerror(dir));
      return 0;
   }
   Dmsg1(10, "<dird: %s", dir->msg);
   stop_bsock_timer(tid);
   if (strncmp(dir->msg, DIROKhello, sizeof(DIROKhello)-1) != 0) {
      Jmsg0(jcr, M_FATAL, 0, _("Director rejected Hello command\n"));
      return 0;
   } else {
      Jmsg0(jcr, M_FATAL, 0, dir->msg);
   }
   return 1;
}

/*
 * Authenticate Storage daemon connection
 */
int authenticate_storage_daemon(JCR *jcr, MONITOR *monitor, STORE* store)
{
   BSOCK *sd = jcr->store_bsock;
   char dirname[MAX_NAME_LENGTH];
   int ssl_need = BNET_SSL_NONE;

   /*
    * Send my name to the Storage daemon then do authentication
    */
   bstrncpy(dirname, monitor->hdr.name, sizeof(dirname));
   bash_spaces(dirname);
   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(sd, 60 * 5);
   if (!bnet_fsend(sd, SDFDhello, dirname)) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Error sending Hello to Storage daemon. ERR=%s\n"), bnet_strerror(sd));
      return 0;
   }
   if (!cram_md5_get_auth(sd, store->password, ssl_need) ||
       !cram_md5_auth(sd, store->password, ssl_need)) {
      stop_bsock_timer(tid);
      Jmsg0(jcr, M_FATAL, 0, _("Director and Storage daemon passwords or names not the same.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"));
      return 0;
   }
   Dmsg1(116, ">stored: %s", sd->msg);
   if (bnet_recv(sd) <= 0) {
      stop_bsock_timer(tid);
      Jmsg1(jcr, M_FATAL, 0, _("bdird<stored: bad response to Hello command: ERR=%s\n"),
	 bnet_strerror(sd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", sd->msg);
   stop_bsock_timer(tid);
   if (strncmp(sd->msg, SDOKhello, sizeof(SDOKhello)) != 0) {
      Jmsg0(jcr, M_FATAL, 0, _("Storage daemon rejected Hello command\n"));
      return 0;
   }
   return 1;
}

/*
 * Authenticate File daemon connection
 */
int authenticate_file_daemon(JCR *jcr, MONITOR *monitor, CLIENT* client)
{
   BSOCK *fd = jcr->file_bsock;
   char dirname[MAX_NAME_LENGTH];
   int ssl_need = BNET_SSL_NONE;

   /*
    * Send my name to the File daemon then do authentication
    */
   bstrncpy(dirname, monitor->hdr.name, sizeof(dirname));
   bash_spaces(dirname);
   /* Timeout Hello after 5 mins */
   btimer_t *tid = start_bsock_timer(fd, 60 * 5);
   if (!bnet_fsend(fd, SDFDhello, dirname)) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Error sending Hello to File daemon. ERR=%s\n"), bnet_strerror(fd));
      return 0;
   }
   if (!cram_md5_get_auth(fd, client->password, ssl_need) ||
       !cram_md5_auth(fd, client->password, ssl_need)) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Director and File daemon passwords or names not the same.\n"
       "Please see http://www.bacula.org/html-manual/faq.html#AuthorizationErrors for help.\n"));
      return 0;
   }
   Dmsg1(116, ">filed: %s", fd->msg);
   if (bnet_recv(fd) <= 0) {
      stop_bsock_timer(tid);
      Jmsg(jcr, M_FATAL, 0, _("Bad response from File daemon to Hello command: ERR=%s\n"),
	 bnet_strerror(fd));
      return 0;
   }
   Dmsg1(110, "<stored: %s", fd->msg);
   stop_bsock_timer(tid);
   if (strncmp(fd->msg, FDOKhello, sizeof(FDOKhello)) != 0) {
      Jmsg(jcr, M_FATAL, 0, _("File daemon rejected Hello command\n"));
      return 0;
   }
   return 1;
}
