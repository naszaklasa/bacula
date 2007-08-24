/*
 * Bacula Catalog Database Delete record interface routines
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
 *    Version $Id: bdb_delete.c 4992 2007-06-07 14:46:43Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

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
 * Delete a Pool record given the Name
 *
 * Returns: 0 on error
 *          the number of records deleted on success
 */
int db_delete_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   int stat;
   POOL_DBR opr;

   db_lock(mdb);
   pr->PoolId = 0;                    /* Search on Pool Name */
   if (!db_get_pool_record(jcr, mdb, pr)) {
      Mmsg1(&mdb->errmsg, "No pool record %s exists\n", pr->Name);
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->poolfd, pr->rec_addr, SEEK_SET);
   memset(&opr, 0, sizeof(opr));
   stat = fwrite(&opr, sizeof(opr), 1, mdb->poolfd);
   db_unlock(mdb);
   return stat;
}

int db_delete_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   int stat;
   MEDIA_DBR omr;

   db_lock(mdb);
   if (!db_get_media_record(jcr, mdb, mr)) {
      Mmsg0(&mdb->errmsg, "Media record not found.\n");
      db_unlock(mdb);
      return 0;
   }
   fseek(mdb->mediafd, mr->rec_addr, SEEK_SET);
   memset(&omr, 0, sizeof(omr));
   stat = fwrite(&omr, sizeof(omr), 1, mdb->mediafd);
   db_unlock(mdb);
   return stat;
}

#endif /* HAVE_BACULA_DB */
