/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2002-2008 Free Software Foundation Europe e.V.

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
 *  This file contains all the SQL commands issued by the Director
 *
 *     Kern Sibbald, July MMII
 *
 *   Version $Id$
 */
/*
 * Note, PostgreSQL imposes some constraints on using DISTINCT and GROUP BY
 *  for example, the following is illegal in PostgreSQL:
 *  SELECT DISTINCT JobId FROM temp ORDER BY StartTime ASC;
 *  because all the ORDER BY expressions must appear in the SELECT list!
 */


#include "bacula.h"
#include "cats.h"

const char *cleanup_created_job =
   "UPDATE Job SET JobStatus='f', StartTime=SchedTime, EndTime=SchedTime "
   "WHERE JobStatus = 'C'";
const char *cleanup_running_job = 
   "UPDATE Job SET JobStatus='f', EndTime=StartTime WHERE JobStatus = 'R'";

/* For sql_update.c db_update_stats */
const char *fill_jobhisto =
        "INSERT INTO JobHisto (" 
           "JobId, Job, Name, Type, Level, ClientId, JobStatus, "
           "SchedTime, StartTime, EndTime, RealEndTime, JobTDate, "
           "VolSessionId, VolSessionTime, JobFiles, JobBytes, ReadBytes, "
           "JobErrors, JobMissingFiles, PoolId, FileSetId, PriorJobId, "
           "PurgedFiles, HasBase ) "
        "SELECT " 
           "JobId, Job, Name, Type, Level, ClientId, JobStatus, "
           "SchedTime, StartTime, EndTime, RealEndTime, JobTDate, "
           "VolSessionId, VolSessionTime, JobFiles, JobBytes, ReadBytes, "
           "JobErrors, JobMissingFiles, PoolId, FileSetId, PriorJobId, "
           "PurgedFiles, HasBase "
          "FROM Job "
         "WHERE JobStatus IN ('T','W','f','A','E') "
           "AND JobId NOT IN (SELECT JobId FROM JobHisto) "
           "AND JobTDate < %s ";

/* For ua_update.c */
const char *list_pool = "SELECT * FROM Pool WHERE PoolId=%s";

/* For ua_dotcmds.c */
const char *client_backups =
   "SELECT DISTINCT Job.JobId,Client.Name as Client,Level,StartTime,"
   "JobFiles,JobBytes,VolumeName,MediaType,FileSet,Media.Enabled as Enabled"
   " FROM Client,Job,JobMedia,Media,FileSet"
   " WHERE Client.Name='%s'"
   " AND FileSet='%s'"
   " AND Client.ClientId=Job.ClientId"
   " AND JobStatus IN ('T','W') AND Type='B'" 
   " AND JobMedia.JobId=Job.JobId AND JobMedia.MediaId=Media.MediaId"
   " AND Job.FileSetId=FileSet.FileSetId"
   " ORDER BY Job.StartTime";


/* ====== ua_prune.c */

const char *del_File     = "DELETE FROM File WHERE JobId=%s";
const char *upd_Purged   = "UPDATE Job Set PurgedFiles=1 WHERE JobId=%s";
const char *cnt_DelCand  = "SELECT count(*) FROM DelCandidates";
const char *del_Job      = "DELETE FROM Job WHERE JobId=%s";
const char *del_JobMedia = "DELETE FROM JobMedia WHERE JobId=%s";
const char *cnt_JobMedia = "SELECT count(*) FROM JobMedia WHERE MediaId=%s";

const char *sel_JobMedia = 
   "SELECT DISTINCT JobMedia.JobId FROM JobMedia,Job "
   "WHERE MediaId=%s AND Job.JobId=JobMedia.JobId "
   "AND Job.JobTDate<%s";

/* Count Select JobIds for File deletion */
const char *count_select_job = 
   "SELECT count(*) from Job "
   "WHERE JobTDate<%s "
   "AND ClientId=%s "
   "AND PurgedFiles=0";


/* Select JobIds for File deletion. */
const char *select_job =
   "SELECT DISTINCT JobId from Job "
   "WHERE JobTDate<%s "
   "AND ClientId=%s "
   "AND PurgedFiles=0";

/* Delete temp tables and indexes  */
const char *drop_deltabs[] = {
   "DROP TABLE DelCandidates",
   NULL};

const char *create_delindex = "CREATE INDEX DelInx1 ON DelCandidates (JobId)";

/* Fill candidates table with all Jobs subject to being deleted.
 *  This is used for pruning Jobs (first the files, then the Jobs).
 */
