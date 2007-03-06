/*
 *  Bacula red-black binary tree routines.
 *
 *    btree is a binary tree with the links being in the data item.
 *
 *   Developped in part from ideas obtained from several online University
 *    courses. 
 *
 *   Kern Sibbald, November MMV
 *
 *   Version $Id: btree.c,v 1.3 2006/11/21 16:13:57 kerns Exp $
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2005-2006 Free Software Foundation Europe e.V.

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

#include "bacula.h"
#include "btree.h"

/* ===================================================================
 *    btree
 */

/*
 *  Insert an item in the tree, but only if it is unique
 *   otherwise, the item is returned non inserted
 *  The big trick is keeping the tree balanced after the 
 *   insert. We use a parent pointer to make it simpler and 
 *   to avoid recursion.
 *
 * Returns: item         if item inserted
 *          other_item   if same value already exists (item not inserted)
 */
bnode *btree::insert(bnode *item, int compare(bnode *item1, bnode *item2))
{
   bnode *x, *y;
   bnode *last = NULL;        /* last leaf if not found */
   bnode *found = NULL;
   int comp = 0;

   /* Search */
   x = head;
   while (x && !found) {
      last = x;
      comp = compare(item, x);
      if (comp < 0) {
         x = x->left;
      } else if (comp > 0) {
         x = x->right;
      } else {
         found = x;
      }
   }

   if (found) {                    /* found? */
      return found;                /* yes, return item found */
   }
   /* Handle empty tree */
   if (num_items == 0) {
      head = item;
      num_items++;
      return item;
   }
   x = last;
   /* Not found, so insert it on appropriate side of tree */
   if (comp < 0) {
      last->left = item;
   } else {
      last->right = item;
   }
   last->red = true;
   item->parent = last;
   num_items++;

   /* Now we must walk up the tree balancing it */
   x = last;
   while (x != head && x->parent->red) {
      if (x->parent == x->parent->parent->left) {
         /* Look at the right side of our grandparent */
         y = x->parent->parent->right;
         if (y && y->red) {
            /* our parent must be black */
            x->parent->red = false;
            y->red = false;
            x->parent->parent->red = true;
            x = x->parent->parent;       /* move up to grandpa */
         } else {
            if (x == x->parent->right) { /* right side of parent? */
               x = x->parent;
               left_rotate(x);
            }
            /* make parent black too */
            x->parent->red = false;
            x->parent->parent->red = true;
            right_rotate(x->parent->parent);
         }
      } else {
         /* Look at left side of our grandparent */
         y = x->parent->parent->left;
         if (y && y->red) {
            x->parent->red = false;
            y->red = false;
            x->parent->parent->red = true;
            x = x->parent->parent;       /* move up to grandpa */
         } else {
            if (x == x->parent->left) {
               x = x->parent;
               right_rotate(x);
            }
            /* make parent black too */
            x->parent->red = false;
            x->parent->parent->red = true;
            left_rotate(x->parent->parent);
         }
      }
   }
   /* Make sure the head is always black */
   head->red = false;
   return item;
}


/*
 * Search for item
 */
bnode *btree::search(bnode *item, int compare(bnode *item1, bnode *item2))
{
   bnode *found = NULL;
   bnode *x;
   int comp;

   x = head;
   while (x) {
      comp = compare(item, x);
      if (comp < 0) {
         x = x->left;
      } else if (comp > 0) {
         x = x->right;
      } else {
         found = x;
         break;
      }
   }
   return found;
}

/* 
 * Get first item (i.e. lowest value) 
 */
bnode *btree::first(void)
{
   bnode *x;

   x = head;
   down = true;
   while (x) {
      if (x->left) {
         x = x->left;
         continue;
      }
      return x;
   }
   /* Tree is empty */
   return NULL;
}

/*
 * This is a non-recursive btree walk routine that returns
 *  the items one at a time in order. I've never seen a
 *  non-recursive tree walk routine published that returns
 *  one item at a time rather than doing a callback.
 *
 * Return the next item in sorted order.  We assume first()
 *  was called once before calling this routine.
 *  We always go down as far as we can to the left, then up, and
 *  down one to the right, and again down as far as we can to the
 *  left.  etc. 
 *
 * Returns: pointer to next larger item 
 *          NULL when no more items in tree
 */
