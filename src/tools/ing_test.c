/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2009-2010 Free Software Foundation Europe e.V.

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
/*
 *
 *  Program to test Ingres DB routines
 *
 *   Stefan Reddig, February 2010
 *
 *   reusing code by
 *   Eric Bollengier, August 2009
 *
 *
 */
#include "bacula.h"
#include "cats/cats.h"
#include "cats/bvfs.h"
#include "findlib/find.h"
 
/* Local variables */
static B_DB *db;
static const char *file = "COPYRIGHT";
//static DBId_t fnid=0;
static const char *db_name = "bacula";
static const char *db_user = "bacula";
static const char *db_password = "";
static const char *db_host = NULL;

static void usage()
{
   fprintf(stderr, _(
PROG_COPYRIGHT
"\nVersion: %s (%s)\n"
"       -d <nn>           set debug level to <nn>\n"
"       -dt               print timestamp in debug output\n"
"       -n <name>         specify the database name (default bacula)\n"
"       -u <user>         specify database user name (default bacula)\n"
"       -P <password      specify database password (default none)\n"
"       -h <host>         specify database host (default NULL)\n"
"       -w <working>      specify working directory\n"
"       -j <jobids>       specify jobids\n"
"       -p <path>         specify path\n"
"       -f <file>         specify file\n"
"       -l <limit>        maximum tuple to fetch\n"
"       -T                truncate cache table before starting\n"
"       -v                verbose\n"
"       -?                print this message\n\n"), 2001, VERSION, BDATE);
   exit(1);
}

/*
 * simple handler for debug output of CRUD example
 */
static int test_handler(void *ctx, int num_fields, char **row)
{
   Dmsg2(200, "   Values are %d, %s\n", str_to_int64(row[0]), row[1]);
   return 0;
}

/*
 * string handler for debug output of simple SELECT tests
 */
static int string_handler(void *ctx, int num_fields, char **row)
{
   Dmsg1(200, "   Value is >>%s<<\n", row[0]);
   return 0;
}


/* number of thread started */

int main (int argc, char *argv[])
{
   int ch;
   char *jobids = (char *)"1";
   char *path=NULL, *client=NULL;
   uint64_t limit=0;
   bool clean=false;
   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");
   init_stack_dump();

   Dmsg0(0, "Starting ing_test tool\n");
   
   my_name_is(argc, argv, "ing_test");
   init_msg(NULL, NULL);

   OSDependentInit();

   while ((ch = getopt(argc, argv, "h:c:l:d:n:P:Su:vf:w:?j:p:f:T")) != -1) {
      switch (ch) {
      case 'd':                    /* debug level */
         if (*optarg == 't') {
            dbg_timestamp = true;
         } else {
            debug_level = atoi(optarg);
            if (debug_level <= 0) {
               debug_level = 1;
            }
         }
         break;
      case 'l':
         limit = str_to_int64(optarg);
         break;

      case 'c':
         client = optarg;
         break;

      case 'h':
         db_host = optarg;
         break;

      case 'n':
         db_name = optarg;
         break;

      case 'w':
         working_directory = optarg;
         break;

      case 'u':
         db_user = optarg;
         break;

      case 'P':
         db_password = optarg;
         break;

      case 'v':
         verbose++;
         break;

      case 'p':
         path = optarg;
         break;

      case 'f':
         file = optarg;
         break;

      case 'j':
         jobids = optarg;
         break;

      case 'T':
         clean = true;
         break;

      case '?':
      default:
         usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (argc != 0) {
      Pmsg0(0, _("Wrong number of arguments: \n"));
      usage();
   }
   
   if ((db=db_init_database(NULL, db_name, db_user, db_password,
                            db_host, 0, NULL, 0)) == NULL) {
      Emsg0(M_ERROR_TERM, 0, _("Could not init Bacula database\n"));
   }
   Dmsg1(0, "db_type=%s\n", db_get_type());

   if (!db_open_database(NULL, db)) {
      Emsg0(M_ERROR_TERM, 0, db_strerror(db));
   }
   Dmsg0(200, "Database opened\n");
   if (verbose) {
      Pmsg2(000, _("Using Database: %s, User: %s\n"), db_name, db_user);
   }
   

   /* simple CRUD test including create/drop table */

   Pmsg0(0, "\nsimple CRUD test...\n\n");

   Dmsg0(200, "DB-Statement: CREATE TABLE t1 ( c1 integer, c2 varchar(29))\n");
   if (!db_sql_query(db, "CREATE TABLE t1 ( c1 integer, c2 varchar(29))", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("CREATE-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: INSERT INTO t1 VALUES (1, 'foo')\n");
   if (!db_sql_query(db, "INSERT INTO t1 VALUES (1, 'foo')", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("INSERT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT c1,c2 FROM t1 (should be 1, foo)\n");
   if (!db_sql_query(db, "SELECT c1,c2 FROM t1", test_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: UPDATE t1 SET c2='bar' WHERE c1=1\n");
   if (!db_sql_query(db, "UPDATE t1 SET c2='bar' WHERE c1=1", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("UPDATE-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT * FROM t1 (should be 1, bar)\n");
   if (!db_sql_query(db, "SELECT * FROM t1", test_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: DELETE FROM t1 WHERE c2 LIKE '\%r'\n");
   if (!db_sql_query(db, "DELETE FROM t1 WHERE c2 LIKE '%r'", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("DELETE-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT * FROM t1 (should be 0 rows)\n");
   if (!db_sql_query(db, "SELECT * FROM t1", test_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: DROP TABLE t1\n");
   if (!db_sql_query(db, "DROP TABLE t1", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("DROP-Stmt went wrong\n"));
   }

   /*
    * simple SELECT test without tables
    */
    
   Dmsg0(200, "DB-Statement: SELECT 'Test of simple SELECT!'\n");
   if (!db_sql_query(db, "SELECT 'Test of simple SELECT!'", string_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT 'Test of simple SELECT!' as Text\n");
   if (!db_sql_query(db, "SELECT 'Test of simple SELECT!' as Text", string_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT VARCHAR(LENGTH('Test of simple SELECT!'))\n");
   if (!db_sql_query(db, "SELECT VARCHAR(LENGTH('Test of simple SELECT!'))", string_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: SELECT DBMSINFO('_version')\n");
   if (!db_sql_query(db, "SELECT DBMSINFO('_version')", string_handler, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("SELECT-Stmt went wrong\n"));
   }

   /* 
    * datatypes test
    */
   Pmsg0(0, "\ndatatypes test... (TODO)\n\n");


   Dmsg0(200, "DB-Statement: CREATE TABLE for datatypes\n");
   if (!db_sql_query(db, "CREATE TABLE t2 ("
     "c1        integer,"
     "c2        varchar(255),"
     "c3        char(255)"
     /* some more datatypes... "c4      ," */
     ")" , NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("CREATE-Stmt went wrong\n"));
   }

   Dmsg0(200, "DB-Statement: DROP TABLE for datatypes\n");
   if (!db_sql_query(db, "DROP TABLE t2", NULL, NULL)) {
      Emsg0(M_ERROR_TERM, 0, _("DROP-Stmt went wrong\n"));
   }


   db_close_database(NULL, db);
   Dmsg0(200, "Database closed\n");

   return 0;
}
