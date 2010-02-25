/*
 * Bacula Catalog Database Get record interface routines
 *  Note, these routines generally get a record by id or
 *        by name.  If more logic is involved, the routine
 *        should be in find.c
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


/* -----------------------------------------------------------------------
 *
 *   Bacula specific defines and subroutines
 *
 * -----------------------------------------------------------------------
 */


/*
 * Get Job record for given JobId
 * Returns: 0 on failure
 *          1 on success
 */

bool db_get_job_record(JCR *jcr, B_DB *mdb, JOB_DBR *jr)
{
   return 0;
}


/*
 * Get the number of pool records
 *
 * Returns: -1 on failure
 *          number on success
 */
int db_get_num_pool_records(JCR *jcr, B_DB *mdb)
{
   return -1;
}

/*
 * This function returns a list of all the Pool record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *          1: on success
 */
int db_get_pool_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   return 0;
}


/*
 * Get Pool Record
 * If the PoolId is non-zero, we get its record,
 *  otherwise, we search on the PoolName
 *
 * Returns: false on failure
 *          true on success
 */
bool db_get_pool_record(JCR *jcr, B_DB *mdb, POOL_DBR *pr)
{
   return 0;
}

/*
 * Get the number of Media records
 *
 * Returns: -1 on failure
 *          number on success
 */
int db_get_num_media_records(JCR *jcr, B_DB *mdb)
{
   return -1;
}

/*
 * This function returns a list of all the Media record ids
 *  for a specified PoolId
 *  The caller must free ids if non-NULL.
 *
 *  Returns false: on failure
 *          true:  on success
 */
bool db_get_media_ids(JCR *jcr, B_DB *mdb, uint32_t PoolId, int *num_ids, uint32_t *ids[])
{
   return false;
}

/*
 * Get Media Record
 * If the MediaId is non-zero, we get its record,
 *  otherwise, we search on the MediaName
 *
 * Returns: false on failure
 *          true on success
 */
bool db_get_media_record(JCR *jcr, B_DB *mdb, MEDIA_DBR *mr)
{
   return false;
}

/*
 * Find VolumeNames for a give JobId
 *  Returns: 0 on error or no Volumes found
 *           number of volumes on success
 *              Volumes are concatenated in VolumeNames
 *              separated by a vertical bar (|).
 */
int db_get_job_volume_names(JCR *jcr, B_DB *mdb, uint32_t JobId, POOLMEM **VolumeNames)
{
   return 0;
}

/*
 * Get Client Record
 * If the ClientId is non-zero, we get its record,
 *  otherwise, we search on the Name
 *
 * Returns: 0 on failure
 *          id on success
 */
int db_get_client_record(JCR *jcr, B_DB *mdb, CLIENT_DBR *cr)
{
   return 0;
}

/*
 * Get FileSet Record   (We read the FILESET_DBR structure)
 * If the FileSetId is non-zero, we get its record,
 *  otherwise, we search on the FileSet (its name).
 *
 * Returns: 0 on failure
 *          id on success
 */
int db_get_fileset_record(JCR *jcr, B_DB *mdb, FILESET_DBR *fsr)
{
   return 0;
}

bool db_get_query_dbids(JCR *jcr, B_DB *mdb, POOL_MEM &query, dbid_list &ids)
{ return false; }

int db_get_file_attributes_record(JCR *jcr, B_DB *mdb, char *fname, JOB_DBR *jr, FILE_DBR *fdbr)
{ return 0; }

int db_get_job_volume_parameters(JCR *jcr, B_DB *mdb, uint32_t JobId, VOL_PARAMS **VolParams)
{ return 0; }

int db_get_client_ids(JCR *jcr, B_DB *mdb, int *num_ids, uint32_t *ids[])
{ return 0; }

int db_get_counter_record(JCR *jcr, B_DB *mdb, COUNTER_DBR *cr)
{ return 0; }


#endif /* HAVE_BACULA_DB */
