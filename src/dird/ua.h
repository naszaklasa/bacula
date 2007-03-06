/*
 * Includes specific to the Director User Agent Server
 *
 *     Kern Sibbald, August MMI
 *
 *     Version $Id: ua.h,v 1.23.8.1 2005/02/14 10:02:21 kerns Exp $
 */
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

#ifndef __UA_H_
#define __UA_H_ 1

struct UAContext {
   BSOCK *UA_sock;
   BSOCK *sd;
   JCR *jcr;
   B_DB *db;
   CAT *catalog;
   CONRES *cons;                      /* console resource */
   POOLMEM *cmd;                      /* return command/name buffer */
   POOLMEM *args;                     /* command line arguments */
   char *argk[MAX_CMD_ARGS];          /* argument keywords */
   char *argv[MAX_CMD_ARGS];          /* argument values */
   int argc;                          /* number of arguments */
   char **prompt;                     /* list of prompts */
   int max_prompts;                   /* max size of list */
   int num_prompts;                   /* current number in list */
   bool auto_display_messages;        /* if set, display messages */
   bool user_notified_msg_pending;    /* set when user notified */
   bool automount;                    /* if set, mount after label */
   bool quit;                         /* if set, quit */
   bool verbose;                      /* set for normal UA verbosity */
   bool batch;                        /* set for non-interactive mode */
   uint32_t pint32_val;               /* positive integer */
   int32_t  int32_val;                /* positive/negative */
};          

/* Context for insert_tree_handler() */
struct TREE_CTX {
   TREE_ROOT *root;                   /* root */
   TREE_NODE *node;                   /* current node */
   TREE_NODE *avail_node;             /* unused node last insert */
   int cnt;                           /* count for user feedback */
   bool all;                          /* if set mark all as default */
   UAContext *ua;
   uint32_t FileEstimate;             /* estimate of number of files */
   uint32_t FileCount;                /* current count of files */
   uint32_t LastCount;                /* last count of files */
   uint32_t DeltaCount;               /* trigger for printing */
};

#endif
