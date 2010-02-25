/*
 * Bacula Catalog Database List records interface routines
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
 * Submit general SQL query
 */
int db_list_sql_query(JCR *jcr, B_DB *mdb, const char *query, DB_LIST_HANDLER *sendit,
                      void *ctx, int verbose)
{
   return 0;
}


/*
 * List all the pool records
 */
void db_list_pool_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx)
{
   return;
}


/*
 * List Media records
 */
void db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr,
                           DB_LIST_HANDLER *sendit, void *ctx)
{
   return;
}

void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, uint32_t JobId,
                              DB_LIST_HANDLER *sendit, void *ctx)
{
   return;
}


/*
 * List Job records
 */
void db_list_job_records(JCR *jcr, B_DB *mdb, JOB_DBR *jr,
                         DB_LIST_HANDLER *sendit, void *ctx)
{
   return;
}


/*
 * List Job Totals
 */
void db_list_job_totals(JCR *jcr, B_DB *mdb, JOB_DBR *jr,
                        DB_LIST_HANDLER *sendit, void *ctx)
{
   return;
}



void db_list_files_for_job(JCR *jcr, B_DB *mdb, uint32_t jobid, DB_LIST_HANDLER *sendit, void *ctx)
{ }

void db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx)
{ }

int db_list_sql_query(JCR *jcr, B_DB *mdb, const char *query, DB_LIST_HANDLER *sendit,
                      void *ctx, int verbose, e_list_type type)
{
   return 0;
}

void
db_list_pool_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void
db_list_media_records(JCR *jcr, B_DB *mdb, MEDIA_DBR *mdbr,
                      DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void db_list_jobmedia_records(JCR *jcr, B_DB *mdb, uint32_t JobId,
                              DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }

void
db_list_job_records(JCR *jcr, B_DB *mdb, JOB_DBR *jr, DB_LIST_HANDLER *sendit,
                    void *ctx, e_list_type type)
{ }

void
db_list_client_records(JCR *jcr, B_DB *mdb, DB_LIST_HANDLER *sendit, void *ctx, e_list_type type)
{ }




#endif /* HAVE_BACULA_DB */
