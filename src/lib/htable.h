/*
 *   Version $Id: htable.h,v 1.6.4.1 2005/02/14 10:02:25 kerns Exp $
 */

/*
   Copyright (C) 2003-2005 Kern Sibbald

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
