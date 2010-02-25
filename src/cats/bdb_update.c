/*
 * Bacula Catalog Database Update record interface routines
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
 *
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2008 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of Kern Sibbald.
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

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


/*
 * This is called at Job start time to add the
 * most current start fields to the job record.
 * It is assumed that you did a db_create_job_record() already.
 */
bool db_update_job_start_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   return false;
}

/*
 * This is called at Job termination time to add all the
 * other fields to the job record.
 */
int db_update_job_end_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr, bool stats_enabled)
{
   return 0;
}


int db_update_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   return 0;
}

int db_update_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   return 0;
}

int db_add_digest_to_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, char *digest, int type)
{
   return 1;
}

int db_mark_file_record(JCR *jcr, B_DB *mdb, FileId_t FileId, JobId_t JobId)
{
   return 1;
}

int db_update_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   return 1;
}

int db_update_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{
   return 0;
}

int db_update_media_defaults(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   return 1;
}

void db_make_inchanger_unique(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
  return;
}

#endif /* HAVE_BACULA_DB */
