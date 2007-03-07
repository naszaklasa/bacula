/*
 * Main routine for finding files on a file system.
 *  The heart of the work to find the files on the
 *    system is done in find_one.c. Here we have the
 *    higher level control as well as the matching
 *    routines for the new syntax Options resource.
 *
 *  Kern E. Sibbald, MM
 *
 *   Version $Id: find.c 4116 2007-02-06 14:37:57Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

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


#include "bacula.h"
#include "find.h"

int32_t name_max;              /* filename max length */
int32_t path_max;              /* path name max length */

#ifdef DEBUG
#undef bmalloc
#define bmalloc(x) sm_malloc(__FILE__, __LINE__, x)
#endif
static int our_callback(FF_PKT *ff, void *hpkt, bool top_level);
static bool accept_file(FF_PKT *ff);

static const int fnmode = 0;

/*
 * Initialize the find files "global" variables
 */
FF_PKT *init_find_files()
{
  FF_PKT *ff;

  ff = (FF_PKT *)bmalloc(sizeof(FF_PKT));
  memset(ff, 0, sizeof(FF_PKT));

  ff->sys_fname = get_pool_memory(PM_FNAME);

   /* Get system path and filename maximum lengths */
   path_max = pathconf(".", _PC_PATH_MAX);
   if (path_max < 1024) {
      path_max = 1024;
   }

   name_max = pathconf(".", _PC_NAME_MAX);
   if (name_max < 1024) {
      name_max = 1024;
   }
   path_max++;                        /* add for EOS */
   name_max++;                        /* add for EOS */

  Dmsg1(100, "init_find_files ff=%p\n", ff);
  return ff;
}

/*
 * Set find_files options. For the moment, we only
 * provide for full/incremental saves, and setting
 * of save_time. For additional options, see above
 */
void
set_find_options(FF_PKT *ff, int incremental, time_t save_time)
{
  Dmsg0(100, "Enter set_find_options()\n");
  ff->incremental = incremental;
  ff->save_time = save_time;
  Dmsg0(100, "Leave set_find_options()\n");
}

/*
 * For VSS we need to know which windows drives
 * are used, because we create a snapshot of all used
 * drives before operation
 *
 * the function returns the number of used drives and
 * fills "drives" with up to 26 (A..Z) drive names
 *
 */
int
get_win32_driveletters(FF_PKT *ff, char* szDrives)
{
   /* szDrives must be at least 27 bytes long */

#if !defined(HAVE_WIN32)
   return 0;
#endif

   szDrives[0] = 0; /* make empty */
   int nCount = 0;
    
   findFILESET *fileset = ff->fileset;
   if (fileset) {
      int i;
      char *fname;
      
      for (i=0; i<fileset->include_list.size(); i++) {
         findINCEXE *incexe = (findINCEXE *)fileset->include_list.get(i);
         
         /* look through all files and check */
         foreach_alist(fname, &incexe->name_list) {
            /* fname should match x:/ */
            if (strlen(fname) >= 2 && B_ISALPHA(fname[0]) 
               && fname[1] == ':') {
               
               /* always add in uppercase */
               char ch = toupper(fname[0]);
               /* if not found in string, add drive letter */
               if (!strchr(szDrives,ch)) {
                  szDrives[nCount] = ch;
                  szDrives[nCount+1] = 0;
                  nCount++;
               }                                
            }            
         }
      }
   }
   return nCount;
}

/*
 * Find all specified files (determined by calls to name_add()
 * This routine calls the (handle_file) subroutine with all
 * sorts of good information for the final disposition of
 * the file.
 *
 * Call this subroutine with a callback subroutine as the first
 * argument and a packet as the second argument, this packet
 * will be passed back to the callback subroutine as the last
 * argument.
 *
 * The callback subroutine gets called with:
 *  arg1 -- the FF_PKT containing filename, link, stat, ftype, flags, etc
 *  arg2 -- the user supplied packet
 *
 */
