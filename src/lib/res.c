/*
 *  This file handles locking and seaching resources
 *
 *     Kern Sibbald, January MM
 *       Split from parse_conf.c April MMV
 *
 *   Version $Id: res.c,v 1.4 2005/08/10 16:35:19 nboichat Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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

extern int debug_level;

/* Each daemon has a slightly different set of
 * resources, so it will define the following
 * global values.
 */
extern int r_first;
extern int r_last;
extern RES_TABLE resources[];
extern RES **res_head;

#ifdef HAVE_WIN32
// work around visual studio name manling preventing external linkage since res_all
// is declared as a different type when instantiated.
extern "C" CURES res_all;
extern "C" int res_all_size;
#else
extern  CURES res_all;
extern int res_all_size;
#endif


brwlock_t res_lock;                   /* resource lock */
static int res_locked = 0;            /* set when resource chains locked -- for debug */


/* #define TRACE_RES */

void b_LockRes(const char *file, int line)
{
   int errstat;
#ifdef TRACE_RES
   Pmsg4(000, "LockRes  locked=%d w_active=%d at %s:%d\n", 
         res_locked, res_lock.w_active, file, line);
    if (res_locked) {
       Pmsg2(000, "LockRes writerid=%d myid=%d\n", res_lock.writer_id,
          pthread_self());
     }
#endif
   if ((errstat=rwl_writelock(&res_lock)) != 0) {
      Emsg3(M_ABORT, 0, _("rwl_writelock failure at %s:%d:  ERR=%s\n"),
           file, line, strerror(errstat));
   }
   res_locked++;
}

void b_UnlockRes(const char *file, int line)
{
   int errstat;
   if ((errstat=rwl_writeunlock(&res_lock)) != 0) {
      Emsg3(M_ABORT, 0, _("rwl_writeunlock failure at %s:%d:. ERR=%s\n"),
           file, line, strerror(errstat));
   }
   res_locked--;
#ifdef TRACE_RES
   Pmsg4(000, "UnLockRes locked=%d wactive=%d at %s:%d\n", 
         res_locked, res_lock.w_active, file, line);
#endif
}

/*
 * Return resource of type rcode that matches name
 */
RES *
GetResWithName(int rcode, char *name)
{
   RES *res;
   int rindex = rcode - r_first;

   LockRes();
   res = res_head[rindex];
   while (res) {
      if (strcmp(res->name, name) == 0) {
         break;
      }
      res = res->next;
   }
   UnlockRes();
   return res;

}

/*
 * Return next resource of type rcode. On first
 * call second arg (res) is NULL, on subsequent
 * calls, it is called with previous value.
 */
RES *
GetNextRes(int rcode, RES *res)
{
   RES *nres;
   int rindex = rcode - r_first;

   if (res == NULL) {
      nres = res_head[rindex];
   } else {
      nres = res->next;
   }
   return nres;
}


/* Parser state */
enum parse_state {
   p_none,
   p_resource
};
