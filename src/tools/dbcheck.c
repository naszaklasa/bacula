/*
 *
 *  Program to check a Bacula database for consistency and to
 *   make repairs
 *
 *   Kern E. Sibbald, August 2002
 *
 *   Version $Id: dbcheck.c,v 1.30 2005/08/10 16:35:37 nboichat Exp $
 *
 */
/*
   Copyright (C) 2002-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "cats/cats.h"
#include "dird/dird_conf.h"

/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) 
   { return 1; }

typedef struct s_id_ctx {
   uint32_t *Id;                      /* ids to be modified */
   int num_ids;                       /* ids stored */
   int max_ids;                       /* size of array */
   int num_del;                       /* number deleted */
   int tot_ids;                       /* total to process */
} ID_LIST;

typedef struct s_name_ctx {
   char **name;                       /* list of names */
   int num_ids;                       /* ids stored */
   int max_ids;                       /* size of array */
   int num_del;                       /* number deleted */
   int tot_ids;                       /* total to process */
} NAME_LIST;



/* Global variables */
static bool fix = false;
static bool batch = false;
static B_DB *db;
static ID_LIST id_list;
static NAME_LIST name_list;
static char buf[2000];

#define MAX_ID_LIST_LEN 10000000

/* Forward referenced functions */
static int make_id_list(const char *query, ID_LIST *id_list);
static int delete_id_list(const char *query, ID_LIST *id_list);
static int make_name_list(const char *query, NAME_LIST *name_list);
static void print_name_list(NAME_LIST *name_list);
static void free_name_list(NAME_LIST *name_list);
static char *get_cmd(const char *prompt);
static void eliminate_duplicate_filenames();
static void eliminate_duplicate_paths();
static void eliminate_orphaned_jobmedia_records();
static void eliminate_orphaned_file_records();
static void eliminate_orphaned_path_records();
static void eliminate_orphaned_filename_records();
static void eliminate_orphaned_fileset_records();
static void eliminate_orphaned_client_records();
static void eliminate_orphaned_job_records();
static void eliminate_admin_records();
static void eliminate_restore_records();
static void repair_bad_paths();
static void repair_bad_filenames();
static void do_interactive_mode();
static int yes_no(const char *prompt);


static void usage()
{
   fprintf(stderr,
"Usage: dbcheck [-c config] [-C catalog name] [-d debug_level] <working-directory> <bacula-database> <user> <password> [<dbhost>]\n"
"       -b              batch mode\n"
"       -C              catalog name in the director conf file\n"
"       -c              director conf filename\n"
"       -dnn            set debug level to nn\n"
"       -f              fix inconsistencies\n"
"       -v              verbose\n"
"       -?              print this message\n\n");
   exit(1);
}

