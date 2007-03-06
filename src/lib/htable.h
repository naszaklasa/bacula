/*
 *   Version $Id: htable.h,v 1.10 2005/07/08 10:03:00 kerns Exp $
 */
/*
   Copyright (C) 2003-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

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
