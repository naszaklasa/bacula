/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2009 Free Software Foundation Europe e.V.

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
/*
   Derived from a SMTPclient:

  ======== Original copyrights ==========  

       SMTPclient -- simple SMTP client

       Copyright (c) 1997 Ralf S. Engelschall, All rights reserved.

       This program is free software; it may be redistributed and/or modified
       only under the terms of either the Artistic License or the GNU General
       Public License, which may be found in the SMTP source distribution.
       Look at the file COPYING.

       This program is distributed in the hope that it will be useful, but
       WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.

       ======================================================================

       smtpclient_main.c -- program source

       Based on smtp.c as of August 11, 1995 from
           W.Z. Venema,
           Eindhoven University of Technology,
           Department of Mathematics and Computer Science,
           Den Dolech 2, P.O. Box 513, 5600 MB Eindhoven, The Netherlands.

   =========


   Kern Sibbald, July 2001

     Note, the original W.Z. Venema smtp.c had no license and no
     copyright.  See:
        http://archives.neohapsis.com/archives/postfix/2000-05/1520.html
 

   Version $Id$

 */


#include "bacula.h"
#include "jcr.h"
#define MY_NAME "bsmtp"

#if defined(HAVE_WIN32)
#include <lmcons.h>
#endif

/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) 
   { return 1; }

#ifndef MAXSTRING
#define MAXSTRING 254
#endif

static FILE *sfp;
static FILE *rfp;

static char *from_addr = NULL;
static char *cc_addr = NULL;
static char *subject = NULL;
static char *err_addr = NULL;
static const char *mailhost = NULL;
static char *reply_addr = NULL;
static int mailport = 25;
static char my_hostname[MAXSTRING];
static bool content_utf8 = false;

/* 
 * Take input that may have names and other stuff and strip
 *  it down to the mail box address ... i.e. what is enclosed
 *  in < >.  Otherwise add < >.
 */
static char *cleanup_addr(char *addr, char *buf, int buf_len)
{
   char *p, *q;

   if ((p = strchr(addr, '<')) == NULL) {
      snprintf(buf, buf_len, "<%s>", addr);
   } else {
      /* Copy <addr> */
      for (q=buf; *p && *p!='>'; ) {
         *q++ = *p++;
      }
      if (*p) {
         *q++ = *p;
      }
      *q = 0;
  }
  Dmsg2(100, "cleanup in=%s out=%s\n", addr, buf);
  return buf;    
}

/*
 *  examine message from server
 */
static void get_response(void)
{
    char buf[1000];

    Dmsg0(50, "Calling fgets on read socket rfp.\n");
    buf[3] = 0;
    while (fgets(buf, sizeof(buf), rfp)) {
        int len = strlen(buf);
        if (len > 0) {
           buf[len-1] = 0;
        }
        if (debug_level >= 10) {
            fprintf(stderr, "%s <-- %s\n", mailhost, buf);
        }
        Dmsg2(10, "%s --> %s\n", mailhost, buf);
        if (!isdigit((int)buf[0]) || buf[0] > '3') {
            Pmsg2(0, _("Fatal malformed reply from %s: %s\n"), mailhost, buf);
            exit(1);
        }
        if (buf[3] != '-') {
            break;
        }
    }
    if (ferror(rfp)) {
        fprintf(stderr, _("Fatal fgets error: ERR=%s\n"), strerror(errno));
    }
    return;
}

/*
 *  say something to server and check the response
 */
static void chat(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(sfp, fmt, ap);
    va_end(ap);
    if (debug_level >= 10) {
       fprintf(stdout, "%s --> ", my_hostname);
       va_start(ap, fmt);
       vfprintf(stdout, fmt, ap);
       va_end(ap);
    }

    fflush(sfp);
    if (debug_level >= 10) {
       fflush(stdout);
    }
    get_response();
}


static void usage()
{
   fprintf(stderr,
_("\n"
"Usage: %s [-f from] [-h mailhost] [-s subject] [-c copy] [recipient ...]\n"
"       -8          set charset to UTF-8\n"
"       -c          set the Cc: field\n"
"       -d <nn>     set debug level to <nn>\n"
"       -dt         print a timestamp in debug output\n"
"       -f          set the From: field\n"
"       -h          use mailhost:port as the SMTP server\n"
"       -s          set the Subject: field\n"
"       -r          set the Reply-To: field\n"
"       -l          set the maximum number of lines to send (default: unlimited)\n"
"       -?          print this message.\n"
"\n"), MY_NAME);

   exit(1);
}

