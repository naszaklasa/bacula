/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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
 /*
  * Originally written by Kern Sibbald for inclusion in apcupsd,
  *  but heavily modified for Bacula
  *
  *   Version $Id: bnet_server.c,v 1.33.4.2 2005/02/27 21:53:28 kerns Exp $
  */

#include "bacula.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef HAVE_LIBWRAP
#include "tcpd.h"
int allow_severity = LOG_NOTICE;
int deny_severity = LOG_WARNING;
#endif

static bool quit = false;

void bnet_stop_thread_server(pthread_t tid)
{
   quit = true;
   if (!pthread_equal(tid, pthread_self())) {
      pthread_kill(tid, TIMEOUT_SIGNAL);
   }
}

/*
	Become Threaded Network Server
    This function is able to handle multiple server ips in
    ipv4 and ipv6 style. The Addresse are give in a comma
    seperated string in bind_addr
    In the moment it is inpossible to bind different ports.
*/
void
bnet_thread_server(dlist *addrs, int max_clients, workq_t *client_wq,
		   void *handle_client_request(void *bsock))
{
   int newsockfd, stat;
   socklen_t clilen;
   struct sockaddr cli_addr;       /* client's address */
   int tlog;
   int turnon = 1;
#ifdef HAVE_LIBWRAP
   struct request_info request;
#endif
   IPADDR *p;
   struct s_sockfd {
      dlink link;		      /* this MUST be the first item */
      int fd;
      int port;
   } *fd_ptr = NULL;
   char buf[128];
   dlist sockfds;

   char allbuf[256 * 10];
   Dmsg1(100, "Addresses %s\n", build_addresses_str(addrs, allbuf, sizeof(allbuf)));

   foreach_dlist(p, addrs) {
      /* Allocate on stack frame -- no need to free */
      fd_ptr = (s_sockfd *)alloca(sizeof(s_sockfd));
      fd_ptr->port = p->get_port_net_order();
      /*
       * Open a TCP socket
       */
      for (tlog= 60; (fd_ptr->fd=socket(p->get_family(), SOCK_STREAM, 0)) < 0; tlog -= 10) {
	 if (tlog <= 0) {
	    berrno be;
	    char curbuf[256];
            Emsg3(M_ABORT, 0, _("Cannot open stream socket. ERR=%s. Current %s All %s\n"),
		       be.strerror(),
		       p->build_address_str(curbuf, sizeof(curbuf)),
		       build_addresses_str(addrs, allbuf, sizeof(allbuf)));
	 }
	 bmicrosleep(10, 0);
      }
      /*
       * Reuse old sockets
       */
      if (setsockopt(fd_ptr->fd, SOL_SOCKET, SO_REUSEADDR, (sockopt_val_t)&turnon,
	   sizeof(turnon)) < 0) {
	 berrno be;
         Emsg1(M_WARNING, 0, _("Cannot set SO_REUSEADDR on socket: %s\n"),
	       be.strerror());
      }

      int tmax = 30 * (60 / 5);    /* wait 30 minutes max */
      /* FIXME i can go to a endless loop i get a invalid address */
      for (tlog = 0; bind(fd_ptr->fd, p->get_sockaddr(), p->get_sockaddr_len()) < 0; tlog -= 5) {
	 berrno be;
	 if (tlog <= 0) {
	    tlog = 2 * 60;	   /* Complain every 2 minutes */
            Emsg2(M_WARNING, 0, _("Cannot bind port %d: ERR=%s. Retrying ...\n"),
		  ntohs(fd_ptr->port), be.strerror());
	 }
	 bmicrosleep(5, 0);
	 if (--tmax <= 0) {
            Emsg2(M_ABORT, 0, _("Cannot bind port %d: ERR=%s.\n"), ntohs(fd_ptr->port),
		  be.strerror());
	 }
      }
      listen(fd_ptr->fd, 5);	   /* tell system we are ready */
      sockfds.append(fd_ptr);
   }
   /* Start work queue thread */
   if ((stat = workq_init(client_wq, max_clients, handle_client_request)) != 0) {
      berrno be;
      be.set_errno(stat);
      Emsg1(M_ABORT, 0, _("Could not init client queue: ERR=%s\n"), be.strerror());
   }
   /*
    * Wait for a connection from the client process.
    */
   for (; !quit;) {
      unsigned int maxfd = 0;
      fd_set sockset;
      FD_ZERO(&sockset);
      foreach_dlist(fd_ptr, &sockfds) {
	 FD_SET((unsigned)fd_ptr->fd, &sockset);
	 maxfd = maxfd > (unsigned)fd_ptr->fd ? maxfd : fd_ptr->fd;
      }
      errno = 0;
      if ((stat = select(maxfd + 1, &sockset, NULL, NULL, NULL)) < 0) {
	 berrno be;		      /* capture errno */
	 if (errno == EINTR || errno == EAGAIN) {
	    continue;
	 }
	 /* Error, get out */
	 foreach_dlist(fd_ptr, &sockfds) {
	    close(fd_ptr->fd);
	    free((void *)fd_ptr);
	 }
         Emsg1(M_FATAL, 0, _("Error in select: %s\n"), be.strerror());
	 break;
      }

      foreach_dlist(fd_ptr, &sockfds) {
	 if (FD_ISSET(fd_ptr->fd, &sockset)) {
	    /* Got a connection, now accept it. */
	    do {
	       clilen = sizeof(cli_addr);
	       newsockfd = accept(fd_ptr->fd, &cli_addr, &clilen);
	    } while (newsockfd < 0 && (errno == EINTR || errno == EAGAIN));
	    if (newsockfd < 0) {
	       continue;
	    }
#ifdef HAVE_LIBWRAP
	    P(mutex);		   /* hosts_access is not thread safe */
	    request_init(&request, RQ_DAEMON, my_name, RQ_FILE, newsockfd, 0);
	    fromhost(&request);
	    if (!hosts_access(&request)) {
	       V(mutex);
	       Jmsg2(NULL, M_SECURITY, 0,
                     _("Connection from %s:%d refused by hosts.access\n"),
		     sockaddr_to_ascii(&cli_addr, buf, sizeof(buf)),
		     sockaddr_get_port(&cli_addr));
	       close(newsockfd);
	       continue;
	    }
	    V(mutex);
#endif

	    /*
	     * Receive notification when connection dies.
	     */
	    if (setsockopt(newsockfd, SOL_SOCKET, SO_KEEPALIVE, (sockopt_val_t)&turnon,
		 sizeof(turnon)) < 0) {
	       berrno be;
               Emsg1(M_WARNING, 0, _("Cannot set SO_KEEPALIVE on socket: %s\n"),
		     be.strerror());
	    }

	    /* see who client is. i.e. who connected to us. */
	    P(mutex);
	    sockaddr_to_ascii(&cli_addr, buf, sizeof(buf));
	    V(mutex);
	    BSOCK *bs;
            bs = init_bsock(NULL, newsockfd, "client", buf, fd_ptr->port, &cli_addr);
	    if (bs == NULL) {
               Jmsg0(NULL, M_ABORT, 0, _("Could not create client BSOCK.\n"));
	    }

	    /* Queue client to be served */
	    if ((stat = workq_add(client_wq, (void *)bs, NULL, 0)) != 0) {
	       berrno be;
	       be.set_errno(stat);
               Jmsg1(NULL, M_ABORT, 0, _("Could not add job to client queue: ERR=%s\n"),
		     be.strerror());
	    }
	 }
      }
   }

   /* Stop work queue thread */
   if ((stat = workq_destroy(client_wq)) != 0) {
      berrno be;
      be.set_errno(stat);
      Emsg1(M_FATAL, 0, _("Could not destroy client queue: ERR=%s\n"),
	    be.strerror());
   }
}