int main (int argc, char *argv[])
{
   int ch;
   const char *user, *password, *db_name, *dbhost;
   char *configfile = NULL;
   char *catalogname = NULL;

   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   my_name_is(argc, argv, "dbcheck");
   init_msg(NULL, NULL);              /* setup message handler */

   memset(&id_list, 0, sizeof(id_list));
   memset(&name_list, 0, sizeof(name_list));


   while ((ch = getopt(argc, argv, "bc:C:d:fv?")) != -1) {
      switch (ch) {
      case 'b':                    /* batch */
         batch = true;
         break;

      case 'C':                    /* CatalogName */
          catalogname = optarg;
         break;

      case 'c':                    /* configfile */
          configfile = optarg;
         break;

      case 'd':                    /* debug level */
         debug_level = atoi(optarg);
         if (debug_level <= 0)
            debug_level = 1;
         break;

      case 'f':                    /* fix inconsistencies */
         fix = true;
         break;

      case 'v':
         verbose++;
         break;

      case '?':
      default:
         usage();
      }
   }
   argc -= optind;
   argv += optind;

   if (configfile) {
      CAT *catalog = NULL;
      int found = 0;
      if (argc > 0) {
         Pmsg0(0, _("Warning skipping the additional parameters for working directory/dbname/user/password/host.\n"));
      }
      parse_config(configfile);
      LockRes();
      foreach_res(catalog, R_CATALOG) {
         if (catalogname && !strcmp(catalog->hdr.name, catalogname)) {
            ++found;
            break;
         } else if (!catalogname) { // stop on first if no catalogname is given
           ++found;
           break;
         }
      }
      UnlockRes();
      if (!found) {
         if (catalogname) {
            Pmsg2(0, _("Error can not find the Catalog name[%s] in the given config file [%s]\n"), catalogname, configfile);
         } else {
            Pmsg1(0, _("Error there is no Catalog section in the given config file [%s]\n"), configfile);
         }
         exit(1);
      } else {
         DIRRES *director;
         LockRes();
         director = (DIRRES *)GetNextRes(R_DIRECTOR, NULL);
         UnlockRes();
         if (!director) {
            Pmsg0(0, _("Error no Director resource defined.\n"));
            exit(1);
         }
         set_working_directory(director->working_directory);
         db_name = catalog->db_name;
         user = catalog->db_user;
         password = catalog->db_password;
         dbhost = catalog->db_address;
         if (dbhost && dbhost[0] == 0) {
            dbhost = NULL;
         }
      }
   } else {
      if (argc > 5) {
         Pmsg0(0, _("Wrong number of arguments.\n"));
         usage();
      }

      if (argc < 1) {
         Pmsg0(0, _("Working directory not supplied.\n"));
         usage();
      }

      /* This is needed by SQLite to find the db */
      working_directory = argv[0];
      db_name = "bacula";
      user = db_name;
      password = "";
      dbhost = NULL;

      if (argc == 2) {
         db_name = argv[1];
         user = db_name;
      } else if (argc == 3) {
         db_name = argv[1];
         user = argv[2];
      } else if (argc == 4) {
         db_name = argv[1];
         user = argv[2];
         password = argv[3];
      } else if (argc == 5) {
         db_name = argv[1];
         user = argv[2];
         password = argv[3];
         dbhost = argv[4];
      }
   }

   /* Open database */
   db = db_init_database(NULL, db_name, user, password, dbhost, 0, NULL, 0);
   if (!db_open_database(NULL, db)) {
      Emsg1(M_FATAL, 0, "%s", db_strerror(db));
          return 1;
   }

   if (batch) {
      repair_bad_paths();
      repair_bad_filenames();
      eliminate_duplicate_filenames();
      eliminate_duplicate_paths();
      eliminate_orphaned_jobmedia_records();
      eliminate_orphaned_file_records();
      eliminate_orphaned_path_records();
      eliminate_orphaned_filename_records();
      eliminate_orphaned_fileset_records();
      eliminate_orphaned_client_records();
      eliminate_orphaned_job_records();
      eliminate_admin_records();
      eliminate_restore_records();
   } else {
      do_interactive_mode();
   }

   db_close_database(NULL, db);
   close_msg(NULL);
   term_msg();
   return 0;
}