const char *insert_delcand =
   "INSERT INTO DelCandidates "
   "SELECT JobId,PurgedFiles,FileSetId,JobFiles,JobStatus FROM Job "
   "WHERE Type='%c' "
   "AND JobTDate<%s "
   "AND ClientId=%s";

/*
 * Select Jobs from the DelCandidates table that have a
 * more recent backup -- i.e. are not the only backup.
 * This is the list of Jobs to delete for a Backup Job.
 * At the same time, we select "orphanned" jobs
 * (i.e. no files, ...) for deletion.
 */
#ifdef old_way
const char *select_backup_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTDate<%s AND ((DelCandidates.JobFiles=0) OR "
   "(DelCandidates.JobStatus NOT IN ('T','W')))) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Level='F' AND Job.JobStatus IN ('T','W') AND Job.Type IN ('B','M') "
   "AND Job.FileSetId=DelCandidates.FileSetId)";

/* Select Jobs from the DelCandidates table that have a
 * more recent InitCatalog -- i.e. are not the only InitCatalog
 * This is the list of Jobs to delete for a Verify Job.
 */
const char *select_verify_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTdate<%s AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='V' AND Job.Level='V' AND Job.JobStatus IN ('T','W') "
   "AND Job.FileSetId=DelCandidates.FileSetId)";


/* Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for a Restore Job.
 */
const char *select_restore_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTdate<%s AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='R')";

/* Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for an Admin Job.
 */
const char *select_admin_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTdate<%s AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='D')";

/*
 * Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for an Migrate Job.
 */
const char *select_migrate_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTdate<%s AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='g')";

/*
 * Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for an Copy Job.
 */
const char *select_copy_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobTdate<%s AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='C')";


#else
/* Faster way */
const char *select_backup_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND ((DelCandidates.JobFiles=0) OR "
   "(DelCandidates.JobStatus NOT IN ('T','W')))) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Level='F' AND Job.JobStatus IN ('T','W') AND Job.Type IN ('B','M') "
   "AND Job.FileSetId=DelCandidates.FileSetId)";

/* Select Jobs from the DelCandidates table that have a
 * more recent InitCatalog -- i.e. are not the only InitCatalog
 * This is the list of Jobs to delete for a Verify Job.
 */
const char *select_verify_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='V' AND Job.Level='V' AND Job.JobStatus IN ('T','W') "
   "AND Job.FileSetId=DelCandidates.FileSetId)";


/* Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for a Restore Job.
 */
const char *select_restore_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='R')";

/* Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for an Admin Job.
 */
const char *select_admin_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='D')";

/*
 * Select Jobs from the DelCandidates table.
 * This is the list of Jobs to delete for an Migrate Job.
 */
const char *select_migrate_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='g')";

const char *select_copy_del =
   "SELECT DISTINCT DelCandidates.JobId,DelCandidates.PurgedFiles "
   "FROM Job,DelCandidates "
   "WHERE (Job.JobId=DelCandidates.JobId AND DelCandidates.JobStatus NOT IN ('T','W')) OR "
   "(Job.JobTDate>%s "
   "AND Job.ClientId=%s "
   "AND Job.Type='C')";


#endif

/* ======= ua_restore.c */
const char *uar_count_files =
   "SELECT JobFiles FROM Job WHERE JobId=%s";

/* List last 20 Jobs */
const char *uar_list_jobs =
   "SELECT JobId,Client.Name as Client,StartTime,Level as "
   "JobLevel,JobFiles,JobBytes "
   "FROM Client,Job WHERE Client.ClientId=Job.ClientId AND JobStatus IN ('T','W') "
   "AND Type='B' ORDER BY StartTime DESC LIMIT 20";

/*
 * Find all files for a particular JobId and insert them into
 *  the tree during a restore.
 */
const char *uar_sel_files =
   "SELECT Path.Path,Filename.Name,FileIndex,JobId,LStat "
   "FROM File,Filename,Path "
   "WHERE File.JobId IN (%s) AND Filename.FilenameId=File.FilenameId "
   "AND Path.PathId=File.PathId";

const char *uar_del_temp  = "DROP TABLE temp";
const char *uar_del_temp1 = "DROP TABLE temp1";

const char *uar_last_full =
   "INSERT INTO temp1 SELECT Job.JobId,JobTdate "
   "FROM Client,Job,JobMedia,Media,FileSet WHERE Client.ClientId=%s "
   "AND Job.ClientId=%s "
   "AND Job.StartTime < '%s' "
   "AND Level='F' AND JobStatus IN ('T','W') AND Type='B' "
   "AND JobMedia.JobId=Job.JobId "
   "AND Media.Enabled=1 "
   "AND JobMedia.MediaId=Media.MediaId "
   "AND Job.FileSetId=FileSet.FileSetId "
   "AND FileSet.FileSet='%s' "
   "%s"
   "ORDER BY Job.JobTDate DESC LIMIT 1";

