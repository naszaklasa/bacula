/*
 * Test program for testing regular expressions.
 */
/*
   Copyright (C) 2006 Kern Sibbald

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
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif


static void usage()
{
   fprintf(stderr,
"\n"
"Usage: regex [-d debug_level] -f <data-file>\n"
"       -f          specify file of data to be matched\n"
"       -l          suppress line numbers\n"
"       -n          print lines that do not match\n"
"       -?          print this message.\n"
"\n\n");

   exit(1);
}


int main(int argc, char *const *argv)
{
#ifndef HAVE_REGEX_H
   printf("The regex libraries don't seem to be available.\n");
   exit(1);
#else
   regex_t preg;
   char prbuf[500];
   char *fname = NULL;
   int rc, ch;
   char data[1000];
   char pat[500];
   FILE *fd;
   bool match_only = true;
   int lineno;
   bool no_linenos = false;
   

   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   while ((ch = getopt(argc, argv, "d:f:n?")) != -1) {
      switch (ch) {
      case 'd':                       /* set debug level */
         debug_level = atoi(optarg);
         if (debug_level <= 0) {
            debug_level = 1;
         }
         break;

      case 'f':                       /* data */
         fname = optarg;
         break;

      case 'l':
         no_linenos = true;
         break;

      case 'n':
         match_only = false;
         break;

      case '?':
      default:
         usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (!fname) {
      printf("A data file must be specified.\n");
      usage();
   }

   for ( ;; ) {
      printf("Enter regex pattern: ");
      if (fgets(pat, sizeof(pat)-1, stdin) == NULL) {
         break;
      }
      strip_trailing_newline(pat);
      if (pat[0] == 0) {
         exit(0);
      }
      rc = regcomp(&preg, pat, REG_EXTENDED);
      if (rc != 0) {
         regerror(rc, &preg, prbuf, sizeof(prbuf));
         printf("Regex compile error: %s\n", prbuf);
         continue;
      }
      fd = fopen(fname, "r");
      if (!fd) {
         printf(_("Could not open data file: %s\n"), fname);
         exit(1);
      }
      lineno = 0;
      while (fgets(data, sizeof(data)-1, fd)) {
         const int nmatch = 30;
         regmatch_t pmatch[nmatch];
         strip_trailing_newline(data);
         lineno++;
         rc = regexec(&preg, data, nmatch, pmatch,  0);
         if ((match_only && rc == 0) || (!match_only && rc != 0)) {
            if (no_linenos) {
               printf("%s\n", data);
            } else {
               printf("%5d: %s\n", lineno, data);
            }
         }
      }
      fclose(fd);
      regfree(&preg);
   }
   exit(0);
#endif
}
