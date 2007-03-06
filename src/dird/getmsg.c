/*
 *
 *   Bacula Director -- routines to receive network data and
 *    handle network signals. These routines handle the connections
 *    to the Storage daemon and the File daemon.
 *
 *     Kern Sibbald, August MM
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *    Handle  network signals (signals).  
 *	 Signals always have return status 0 from bnet_recv() and
 *	 a zero or negative message length.
 *    Pass appropriate messages back to the caller (responses).  
 *	 Responses always have a digit as the first character.
 *    Handle requests for message and catalog services (requests).  
 *	 Requests are any message that does not begin with a digit.
 *	 In affect, they are commands.
 *
 *   Version $Id: getmsg.c,v 1.19.8.1 2005/02/14 10:02:21 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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
#include "dird.h"

/* Forward referenced functions */
static char *find_msg_start(char *msg);

static char Job_status[] = "Status Job=%127s JobStatus=%d\n";

static char OK_msg[] = "1000 OK\n";

/*
 * Get a message
 *  Call appropriate processing routine
 *  If it is not a Jmsg or a ReqCat message,
 *   return it to the caller.
 *
 *  This routine is called to get the next message from
 *  another daemon. If the message is in canonical message
 *  format and the type is known, it will be dispatched
 *  to the appropriate handler.  If the message is
 *  in any other format, it will be returned. 
 *
 *  E.g. any message beginning with a digit will be returned.
 *	 any message beginning with Jmsg will be processed.
 *
 */