static void do_interactive_mode()
{
   bool quit = false;
   const char *cmd;

   printf(_("Hello, this is the database check/correct program.\n"));
   if (fix)
      printf(_("Modify database is on."));
   else
      printf(_("Modify database is off."));
   if (verbose)
      printf(_(" Verbose is on.\n"));
   else
      printf(_(" Verbose is off.\n"));

   printf(_("Please select the fuction you want to perform.\n"));

   while (!quit) {
      if (fix) {
         printf(_("\n"
"     1) Toggle modify database flag\n"
"     2) Toggle verbose flag\n"
"     3) Repair bad Filename records\n"
"     4) Repair bad Path records\n"
"     5) Eliminate duplicate Filename records\n"
"     6) Eliminate duplicate Path records\n"
"     7) Eliminate orphaned Jobmedia records\n"
"     8) Eliminate orphaned File records\n"
"     9) Eliminate orphaned Path records\n"
"    10) Eliminate orphaned Filename records\n"
"    11) Eliminate orphaned FileSet records\n"
"    12) Eliminate orphaned Client records\n"
"    13) Eliminate orphaned Job records\n"
"    14) Eliminate all Admin records\n"
"    15) Eliminate all Restore records\n"
"    16) All (3-15)\n"
"    17) Quit\n"));
       } else {
         printf(_("\n"
"     1) Toggle modify database flag\n"
"     2) Toggle verbose flag\n"
"     3) Check for bad Filename records\n"
"     4) Check for bad Path records\n"
"     5) Check for duplicate Filename records\n"
"     6) Check for duplicate Path records\n"
"     7) Check for orphaned Jobmedia records\n"
"     8) Check for orphaned File records\n"
"     9) Check for orphaned Path records\n"
"    10) Check for orphaned Filename records\n"
"    11) Check for orphaned FileSet records\n"
"    12) Check for orphaned Client records\n"
"    13) Check for orphaned Job records\n"
"    14) Check for all Admin records\n"
"    15) Check for all Restore records\n"
"    16) All (3-15)\n"
"    17) Quit\n"));
       }

      cmd = get_cmd(_("Select function number: "));
      if (cmd) {
         int item = atoi(cmd);
         switch (item) {
         case 1:
            fix = !fix;
            if (fix)
               printf(_("Database will be modified.\n"));
            else
               printf(_("Database will NOT be modified.\n"));
            break;
         case 2:
            verbose = verbose?0:1;
            if (verbose)
               printf(_(" Verbose is on.\n"));
            else
               printf(_(" Verbose is off.\n"));
            break;
         case 3:
            repair_bad_filenames();
            break;
         case 4:
            repair_bad_paths();
            break;
         case 5:
            eliminate_duplicate_filenames();
            break;
         case 6:
            eliminate_duplicate_paths();
            break;
         case 7:
            eliminate_orphaned_jobmedia_records();
            break;
         case 8:
            eliminate_orphaned_file_records();
            break;
         case 9:
            eliminate_orphaned_path_records();
            break;
         case 10:
            eliminate_orphaned_filename_records();
            break;
         case 11:
            eliminate_orphaned_fileset_records();
            break;
         case 12:
            eliminate_orphaned_client_records();
            break;
         case 13:
            eliminate_orphaned_job_records();
            break;
         case 14:
            eliminate_admin_records();
            break;
         case 15:
            eliminate_restore_records();
            break;
         case 16:
            repair_bad_filenames();
            repair_bad_paths();
            eliminate_duplicate_filenames();
            eliminate_duplicate_paths();
            eliminate_orphaned_jobmedia_records();
            eliminate_orphaned_file_records();
            eliminate_orphaned_path_records();
            eliminate_orphaned_filename_records();
            eliminate_orphaned_fileset_records();
            eliminate_orphaned_client_records();
            eliminate_orphaned_job_records();
            eliminate_admin_records();
            eliminate_restore_records();
            break;
         case 17:
            quit = true;
            break;
         }
      }
   }
}

static int print_name_handler(void *ctx, int num_fields, char **row)
{
   if (row[0]) {
      printf("%s\n", row[0]);
   }
   return 0;
}

static int get_name_handler(void *ctx, int num_fields, char **row)
{
   POOLMEM *buf = (POOLMEM *)ctx;
   if (row[0]) {
      pm_strcpy(&buf, row[0]);
   }
   return 0;
}

static int print_job_handler(void *ctx, int num_fields, char **row)
{
   printf(_("JobId=%s Name=\"%s\" StartTime=%s\n"),
              NPRT(row[0]), NPRT(row[1]), NPRT(row[2]));
   return 0;
}


static int print_jobmedia_handler(void *ctx, int num_fields, char **row)
{
   printf(_("Orphaned JobMediaId=%s JobId=%s Volume=\"%s\"\n"),
              NPRT(row[0]), NPRT(row[1]), NPRT(row[2]));
   return 0;
}

static int print_file_handler(void *ctx, int num_fields, char **row)
{
   printf(_("Orphaned FileId=%s JobId=%s Volume=\"%s\"\n"),
              NPRT(row[0]), NPRT(row[1]), NPRT(row[2]));
   return 0;
}

static int print_fileset_handler(void *ctx, int num_fields, char **row)
{
   printf(_("Orphaned FileSetId=%s FileSet=\"%s\" MD5=%s\n"),
              NPRT(row[0]), NPRT(row[1]), NPRT(row[2]));
   return 0;
}

static int print_client_handler(void *ctx, int num_fields, char **row)
{
   printf(_("Orphaned ClientId=%s Name=\"%s\"\n"),
              NPRT(row[0]), NPRT(row[1]));
   return 0;
}


