/*
 * Bacula Sock Structure definition
 *
 * Kern Sibbald, May MM
 *
 * Zero msglen from other end indicates soft eof (usually
 *   end of some binary data stream, but not end of conversation).
 *
 * Negative msglen, is special "signal" (no data follows).
 *   See below for SIGNAL codes.
 *
 *   Version $Id: bsock.h 3668 2006-11-21 13:20:11Z kerns $
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

struct BSOCK {
   uint64_t read_seqno;               /* read sequence number */
   uint32_t in_msg_no;                /* input message number */
   uint32_t out_msg_no;               /* output message number */
   int fd;                            /* socket file descriptor */
   TLS_CONNECTION *tls;               /* associated tls connection */
   int32_t msglen;                    /* message length */
   int b_errno;                       /* bsock errno */
   int port;                          /* desired port */
   int blocking;                      /* blocking state (0 = nonblocking, 1 = blocking) */
   volatile int errors;               /* incremented for each error on socket */
   volatile bool suppress_error_msgs: 1; /* set to suppress error messages */
   volatile bool timed_out: 1;        /* timed out in read/write */
   volatile bool terminated: 1;       /* set when BNET_TERMINATE arrives */
   bool duped: 1;                     /* set if duped BSOCK */
   bool spool: 1;                     /* set for spooling */
   volatile time_t timer_start;       /* time started read/write */
   volatile time_t timeout;           /* timeout BSOCK after this interval */
   POOLMEM *msg;                      /* message pool buffer */
   char *who;                         /* Name of daemon to which we are talking */
   char *host;                        /* Host name/IP */
   POOLMEM *errmsg;                   /* edited error message */
   RES *res;                          /* Resource to which we are connected */
   BSOCK *next;                       /* next BSOCK if duped */
   FILE *spool_fd;                    /* spooling file */
   JCR *jcr;                          /* jcr or NULL for error msgs */
   struct sockaddr client_addr;    /* client's IP address */
   struct sockaddr_in peer_addr;    /* peer's IP address */
};

/* Signal definitions for use in bnet_sig() */
enum {
   BNET_EOD            = -1,          /* End of data stream, new data may follow */
   BNET_EOD_POLL       = -2,          /* End of data and poll all in one */
   BNET_STATUS         = -3,          /* Send full status */
   BNET_TERMINATE      = -4,          /* Conversation terminated, doing close() */
   BNET_POLL           = -5,          /* Poll request, I'm hanging on a read */
   BNET_HEARTBEAT      = -6,          /* Heartbeat Response requested */
   BNET_HB_RESPONSE    = -7,          /* Only response permited to HB */
   BNET_PROMPT         = -8,          /* Prompt for UA */
   BNET_BTIME          = -9,          /* Send UTC btime */
   BNET_BREAK          = -10,         /* Stop current command -- ctl-c */
   BNET_START_SELECT   = -11,         /* Start of a selection list */
   BNET_END_SELECT     = -12          /* End of a select list */
};

#define BNET_SETBUF_READ  1           /* Arg for bnet_set_buffer_size */
#define BNET_SETBUF_WRITE 2           /* Arg for bnet_set_buffer_size */

/* Return status from bnet_recv() */
#define BNET_SIGNAL  -1
#define BNET_HARDEOF -2
#define BNET_ERROR   -3

/*
 * TLS enabling values. Value is important for comparison, ie:
 * if (tls_remote_need < BNET_TLS_REQUIRED) { ... }
 */
#define BNET_TLS_NONE     0           /* cannot do TLS */
#define BNET_TLS_OK       1           /* can do, but not required on my end */
#define BNET_TLS_REQUIRED 2           /* TLS is required */