int bget_dirmsg(BSOCK *bs)
{
   int32_t n;
   char Job[MAX_NAME_LENGTH];
   char MsgType[20];
   int type, level;
   JCR *jcr;
   char *msg;

   for (;;) {
      n = bnet_recv(bs);
      Dmsg2(120, "bget_dirmsg %d: %s\n", n, bs->msg);

      if (is_bnet_stop(bs)) {
	 return n;		      /* error or terminate */
      }
      if (n == BNET_SIGNAL) {	       /* handle signal */
	 /* BNET_SIGNAL (-1) return from bnet_recv() => network signal */
	 switch (bs->msglen) {
	 case BNET_EOD: 	   /* end of data */
	    return n;
	 case BNET_EOD_POLL:
	    bnet_fsend(bs, OK_msg);/* send response */
	    return n;		   /* end of data */
	 case BNET_TERMINATE:
	    bs->terminated = 1;
	    return n;
	 case BNET_POLL:
	    bnet_fsend(bs, OK_msg); /* send response */
	    break;
	 case BNET_HEARTBEAT:
//	    encode_time(time(NULL), Job);
//          Dmsg1(100, "%s got heartbeat.\n", Job);
	    break;
	 case BNET_HB_RESPONSE:
	    break;
	 case BNET_STATUS:
	    /* *****FIXME***** Implement more completely */
            bnet_fsend(bs, "Status OK\n");
	    bnet_sig(bs, BNET_EOD);
	    break;
	 case BNET_BTIME:	      /* send Bacula time */
	    char ed1[50];
            bnet_fsend(bs, "btime %s\n", edit_uint64(get_current_btime(),ed1));
	    break;
	 default:
            Emsg1(M_WARNING, 0, _("bget_dirmsg: unknown bnet signal %d\n"), bs->msglen);
	    return n;
	 }
	 continue;
      }
     
      /* Handle normal data */

      if (n > 0 && B_ISDIGIT(bs->msg[0])) {	 /* response? */
	 return n;		      /* yes, return it */
      }
	
      /*
       * If we get here, it must be a request.	Either
       *  a message to dispatch, or a catalog request.
       *  Try to fulfill it.
       */
      if (sscanf(bs->msg, "%020s Job=%127s ", MsgType, Job) != 2) {
         Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
	 continue;
      }
      if (!(jcr=get_jcr_by_full_name(Job))) {
         Emsg1(M_ERROR, 0, _("Job not found: %s\n"), bs->msg);
	 continue;
      }
      Dmsg1(200, "Getmsg got jcr 0x%x\n", jcr);

      /* Skip past "Jmsg Job=nnn" */
      if (!(msg=find_msg_start(bs->msg))) {
         Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
	 free_jcr(jcr);
	 continue;
      }

      /* 
       * Here we are expecting a message of the following format:
       *   Jmsg Job=nnn type=nnn level=nnn Message-string
       */
      if (bs->msg[0] == 'J') {           /* Job message */
         if (sscanf(bs->msg, "Jmsg Job=%127s type=%d level=%d", 
		    Job, &type, &level) != 3) {
            Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
	    free_jcr(jcr);
	    continue;
	 }
         Dmsg1(120, "Got msg: %s\n", bs->msg);
	 skip_spaces(&msg);
	 skip_nonspaces(&msg);	      /* skip type=nnn */
	 skip_spaces(&msg);
	 skip_nonspaces(&msg);	      /* skip level=nnn */
         if (*msg == ' ') {
	    msg++;		      /* skip leading space */
	 }
         Dmsg1(120, "Dispatch msg: %s", msg);
	 dispatch_message(jcr, type, level, msg);
	 free_jcr(jcr);
	 continue;
      }
      /* 
       * Here we expact a CatReq message
       *   CatReq Job=nn Catalog-Request-Message
       */
      if (bs->msg[0] == 'C') {        /* Catalog request */
         Dmsg2(120, "Catalog req jcr 0x%x: %s", jcr, bs->msg);
	 catalog_request(jcr, bs, msg);
         Dmsg1(200, "Calling freejcr 0x%x\n", jcr);
	 free_jcr(jcr);
	 continue;
      }
      if (bs->msg[0] == 'U') {        /* Catalog update */
         Dmsg2(120, "Catalog upd jcr 0x%x: %s", jcr, bs->msg);
	 catalog_update(jcr, bs, msg);
         Dmsg1(200, "Calling freejcr 0x%x\n", jcr);
	 free_jcr(jcr);
	 continue;
      }
      if (bs->msg[0] == 'M') {        /* Mount request */
         Dmsg1(120, "Mount req: %s", bs->msg);
	 mount_request(jcr, bs, msg);
	 free_jcr(jcr);
	 continue;
      }
      if (bs->msg[0] == 'S') {       /* Status change */
	int JobStatus;
	char Job[MAX_NAME_LENGTH];
	if (sscanf(bs->msg, Job_status, &Job, &JobStatus) == 2) {
	   jcr->SDJobStatus = JobStatus; /* current status */
	   free_jcr(jcr);
	   continue;
	}
      }
      return n;
   }
}

static char *find_msg_start(char *msg)
{
   char *p = msg;

   skip_nonspaces(&p);		      /* skip message type */
   skip_spaces(&p);
   skip_nonspaces(&p);		      /* skip Job */
   skip_spaces(&p);		      /* after spaces come the message */
   return p;
}

/*
 * Get response from File daemon to a command we
 * sent. Check that the response agrees with what we expect.
 *
 *  Returns: 0 on failure
 *	     1 on success
 */
int response(JCR *jcr, BSOCK *fd, char *resp, const char *cmd, e_prtmsg prtmsg)
{
   int n;

   if (is_bnet_error(fd)) {
      return 0;
   }
   if ((n = bget_dirmsg(fd)) >= 0) {
      Dmsg0(110, fd->msg);
      if (strcmp(fd->msg, resp) == 0) {
	 return 1;
      }
      if (prtmsg == DISPLAY_ERROR) {
         Jmsg(jcr, M_FATAL, 0, _("FD gave bad response to %s command: wanted %s got: %s\n"),
	    cmd, resp, fd->msg);
      }
      return 0;
   } 
   Jmsg(jcr, M_FATAL, 0, _("Socket error from Filed on %s command: ERR=%s\n"),
	 cmd, bnet_strerror(fd));
   return 0;
}