/*
 * Called here with each id to be added to the list
 */
static int id_list_handler(void *ctx, int num_fields, char **row)
{
   ID_LIST *lst = (ID_LIST *)ctx;

   if (lst->num_ids == MAX_ID_LIST_LEN) {
      return 1;
   }
   if (lst->num_ids == lst->max_ids) {
      if (lst->max_ids == 0) {
         lst->max_ids = 1000;
         lst->Id = (uint32_t *)bmalloc(sizeof(uint32_t) * lst->max_ids);
      } else {
         lst->max_ids = (lst->max_ids * 3) / 2;
         lst->Id = (uint32_t *)brealloc(lst->Id, sizeof(uint32_t) * lst->max_ids);
      }
   }
   lst->Id[lst->num_ids++] = (uint32_t)strtod(row[0], NULL);
   return 0;
}

/*
 * Construct record id list
 */
static int make_id_list(const char *query, ID_LIST *id_list)
{
   id_list->num_ids = 0;
   id_list->num_del = 0;
   id_list->tot_ids = 0;

   if (!db_sql_query(db, query, id_list_handler, (void *)id_list)) {
      printf("%s", db_strerror(db));
      return 0;
   }
   return 1;
}

/*
 * Delete all entries in the list
 */
static int delete_id_list(const char *query, ID_LIST *id_list)
{
   for (int i=0; i < id_list->num_ids; i++) {
      bsnprintf(buf, sizeof(buf), query, id_list->Id[i]);
      if (verbose) {
         printf(_("Deleting: %s\n"), buf);
      }
      db_sql_query(db, buf, NULL, NULL);
   }
   return 1;
}

/*
 * Called here with each name to be added to the list
 */
static int name_list_handler(void *ctx, int num_fields, char **row)
{
   NAME_LIST *name = (NAME_LIST *)ctx;

   if (name->num_ids == MAX_ID_LIST_LEN) {
      return 1;
   }
   if (name->num_ids == name->max_ids) {
      if (name->max_ids == 0) {
         name->max_ids = 1000;
         name->name = (char **)bmalloc(sizeof(char *) * name->max_ids);
      } else {
         name->max_ids = (name->max_ids * 3) / 2;
         name->name = (char **)brealloc(name->name, sizeof(char *) * name->max_ids);
      }
   }
   name->name[name->num_ids++] = bstrdup(row[0]);
   return 0;
}


/*
 * Construct name list
 */
static int make_name_list(const char *query, NAME_LIST *name_list)
{
   name_list->num_ids = 0;
   name_list->num_del = 0;
   name_list->tot_ids = 0;

   if (!db_sql_query(db, query, name_list_handler, (void *)name_list)) {
      printf("%s", db_strerror(db));
      return 0;
   }
   return 1;
}

/*
 * Print names in the list
 */
static void print_name_list(NAME_LIST *name_list)
{
   for (int i=0; i < name_list->num_ids; i++) {
      printf("%s\n", name_list->name[i]);
   }
}


/*
 * Free names in the list
 */
static void free_name_list(NAME_LIST *name_list)
{
   for (int i=0; i < name_list->num_ids; i++) {
      free(name_list->name[i]);
   }
   name_list->num_ids = 0;
}