bnode *btree::next(bnode *item)
{
   bnode *x;

   x = item;
   if ((down && !x->left && x->right) || (!down && x->right)) {
      /* Move down to right one */
      down = true;
      x = x->right;                       
      /* Then all the way down left */
      while (x->left)  {
         x = x->left;
      }
      return x;
   }

   /* We have gone down all we can, so now go up */
   for ( ;; ) {
      /* If at head, we are done */
      if (!x->parent) {
         return NULL;
      }
      /* Move up in tree */
      down = false;
      /* if coming from right, continue up */
      if (x->parent->right == x) {
         x = x->parent;
         continue;
      }
      /* Coming from left, go up one -- ie. return parent */
      return x->parent;
   }
}

/*
 * Similer to next(), but visits all right nodes when
 *  coming up the tree.
 */
bnode *btree::any(bnode *item)
{
   bnode *x;

   x = item;
   if ((down && !x->left && x->right) || (!down && x->right)) {
      /* Move down to right one */
      down = true;
      x = x->right;                       
      /* Then all the way down left */
      while (x->left)  {
         x = x->left;
      }
      return x;
   }

   /* We have gone down all we can, so now go up */
   for ( ;; ) {
      /* If at head, we are done */
      if (!x->parent) {
         return NULL;
      }
      down = false;
      /* Go up one and return parent */
      return x->parent;
   }
}


/* x is item, y is below and to right, then rotated to below left */
void btree::left_rotate(bnode *item)
{
   bnode *y;
   bnode *x;

   x = item;
   y = x->right;
   x->right = y->left;
   if (y->left) {
      y->left->parent = x;
   }
   y->parent = x->parent;
   /* if no parent then we have a new head */
   if (!x->parent) {
      head = y;
   } else if (x == x->parent->left) {
      x->parent->left = y;
   } else {
      x->parent->right = y;
   }
   y->left = x;
   x->parent = y;
}

void btree::right_rotate(bnode *item)
{
   bnode *x, *y;

   y = item;
   x = y->left;
   y->left = x->right;
   if (x->right) {
      x->right->parent = y;
   }
   x->parent = y->parent;
   /* if no parent then we have a new head */
   if (!y->parent) {
      head = x;
   } else if (y == y->parent->left) {
      y->parent->left = x;
   } else {
      y->parent->right = x;
   }
   x->right = y;
   y->parent = x;
}


void btree::remove(bnode *item)
{
}

/* Destroy the tree contents.  Not totally working */
void btree::destroy()
{
   bnode *x, *y = NULL;

   x = first();
// printf("head=%p first=%p left=%p right=%p\n", head, x, x->left, x->right);

   for (  ; (y=any(x)); ) {
      /* Prune the last item */
      if (x->parent) {
         if (x == x->parent->left) {
            x->parent->left = NULL;
         } else if (x == x->parent->right) {
            x->parent->right = NULL;
         }
      }
      if (!x->left && !x->right) {
         if (head == x) {  
            head = NULL;
         }
//       if (num_items<30) {
//          printf("free nitems=%d item=%p left=%p right=%p\n", num_items, x, x->left, x->right);
//       }   
         free((void *)x);      /* free previous node */
         num_items--;
      }
      x = y;                  /* save last node */
   }
   if (x) {
      if (x == head) {
         head = NULL;
      }
//    printf("free nitems=%d item=%p left=%p right=%p\n", num_items, x, x->left, x->right);
      free((void *)x);
      num_items--;
   }
   if (head) {
//    printf("Free head\n");
      free((void *)head);
   }
// printf("free nitems=%d\n", num_items);

   head = NULL;
}



#ifdef TEST_PROGRAM

struct MYJCR {
   bnode link;
   char *buf;
};