const char *uar_full =
   "INSERT INTO temp SELECT Job.JobId,Job.JobTDate,"
   "Job.ClientId,Job.Level,Job.JobFiles,Job.JobBytes,"
   "StartTime,VolumeName,JobMedia.StartFile,VolSessionId,VolSessionTime "
   "FROM temp1,Job,JobMedia,Media WHERE temp1.JobId=Job.JobId "
   "AND Level='F' AND JobStatus IN ('T','W') AND Type='B' "
   "AND Media.Enabled=1 "
   "AND JobMedia.JobId=Job.JobId "
   "AND JobMedia.MediaId=Media.MediaId";

const char *uar_dif =
   "INSERT INTO temp SELECT Job.JobId,Job.JobTDate,Job.ClientId,"
   "Job.Level,Job.JobFiles,Job.JobBytes,"
   "Job.StartTime,Media.VolumeName,JobMedia.StartFile,"
   "Job.VolSessionId,Job.VolSessionTime "
   "FROM Job,JobMedia,Media,FileSet "
   "WHERE Job.JobTDate>%s AND Job.StartTime < '%s' "
   "AND Job.ClientId=%s "
   "AND JobMedia.JobId=Job.JobId "
   "AND Media.Enabled=1 "
   "AND JobMedia.MediaId=Media.MediaId "
   "AND Job.Level='D' AND JobStatus IN ('T','W') AND Type='B' "
   "AND Job.FileSetId=FileSet.FileSetId "
   "AND FileSet.FileSet='%s' "
   "%s"
   "ORDER BY Job.JobTDate DESC LIMIT 1";

const char *uar_inc =
   "INSERT INTO temp SELECT Job.JobId,Job.JobTDate,Job.ClientId,"
   "Job.Level,Job.JobFiles,Job.JobBytes,"
   "Job.StartTime,Media.VolumeName,JobMedia.StartFile,"
   "Job.VolSessionId,Job.VolSessionTime "
   "FROM Job,JobMedia,Media,FileSet "
   "WHERE Job.JobTDate>%s AND Job.StartTime < '%s' "
   "AND Job.ClientId=%s "
   "AND Media.Enabled=1 "
   "AND JobMedia.JobId=Job.JobId "
   "AND JobMedia.MediaId=Media.MediaId "
   "AND Job.Level='I' AND JobStatus IN ('T','W') AND Type='B' "
   "AND Job.FileSetId=FileSet.FileSetId "
   "AND FileSet.FileSet='%s' "
   "%s";

const char *uar_list_temp =
   "SELECT DISTINCT JobId,Level,JobFiles,JobBytes,StartTime,VolumeName"
   " FROM temp"
   " ORDER BY StartTime ASC";


const char *uar_sel_jobid_temp = 
   "SELECT DISTINCT JobId,StartTime FROM temp ORDER BY StartTime ASC";

const char *uar_sel_all_temp1 = "SELECT * FROM temp1";

const char *uar_sel_all_temp = "SELECT * FROM temp";



/* Select FileSet names for this Client */
const char *uar_sel_fileset =
   "SELECT DISTINCT FileSet.FileSet FROM Job,"
   "Client,FileSet WHERE Job.FileSetId=FileSet.FileSetId "
   "AND Job.ClientId=%s AND Client.ClientId=%s "
   "ORDER BY FileSet.FileSet";

/* Find MediaType used by this Job */
const char *uar_mediatype =
   "SELECT MediaType FROM JobMedia,Media WHERE JobMedia.JobId=%s "
   "AND JobMedia.MediaId=Media.MediaId";

/*
 *  Find JobId, FileIndex for a given path/file and date
 *  for use when inserting individual files into the tree.
 */
const char *uar_jobid_fileindex =
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId=File.JobId "
   "AND Job.StartTime<='%s' "
   "AND Path.Path='%s' "
   "AND Filename.Name='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.PathId "
   "AND Filename.FilenameId=File.FilenameId "
   "AND JobStatus IN ('T','W') AND Type='B' "
   "ORDER BY Job.StartTime DESC LIMIT 1";

