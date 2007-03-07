/*
 *
 * Written by Kern Sibbald, MMIV
 *
 *   Version $Id: htable.h 3670 2006-11-21 16:13:58Z kerns $
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2006 Free Software Foundation Europe e.V.

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

/* ========================================================================
 *
 *   Hash table class -- htable
 *
 */

/*
 * Loop var through each member of table
 */
#define foreach_htable(var, tbl) \
        for((*((void **)&(var))=(void *)((tbl)->first())); \
            (var); \
            (*((void **)&(var))=(void *)((tbl)->next())))

struct hlink {
   void *next;                        /* next hash item */
   char *key;                         /* key this item */
   uint32_t hash;                     /* hash for this key */
};

class htable : public SMARTALLOC {
   hlink **table;                     /* hash table */
   int loffset;                       /* link offset in item */
   uint32_t num_items;                /* current number of items */
   uint32_t max_items;                /* maximum items before growing */
   uint32_t buckets;                  /* size of hash table */
   uint32_t hash;                     /* temp storage */
   uint32_t index;                    /* temp storage */
   uint32_t mask;                     /* "remainder" mask */
   uint32_t rshift;                   /* amount to shift down */
   hlink *walkptr;                    /* table walk pointer */
   uint32_t walk_index;               /* table walk index */
   void hash_index(char *key);        /* produce hash key,index */
   void grow_table();                 /* grow the table */
public:
   htable(void *item, void *link, int tsize = 31);
   ~htable() { destroy(); }
   void init(void *item, void *link, int tsize = 31);
   bool  insert(char *key, void *item);
   void *lookup(char *key);
   void *first();                     /* get first item in table */
   void *next();                      /* get next item in table */
   void destroy();
   void stats();                      /* print stats about the table */
   uint32_t size();                   /* return size of table */
};