static int my_compare(bnode *item1, bnode *item2)
{
   MYJCR *jcr1, *jcr2;
   int comp;
   jcr1 = (MYJCR *)item1;
   jcr2 = (MYJCR *)item2;
   comp = strcmp(jcr1->buf, jcr2->buf);
 //Dmsg3(000, "compare=%d: %s to %s\n", comp, jcr1->buf, jcr2->buf);
   return comp;
}

int main()
{
   char buf[30];
   btree *jcr_chain;
   MYJCR *jcr = NULL;
   MYJCR *jcr1;


   /* Now do a binary insert for the tree */
   jcr_chain = New(btree());
#define CNT 26
   printf("append %d items\n", CNT*CNT*CNT);
   strcpy(buf, "ZZZ");
   int count = 0;
   for (int i=0; i<CNT; i++) {
      for (int j=0; j<CNT; j++) {
         for (int k=0; k<CNT; k++) {
            count++;
            if ((count & 0x3FF) == 0) {
               Dmsg1(000, "At %d\n", count);
            }
            jcr = (MYJCR *)malloc(sizeof(MYJCR));
            memset(jcr, 0, sizeof(MYJCR));
            jcr->buf = bstrdup(buf);
//          printf("buf=%p %s\n", jcr, jcr->buf);
            jcr1 = (MYJCR *)jcr_chain->insert((bnode *)jcr, my_compare);
            if (jcr != jcr1) {
               Dmsg2(000, "Insert of %s vs %s failed.\n", jcr->buf, jcr1->buf);
            }
            buf[1]--;
         }
         buf[1] = 'Z';
         buf[2]--;
      }
      buf[2] = 'Z';
      buf[0]--;
   }
   printf("%d items appended\n", CNT*CNT*CNT);
   printf("num_items=%d\n", jcr_chain->size());

   jcr = (MYJCR *)malloc(sizeof(MYJCR));
   memset(jcr, 0, sizeof(MYJCR));

   jcr->buf = bstrdup("a");
   if ((jcr1=(MYJCR *)jcr_chain->search((bnode *)jcr, my_compare))) {
      printf("One less failed!!!! Got: %s\n", jcr1->buf);
   } else {
      printf("One less: OK\n");
   }
   free(jcr->buf);

   jcr->buf = bstrdup("ZZZZZZZZZZZZZZZZ");
   if ((jcr1=(MYJCR *)jcr_chain->search((bnode *)jcr, my_compare))) {
      printf("One greater failed!!!! Got:%s\n", jcr1->buf);
   } else {
      printf("One greater: OK\n");
   }
   free(jcr->buf);

   jcr->buf = bstrdup("AAA");
   if ((jcr1=(MYJCR *)jcr_chain->search((bnode *)jcr, my_compare))) {
      printf("Search for AAA got %s\n", jcr1->buf);
   } else {
      printf("Search for AAA not found\n"); 
   }
   free(jcr->buf);

   jcr->buf = bstrdup("ZZZ");
   if ((jcr1 = (MYJCR *)jcr_chain->search((bnode *)jcr, my_compare))) {
      printf("Search for ZZZ got %s\n", jcr1->buf);
   } else {
      printf("Search for ZZZ not found\n"); 
   }
   free(jcr->buf);
   free(jcr);


   printf("Find each of %d items in tree.\n", count);
   for (jcr=(MYJCR *)jcr_chain->first(); jcr; (jcr=(MYJCR *)jcr_chain->next((bnode *)jcr)) ) {
//    printf("Got: %s\n", jcr->buf);
      if (!jcr_chain->search((bnode *)jcr, my_compare)) {
         printf("btree binary_search item not found = %s\n", jcr->buf);
      }
   }
   printf("Free each of %d items in tree.\n", count);
   for (jcr=(MYJCR *)jcr_chain->first(); jcr; (jcr=(MYJCR *)jcr_chain->next((bnode *)jcr)) ) {
//    printf("Free: %p %s\n", jcr, jcr->buf);
      free(jcr->buf);
      jcr->buf = NULL;
   }
   printf("num_items=%d\n", jcr_chain->size());
   delete jcr_chain;


   sm_dump(true);

}
#endif
