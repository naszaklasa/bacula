/*
 *
 *   Bacula Director -- User Agent Input and scanning code
 *
 *     Kern Sibbald, October MMI
 *
 *   Version $Id: ua_input.c,v 1.27 2005/08/10 16:35:19 nboichat Exp $
 */
/*
   Copyright (C) 2001-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "dird.h"


/* Imported variables */


/* Exported functions */

int get_cmd(UAContext *ua, const char *prompt)
{
   BSOCK *sock = ua->UA_sock;
   int stat;

   ua->cmd[0] = 0;
   if (!sock) {                       /* No UA */
      return 0;
   }
   bnet_fsend(sock, "%s", prompt);
   bnet_sig(sock, BNET_PROMPT);       /* request more input */
   for ( ;; ) {
      stat = bnet_recv(sock);
      if (stat == BNET_SIGNAL) {
         continue;                    /* ignore signals */
      }
      if (is_bnet_stop(sock)) {
         return 0;                    /* error or terminate */
      }
      pm_strcpy(ua->cmd, sock->msg);
      strip_trailing_junk(ua->cmd);
      if (strcmp(ua->cmd, ".messages") == 0) {
         qmessagescmd(ua, ua->cmd);
      }
      /* Lone dot => break */
      if (ua->cmd[0] == '.' && ua->cmd[1] == 0) {
         return 0;
      }
      break;
   }
   return 1;
}

/*
 * Get a positive integer
 *  Returns:  false if failure
 *            true  if success => value in ua->pint32_val
 */
bool get_pint(UAContext *ua, const char *prompt)
{
   double dval;
   ua->pint32_val = 0;
   ua->int64_val = 0;
   for (;;) {
      ua->cmd[0] = 0;
      if (!get_cmd(ua, prompt)) {
         return false;
      }
      /* Kludge for slots blank line => 0 */
      if (ua->cmd[0] == 0 && strncmp(prompt, _("Enter slot"), strlen(_("Enter slot"))) == 0) {
         return true;
      }
      if (!is_a_number(ua->cmd)) {
         bsendmsg(ua, _("Expected a positive integer, got: %s\n"), ua->cmd);
         continue;
      }
      errno = 0;
      dval = strtod(ua->cmd, NULL);
      if (errno != 0 || dval < 0) {
         bsendmsg(ua, _("Expected a positive integer, got: %s\n"), ua->cmd);
         continue;
      }
      ua->pint32_val = (uint32_t)dval;
      ua->int64_val = (int64_t)dval;
      return true;
   }
}

/*
 * Gets a yes or no response
 *  Returns:  false if failure
 *            true  if success => ua->pint32_val == 1 for yes
 *                                ua->pint32_val == 0 for no
 */
bool get_yesno(UAContext *ua, const char *prompt)
{
   int len;

   ua->pint32_val = 0;
   for (;;) {
      if (!get_cmd(ua, prompt)) {
         return false;
      }
      len = strlen(ua->cmd);
      if (len < 1 || len > 3) {
         continue;
      }
      if (strncasecmp(ua->cmd, _("yes"), len) == 0) {
         ua->pint32_val = 1;
         return true;
      }
      if (strncasecmp(ua->cmd, _("no"), len) == 0) {
         return true;
      }
      bsendmsg(ua, _("Invalid response. You must answer yes or no.\n"));
   }
}


void parse_ua_args(UAContext *ua)
{
   parse_args(ua->cmd, &ua->args, &ua->argc, ua->argk, ua->argv, MAX_CMD_ARGS);
}
