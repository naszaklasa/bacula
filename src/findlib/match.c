/*
 *  Routines used to keep and match include and exclude
 *   filename/pathname patterns.
 *
 *  Note, this file is used for the old style include and
 *   excludes, so is deprecated. The new style code is
 *   found in find.c
 *
 *   Kern E. Sibbald, December MMI
 *
 */
/*
   Copyright (C) 2001-2004 Kern Sibbald

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

#include "bacula.h"
#include "find.h"

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR 0
#endif

/* Fold case in fnmatch() on Win32 */
#ifdef WIN32
static const int fnmode = FNM_CASEFOLD;
#else
static const int fnmode = 0;
#endif


#undef bmalloc
#define bmalloc(x) sm_malloc(__FILE__, __LINE__, x)

extern const int win32_client;

/*
 * Initialize structures for filename matching
 */
void init_include_exclude_files(FF_PKT *ff)
{
}

/*
 * Done doing filename matching, release all
 *  resources used.
 */
void term_include_exclude_files(FF_PKT *ff)
{
   struct s_included_file *inc, *next_inc;
   struct s_excluded_file *exc, *next_exc;

   for (inc=ff->included_files_list; inc; ) {
      next_inc = inc->next;
      free(inc);
      inc = next_inc;
   }
   ff->included_files_list = NULL;

   for (exc=ff->excluded_files_list; exc; ) {
      next_exc = exc->next;
      free(exc);
      exc = next_exc;
   }
   ff->excluded_files_list = NULL;

   for (exc=ff->excluded_paths_list; exc; ) {
      next_exc = exc->next;
      free(exc);
      exc = next_exc;
   }
   ff->excluded_paths_list = NULL;
}

/*
 * Add a filename to list of included files
 */
void add_fname_to_include_list(FF_PKT *ff, int prefixed, const char *fname)
{
   int len, j;
   struct s_included_file *inc;
   char *p;
   const char *rp;

   len = strlen(fname);

   inc =(struct s_included_file *)bmalloc(sizeof(struct s_included_file) + len + 1);
   inc->options = 0;
   inc->VerifyOpts[0] = 'V';
   inc->VerifyOpts[1] = ':';
   inc->VerifyOpts[2] = 0;

   /* prefixed = preceded with options */
   if (prefixed) {
      for (rp=fname; *rp && *rp != ' '; rp++) {
	 switch (*rp) {
         case 'a':                 /* alway replace */
         case '0':                 /* no option */
	    break;
         case 'f':
	    inc->options |= FO_MULTIFS;
	    break;
         case 'h':                 /* no recursion */
	    inc->options |= FO_NO_RECURSION;
	    break;
         case 'M':                 /* MD5 */
	    inc->options |= FO_MD5;
	    break;
         case 'n':
	    inc->options |= FO_NOREPLACE;
	    break;
         case 'p':                 /* use portable data format */
	    inc->options |= FO_PORTABLE;
	    break;
         case 'r':                 /* read fifo */
	    inc->options |= FO_READFIFO;
	    break;
         case 'S':
	    inc->options |= FO_SHA1;
	    break;
         case 's':
	    inc->options |= FO_SPARSE;
	    break;
         case 'm':
	    inc->options |= FO_MTIMEONLY;
	    break;
         case 'k':
	    inc->options |= FO_KEEPATIME;
	    break;
         case 'V':                  /* verify options */
	    /* Copy Verify Options */
            for (j=0; *rp && *rp != ':'; rp++) {
	       inc->VerifyOpts[j] = *rp;
	       if (j < (int)sizeof(inc->VerifyOpts) - 1) {
		  j++;
	       }
	    }
	    inc->VerifyOpts[j] = 0;
	    break;
         case 'w':
	    inc->options |= FO_IF_NEWER;
	    break;
         case 'A':
	    inc->options |= FO_ACL;
	    break;
         case 'Z':                 /* gzip compression */
	    inc->options |= FO_GZIP;
            inc->level = *++rp - '0';
            Dmsg1(200, "Compression level=%d\n", inc->level);
	    break;
	 default:
            Emsg1(M_ERROR, 0, "Unknown include/exclude option: %c\n", *rp);
	    break;
	 }
      }
      /* Skip past space(s) */
      for ( ; *rp == ' '; rp++)
	 {}
   } else {
      rp = fname;
   }

   strcpy(inc->fname, rp);
   p = inc->fname;
   len = strlen(p);
   /* Zap trailing slashes.  */
   p += len - 1;
   while (p > inc->fname && *p == '/') {
      *p-- = 0;
      len--;
   }
   inc->len = len;
   /* Check for wild cards */
   inc->pattern = 0;
   for (p=inc->fname; *p; p++) {
      if (*p == '*' || *p == '[' || *p == '?') {
	 inc->pattern = 1;
	 break;
      }
   }
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
   /* Convert any \'s into /'s */
   for (p=inc->fname; *p; p++) {
      if (*p == '\\') {
         *p = '/';
      }
   }
#endif
   inc->next = NULL;
   /* Chain this one on the end of the list */
   if (!ff->included_files_list) {
      /* First one, so set head */
      ff->included_files_list = inc;
   } else {
      struct s_included_file *next;
      /* Walk to end of list */
      for (next=ff->included_files_list; next->next; next=next->next)
	 { }
      next->next = inc;
   }
   Dmsg1(50, "add_fname_to_include fname=%s\n", inc->fname);
}

