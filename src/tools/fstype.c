/*
 * Program for determining file system type
 *
 *   Written by Preben 'Peppe' Guldberg, December MMIV
 *
 *   Version $Id: fstype.c,v 1.4.2.1 2005/02/14 10:09:53 kerns Exp $
 *
 */

/*
   Copyright (C) 2004 Kern Sibbald

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
#include "findlib/find.h"

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
         fprintf(stderr, "%s: unknown\n", *argv);
	 status = 1;
      }
   }

   exit(status);
}