static void eliminate_duplicate_filenames()
{
   const char *query;
   char esc_name[5000];

   printf(_("Checking for duplicate Filename entries.\n"));

   /* Make list of duplicated names */
   query = "SELECT Name, count(Name) as Count FROM Filename GROUP BY  Name "
           "HAVING count(Name) > 1";

   if (!make_name_list(query, &name_list)) {
      exit(1);
   }
   printf(_("Found %d duplicate Filename records.\n"), name_list.num_ids);
   if (name_list.num_ids && verbose && yes_no(_("Print the list? (yes/no): "))) {
      print_name_list(&name_list);
   }
   if (fix) {
      /* Loop through list of duplicate names */
      for (int i=0; i<name_list.num_ids; i++) {
         /* Get all the Ids of each name */
         db_escape_string(esc_name, name_list.name[i], strlen(name_list.name[i]));
         bsnprintf(buf, sizeof(buf), "SELECT FilenameId FROM Filename WHERE Name='%s'", esc_name);
         if (verbose > 1) {
            printf("%s\n", buf);
         }
         if (!make_id_list(buf, &id_list)) {
            exit(1);
         }
         if (verbose) {
            printf(_("Found %d for: %s\n"), id_list.num_ids, name_list.name[i]);
         }
         /* Force all records to use the first id then delete the other ids */
         for (int j=1; j<id_list.num_ids; j++) {
            bsnprintf(buf, sizeof(buf), "UPDATE File SET FilenameId=%u WHERE FilenameId=%u",
               id_list.Id[0], id_list.Id[j]);
            if (verbose > 1) {
               printf("%s\n", buf);
            }
            db_sql_query(db, buf, NULL, NULL);
            bsnprintf(buf, sizeof(buf), "DELETE FROM Filename WHERE FilenameId=%u",
               id_list.Id[j]);
            if (verbose > 2) {
               printf("%s\n", buf);
            }
            db_sql_query(db, buf, NULL, NULL);
         }
      }
   }
   free_name_list(&name_list);
}

static void eliminate_duplicate_paths()
{
   const char *query;
   char esc_name[5000];

   printf(_("Checking for duplicate Path entries.\n"));

   /* Make list of duplicated names */

   query = "SELECT Path, count(Path) as Count FROM Path "
           "GROUP BY Path HAVING count(Path) > 1";

   if (!make_name_list(query, &name_list)) {
      exit(1);
   }
   printf(_("Found %d duplicate Path records.\n"), name_list.num_ids);
   if (name_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      print_name_list(&name_list);
   }
   if (fix) {
      /* Loop through list of duplicate names */
      for (int i=0; i<name_list.num_ids; i++) {
         /* Get all the Ids of each name */
         db_escape_string(esc_name, name_list.name[i], strlen(name_list.name[i]));
         bsnprintf(buf, sizeof(buf), "SELECT PathId FROM Path WHERE Path='%s'", esc_name);
         if (verbose > 1) {
            printf("%s\n", buf);
         }
         if (!make_id_list(buf, &id_list)) {
            exit(1);
         }
         if (verbose) {
            printf(_("Found %d for: %s\n"), id_list.num_ids, name_list.name[i]);
         }
         /* Force all records to use the first id then delete the other ids */
         for (int j=1; j<id_list.num_ids; j++) {
            bsnprintf(buf, sizeof(buf), "UPDATE File SET PathId=%u WHERE PathId=%u",
               id_list.Id[0], id_list.Id[j]);
            if (verbose > 1) {
               printf("%s\n", buf);
            }
            db_sql_query(db, buf, NULL, NULL);
            bsnprintf(buf, sizeof(buf), "DELETE FROM Path WHERE PathId=%u",
               id_list.Id[j]);
            if (verbose > 2) {
               printf("%s\n", buf);
            }
            db_sql_query(db, buf, NULL, NULL);
         }
      }
   }
   free_name_list(&name_list);
}

static void eliminate_orphaned_jobmedia_records()
{
   const char *query;

   printf(_("Checking for orphaned JobMedia entries.\n"));
   query = "SELECT JobMedia.JobMediaId,Job.JobId FROM JobMedia "
           "LEFT OUTER JOIN Job ON (JobMedia.JobId=Job.JobId) "
           "WHERE Job.JobId IS NULL";
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned JobMedia records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf),
"SELECT JobMedia.JobMediaId,JobMedia.JobId,Media.VolumeName FROM JobMedia,Media "
"WHERE JobMedia.JobMediaId=%u AND Media.MediaId=JobMedia.MediaId", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_jobmedia_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned JobMedia records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM JobMedia WHERE JobMediaId=%u", &id_list);
   }
}

static void eliminate_orphaned_file_records()
{
   const char *query;

   printf(_("Checking for orphaned File entries. This may take some time!\n"));
   query = "SELECT File.FileId,Job.JobId FROM File "
           "LEFT OUTER JOIN Job ON (File.JobId=Job.JobId) "
           "WHERE Job.JobId IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned File records.\n"), id_list.num_ids);
   if (name_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf),