int
find_files(JCR *jcr, FF_PKT *ff, int callback(FF_PKT *ff_pkt, void *hpkt, bool top_level), 
           void *his_pkt)
{
   ff->callback = callback;

   /* This is the new way */
   findFILESET *fileset = ff->fileset;
   if (fileset) {
      int i, j;
      ff->flags = 0;
      ff->VerifyOpts[0] = 'V';
      ff->VerifyOpts[1] = 0;
      for (i=0; i<fileset->include_list.size(); i++) {
         findINCEXE *incexe = (findINCEXE *)fileset->include_list.get(i);
         fileset->incexe = incexe;
         /*
          * By setting all options, we in effect or the global options
          *   which is what we want.
          */
         for (j=0; j<incexe->opts_list.size(); j++) {
            findFOPTS *fo = (findFOPTS *)incexe->opts_list.get(j);
            ff->flags |= fo->flags;
            ff->GZIP_level = fo->GZIP_level;
            ff->fstypes = fo->fstype;
            ff->drivetypes = fo->drivetype;
            bstrncat(ff->VerifyOpts, fo->VerifyOpts, sizeof(ff->VerifyOpts));
         }
         char *fname;
         foreach_alist(fname, &incexe->name_list) {
            Dmsg1(100, "F %s\n", fname);
            ff->top_fname = fname;
            if (find_one_file(jcr, ff, our_callback, his_pkt, ff->top_fname, (dev_t)-1, true) == 0) {
               return 0;                  /* error return */
            }
         }
      }
   }
   return 1;
}

static bool accept_file(FF_PKT *ff)
{
   int i, j, k;
   int fnm_flags;
   findFILESET *fileset = ff->fileset;
   findINCEXE *incexe = fileset->incexe;
   const char *basename;
   int (*match_func)(const char *pattern, const char *string, int flags);

   if (ff->flags & FO_ENHANCEDWILD) {
      match_func = enh_fnmatch;
      if ((basename = last_path_separator(ff->fname)) != NULL)
         basename++;
      else
         basename = ff->fname;
   } else {
      match_func = fnmatch;
      basename = ff->fname;
   }

   for (j = 0; j < incexe->opts_list.size(); j++) {
      findFOPTS *fo = (findFOPTS *)incexe->opts_list.get(j);
      ff->flags = fo->flags;
      ff->GZIP_level = fo->GZIP_level;
      ff->reader = fo->reader;
      ff->writer = fo->writer;
      ff->fstypes = fo->fstype;
      ff->drivetypes = fo->drivetype;

      fnm_flags = (ff->flags & FO_IGNORECASE) ? FNM_CASEFOLD : 0;
      fnm_flags |= (ff->flags & FO_ENHANCEDWILD) ? FNM_PATHNAME : 0;

      if (S_ISDIR(ff->statp.st_mode)) {
         for (k=0; k<fo->wilddir.size(); k++) {
            if (match_func((char *)fo->wilddir.get(k), ff->fname, fnmode|fnm_flags) == 0) {
               if (ff->flags & FO_EXCLUDE) {
                  Dmsg2(100, "Exclude wilddir: %s file=%s\n", (char *)fo->wilddir.get(k),
                     ff->fname);
                  return false;       /* reject dir */
               }
               return true;           /* accept dir */
            }
         }
      } else {
         for (k=0; k<fo->wildfile.size(); k++) {
            if (match_func((char *)fo->wildfile.get(k), ff->fname, fnmode|fnm_flags) == 0) {
               if (ff->flags & FO_EXCLUDE) {
                  Dmsg2(100, "Exclude wildfile: %s file=%s\n", (char *)fo->wildfile.get(k),
                     ff->fname);
                  return false;       /* reject file */
               }
               return true;           /* accept file */
            }
         }

         for (k=0; k<fo->wildbase.size(); k++) {
            if (match_func((char *)fo->wildbase.get(k), basename, fnmode|fnm_flags) == 0) {
               if (ff->flags & FO_EXCLUDE) {
                  Dmsg2(100, "Exclude wildbase: %s file=%s\n", (char *)fo->wildbase.get(k),
                     basename);
                  return false;       /* reject file */
               }
               return true;           /* accept file */
            }
         }
      }
      for (k=0; k<fo->wild.size(); k++) {
         if (match_func((char *)fo->wild.get(k), ff->fname, fnmode|fnm_flags) == 0) {
            if (ff->flags & FO_EXCLUDE) {
               Dmsg2(100, "Exclude wild: %s file=%s\n", (char *)fo->wild.get(k),
                  ff->fname);
               return false;          /* reject file */
            }
            return true;              /* accept file */
         }
      }
      if (S_ISDIR(ff->statp.st_mode)) {
         for (k=0; k<fo->regexdir.size(); k++) {
            const int nmatch = 30;
            regmatch_t pmatch[nmatch];
            if (regexec((regex_t *)fo->regexdir.get(k), ff->fname, nmatch, pmatch,  0) == 0) {
               if (ff->flags & FO_EXCLUDE) {
                  return false;       /* reject file */
               }
               return true;           /* accept file */
            }
         }
      } else {
         for (k=0; k<fo->regexfile.size(); k++) {
            const int nmatch = 30;
            regmatch_t pmatch[nmatch];
            if (regexec((regex_t *)fo->regexfile.get(k), ff->fname, nmatch, pmatch,  0) == 0) {
               if (ff->flags & FO_EXCLUDE) {
                  return false;       /* reject file */
               }
               return true;           /* accept file */
            }
         }
      }
      for (k=0; k<fo->regex.size(); k++) {
         const int nmatch = 30;
         regmatch_t pmatch[nmatch];
         if (regexec((regex_t *)fo->regex.get(k), ff->fname, nmatch, pmatch,  0) == 0) {
            if (ff->flags & FO_EXCLUDE) {
               return false;          /* reject file */
            }
            return true;              /* accept file */
         }
      }
      /*
       * If we have an empty Options clause with exclude, then
       *  exclude the file
       */
      if (ff->flags & FO_EXCLUDE &&
          fo->regex.size() == 0     && fo->wild.size() == 0 &&
          fo->regexdir.size() == 0  && fo->wilddir.size() == 0 &&
          fo->regexfile.size() == 0 && fo->wildfile.size() == 0 &&
          fo->wildbase.size() == 0) {
         return false;              /* reject file */
      }
   }

   /* Now apply the Exclude { } directive */
   for (i=0; i<fileset->exclude_list.size(); i++) {
      findINCEXE *incexe = (findINCEXE *)fileset->exclude_list.get(i);
      for (j=0; j<incexe->opts_list.size(); j++) {
         findFOPTS *fo = (findFOPTS *)incexe->opts_list.get(j);
         fnm_flags = (fo->flags & FO_IGNORECASE) ? FNM_CASEFOLD : 0;
         for (k=0; k<fo->wild.size(); k++) {
            if (fnmatch((char *)fo->wild.get(k), ff->fname, fnmode|fnm_flags) == 0) {
               Dmsg1(100, "Reject wild1: %s\n", ff->fname);
               return false;          /* reject file */
            }
         }
      }
      fnm_flags = (incexe->current_opts != NULL && incexe->current_opts->flags & FO_IGNORECASE)
             ? FNM_CASEFOLD : 0;
      char *fname;
      foreach_alist(fname, &incexe->name_list) {
         if (fnmatch(fname, ff->fname, fnmode|fnm_flags) == 0) {
            Dmsg1(100, "Reject wild2: %s\n", ff->fname);
            return false;          /* reject file */
         }
      }
   }
   return true;
}