const char *uar_jobids_fileindex =
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId IN (%s) "
   "AND Job.JobId=File.JobId "
   "AND Job.StartTime<='%s' "
   "AND Path.Path='%s' "
   "AND Filename.Name='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.PathId "
   "AND Filename.FilenameId=File.FilenameId "
   "ORDER BY Job.StartTime DESC LIMIT 1";

/* Query to get list of files from table -- presuably built by an external program */
const char *uar_jobid_fileindex_from_table = 
   "SELECT JobId,FileIndex from %s";


/*
 *
 *  This file contains all the SQL commands issued by the Director
 *
 *     Kern Sibbald, July MMII
 *
 *   Version $Id$
 */
/*
 * Note, PostgreSQL imposes some constraints on using DISTINCT and GROUP BY
 *  for example, the following is illegal in PostgreSQL:
 *  SELECT DISTINCT JobId FROM temp ORDER BY StartTime ASC;
 *  because all the ORDER BY expressions must appear in the SELECT list!
 */


#include "bacula.h"
#include "cats.h"

/* ====== ua_prune.c */

/* List of SQL commands to create temp table and indicies  */
const char *create_deltabs[4] = {
   /* MySQL */
   "CREATE TEMPORARY TABLE DelCandidates ("
   "JobId INTEGER UNSIGNED NOT NULL, "
   "PurgedFiles TINYINT, "
   "FileSetId INTEGER UNSIGNED, "
   "JobFiles INTEGER UNSIGNED, "
   "JobStatus BINARY(1))",
   /* Postgresql */
   "CREATE TEMPORARY TABLE DelCandidates (" 
   "JobId INTEGER NOT NULL, "
   "PurgedFiles SMALLINT, "
   "FileSetId INTEGER, "
   "JobFiles INTEGER, "
   "JobStatus char(1))",
   /* SQLite */
   "CREATE TEMPORARY TABLE DelCandidates ("
   "JobId INTEGER UNSIGNED NOT NULL, "
   "PurgedFiles TINYINT, "
   "FileSetId INTEGER UNSIGNED, "
   "JobFiles INTEGER UNSIGNED, "
   "JobStatus CHAR)",
   /* SQLite3 */
   "CREATE TEMPORARY TABLE DelCandidates ("
   "JobId INTEGER UNSIGNED NOT NULL, "
   "PurgedFiles TINYINT, "
   "FileSetId INTEGER UNSIGNED, "
   "JobFiles INTEGER UNSIGNED, "
   "JobStatus CHAR)"};

/* ======= ua_restore.c */

/* List Jobs where a particular file is saved */
const char *uar_file[4] = {
   /* Mysql */
   "SELECT Job.JobId as JobId,"
   "CONCAT(Path.Path,Filename.Name) as Name, "
   "StartTime,Type as JobType,JobStatus,JobFiles,JobBytes "
   "FROM Client,Job,File,Filename,Path WHERE Client.Name='%s' "
   "AND Client.ClientId=Job.ClientId "
   "AND Job.JobId=File.JobId AND File.FileIndex > 0 "
   "AND Path.PathId=File.PathId AND Filename.FilenameId=File.FilenameId "
   "AND Filename.Name='%s' ORDER BY StartTime DESC LIMIT 20",
   /* Postgresql */
   "SELECT Job.JobId as JobId,"
   "Path.Path||Filename.Name as Name, "
   "StartTime,Type as JobType,JobStatus,JobFiles,JobBytes "
   "FROM Client,Job,File,Filename,Path WHERE Client.Name='%s' "
   "AND Client.ClientId=Job.ClientId "
   "AND Job.JobId=File.JobId AND File.FileIndex > 0 "
   "AND Path.PathId=File.PathId AND Filename.FilenameId=File.FilenameId "
   "AND Filename.Name='%s' ORDER BY StartTime DESC LIMIT 20",
   /* SQLite */
   "SELECT Job.JobId as JobId,"
   "Path.Path||Filename.Name as Name, "
   "StartTime,Type as JobType,JobStatus,JobFiles,JobBytes "
   "FROM Client,Job,File,Filename,Path WHERE Client.Name='%s' "
   "AND Client.ClientId=Job.ClientId "
   "AND Job.JobId=File.JobId AND File.FileIndex > 0 "
   "AND Path.PathId=File.PathId AND Filename.FilenameId=File.FilenameId "
   "AND Filename.Name='%s' ORDER BY StartTime DESC LIMIT 20",
   /* SQLite3 */
   "SELECT Job.JobId as JobId,"
   "Path.Path||Filename.Name as Name, "
   "StartTime,Type as JobType,JobStatus,JobFiles,JobBytes "
   "FROM Client,Job,File,Filename,Path WHERE Client.Name='%s' "
   "AND Client.ClientId=Job.ClientId "
   "AND Job.JobId=File.JobId AND File.FileIndex > 0 "
   "AND Path.PathId=File.PathId AND Filename.FilenameId=File.FilenameId "
   "AND Filename.Name='%s' ORDER BY StartTime DESC LIMIT 20"};

