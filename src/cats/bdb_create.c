/*
 * Bacula Catalog Database Create record routines
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
 *    Version $Id$
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

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

/* Forward referenced functions */
bool db_create_pool_record(B_DB *mdb, POOL_DBR *pr);

/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */

bool db_create_file_attributes_record(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   return true;
}

int db_create_file_item(JCR *jcr, B_DB *mdb, ATTR_DBR *ar)
{
   return 1;
}


/*
 * Create a new record for the Job
 *   This record is created at the start of the Job,
 *   it is updated in bdb_update.c when the Job terminates.
 *
 * Returns: 0 on failure
 *          1 on success
 */
bool db_create_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   return 0;
}

/* Create a JobMedia record for Volume used this job
 * Returns: 0 on failure
 *          record-id on success
 */
bool db_create_jobmedia_record(JCR *jcr, B_DB *mdb, JOBMEDIA_DBR *jm)
{
   return 0;
}


/*
 *  Create a unique Pool record
 * Returns: 0 on failure
 *          1 on success
 */
bool db_create_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   return 0;
}

bool db_create_device_record(JCR *jcr, B_DB *mdb, DEVICE_DBR *dr)
{ return false; }

bool db_create_storage_record(JCR *jcr, B_DB *mdb, STORAGE_DBR *dr)
{ return false; }

bool db_create_mediatype_record(JCR *jcr, B_DB *mdb, MEDIATYPE_DBR *dr)
{ return false; }


/*
 * Create Unique Media record.  This record
 *   contains all the data pertaining to a specific
 *   Volume.
 *
 * Returns: 0 on failure
 *          1 on success
 */
int db_create_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   return 0;
}


/*
 *  Create a unique Client record or return existing record
 * Returns: 0 on failure
 *          1 on success
 */
int db_create_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   return 0;
}

/*
 *  Create a unique FileSet record or return existing record
 *
 *   Note, here we write the FILESET_DBR structure
 *
 * Returns: 0 on failure
 *          1 on success
 */
bool db_create_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   return false;
}

int db_create_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{ return 0; }

bool db_write_batch_file_records(JCR *jcr) { return false; }
bool my_batch_start(JCR *jcr, B_DB *mdb) { return false; }
bool my_batch_end(JCR *jcr, B_DB *mdb, const char *error) { return false; }
bool my_batch_insert(JCR *jcr, B_DB *mdb, ATTR_DBR *ar) { return false; }
                                 

#endif /* HAVE_BACULA_DB */