/*
 * The code comes here for each file examined.
 * We filter the files, then call the user's callback if
 *    the file is included.
 */
static int our_callback(FF_PKT *ff, void *hpkt, bool top_level)
{
   if (top_level) {
      return ff->callback(ff, hpkt, top_level);   /* accept file */
   }
   switch (ff->type) {
   case FT_NOACCESS:
   case FT_NOFOLLOW:
   case FT_NOSTAT:
   case FT_NOCHG:
   case FT_ISARCH:
   case FT_NORECURSE:
   case FT_NOFSCHG:
   case FT_INVALIDFS:
   case FT_INVALIDDT:
   case FT_NOOPEN:
//    return ff->callback(ff, hpkt, top_level);

   /* These items can be filtered */
   case FT_LNKSAVED:
   case FT_REGE:
   case FT_REG:
   case FT_LNK:
   case FT_DIRBEGIN:
   case FT_DIREND:
   case FT_RAW:
   case FT_FIFO:
   case FT_SPEC:
   case FT_DIRNOCHG:
      if (accept_file(ff)) {
//       Dmsg2(000, "Accept file %s; reader=%s\n", ff->fname, NPRT(ff->reader));
         return ff->callback(ff, hpkt, top_level);
      } else {
         Dmsg1(100, "Skip file %s\n", ff->fname);
         return -1;                   /* ignore this file */
      }

   default:
      Dmsg1(000, "Unknown FT code %d\n", ff->type);
      return 0;
   }
}


/*
 * Terminate find_files() and release
 * all allocated memory
 */
int
term_find_files(FF_PKT *ff)
{
  int hard_links;

  free_pool_memory(ff->sys_fname);
  hard_links = term_find_one(ff);
  free(ff);
  return hard_links;
}
