/*
 * Program for determining file system type
 *
 *   Written by Preben 'Peppe' Guldberg, December MMIV
 *
 *   Version $Id: fstype.c,v 1.7 2005/08/10 16:35:37 nboichat Exp $
 *
 */
/*
   Copyright (C) 2004-2005 Kern Sibbald

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
#include "findlib/find.h"

/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) 
   { return 1; }

static void usage()
{
   fprintf(stderr, _(
"\n"
"Usage: fstype [-d debug_level] path ...\n"
"\n"
"       Print the file system type a given file/directory is on.\n"
"       The following options are supported:\n"
"\n"
"       -v     print both path and file system type.\n"
"       -?     print this message.\n"
"\n"));

   exit(1);
}


int
main (int argc, char *const *argv)
{
   char fs[1000];
   int verbose = 0;
   int status = 0;
   int ch, i;

   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   while ((ch = getopt(argc, argv, "v?")) != -1) {
      switch (ch) {
         case 'v':
            verbose = 1;
            break;
         case '?':
         default:
            usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (argc < 1) {
      usage();
   }

   for (i = 0; i < argc; --argc, ++argv) {
      if (fstype(*argv, fs, sizeof(fs))) {
         if (verbose) {
            printf("%s: %s\n", *argv, fs);
         } else {
            puts(fs);
         }
      } else {
         fprintf(stderr, _("%s: unknown\n"), *argv);
         status = 1;
      }
   }

   exit(status);
}