const char *uar_create_temp[4] = {
   /* Mysql */
   "CREATE TEMPORARY TABLE temp ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED,"
   "ClientId INTEGER UNSIGNED,"
   "Level CHAR,"
   "JobFiles INTEGER UNSIGNED,"
   "JobBytes BIGINT UNSIGNED,"
   "StartTime TEXT,"
   "VolumeName TEXT,"
   "StartFile INTEGER UNSIGNED,"
   "VolSessionId INTEGER UNSIGNED,"
   "VolSessionTime INTEGER UNSIGNED)",
   /* Postgresql */
   "CREATE TEMPORARY TABLE temp ("
   "JobId INTEGER NOT NULL,"
   "JobTDate BIGINT,"
   "ClientId INTEGER,"
   "Level CHAR,"
   "JobFiles INTEGER,"
   "JobBytes BIGINT,"
   "StartTime TEXT,"
   "VolumeName TEXT,"
   "StartFile INTEGER,"
   "VolSessionId INTEGER,"
   "VolSessionTime INTEGER)",
   /* SQLite */
   "CREATE TEMPORARY TABLE temp ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED,"
   "ClientId INTEGER UNSIGNED,"
   "Level CHAR,"
   "JobFiles INTEGER UNSIGNED,"
   "JobBytes BIGINT UNSIGNED,"
   "StartTime TEXT,"
   "VolumeName TEXT,"
   "StartFile INTEGER UNSIGNED,"
   "VolSessionId INTEGER UNSIGNED,"
   "VolSessionTime INTEGER UNSIGNED)",
   /* SQLite3 */
   "CREATE TEMPORARY TABLE temp ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED,"
   "ClientId INTEGER UNSIGNED,"
   "Level CHAR,"
   "JobFiles INTEGER UNSIGNED,"
   "JobBytes BIGINT UNSIGNED,"
   "StartTime TEXT,"
   "VolumeName TEXT,"
   "StartFile INTEGER UNSIGNED,"
   "VolSessionId INTEGER UNSIGNED,"
   "VolSessionTime INTEGER UNSIGNED)"};

const char *uar_create_temp1[4] = {
   /* Mysql */
   "CREATE TEMPORARY TABLE temp1 ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED)",
   /* Postgresql */
   "CREATE TEMPORARY TABLE temp1 ("
   "JobId INTEGER NOT NULL,"
   "JobTDate BIGINT)",
   /* SQLite */
   "CREATE TEMPORARY TABLE temp1 ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED)",
   /* SQLite3 */
   "CREATE TEMPORARY TABLE temp1 ("
   "JobId INTEGER UNSIGNED NOT NULL,"
   "JobTDate BIGINT UNSIGNED)"};

/* Query to get all files in a directory -- no recursing   
 *  Note, for PostgreSQL since it respects the "Single Value
 *  rule", the results of the SELECT will be unoptimized.
 *  I.e. the same file will be restored multiple times, once
 *  for each time it was backed up.
 */

const char *uar_jobid_fileindex_from_dir[4] = {
   /* Mysql */
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId IN (%s) "
   "AND Job.JobId=File.JobId "
   "AND Path.Path='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.Pathid "
   "AND Filename.FilenameId=File.FilenameId "
   "GROUP BY File.FileIndex ",
   /* Postgresql */
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId IN (%s) "
   "AND Job.JobId=File.JobId "
   "AND Path.Path='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.Pathid "
   "AND Filename.FilenameId=File.FilenameId",
   /* SQLite */
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId IN (%s) "
   "AND Job.JobId=File.JobId "
   "AND Path.Path='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.Pathid "
   "AND Filename.FilenameId=File.FilenameId "
   "GROUP BY File.FileIndex ",
   /* SQLite3 */
   "SELECT Job.JobId,File.FileIndex FROM Job,File,Path,Filename,Client "
   "WHERE Job.JobId IN (%s) "
   "AND Job.JobId=File.JobId "
   "AND Path.Path='%s' "
   "AND Client.Name='%s' "
   "AND Job.ClientId=Client.ClientId "
   "AND Path.PathId=File.Pathid "
   "AND Filename.FilenameId=File.FilenameId "
   "GROUP BY File.FileIndex "};
