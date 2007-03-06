/*
 * File types as returned by find_files()
 *
 *     Kern Sibbald MMI
 */
/*
   Copyright (C) 2001-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifndef __FILES_H
#define __FILES_H

#include "jcr.h"
#include "bfile.h"

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMELEN(dirent) (strlen((dirent)->d_name))
#endif

#include <sys/file.h>
#if HAVE_UTIME_H
#include <utime.h>
#else
struct utimbuf {
    long actime;
    long modtime;
};
#endif

#define MODE_RALL (S_IRUSR|S_IRGRP|S_IROTH)

#include "lib/fnmatch.h"

#ifndef HAVE_REGEX_H
#include "lib/bregex.h"
#else
#include <regex.h>
#endif

#include "save-cwd.h"

#ifndef HAVE_READDIR_R
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
#endif


/*
 * Status codes returned by create_file()
 */
enum {
   CF_SKIP = 1,                       /* skip file (not newer or something) */
   CF_ERROR,                          /* error creating file */
   CF_EXTRACT,                        /* file created, data to extract */
   CF_CREATED                         /* file created, no data to extract */
};


/* Options saved int "options" of the include/exclude lists.
 * They are directly jammed ito  "flag" of ff packet
 */
#define FO_MD5          (1<<1)        /* Do MD5 checksum */
#define FO_GZIP         (1<<2)        /* Do Zlib compression */
#define FO_NO_RECURSION (1<<3)        /* no recursion in directories */
#define FO_MULTIFS      (1<<4)        /* multiple file systems */
#define FO_SPARSE       (1<<5)        /* do sparse file checking */
#define FO_IF_NEWER     (1<<6)        /* replace if newer */
#define FO_NOREPLACE    (1<<7)        /* never replace */
#define FO_READFIFO     (1<<8)        /* read data from fifo */
#define FO_SHA1         (1<<9)        /* Do SHA1 checksum */
#define FO_PORTABLE     (1<<10)       /* Use portable data format -- no BackupWrite */
#define FO_MTIMEONLY    (1<<11)       /* Use mtime rather than mtime & ctime */
#define FO_KEEPATIME    (1<<12)       /* Reset access time */
#define FO_EXCLUDE      (1<<13)       /* Exclude file */
#define FO_ACL          (1<<14)       /* Backup ACLs */
#define FO_NO_HARDLINK  (1<<15)       /* don't handle hard links */
#define FO_IGNORECASE   (1<<16)       /* Ignore file name case */
#define FO_HFSPLUS      (1<<17)       /* Resource forks and Finder Info */
#define FO_WIN32DECOMP  (1<<18)       /* Use BackupRead decomposition */
#define FO_SHA256       (1<<19)       /* Do SHA256 checksum */
#define FO_SHA512       (1<<20)       /* Do SHA512 checksum */
#define FO_ENCRYPT      (1<<21)       /* Encrypt data stream */

struct s_included_file {
   struct s_included_file *next;
   uint32_t options;                  /* backup options */
   int level;                         /* compression level */
   int len;                           /* length of fname */
   int pattern;                       /* set if wild card pattern */
   char VerifyOpts[20];               /* Options for verify */
   char fname[1];
};

struct s_excluded_file {
   struct s_excluded_file *next;
   int len;
   char fname[1];
};

/* FileSet definitions very similar to the resource
 *  contained in the Director because the components
 *  of the structure are passed by the Director to the
 *  File daemon and recompiled back into this structure
 */
#undef  MAX_FOPTS
#define MAX_FOPTS 30

enum {
   state_none,
   state_options,
   state_include,
   state_error
};

/* File options structure */
struct findFOPTS {
   uint32_t flags;                    /* options in bits */
   int GZIP_level;                    /* GZIP level */
   char VerifyOpts[MAX_FOPTS];        /* verify options */
   alist regex;                       /* regex string(s) */
   alist regexdir;                    /* regex string(s) for directories */
   alist regexfile;                   /* regex string(s) for files */
   alist wild;                        /* wild card strings */
   alist wilddir;                     /* wild card strings for directories */
   alist wildfile;                    /* wild card strings for files */
   alist base;                        /* list of base names */
   alist fstype;                      /* file system type limitation */
   char *reader;                      /* reader program */
   char *writer;                      /* writer program */
};


/* This is either an include item or an exclude item */
struct findINCEXE {
   findFOPTS *current_opts;           /* points to current options structure */
   alist opts_list;                   /* options list */
   alist name_list;                   /* filename list -- holds char * */
};

/*
 *   FileSet Resource
 *
 */
struct findFILESET {
   int state;
   findINCEXE *incexe;                /* current item */
   alist include_list;
   alist exclude_list;
};

#ifdef HAVE_DARWIN_OS
struct HFSPLUS_INFO {
   unsigned long length;              /* Mandatory field */
   char fndrinfo[32];                 /* Finder Info */
   off_t rsrclength;                  /* Size of resource fork */
};
#endif

/*
 * Definition of the find_files packet passed as the
 * first argument to the find_files callback subroutine.
 */
struct FF_PKT {
   char *fname;                       /* full filename */
   char *link;                        /* link if file linked */
   POOLMEM *sys_fname;                /* system filename */
   struct stat statp;                 /* stat packet */
   int32_t FileIndex;                 /* FileIndex of this file */
   int32_t LinkFI;                    /* FileIndex of main hard linked file */
   struct f_link *linked;             /* Set if this file is hard linked */
   int type;                          /* FT_ type from above */
   int ff_errno;                      /* errno */
   BFILE bfd;                         /* Bacula file descriptor */
   time_t save_time;                  /* start of incremental time */
   bool dereference;                  /* follow links (not implemented) */
   bool null_output_device;           /* using null output device */
   bool incremental;                  /* incremental save */
   char VerifyOpts[20];
   struct s_included_file *included_files_list;
   struct s_excluded_file *excluded_files_list;
   struct s_excluded_file *excluded_paths_list;
   findFILESET *fileset;
   int (*callback)(FF_PKT *, void *, bool); /* User's callback */

   /* Values set by accept_file while processing Options */
   uint32_t flags;                    /* backup options */
   int GZIP_level;                    /* compression level */
   char *reader;                      /* reader program */
   char *writer;                      /* writer program */
   alist fstypes;                     /* allowed file system types */

   /* List of all hard linked files found */
   struct f_link *linklist;           /* hard linked files */

   /* Darwin specific things.
    * To avoid clutter, we always include rsrc_bfd and volhas_attrlist */
   BFILE rsrc_bfd;                    /* fd for resource forks */
   bool volhas_attrlist;              /* Volume supports getattrlist() */
#ifdef HAVE_DARWIN_OS
   struct HFSPLUS_INFO hfsinfo;       /* Finder Info and resource fork size */
#endif
};


#include "protos.h"

#endif /* __FILES_H */
