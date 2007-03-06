/*
 *
 *   Bacula Director -- User Agent Access Control List (ACL) handling
 *
 *     Kern Sibbald, January MMIV
 *
 *   Version  $Id: ua_acl.c,v 1.2.16.1 2005/02/14 10:02:21 kerns Exp $
 */

/*
   Copyright (C) 2004-2005 Kern Sibbald

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

/*
 * Check if access is permitted to item in acl
 */
bool acl_access_ok(UAContext *ua, int acl, char *item)
{
   return acl_access_ok(ua, acl, item, strlen(item));
}


/* This version expects the length of the item which we must check. */
bool acl_access_ok(UAContext *ua, int acl, char *item, int len)
{

   /* If no console resource => default console and all is permitted */
   if (!ua->cons) {
      Dmsg0(1400, "Root cons access OK.\n");
      return true;		      /* No cons resource -> root console OK for everything */
   }

   alist *list = ua->cons->ACL_lists[acl];
   if (!list) {
      return false;		      /* List empty, reject everything */
   }

   /* Special case *all* gives full access */
   if (list->size() == 1 && strcasecmp("*all*", (char *)list->get(0)) == 0) {
      return true;
   }

   /* Search list for item */
   for (int i=0; i<list->size(); i++) {
      if (strcasecmp(item, (char *)list->get(i)) == 0) {
         Dmsg3(1400, "ACL found %s in %d %s\n", item, acl, (char *)list->get(i));
	 return true;
      }
   }
   return false;
}