/*
 * Return the offset west from localtime to UTC in minutes
  * Same as timezone.tz_minuteswest
  *   Unix tz_offset coded by:  Attila Fülöp
  */

static long tz_offset(time_t lnow, struct tm &tm)
{
#if defined(HAVE_WIN32)
#if defined(HAVE_MINGW)
__MINGW_IMPORT long     _dstbias;
#endif
#if defined(MINGW64)
# define _tzset tzset
#endif
   /* Win32 code */
   long offset;
   _tzset();
   offset = _timezone;
   if (tm.tm_isdst) {
      offset += _dstbias;
   }
   return offset /= 60;
#else

   /* Unix/Linux code */
   struct tm tm_utc;
   time_t now = lnow;

   (void)gmtime_r(&now, &tm_utc);
   tm_utc.tm_isdst = tm.tm_isdst;
   return (long)difftime(mktime(&tm_utc), now) / 60;
#endif
}

static void get_date_string(char *buf, int buf_len)
{
   time_t now = time(NULL);
   struct tm tm;
   char tzbuf[MAXSTRING];
   long my_timezone;

   /* Add RFC822 date */
   (void)localtime_r(&now, &tm);

   my_timezone = tz_offset(now, tm);
   strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S", &tm);
   sprintf(tzbuf, " %+2.2ld%2.2u", -my_timezone / 60, abs(my_timezone) % 60);
   strcat(buf, tzbuf);              /* add +0100 */
   strftime(tzbuf, sizeof(tzbuf), " (%Z)", &tm);
   strcat(buf, tzbuf);              /* add (CEST) */
}


/*********************************************************************
 *
 *  Program to send email
 */
