/*
   Derived from a SMTPclient:

       SMTPclient -- simple SMTP client

       Copyright (C) 1997 Ralf S. Engelschall, All Rights Reserved.
       rse@engelschall.com
       www.engelschall.com

   Kern Sibbald, July 2001

   Version $Id: bsmtp.c,v 1.21 2006/11/22 14:26:39 kerns Exp $

 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2001-2006 Free Software Foundation Europe e.V.

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


/*
 *  examine message from server
 */
static void get_response(void)
{
    char buf[MAXSTRING];

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
    if (debug_level >= 10) {
       fprintf(stdout, "%s --> ", my_hostname);
       vfprintf(stdout, fmt, ap);
    }
    va_end(ap);

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
"       -c          set the Cc: field\n"
"       -dnn        set debug level to nn\n"
"       -f          set the From: field\n"
"       -h          use mailhost:port as the SMTP server\n"
"       -s          set the Subject: field\n"
"       -r          set the Reply-To: field\n"
"       -l          set the maximum number of lines that should be sent (default: unlimited)\n"
"       -?          print this message.\n"
"\n"), MY_NAME);

   exit(1);
}


/*********************************************************************
 *
 *  Program to send email
 */
int main (int argc, char *argv[])
{
    char buf[MAXSTRING];
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
    time_t now = time(NULL);
    struct tm tm;
    
   setlocale(LC_ALL, "en_US");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   my_name_is(argc, argv, "bsmtp");
   maxlines = 0;

   while ((ch = getopt(argc, argv, "c:d:f:h:r:s:l:?")) != -1) {
      switch (ch) {
      case 'c':
         Dmsg1(20, "cc=%s\n", optarg);
         cc_addr = optarg;
         break;

      case 'd':                    /* set debug level */
         debug_level = atoi(optarg);
         if (debug_level <= 0) {
            debug_level = 1;
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

#if defined(HAVE_WIN32)
   _setmode(0, _O_BINARY);
#endif

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
    *  Send SMTP headers
    */
   get_response(); /* banner */
   chat("helo %s\r\n", my_hostname);
   chat("mail from:<%s>\r\n", from_addr);

   for (i = 0; i < argc; i++) {
      Dmsg1(20, "rcpt to: %s\n", argv[i]);
      chat("rcpt to:<%s>\r\n", argv[i]);
   }

   if (cc_addr) {
      chat("rcpt to:<%s>\r\n", cc_addr);
   }
   Dmsg0(20, "Data\n");
   chat("data\r\n");

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

   /* Add RFC822 date */
   (void)localtime_r(&now, &tm);
#if defined(HAVE_WIN32)
#if defined(HAVE_MINGW)
__MINGW_IMPORT long     _dstbias;
#endif
   long tzoffset = 0;

   _tzset();

   tzoffset = _timezone;
   tzoffset += _dstbias;
   tzoffset /= 60;

   size_t length = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S", &tm);
   sprintf(&buf[length], " %+2.2ld%2.2u", -tzoffset / 60, abs(tzoffset) % 60);
#else
   strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", &tm);
#endif
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
         break;
      }
      buf[sizeof(buf)-1] = '\0';
      buf[strlen(buf)-1] = '\0';
      if (buf[0] == '.' && buf[1] == '\0') { /* quote lone dots */
         fputs("..\r\n", sfp);
      } else {                     /* pass body through unchanged */
         fputs(buf, sfp);
         fputs("\r\n", sfp);
      }
   }

   if (lines > maxlines) {
      Dmsg1(10, "hit maxlines limit: %lu\n", maxlines);
      fprintf(sfp, "\r\n[maximum of %lu lines exceeded, skipped %lu lines of output]\r\n", maxlines, lines-maxlines);
   }

   /*
    *  Send SMTP quit command
    */
   chat(".\r\n");
   chat("quit\r\n");

   /*
    *  Go away gracefully ...
    */
   exit(0);
}
