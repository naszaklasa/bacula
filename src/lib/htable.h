/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 * Written by Kern Sibbald, MMIV
 *
 *   Version $Id$
 */

/* ========================================================================
 *
 *   Hash table class -- htable
 *
 */

/* 
 * BIG_MALLOC is to provide a large malloc service to htable
 */
#define BIG_MALLOC

/*
 * Loop var through each member of table
 */
#ifdef HAVE_TYPEOF
#define foreach_htable(var, tbl) \
        for((var)=(typeof(var))((tbl)->first()); \
           (var); \
           (var)=(typeof(var))((tbl)->next()))
#else
#define foreach_htable(var, tbl) \
        for((*((void **)&(var))=(void *)((tbl)->first())); \
            (var); \
            (*((void **)&(var))=(void *)((tbl)->next())))
#endif



struct hlink {
   void *next;                        /* next hash item */
   char *key;                         /* key this item */
   uint32_t hash;                     /* hash for this key */
};

struct h_mem {
   struct h_mem *next;                /* next buffer */
   int32_t rem;                       /* remaining bytes in big_buffer */
   char *mem;                         /* memory pointer */
   char first[1];                     /* first byte */
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
   uint32_t total_size;               /* total bytes malloced */
   uint32_t blocks;                   /* blocks malloced */
#ifdef BIG_MALLOC
   struct h_mem *mem_block;           /* malloc'ed memory block chain */
   void malloc_big_buf(int size);     /* Get a big buffer */
#endif
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
   char *hash_malloc(int size);       /* malloc bytes for a hash entry */
#ifdef BIG_MALLOC
   void hash_big_free();              /* free all hash allocated big buffers */
#endif
};