"SELECT File.FileId,File.JobId,Filename.Name FROM File,Filename "
"WHERE File.FileId=%u AND File.FilenameId=Filename.FilenameId", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_file_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned File records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM File WHERE FileId=%u", &id_list);
   }
}

static void eliminate_orphaned_path_records()
{
   const char *query;

   printf(_("Checking for orphaned Path entries. This may take some time!\n"));
   query = "SELECT DISTINCT Path.PathId,File.PathId FROM Path "
           "LEFT OUTER JOIN File ON (Path.PathId=File.PathId) "
           "WHERE File.PathId IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned Path records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT Path FROM Path WHERE PathId=%u", id_list.Id[i]);
         db_sql_query(db, buf, print_name_handler, NULL);
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned Path records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Path WHERE PathId=%u", &id_list);
   }
}

static void eliminate_orphaned_filename_records()
{
   const char *query;

   printf(_("Checking for orphaned Filename entries. This may take some time!\n"));
   query = "SELECT Filename.FilenameId,File.FilenameId FROM Filename "
           "LEFT OUTER JOIN File ON (Filename.FilenameId=File.FilenameId) "
           "WHERE File.FilenameId IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned Filename records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT Name FROM Filename WHERE FilenameId=%u", id_list.Id[i]);
         db_sql_query(db, buf, print_name_handler, NULL);
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned Filename records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Filename WHERE FilenameId=%u", &id_list);
   }
}

static void eliminate_orphaned_fileset_records()
{
   const char *query;

   printf(_("Checking for orphaned FileSet entries. This takes some time!\n"));
   query = "SELECT FileSet.FileSetId,Job.FileSetId FROM FileSet "
           "LEFT OUTER JOIN Job ON (FileSet.FileSetId=Job.FileSetId) "
           "WHERE Job.FileSetId IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned FileSet records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT FileSetId,FileSet,MD5 FROM FileSet "
                      "WHERE FileSetId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_fileset_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned FileSet records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM FileSet WHERE FileSetId=%u", &id_list);
   }
}

static void eliminate_orphaned_client_records()
{
   const char *query;

   printf(_("Checking for orphaned Client entries.\n"));
   /* In English:
    *   Wiffle through Client for every Client
    *   joining with the Job table including every Client even if
    *   there is not a match in Job (left outer join), then
    *   filter out only those where no Job points to a Client
    *   i.e. Job.Client is NULL
    */
   query = "SELECT Client.ClientId,Client.Name FROM Client "
           "LEFT OUTER JOIN Job ON (Client.ClientId=Job.ClientId) "
           "WHERE Job.ClientId IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned Client records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT ClientId,Name FROM Client "
                      "WHERE ClientId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_client_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned Client records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Client WHERE ClientId=%u", &id_list);
   }
}

static void eliminate_orphaned_job_records()
{
   const char *query;

   printf(_("Checking for orphaned Job entries.\n"));
   /* In English:
    *   Wiffle through Job for every Job
    *   joining with the Client table including every Job even if
    *   there is not a match in Client (left outer join), then
    *   filter out only those where no Client exists
    *   i.e. Client.Name is NULL
    */
   query = "SELECT Job.JobId,Job.Name FROM Job "
           "LEFT OUTER JOIN Client ON (Job.ClientId=Client.ClientId) "
           "WHERE Client.Name IS NULL";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d orphaned Job records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT JobId,Name,StartTime FROM Job "
                      "WHERE JobId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_job_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d orphaned Job records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Job WHERE JobId=%u", &id_list);
   }
}


static void eliminate_admin_records()
{
   const char *query;

   printf(_("Checking for Admin Job entries.\n"));
   query = "SELECT Job.JobId FROM Job "
           "WHERE Job.Type='D'";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d Admin Job records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT JobId,Name,StartTime FROM Job "
                      "WHERE JobId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_job_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d Admin Job records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Job WHERE JobId=%u", &id_list);
   }
}

static void eliminate_restore_records()
{
   const char *query;

   printf(_("Checking for Restore Job entries.\n"));
   query = "SELECT Job.JobId FROM Job "
           "WHERE Job.Type='R'";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d Restore Job records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (int i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf), "SELECT JobId,Name,StartTime FROM Job "
                      "WHERE JobId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_job_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      printf(_("Deleting %d Restore Job records.\n"), id_list.num_ids);
      delete_id_list("DELETE FROM Job WHERE JobId=%u", &id_list);
   }
}