/*
 * We add an exclude name to either the exclude path
 *  list or the exclude filename list.
 */
void add_fname_to_exclude_list(FF_PKT *ff, const char *fname)
{
   int len;
   struct s_excluded_file *exc, **list;

   Dmsg1(20, "Add name to exclude: %s\n", fname);

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
   if (strchr(fname, '/') || strchr(fname, '\\')) {
#else
   if (strchr(fname, '/')) {
#endif
      list = &ff->excluded_paths_list;
   } else {
      list = &ff->excluded_files_list;
   }

   len = strlen(fname);

   exc = (struct s_excluded_file *)bmalloc(sizeof(struct s_excluded_file) + len + 1);
   exc->next = *list;
   exc->len = len;
   strcpy(exc->fname, fname);
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
   /* Convert any \'s into /'s */
   for (char *p=exc->fname; *p; p++) {
      if (*p == '\\') {
         *p = '/';
      }
   }
#endif
   *list = exc;
}


/*
 * Get next included file
 */
struct s_included_file *get_next_included_file(FF_PKT *ff, struct s_included_file *ainc)
{
   struct s_included_file *inc;

   if (ainc == NULL) {
      inc = ff->included_files_list;
   } else {
      inc = ainc->next;
   }
   /*
    * copy inc_options for this file into the ff packet
    */
   if (inc) {
      ff->flags = inc->options;
      ff->GZIP_level = inc->level;
   }
   return inc;
}

/*
 * Walk through the included list to see if this
 *  file is included possibly with wild-cards.
 */

int file_is_included(FF_PKT *ff, const char *file)
{
   struct s_included_file *inc = ff->included_files_list;
   int len;

   for ( ; inc; inc=inc->next ) {
      if (inc->pattern) {
	 if (fnmatch(inc->fname, file, fnmode|FNM_LEADING_DIR) == 0) {
	    return 1;
	 }
	 continue;
      }
      /*
       * No wild cards. We accept a match to the
       *  end of any component.
       */
      Dmsg2(900, "pat=%s file=%s\n", inc->fname, file);
      len = strlen(file);
      if (inc->len == len && strcmp(inc->fname, file) == 0) {
	 return 1;
      }
      if (inc->len < len && file[inc->len] == '/' &&
	  strncmp(inc->fname, file, inc->len) == 0) {
	 return 1;
      }
      if (inc->len == 1 && inc->fname[0] == '/') {
	 return 1;
      }
   }
   return 0;
}


/*
 * This is the workhorse of excluded_file().
 * Determine if the file is excluded or not.
 */
static int
file_in_excluded_list(struct s_excluded_file *exc, const char *file)
{
   if (exc == NULL) {
      Dmsg0(900, "exc is NULL\n");
   }
   for ( ; exc; exc=exc->next ) {
      if (fnmatch(exc->fname, file, fnmode|FNM_PATHNAME) == 0) {
         Dmsg2(900, "Match exc pat=%s: file=%s:\n", exc->fname, file);
	 return 1;
      }
      Dmsg2(900, "No match exc pat=%s: file=%s:\n", exc->fname, file);
   }
   return 0;
}


/*
 * Walk through the excluded lists to see if this
 *  file is excluded, or if it matches a component
 *  of an excluded directory.
 */

int file_is_excluded(FF_PKT *ff, const char *file)
{
   const char *p;

   /*
    *  ***NB*** this removes the drive from the exclude
    *  rule.  Why?????
    */
   if (win32_client && file[1] == ':') {
      file += 2;
   }

   if (file_in_excluded_list(ff->excluded_paths_list, file)) {
      return 1;
   }

   /* Try each component */
   for (p = file; *p; p++) {
      /* Match from the beginning of a component only */
      if ((p == file || (*p != '/' && *(p-1) == '/'))
	   && file_in_excluded_list(ff->excluded_files_list, p)) {
	 return 1;
      }
   }
   return 0;
}