#ifdef REALLY_USED
/*
 * Bind an address so that we may accept connections
 * one at a time.
 */
BSOCK *bnet_bind(int port)
{
   int sockfd;
   struct sockaddr_in serv_addr;   /* our address */
   int tlog;
   int turnon = 1;

   /*
    * Open a TCP socket
    */
   for (tlog = 0; (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0; tlog -= 10) {
      if (errno == EINTR || errno == EAGAIN) {
	 continue;
      }
      if (tlog <= 0) {
	 tlog = 2 * 60;
         Emsg1(M_ERROR, 0, _("Cannot open stream socket: %s\n"), strerror(errno));
      }
      bmicrosleep(60, 0);
   }

   /*
    * Reuse old sockets
    */
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (sockopt_val_t)&turnon, sizeof(turnon)) < 0) {
      Emsg1(M_WARNING, 0, _("Cannot set SO_REUSEADDR on socket: %s\n"),
	    strerror(errno));
   }

   /*
    * Bind our local address so that the client can send to us.
    */
   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(port);

   for (tlog = 0; bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0;
	tlog -= 5) {
      berrno be;
      if (errno == EINTR || errno == EAGAIN) {
	 continue;
      }
      if (tlog <= 0) {
	 tlog = 2 * 60;
         Emsg2(M_WARNING, 0, _("Cannot bind port %d: ERR=%s: retrying ...\n"), port,
	       be.strerror());
      }
      bmicrosleep(5, 0);
   }
   listen(sockfd, 1);		   /* tell system we are ready */
   return init_bsock(NULL, sockfd, _("Server socket"), _("client"), port,
		     &serv_addr);
}

