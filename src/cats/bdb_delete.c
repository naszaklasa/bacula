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
 *    Version $Id: bdb_delete.c,v 1.6 2003/06/02 22:19:53 kerns Exp $
 */

/*
   Copyright (C) 2001-2003 Kern Sibbald and John Walker

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


/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C 		      /* indicate that this is sql.c */

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
 *	    the number of records deleted on success 
 */
int db_delete_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   int stat;
   POOL_DBR opr;

   db_lock(mdb);
   pr->PoolId = 0;		      /* Search on Pool Name */
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
