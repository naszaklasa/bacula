/*
 * Test program for testing regular expressions.
 *
 *  Kern Sibbald, MMVI
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2006-2006 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

/*
 *  If you define BACULA_REGEX, bregex will be built with the
 *  Bacula bregex library, which is the same code that we
 *  use on Win32, thus using Linux, you can test your Win32
 *  expressions. Otherwise, this program will link with the
 *  system library routines.
 */
//#define BACULA_REGEX

#include "bacula.h"
#include <stdio.h>
#include "lib/breg.h"


static void usage()
{
   fprintf(stderr,
"\n"
"Usage: bregex [-d debug_level] -f <data-file> -e /test/test2/\n"
"       -f          specify file of data to be matched\n"
"       -e          specify expression\n"
"       -?          print this message.\n"
"\n");

   exit(1);
}


int main(int argc, char *const *argv)
{
   regex_t preg;
   char prbuf[500];
   char *fname = NULL;
   char *expr = NULL;
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

   while ((ch = getopt(argc, argv, "d:f:e:")) != -1) {
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

      case 'e':
         expr = optarg;
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

   if (!expr) {
      printf("An expression must be specified.\n");
      usage();
   }

   OSDependentInit();

   BREGEXP *reg;
   
   reg = new_bregexp(expr);
   
   if (!reg) {
      printf("Can't use %s as 'sed' expression\n", expr);
      exit (1);
   }

   fd = fopen(fname, "r");
   if (!fd) {
      printf(_("Could not open data file: %s\n"), fname);
      exit(1);
   }

   while (fgets(data, sizeof(data)-1, fd)) {
      strip_trailing_newline(data);
      reg->replace(data);
      printf("%s\n", reg->result);
   }
   fclose(fd);
   free_bregexp(reg);
   exit(0);
}
/*
  TODO: 
   - ajout /g

   - tests 
   * test avec /i (visiblement il ne marche pas sur bregexp.c)
   * test avec un sed et faire un diff
   * test avec une boucle pour voir les fuites
   * tester les cas possibles pour la compilation d'une expression
     - manque le depart, le milieu, la fin, utilise des groupes sans
       reference...

*/