static void repair_bad_filenames()
{
   const char *query;
   int i;

   printf(_("Checking for Filenames with a trailing slash\n"));
   query = "SELECT FilenameId,Name from Filename "
           "WHERE Name LIKE '%/'";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d bad Filename records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf),
            "SELECT Name FROM Filename WHERE FilenameId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_name_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      POOLMEM *name = get_pool_memory(PM_FNAME);
      char esc_name[5000];
      printf(_("Reparing %d bad Filename records.\n"), id_list.num_ids);
      for (i=0; i < id_list.num_ids; i++) {
         int len;
         bsnprintf(buf, sizeof(buf),
            "SELECT Name FROM Filename WHERE FilenameId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, get_name_handler, name)) {
            printf("%s\n", db_strerror(db));
         }
         /* Strip trailing slash(es) */
         for (len=strlen(name); len > 0 && name[len-1]=='/'; len--)
            {  }
         if (len == 0) {
            len = 1;
            esc_name[0] = ' ';
            esc_name[1] = 0;
         } else {
            name[len-1] = 0;
            db_escape_string(esc_name, name, len);
         }
         bsnprintf(buf, sizeof(buf),
            "UPDATE Filename SET Name='%s' WHERE FilenameId=%u",
            esc_name, id_list.Id[i]);
         if (verbose > 1) {
            printf("%s\n", buf);
         }
         db_sql_query(db, buf, NULL, NULL);
      }
   }
}

static void repair_bad_paths()
{
   const char *query;
   int i;

   printf(_("Checking for Paths without a trailing slash\n"));
   query = "SELECT PathId,Path from Path "
           "WHERE Path NOT LIKE '%/'";
   if (verbose > 1) {
      printf("%s\n", query);
   }
   if (!make_id_list(query, &id_list)) {
      exit(1);
   }
   printf(_("Found %d bad Path records.\n"), id_list.num_ids);
   if (id_list.num_ids && verbose && yes_no(_("Print them? (yes/no): "))) {
      for (i=0; i < id_list.num_ids; i++) {
         bsnprintf(buf, sizeof(buf),
            "SELECT Path FROM Path WHERE PathId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, print_name_handler, NULL)) {
            printf("%s\n", db_strerror(db));
         }
      }
   }

   if (fix && id_list.num_ids > 0) {
      POOLMEM *name = get_pool_memory(PM_FNAME);
      char esc_name[5000];
      printf(_("Reparing %d bad Filename records.\n"), id_list.num_ids);
      for (i=0; i < id_list.num_ids; i++) {
         int len;
         bsnprintf(buf, sizeof(buf),
            "SELECT Path FROM Path WHERE PathId=%u", id_list.Id[i]);
         if (!db_sql_query(db, buf, get_name_handler, name)) {
            printf("%s\n", db_strerror(db));
         }
         /* Strip trailing blanks */
         for (len=strlen(name); len > 0 && name[len-1]==' '; len--) {
            name[len-1] = 0;
         }
         /* Add trailing slash */
         len = pm_strcat(&name, "/");
         db_escape_string(esc_name, name, len);
         bsnprintf(buf, sizeof(buf), "UPDATE Path SET Path='%s' WHERE PathId=%u",
            esc_name, id_list.Id[i]);
         if (verbose > 1) {
            printf("%s\n", buf);
         }
         db_sql_query(db, buf, NULL, NULL);
      }
   }
}


/*
 * Gen next input command from the terminal
 */
static char *get_cmd(const char *prompt)
{
   static char cmd[1000];

   printf("%s", prompt);
   if (fgets(cmd, sizeof(cmd), stdin) == NULL)
      return NULL;
   printf("\n");
   strip_trailing_junk(cmd);
   return cmd;
}

static int yes_no(const char *prompt)
{
   char *cmd;
   cmd = get_cmd(prompt);
   return (strcasecmp(cmd, "yes") == 0) || (strcasecmp(cmd, _("yes")) == 0);
}

bool python_set_prog(JCR*, char const*) { return false; }