int main (int argc, char *argv[])
{
    char buf[1000];
    struct sockaddr_in sin;
    struct hostent *hp;
    int i, ch;
    unsigned long maxlines, lines;
#if defined(HAVE_WIN32)
    SOCKET s;
#else
    int s, r;
    struct passwd *pwd;
#endif
    char *cp, *p;
    
   setlocale(LC_ALL, "en_US");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   my_name_is(argc, argv, "bsmtp");
   maxlines = 0;

   while ((ch = getopt(argc, argv, "8c:d:f:h:r:s:l:?")) != -1) {
      switch (ch) {
      case '8':
         content_utf8 = true;
         break;
      case 'c':
         Dmsg1(20, "cc=%s\n", optarg);
         cc_addr = optarg;
         break;

      case 'd':                    /* set debug level */
         if (*optarg == 't') {
            dbg_timestamp = true;
         } else {
            debug_level = atoi(optarg);
            if (debug_level <= 0) {
               debug_level = 1;
            }
         }
         Dmsg1(20, "Debug level = %d\n", debug_level);
         break;

      case 'f':                    /* from */
         from_addr = optarg;
         break;

      case 'h':                    /* smtp host */
         Dmsg1(20, "host=%s\n", optarg);
         p = strchr(optarg, ':');
         if (p) {
            *p++ = 0;
            mailport = atoi(p);
         }
         mailhost = optarg;
         break;

      case 's':                    /* subject */
         Dmsg1(20, "subject=%s\n", optarg);
         subject = optarg;
         break;

      case 'r':                    /* reply address */
         reply_addr = optarg;
         break;

      case 'l':
         Dmsg1(20, "maxlines=%s\n", optarg);
         maxlines = (unsigned long) atol(optarg);
         break;

      case '?':
      default:
         usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (argc < 1) {
      Pmsg0(0, _("Fatal error: no recipient given.\n"));
      usage();
      exit(1);
   }


   /*
    *  Determine SMTP server
    */
   if (mailhost == NULL) {
      if ((cp = getenv("SMTPSERVER")) != NULL) {
         mailhost = cp;
      } else {
         mailhost = "localhost";
      }
   }

#if defined(HAVE_WIN32)
   WSADATA  wsaData;

   _setmode(0, _O_BINARY);
   WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

   /*
    *  Find out my own host name for HELO;
    *  if possible, get the fully qualified domain name
    */
   if (gethostname(my_hostname, sizeof(my_hostname) - 1) < 0) {
      Pmsg1(0, _("Fatal gethostname error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
   if ((hp = gethostbyname(my_hostname)) == NULL) {
      Pmsg2(0, _("Fatal gethostbyname for myself failed \"%s\": ERR=%s\n"), my_hostname,
         strerror(errno));
      exit(1);
   }
   strcpy(my_hostname, hp->h_name);
   Dmsg1(20, "My hostname is: %s\n", my_hostname);

   /*
    *  Determine from address.
    */
   if (from_addr == NULL) {
#if defined(HAVE_WIN32)
      DWORD dwSize = UNLEN + 1;
      LPSTR lpszBuffer = (LPSTR)alloca(dwSize);

      if (GetUserName(lpszBuffer, &dwSize)) {
         sprintf(buf, "%s@%s", lpszBuffer, my_hostname);
      } else {
         sprintf(buf, "unknown-user@%s", my_hostname);
      }
#else
      if ((pwd = getpwuid(getuid())) == 0) {
         sprintf(buf, "userid-%d@%s", (int)getuid(), my_hostname);
      } else {
         sprintf(buf, "%s@%s", pwd->pw_name, my_hostname);
      }
#endif
      from_addr = bstrdup(buf);
   }
   Dmsg1(20, "From addr=%s\n", from_addr);

   /*
    *  Connect to smtp daemon on mailhost.
    */
hp:
   if ((hp = gethostbyname(mailhost)) == NULL) {
      Pmsg2(0, _("Error unknown mail host \"%s\": ERR=%s\n"), mailhost,
         strerror(errno));
      if (strcasecmp(mailhost, "localhost") != 0) {
         Pmsg0(0, _("Retrying connection using \"localhost\".\n"));
         mailhost = "localhost";
         goto hp;
      }
      exit(1);
   }

   if (hp->h_addrtype != AF_INET) {
      Pmsg1(0, _("Fatal error: Unknown address family for smtp host: %d\n"), hp->h_addrtype);
      exit(1);
   }
   memset((char *)&sin, 0, sizeof(sin));
   memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;
   sin.sin_port = htons(mailport);
#if defined(HAVE_WIN32)
   if ((s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0)) < 0) {
      Pmsg1(0, _("Fatal socket error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
#else
   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      Pmsg1(0, _("Fatal socket error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
#endif
   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      Pmsg2(0, _("Fatal connect error to %s: ERR=%s\n"), mailhost, strerror(errno));
      exit(1);
   }
   Dmsg0(20, "Connected\n");

#if defined(HAVE_WIN32)
   int fdSocket = _open_osfhandle(s, _O_RDWR | _O_BINARY);
   if (fdSocket == -1) {
      Pmsg1(0, _("Fatal _open_osfhandle error: ERR=%s\n"), strerror(errno));
      exit(1);
   }

   int fdSocket2 = dup(fdSocket);

   if ((sfp = fdopen(fdSocket, "wb")) == NULL) {
      Pmsg1(0, _("Fatal fdopen error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
   if ((rfp = fdopen(fdSocket2, "rb")) == NULL) {
      Pmsg1(0, _("Fatal fdopen error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
#else
   if ((r = dup(s)) < 0) {
      Pmsg1(0, _("Fatal dup error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
   if ((sfp = fdopen(s, "w")) == 0) {
      Pmsg1(0, _("Fatal fdopen error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
   if ((rfp = fdopen(r, "r")) == 0) {
      Pmsg1(0, _("Fatal fdopen error: ERR=%s\n"), strerror(errno));
      exit(1);
   }
#endif

   /*
    *  Send SMTP headers.  Note, if any of the strings have a <
    *   in them already, we do not enclose the string in < >, otherwise
    *   we do.
    */
   get_response(); /* banner */
   chat("HELO %s\r\n", my_hostname);
   chat("MAIL FROM:%s\r\n", cleanup_addr(from_addr, buf, sizeof(buf)));
   
   for (i = 0; i < argc; i++) {
      Dmsg1(20, "rcpt to: %s\n", argv[i]);
      chat("RCPT TO:%s\r\n", cleanup_addr(argv[i], buf, sizeof(buf)));
   }

   if (cc_addr) {
      chat("RCPT TO:%s\r\n", cleanup_addr(cc_addr, buf, sizeof(buf)));
   }
   Dmsg0(20, "Data\n");
   chat("DATA\r\n");

   /*
    *  Send message header
    */
   fprintf(sfp, "From: %s\r\n", from_addr);
   Dmsg1(10, "From: %s\r\n", from_addr);
   if (subject) {
      fprintf(sfp, "Subject: %s\r\n", subject);
      Dmsg1(10, "Subject: %s\r\n", subject);
   }
   if (reply_addr) {
      fprintf(sfp, "Reply-To: %s\r\n", reply_addr);
      Dmsg1(10, "Reply-To: %s\r\n", reply_addr);
   }
   if (err_addr) {
      fprintf(sfp, "Errors-To: %s\r\n", err_addr);
      Dmsg1(10, "Errors-To: %s\r\n", err_addr);
   }

#if defined(HAVE_WIN32)
   DWORD dwSize = UNLEN + 1;
   LPSTR lpszBuffer = (LPSTR)alloca(dwSize);

   if (GetUserName(lpszBuffer, &dwSize)) {
      fprintf(sfp, "Sender: %s@%s\r\n", lpszBuffer, my_hostname);
      Dmsg2(10, "Sender: %s@%s\r\n", lpszBuffer, my_hostname);
   } else {
      fprintf(sfp, "Sender: unknown-user@%s\r\n", my_hostname);
      Dmsg1(10, "Sender: unknown-user@%s\r\n", my_hostname);
   }
#else
   if ((pwd = getpwuid(getuid())) == 0) {
      fprintf(sfp, "Sender: userid-%d@%s\r\n", (int)getuid(), my_hostname);
      Dmsg2(10, "Sender: userid-%d@%s\r\n", (int)getuid(), my_hostname);
   } else {
      fprintf(sfp, "Sender: %s@%s\r\n", pwd->pw_name, my_hostname);
      Dmsg2(10, "Sender: %s@%s\r\n", pwd->pw_name, my_hostname);
   }
#endif

   fprintf(sfp, "To: %s", argv[0]);
   Dmsg1(10, "To: %s", argv[0]);
   for (i = 1; i < argc; i++) {
      fprintf(sfp, ",%s", argv[i]);
      Dmsg1(10, ",%s", argv[i]);
   }

   fprintf(sfp, "\r\n");
   Dmsg0(10, "\r\n");
   if (cc_addr) {
      fprintf(sfp, "Cc: %s\r\n", cc_addr);
      Dmsg1(10, "Cc: %s\r\n", cc_addr);
   }

   if (content_utf8) {
      fprintf(sfp, "Content-Type: text/plain; charset=UTF-8\r\n");
      Dmsg0(10, "Content-Type: text/plain; charset=UTF-8\r\n");
   }

   get_date_string(buf, sizeof(buf));
   fprintf(sfp, "Date: %s\r\n", buf);
   Dmsg1(10, "Date: %s\r\n", buf);

   fprintf(sfp, "\r\n");

   /*
    *  Send message body
    */
   lines = 0;
   while (fgets(buf, sizeof(buf), stdin)) {
      if (maxlines > 0 && ++lines > maxlines) {
         Dmsg1(20, "skip line because of maxlines limit: %lu\n", maxlines);
         while (fgets(buf, sizeof(buf), stdin)) {
            ++lines;
         }
         break;
      }
      buf[sizeof(buf)-1] = '\0';
      buf[strlen(buf)-1] = '\0';
      if (buf[0] == '.') {         /* add extra . see RFC 2821 4.5.2 */
         fputs(".", sfp);
      }
      fputs(buf, sfp);
      fputs("\r\n", sfp);
   }

   if (lines > maxlines) {
      Dmsg1(10, "hit maxlines limit: %lu\n", maxlines);
      fprintf(sfp, "\r\n\r\n[maximum of %lu lines exceeded, skipped %lu lines of output]\r\n", maxlines, lines-maxlines);
   }

   /*
    *  Send SMTP quit command
    */
   chat(".\r\n");
   chat("QUIT\r\n");

   /*
    *  Go away gracefully ...
    */
   exit(0);
}
