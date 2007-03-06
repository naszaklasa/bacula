/* idcache.c -- map user and group IDs, cached for speed
   Copyright (C) 1985, 1988, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "bacula.h"

struct userid {
  union {
      uid_t u;
      gid_t g;
  } id;
  char *name;
  struct userid *next;
};

static struct userid *user_alist = NULL;
/* Use the same struct as for userids.	*/
static struct userid *group_alist = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Translate UID to a login name or a stringified number,
   with cache.	*/

char *getuser(uid_t uid, char *name, int len)
{
  register struct userid *tail;
  struct passwd *pwent;
  char usernum_string[20];

  P(mutex);
  for (tail = user_alist; tail; tail = tail->next) {
    if (tail->id.u == uid) {
      goto uid_done;
    }
  }

  pwent = getpwuid(uid);
  tail = (struct userid *)malloc(sizeof (struct userid));
  tail->id.u = uid;
#ifndef HAVE_WIN32
  if (pwent == NULL || strcmp(pwent->pw_name, "????????") == 0) {
      sprintf(usernum_string, "%u", (uint32_t)uid);
      tail->name = bstrdup(usernum_string);
  } else {
      tail->name = bstrdup(pwent->pw_name);
  }
#else
      sprintf(usernum_string, "%u", (uint32_t)uid);
      tail->name = bstrdup(usernum_string);
#endif

  /* Add to the head of the list, so most recently used is first.  */
  tail->next = user_alist;
  user_alist = tail;

uid_done:
  bstrncpy(name, tail->name, len);
  V(mutex);
  return name;
}

void free_getuser_cache()
{
  register struct userid *tail;

  P(mutex);
  for (tail = user_alist; tail; ) {
     struct userid *otail = tail;
     free(tail->name);
     tail = tail->next;
     free(otail);
  }
  user_alist = NULL;
  V(mutex);
}



/* Translate GID to a group name or a stringified number,
   with cache.	*/
char *getgroup(gid_t gid, char *name, int len)
{
  register struct userid *tail;
  struct group *grent;
  char groupnum_string[20];

  P(mutex);
  for (tail = group_alist; tail; tail = tail->next) {
    if (tail->id.g == gid) {
      goto gid_done;
    }
  }

  grent = getgrgid(gid);
  tail = (struct userid *)malloc(sizeof (struct userid));
  tail->id.g = gid;
#ifndef HAVE_WIN32
  if (grent == NULL || strcmp(grent->gr_name, "????????") == 0) {
      sprintf (groupnum_string, "%u", (uint32_t)gid);
      tail->name = bstrdup(groupnum_string);
  } else {
      tail->name = bstrdup(grent->gr_name);
  }
#else
      sprintf (groupnum_string, "%u", (uint32_t)gid);
      tail->name = bstrdup(groupnum_string);
#endif
  /* Add to the head of the list, so most recently used is first.  */
  tail->next = group_alist;
  group_alist = tail;

gid_done:
  bstrncpy(name, tail->name, len);
  V(mutex);
  return name;
}

void free_getgroup_cache()
{
  register struct userid *tail;

  P(mutex);
  for (tail = group_alist; tail; ) {
     struct userid *otail = tail;
     free(tail->name);
     tail = tail->next;
     free(otail);
  }
  group_alist = NULL;
  V(mutex);
}
