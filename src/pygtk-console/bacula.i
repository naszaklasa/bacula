/*
  bacula.i: SWIG interface to Python module

  Author: Lucas Di Pentima <lucas@lunix.com.ar>

  Version $Id: bacula.i,v 1.5 2006/11/27 10:03:01 kerns Exp $
  
  This file was contributed to the Bacula project by Lucas Di
  Pentima and LUNIX S.R.L.

  LUNIX S.R.L. has been granted a perpetual, worldwide,
  non-exclusive, no-charge, royalty-free, irrevocable copyright
  license to reproduce, prepare derivative works of, publicly
  display, publicly perform, sublicense, and distribute the original
  work contributed by LUNIX S.R.L. and its employees to the Bacula 
  project in source or object form.
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2005-2006 Free Software Foundation Europe e.V.

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



%module bacula
%{
#include "bacula.h"

/* Wrapper function to avoid int*a on 'tls_remote_need' and 'compatible' args */
int wrap_cram_md5_respond(BSOCK *bs, char *password, int tls_remote_need, int compatible)
{
  return cram_md5_respond(bs, password, &tls_remote_need, &compatible);
}

char * get_md5(char *clear_password)
{
   unsigned int i, j;
   unsigned char sig[16];     
   char password[100];
   char *ret = (char *)malloc(sizeof(char)*100);
   struct MD5Context md5c;

   /* Hash password */
   MD5Init(&md5c);
   MD5Update(&md5c, (unsigned char *)clear_password, strlen(clear_password));
   MD5Final(sig, &md5c);
   for (i=j=0; i < sizeof(sig); i++) {
      sprintf(&password[j], "%02x", sig[i]);
      j += 2;
   }

   strncpy(ret, &password[0], sizeof(char)*100);
   return ret;
}

int get_msglen(BSOCK *bs)
{
  return (int)bs->msglen;
}
%}

/* Several constants inclusion */
%include "config.h"
%include "lib/bsock.h"
%include "bc_types.h"

/* Global Variables */
extern int debug_level;

/* Wrapped functions */
extern char * get_md5(char * clear_password);
extern BSOCK *bnet_connect(JCR * jcr, int retry_interval, int max_retry_time,
                    const char *name, char *host, char *service, int port,
                    int verbose);
extern bool bnet_send(BSOCK * bsock);
extern bool bnet_fsend(BSOCK *bs, const char *fmt, ...);

/* NOTE: This returns an 'int' because I couldn't handle int32_t */
extern int bnet_recv(BSOCK * bsock);

extern bool bnet_sig(BSOCK * bs, int sig);
extern void bnet_close(BSOCK * bsock);
extern int wrap_cram_md5_respond(BSOCK *bs, char *password, int tls_remote_need, int compatible);
extern int cram_md5_challenge(BSOCK *bs, char *password, int tls_local_need, int compatible);
extern bool is_bnet_stop (BSOCK *bsock);
extern int bnet_set_blocking (BSOCK *bsock);
extern int bnet_set_nonblocking (BSOCK *bsock);
extern int get_msglen(BSOCK *bs);
