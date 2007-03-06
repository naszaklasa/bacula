/*
 * Bacula Catalog Database Find record interface routines
 *
 *  Note, generally, these routines are more complicated
 *        that a simple search by name or id. Such simple
 *        request are in get.c
 *
 * Bacula Catalog Database routines written specifically
 *  for Bacula.  Note, these routines are VERY dumb and
 *  do not provide all the functionality of an SQL database.
 *  The purpose of these routines is to ensure that Bacula
 *  can limp along if no real database is loaded on the
 *  system.
 *
 *    Kern Sibbald, January MMI
 *
 *    Version $Id: bdb_find.c,v 1.19 2006/11/27 10:02:58 kerns Exp $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

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

/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"
#include "bdb.h"

#ifdef HAVE_BACULA_DB

/* Forward referenced functions */

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


/*
 * Find job start time. Used to find last full save that terminated normally
 * so we can do Incremental and Differential saves.
 *
 * Returns: 0 on failure
 *          1 on success, jr unchanged, but stime set
 */
bool db_find_job_start_time(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM **stime)
{
   char cmd[MAXSTRING], Name[MAX_NAME_LENGTH], StartTime[MAXSTRING];
   int Type, Level;
   uint32_t JobId, EndId, ClientId;
   char cType[10], cLevel[10], JobStatus[10];
   bool found = false;
   long addr;

   db_lock(mdb);
   pm_strcpy(stime, "0000-00-00 00:00:00");   /* default */
   if (!bdb_open_jobs_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->jobfd, 0L, SEEK_SET);   /* rewind file */
   /* Linear search through JobStart records
    */

   while (fgets(cmd, sizeof(cmd), mdb->jobfd)) {
      if (sscanf(cmd, "JobStart JobId=%d Name=%127s Type=%1s Level=%1s "
"StartTime=%100s", &JobId, Name, cType, cLevel, StartTime) == 5) {
         if (JobId < jr->JobId) {
            continue;                 /* older not a candidate */
         }
         Type = cType[0];
         Level = cLevel[0];
         unbash_spaces(Name);
         unbash_spaces(StartTime);
         Dmsg4(200, "Got Type=%c Level=%c Name=%s StartTime=%s\n",
            Type, Level, Name, StartTime);
         Dmsg3(200, "Want Type=%c Level=%c Name=%s\n", jr->JobType, jr->JobLevel,
            jr->Name);
         /* Differential is since last Full backup */
         /* Incremental is since last FULL or Incremental or Differential */
         if (((jr->JobLevel == L_DIFFERENTIAL) && (Type == jr->JobType &&
               Level == L_FULL && strcmp(Name, jr->Name) == 0)) ||
             ((jr->JobLevel == L_INCREMENTAL) && (Type == jr->JobType &&
               (Level == L_FULL || Level == L_INCREMENTAL ||
                Level == L_DIFFERENTIAL) && strcmp(Name, jr->Name) == 0))) {
            addr = ftell(mdb->jobfd);    /* save current location */
            JobStatus[0] = 0;
            found = false;
            /* Search for matching JobEnd record */
            while (!found && fgets(cmd, sizeof(cmd), mdb->jobfd)) {
               if (sscanf(cmd, "JobEnd JobId=%d JobStatus=%1s ClientId=%d",
                  &EndId, JobStatus, &ClientId) == 3) {
                  if (EndId == JobId && *JobStatus == 'T' && ClientId == jr->ClientId) {
                     Dmsg0(200, "====found EndJob matching Job\n");
                     found = true;
                     break;
                  }
               }
            }
            /* Reset for next read */
            fseek(mdb->jobfd, addr, SEEK_SET);
            if (found) {
               pm_strcpy(stime, StartTime);
               Dmsg5(200, "Got candidate JobId=%d Type=%c Level=%c Name=%s StartTime=%s\n",
                  JobId, Type, Level, Name, StartTime);
            }
         }
      }
   }
   db_unlock(mdb);
   return found;
}


/*
 * Find Available Media (Volume) for Pool
 *
 * Find a Volume for a given PoolId, MediaType, and VolStatus
 *
 *   Note! this does not correctly implement InChanger.
 *
 * Returns: 0 on failure
 *          numrows on success
 */
int
db_find_next_volume(JCR *jcr, B_DB *mdb, int item, bool InChanger, MEDIA_DBR *mr)
{
   MEDIA_DBR omr;
   int stat = 0;
   int index = 0;
   int len;

   db_lock(mdb);
   if (!bdb_open_media_file(mdb)) {
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->mediafd, 0L, SEEK_SET);   /* rewind file */
   len = sizeof(omr);
   while (fread(&omr, len, 1, mdb->mediafd) > 0) {
      if (mr->PoolId == omr.PoolId && strcmp(mr->VolStatus, omr.VolStatus) == 0 &&
          strcmp(mr->MediaType, omr.MediaType) == 0) {
         if (!(++index == item)) {    /* looking for item'th entry */
            Dmsg0(200, "Media record matches, but not index\n");
            continue;
         }
         Dmsg0(200, "Media record matches\n");
         memcpy(mr, &omr, len);
         Dmsg1(200, "Findnextvol MediaId=%d\n", mr->MediaId);
         stat = 1;
         break;                       /* found it */
      }
   }
   db_unlock(mdb);
   return stat;
}

bool
db_find_last_jobid(JCR *jcr, B_DB *mdb, const char *Name, JOB_DBR *jr)
{ return false; }

bool
db_find_failed_job_since(JCR *jcr, B_DB *mdb, JOB_DBR *jr, POOLMEM *stime, int &JobLevel)
{ return false; }


#endif /* HAVE_BACULA_DB */