/*
 * Accept a single connection
 */
BSOCK *bnet_accept(BSOCK * bsock, char *who)
{
   fd_set ready, sockset;
   int newsockfd, stat, len;
   socklen_t clilen;
   struct sockaddr_in cli_addr;    /* client's address */
   char *caller, *buf;
   BSOCK *bs;
   int turnon = 1;
#ifdef HAVE_LIBWRAP
   struct request_info request;
#endif

   /*
    * Wait for a connection from the client process.
    */
   FD_ZERO(&sockset);
   FD_SET((unsigned)bsock->fd, &sockset);

   for (;;) {
      /*
       * Wait for a connection from a client process.
       */
      ready = sockset;
      if ((stat = select(bsock->fd + 1, &ready, NULL, NULL, NULL)) < 0) {
	 if (errno == EINTR || errno = EAGAIN) {
	    errno = 0;
	    continue;
	 }
         Emsg1(M_FATAL, 0, _("Error in select: %s\n"), strerror(errno));
	 newsockfd = -1;
	 break;
      }
      do {
	 clilen = sizeof(cli_addr);
	 newsockfd = accept(bsock->fd, (struct sockaddr *)&cli_addr, &clilen);
      } while (newsockfd < 0 && (errno == EINTR || errno = EAGAIN));
      if (newsockfd >= 0) {
	 break;
      }
   }

#ifdef HAVE_LIBWRAP
   P(mutex);
   request_init(&request, RQ_DAEMON, my_name, RQ_FILE, newsockfd, 0);
   fromhost(&request);
   if (!hosts_access(&request)) {
      V(mutex);
      Emsg2(M_SECURITY, 0, _("Connection from %s:%d refused by hosts.access\n"),
	    inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
      close(newsockfd);
      return NULL;
   }
   V(mutex);
#endif

   /*
    * Receive notification when connection dies.
    */
   if (setsockopt(newsockfd, SOL_SOCKET, SO_KEEPALIVE, (sockopt_val_t)&turnon, sizeof(turnon)) < 0) {
      Emsg1(M_WARNING, 0, _("Cannot set SO_KEEPALIVE on socket: %s\n"),
	    strerror(errno));
   }

   /* see who client is. I.e. who connected to us.
    * return it in the input message buffer.
    */
   if ((caller = inet_ntoa(cli_addr.sin_addr)) != NULL) {
      pm_strcpy(&bsock->msg, caller);
   } else {
      bsock->msg[0] = 0;
   }
   bsock->msglen = strlen(bsock->msg);

   if (newsockfd < 0) {
      Emsg2(M_FATAL, 0, _("Socket accept error for %s. ERR=%s\n"), who,
	    strerror(errno));
      return NULL;
   } else {
      if (caller == NULL) {
         caller = "unknown";
      }
      len = strlen(caller) + strlen(who) + 3;
      buf = (char *)malloc(len);
      bstrncpy(buf, len, who);
      bstrncat(buf, len, ": ");
      bstrncat(buf, len, caller);
      bs = init_bsock(NULL, newsockfd, "client", buf, bsock->port, &cli_addr);
      free(buf);
      return bs;		   /* return new BSOCK */
   }
}

#endif
