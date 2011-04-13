/*
 * Program for determining drive type
 *
 *   Written by Robert Nelson, June 2006
 *
 *   Version $Id$
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2006-2006 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
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
"Usage: drivetype [-v] path ...\n"
"\n"
"       Print the drive type a given file/directory is on.\n"
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
   char dt[100];
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

   OSDependentInit();
 
   for (i = 0; i < argc; --argc, ++argv) {
      if (drivetype(*argv, dt, sizeof(dt))) {
         if (verbose) {
            printf("%s: %s\n", *argv, dt);
         } else {
            puts(dt);
         }
      } else {
         fprintf(stderr, _("%s: unknown\n"), *argv);
         status = 1;
      }
   }

   exit(status);
}
