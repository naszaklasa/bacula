/*
 *  Bacula Event File handling
 *
 *    Kern Sibbald, August MMI
 *
 *   Version $Id: events.c,v 1.7 2005/08/10 16:35:19 nboichat Exp $
 *
 */
/*
   Copyright (C) 2001-2004 Kern Sibbald and John Walker

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

#ifdef IMPLEMENTED
void log_event(UPSINFO *ups, int level, char *fmt, ...)
{
    va_list  arg_ptr;
    char msg[2*MAXSTRING];
    char datetime[MAX_TIME_LENGTH];
    int event_fd = ups->event_fd;

    event_fd = ups->event_fd;

    /*****FIXME***** use pool memory */
    va_start(arg_ptr, fmt);
    vsprintf(msg, fmt, arg_ptr);
    va_end(arg_ptr);

    syslog(level, "%s", msg);         /* log the event */

    /* Write out to our temp file. LOG_INFO is DATA logging, so
       do not write it to our temp events file. */
    if (event_fd >= 0 && level != LOG_INFO) {
	int lm;
	time_t nowtime;
	struct tm tm;

	time(&nowtime);
	localtime_r(&nowtime, &tm);
	strftime(datetime, sizeof(datetime), "%a %b %d %X %Z %Y  ", &tm);
	write(event_fd, datetime, strlen(datetime));
	lm = strlen(msg);
	if (msg[lm-1] != '\n')
	   msg[lm++] = '\n';
	write(event_fd, msg, lm);
    }
}


#define NLE   10		      /* number of events to send and keep */
#define MAXLE 50		      /* truncate file when this many events */

/*
 * If the EVENTS file exceeds MAXLE records, truncate it.
 *
 * Returns:
 *
 *  0 if file not truncated
 *  1 if file truncated
 */
int truncate_events_file(UPSINFO *ups)
{
    char *le[NLE], *buf;
    int i, j;
    int nrec = 0;
    int tlen = 0;
    int trunc = FALSE;
    FILE *events_file;
    int stat = 0;

    if ((events_file = fopen(ups->eventfile, "r+")) == NULL)
	return 0;
    for (i=0; i<NLE; i++)
	le[i] = NULL;
    for (i=0; i<NLE; i++)
	if ((le[i] = malloc(MAXSTRING)) == NULL)
	    goto bailout;
    i = 0;
    while (fgets(le[i], MAXSTRING, events_file) != NULL) {
	nrec++;
	i++;
	if (i >= NLE)		       /* wrap */
	    i = 0;
    }
    if (nrec > MAXLE)
	trunc = TRUE;		       /* file too large, truncate it */
    if (nrec > NLE) {
	nrec = NLE;		       /* number of records to output */
	i -= (i/NLE)*NLE;	       /* first record to output */
    } else
	i = 0;
    /* get total length to output */
    for (j=0; j < nrec; j++)
	tlen += strlen(le[j]);
    if ((buf = malloc(tlen+1)) == NULL)
	goto bailout;
    *buf = 0;
    /* Put records in single buffer in correct order */
    for (j=0; j < nrec; j++) {
	strcat(buf, le[i++]);
	if (i >= NLE)
	    i = 0;
    }
    if (trunc) {
	ftruncate(fileno(events_file), 0L);
	rewind(events_file);
	fwrite(buf, tlen, 1, events_file); /* write last NLE records to file */
	stat = 1;			   /* we truncated it */
    }

    free(buf);

bailout:

   fclose(events_file);
   for (i=0; i<NLE; i++)
       if (le[i] != NULL)
	   free(le[i]);
   return stat;
}

#endif /* IMPLEMENTED */

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)

#include <windows.h>

extern UPSINFO myUPS;
extern int shm_OK;

/*
 * Fill the Events list box with the last events
 *
 */
void FillEventsBox(HWND hwnd, int idlist)
{
    char buf[1000];
    int len;
    FILE *events_file;

    if (!shm_OK || myUPS.eventfile[0] == 0 ||
	(events_file = fopen(myUPS.eventfile, "r")) == NULL) {
	SendDlgItemMessage(hwnd, idlist, LB_ADDSTRING, 0,
	   (LONG)_("Events not available"));
	return;
    }

    while (fgets(buf, sizeof(buf), events_file) != NULL) {
	len = strlen(buf);
	/* strip trailing cr/lfs */
	while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
	    buf[--len] = 0;
	SendDlgItemMessage(hwnd, idlist, LB_ADDSTRING, 0, (LONG)buf);
    }
    return;
}

#endif /* HAVE_CYGWIN */
