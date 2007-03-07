/*
 *  Written by Kern Sibbald MMIV
 *
 *   Version $Id: dlist.h 3668 2006-11-21 13:20:11Z kerns $
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
 *   Doubly linked list  -- dlist
 *
 *    Kern Sibbald, MMIV
 *
 */

#define M_ABORT 1

/* In case you want to specifically specify the offset to the link */
#define OFFSET(item, link) (int)((char *)(link) - (char *)(item))
/*
 * There is a lot of extra casting here to work around the fact
 * that some compilers (Sun and Visual C++) do not accept
 * (void *) as an lvalue on the left side of an equal.
 *
 * Loop var through each member of list
 */
#ifdef HAVE_TYPEOF
#define foreach_dlist(var, list) \
        for((var)=NULL; ((var)=(typeof(var))(list)->next(var)); )
#else
#define foreach_dlist(var, list) \
    for((var)=NULL; (*((void **)&(var))=(void*)((list)->next(var))); )
#endif



struct dlink {
   void *next;
   void *prev;
};

class dlist : public SMARTALLOC {
   void *head;
   void *tail;
   int16_t loffset;
   uint32_t num_items;
public:
   dlist(void *item, dlink *link);
   dlist(void);
   ~dlist() { destroy(); }
   void init(void *item, dlink *link);
   void prepend(void *item);
   void append(void *item);
   void insert_before(void *item, void *where);
   void insert_after(void *item, void *where);
   void *binary_insert(void *item, int compare(void *item1, void *item2));
   void *binary_search(void *item, int compare(void *item1, void *item2));
   void binary_insert_multiple(void *item, int compare(void *item1, void *item2));
   void remove(void *item);
   bool empty() const;
   int  size() const;
   void *next(const void *item) const;
   void *prev(const void *item) const;
   void destroy();
   void *first() const;
   void *last() const;
};


/*
 * This allows us to do explicit initialization,
 *   allowing us to mix C++ classes inside malloc'ed
 *   C structures. Define before called in constructor.
 */
inline void dlist::init(void *item, dlink *link)
{
   head = tail = NULL;
   loffset = (int)((char *)link - (char *)item);
   if (loffset < 0 || loffset > 5000) {
      Emsg0(M_ABORT, 0, "Improper dlist initialization.\n");
   }
   num_items = 0;
}

/*
 * Constructor called with the address of a
 *   member of the list (not the list head), and
 *   the address of the link within that member.
 * If the link is at the beginning of the list member,
 *   then there is no need to specify the link address
 *   since the offset is zero.
 */
inline dlist::dlist(void *item, dlink *link)
{
   init(item, link);
}

/* Constructor with link at head of item */
inline dlist::dlist(void) : head(0), tail(0), loffset(0), num_items(0)
{
}

inline bool dlist::empty() const
{
   return head == NULL;
}

inline int dlist::size() const
{
   return num_items;
}



inline void * dlist::first() const
{
   return head;
}

inline void * dlist::last() const
{
   return tail;
}
