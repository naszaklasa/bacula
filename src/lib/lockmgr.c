/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2008-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation, which is 
   listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#define _LOCKMGR_COMPLIANT
#include "lockmgr.h"

#undef ASSERT
#define ASSERT(x) if (!(x)) { \
   char *jcr = NULL; \
   Pmsg3(000, _("%s:%i Failed ASSERT: %s\n"), __FILE__, __LINE__, #x); \
   jcr[0] = 0; }

/*
  Inspired from 
  http://www.cs.berkeley.edu/~kamil/teaching/sp03/041403.pdf

  This lock manager will replace some pthread calls. It can be
  enabled with _USE_LOCKMGR

  Some part of the code can't use this manager, for example the
  rwlock object or the smartalloc lib. To disable LMGR, just add
  _LOCKMGR_COMPLIANT before the inclusion of "bacula.h"

  cd build/src/tools
  g++ -g -c lockmgr.c -I.. -I../lib -D_USE_LOCKMGR -D_TEST_IT
  g++ -o lockmgr lockmgr.o -lbac -L../lib/.libs -lssl -lpthread

*/


/*
 * pthread_mutex_lock for memory allocator and other
 * parts that are _LOCKMGR_COMPLIANT
 */
void lmgr_p(pthread_mutex_t *m)
{
   int errstat;
   if ((errstat=pthread_mutex_lock(m))) {
      berrno be;
      e_msg(__FILE__, __LINE__, M_ABORT, 0, _("Mutex lock failure. ERR=%s\n"),
            be.bstrerror(errstat));
   }
}

void lmgr_v(pthread_mutex_t *m)
{
   int errstat;
   if ((errstat=pthread_mutex_unlock(m))) {
      berrno be;
      e_msg(__FILE__, __LINE__, M_ABORT, 0, _("Mutex unlock failure. ERR=%s\n"),
            be.bstrerror(errstat));
   }
}

#ifdef _USE_LOCKMGR

typedef enum
{
   LMGR_WHITE,                  /* never seen */
   LMGR_BLACK,                  /* no loop */
   LMGR_GREY,                   /* seen before */
} lmgr_color_t;

/*
 * Node used by the Lock Manager
 * If the lock is GRANTED, we have mutex -> proc, else it's a proc -> mutex
 * relation.
 *
 * Note, each mutex can be GRANTED once, and each proc can have only one WANTED
 * mutex.
 */
class lmgr_node_t: public SMARTALLOC
{
public:
   dlink link;
   void *node;
   void *child;
   lmgr_color_t seen;

   lmgr_node_t() {
      child = node = NULL;
      seen = LMGR_WHITE;
   }

   lmgr_node_t(void *n, void *c) {
      init(n,c);
   }

   void init(void *n, void *c) {
      node = n;
      child = c;
      seen = LMGR_WHITE;      
   }

   void mark_as_seen(lmgr_color_t c) {
      seen = c;
   }

   ~lmgr_node_t() {printf("delete node\n");}
};

typedef enum {
   LMGR_LOCK_EMPTY   = 'E',      /* unused */
   LMGR_LOCK_WANTED  = 'W',      /* before mutex_lock */
   LMGR_LOCK_GRANTED = 'G'       /* after mutex_lock */
} lmgr_state_t;

/*
 * Object associated with each mutex per thread
 */
class lmgr_lock_t: public SMARTALLOC
{
public:
   dlink link;
   void *lock;
   lmgr_state_t state;
   
   const char *file;
   int line;

   lmgr_lock_t() {
      lock = NULL;
      state = LMGR_LOCK_EMPTY;
   }

   lmgr_lock_t(void *l) {
      lock = l;
      state = LMGR_LOCK_WANTED;
   }

   void set_granted() {
      state = LMGR_LOCK_GRANTED;
   }

   ~lmgr_lock_t() {}

};

/* 
 * Get the child list, ret must be already allocated
 */
static void search_all_node(dlist *g, lmgr_node_t *v, alist *ret)
{
   lmgr_node_t *n;
   foreach_dlist(n, g) {
      if (v->child == n->node) {
         ret->append(n);
      }
   }
}

static bool visite(dlist *g, lmgr_node_t *v)
{
   bool ret=false;
   lmgr_node_t *n;
   v->mark_as_seen(LMGR_GREY);

   alist *d = New(alist(5, false)); /* use alist because own=false */
   search_all_node(g, v, d);

   //foreach_alist(n, d) {
   //   printf("node n=%p c=%p s=%c\n", n->node, n->child, n->seen);
   //}

   foreach_alist(n, d) {
      if (n->seen == LMGR_GREY) { /* already seen this node */
         ret = true;
         goto bail_out;
      } else if (n->seen == LMGR_WHITE) {
         if (visite(g, n)) {
            ret = true;
            goto bail_out;
         }
      }
   }
   v->mark_as_seen(LMGR_BLACK); /* no loop detected, node is clean */
bail_out:
   delete d;
   return ret;
}

static bool contains_cycle(dlist *g)
{
   lmgr_node_t *n;
   foreach_dlist(n, g) {
      if (n->seen == LMGR_WHITE) {
         if (visite(g, n)) {
            return true;
         }
      }
   }
   return false;
}

/****************************************************************/

class lmgr_thread_t: public SMARTALLOC
{
public:
   dlink link;
   pthread_mutex_t mutex;
   pthread_t       thread_id;
   lmgr_lock_t     lock_list[LMGR_MAX_LOCK];
   int current;
   int max;

   lmgr_thread_t() {
      int status;
      if ((status = pthread_mutex_init(&mutex, NULL)) != 0) {
         berrno be;
         Pmsg1(000, _("pthread key create failed: ERR=%s\n"),
                 be.bstrerror(status));
         ASSERT(0);
      }
      thread_id = pthread_self();
      current = -1;
      max = 0;
   }

   void _dump(FILE *fp) {
      fprintf(fp, "threadid=0x%x max=%i current=%i\n", (int)thread_id, max, current);
      for(int i=0; i<=current; i++) {
         fprintf(fp, "   lock=%p state=%c %s:%i\n", 
               lock_list[i].lock, lock_list[i].state,
               lock_list[i].file, lock_list[i].line);
      }
   }

   void dump(FILE *fp) {
      pthread_mutex_lock(&mutex);
      {
         _dump(fp);
      }
      pthread_mutex_unlock(&mutex);
   }

   /*
    * Call before a lock operation (mark mutex as WANTED)
    */
   virtual void pre_P(void *m, const char *f="*unknown*", int l=0) {
      ASSERT(current < LMGR_MAX_LOCK);
      ASSERT(current >= -1);
      pthread_mutex_lock(&mutex);
      {
         current++;
         lock_list[current].lock = m;
         lock_list[current].state = LMGR_LOCK_WANTED;
         lock_list[current].file = f;
         lock_list[current].line = l;
         max = MAX(current, max);
      }
      pthread_mutex_unlock(&mutex);
   }

   /*
    * Call after the lock operation (mark mutex as GRANTED)
    */
   virtual void post_P() {
      ASSERT(current >= 0);
      ASSERT(lock_list[current].state == LMGR_LOCK_WANTED);
      lock_list[current].state = LMGR_LOCK_GRANTED;
   }
   
   void shift_list(int i) {
      for(int j=i+1; j<=current; j++) {
         lock_list[i] = lock_list[j];
      }
      if (current >= 0) {
         lock_list[current].lock = NULL;
         lock_list[current].state = LMGR_LOCK_EMPTY;
      }
   }

   /*
    * Remove the mutex from the list
    */
   virtual void do_V(void *m, const char *f="*unknown*", int l=0) {
      ASSERT(current >= 0);
      pthread_mutex_lock(&mutex);
      {
         if (lock_list[current].lock == m) {
            lock_list[current].lock = NULL;
            lock_list[current].state = LMGR_LOCK_EMPTY;
            current--;
         } else {
            ASSERT(current > 0);
            Pmsg3(0, "ERROR: wrong P/V order search lock=%p %s:%i\n", m, f, l);
            Pmsg4(000, "ERROR: wrong P/V order pos=%i lock=%p %s:%i\n",
                    current, lock_list[current].lock, lock_list[current].file, 
                    lock_list[current].line);
            for (int i=current-1; i >= 0; i--) { /* already seen current */
               Pmsg4(000, "ERROR: wrong P/V order pos=%i lock=%p %s:%i\n",
                     i, lock_list[i].lock, lock_list[i].file, lock_list[i].line);
               if (lock_list[i].lock == m) {
                  Pmsg3(000, "ERROR: FOUND P pos=%i %s:%i\n", i, f, l);
                  shift_list(i);
                  current--;
                  break;
               }
            }
         }
      }
      pthread_mutex_unlock(&mutex);
   }

   virtual ~lmgr_thread_t() {destroy();}

   void destroy() {
      pthread_mutex_destroy(&mutex);
   }
} ;

class lmgr_dummy_thread_t: public lmgr_thread_t
{
   void do_V(void *m, const char *file, int l)  {}
   void post_P()                                {}
   void pre_P(void *m, const char *file, int l) {}
};

/*
 * LMGR - Lock Manager
 *
 *
 *
 */

pthread_once_t key_lmgr_once = PTHREAD_ONCE_INIT; 
static pthread_key_t lmgr_key;  /* used to get lgmr_thread_t object */

static dlist *global_mgr=NULL;  /* used to store all lgmr_thread_t objects */
static pthread_mutex_t lmgr_global_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t undertaker;

#define lmgr_is_active() (global_mgr != NULL)

/*
 * Add a new lmgr_thread_t object to the global list
 */
void lmgr_register_thread(lmgr_thread_t *item)
{
   pthread_mutex_lock(&lmgr_global_mutex);
   {
      global_mgr->prepend(item);
   }
   pthread_mutex_unlock(&lmgr_global_mutex);
}

/*
 * Call this function to cleanup specific lock thread data
 */
void lmgr_unregister_thread(lmgr_thread_t *item)
{
   if (!lmgr_is_active()) {
      return;
   }
   pthread_mutex_lock(&lmgr_global_mutex);
   {
      global_mgr->remove(item);
   }
   pthread_mutex_unlock(&lmgr_global_mutex);
}

/*
 * Search for a deadlock when it's secure to walk across
 * locks list. (after lmgr_detect_deadlock or a fatal signal)
 */
bool lmgr_detect_deadlock_unlocked()
{
   bool ret=false;
   lmgr_node_t *node=NULL;
   lmgr_lock_t *lock;
   lmgr_thread_t *item;
   dlist *g = New(dlist(node, &node->link));

   /* First, get a list of all node */
   foreach_dlist(item, global_mgr) {
      for(int i=0; i<=item->current; i++) {
         node = NULL;
         lock = &item->lock_list[i];
         /* Depending if the lock is granted or not, it's a child or a root
          *  Granted:  Mutex  -> Thread
          *  Wanted:   Thread -> Mutex
          *
          * Note: a Mutex can be locked only once, a thread can request only
          * one mutex.
          *
          */
         if (lock->state == LMGR_LOCK_GRANTED) {
            node = New(lmgr_node_t((void*)lock->lock, (void*)item->thread_id));
         } else if (lock->state == LMGR_LOCK_WANTED) {
            node = New(lmgr_node_t((void*)item->thread_id, (void*)lock->lock));
         }
         if (node) {
            g->append(node);
         }
      }
   }      
   
   //foreach_dlist(node, g) {
   //   printf("g n=%p c=%p\n", node->node, node->child);
   //}

   ret = contains_cycle(g);
   if (ret) {
      printf("Found a deadlock !!!!\n");
   }
   
   delete g;
   return ret;
}

/*
 * Search for a deadlock in during the runtime
 * It will lock all thread specific lock manager, nothing
 * can be locked during this check.
 */
bool lmgr_detect_deadlock()
{
   bool ret=false;
   if (!lmgr_is_active()) {
      return ret;
   } 

   pthread_mutex_lock(&lmgr_global_mutex);
   {
      lmgr_thread_t *item;
      foreach_dlist(item, global_mgr) {
         pthread_mutex_lock(&item->mutex);
      }

      ret = lmgr_detect_deadlock_unlocked();

      foreach_dlist(item, global_mgr) {
         pthread_mutex_unlock(&item->mutex);
      }
   }
   pthread_mutex_unlock(&lmgr_global_mutex);

   return ret;
}

/*
 * !!! WARNING !!! 
 * Use this function only after a fatal signal
 * We don't use any lock to display information
 */
void dbg_print_lock(FILE *fp)
{
   fprintf(fp, "Attempt to dump locks\n");
   if (!lmgr_is_active()) {
      return ;
   }
   lmgr_thread_t *item;
   foreach_dlist(item, global_mgr) {
      item->_dump(fp);
   }
}

/*
 * Dump each lmgr_thread_t object
 */
void lmgr_dump()
{
   pthread_mutex_lock(&lmgr_global_mutex);
   {
      lmgr_thread_t *item;
      foreach_dlist(item, global_mgr) {
         item->dump(stderr);
      }
   }
   pthread_mutex_unlock(&lmgr_global_mutex);
}

void cln_hdl(void *a)
{
   lmgr_cleanup_thread();
}

void *check_deadlock(void *)
{
   int old;
   lmgr_init_thread();
   pthread_cleanup_push(cln_hdl, NULL);

   while (!bmicrosleep(30, 0)) {
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
      if (lmgr_detect_deadlock()) {
         lmgr_dump();
         ASSERT(0);
      }
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
      pthread_testcancel();
   }
   Pmsg0(000, "Undertaker is leaving...\n");
   pthread_cleanup_pop(1);
   return NULL;
}

/* This object is used when LMGR is not initialized */
lmgr_dummy_thread_t dummy_lmgr;

/*
 * Retrieve the lmgr_thread_t object from the stack
 */
inline lmgr_thread_t *lmgr_get_thread_info()
{
   if (lmgr_is_active()) {
      return (lmgr_thread_t *)pthread_getspecific(lmgr_key);
   } else {
      return &dummy_lmgr;
   }
}

/*
 * launch once for all threads
 */
void create_lmgr_key()
{
   int status = pthread_key_create(&lmgr_key, NULL);
   if (status != 0) {
      berrno be;
      Pmsg1(000, _("pthread key create failed: ERR=%s\n"),
            be.bstrerror(status));
      ASSERT(0);
   }

   lmgr_thread_t *n=NULL;
   global_mgr = New(dlist(n, &n->link));

   if (pthread_create(&undertaker, NULL, check_deadlock, NULL) != 0) {
      berrno be;
      Pmsg1(000, _("pthread_create failed: ERR=%s\n"),
            be.bstrerror(status));
      ASSERT(0);
   }
}

/*
 * Each thread have to call this function to put a lmgr_thread_t object
 * in the stack and be able to call mutex_lock/unlock
 */
void lmgr_init_thread()
{
   int status = pthread_once(&key_lmgr_once, create_lmgr_key);
   if (status != 0) {
      berrno be;
      Pmsg1(000, _("pthread key create failed: ERR=%s\n"),
            be.bstrerror(status));
      ASSERT(0);
   }
   lmgr_thread_t *l = New(lmgr_thread_t());
   pthread_setspecific(lmgr_key, l);
   lmgr_register_thread(l);
}

/*
 * Call this function at the end of the thread
 */
void lmgr_cleanup_thread()
{
   if (!lmgr_is_active()) {
      return ;
   }
   lmgr_thread_t *self = lmgr_get_thread_info();
   lmgr_unregister_thread(self);
   delete(self);
}

/*
 * This function should be call at the end of the main thread
 * Some thread like the watchdog are already present, so the global_mgr
 * list is never empty. Should carefully clear the memory.
 */
void lmgr_cleanup_main()
{
   dlist *temp;
   
   pthread_cancel(undertaker);
   lmgr_cleanup_thread();
   pthread_mutex_lock(&lmgr_global_mutex);
   {
      temp = global_mgr;
      global_mgr=NULL;
      delete temp;
   }
   pthread_mutex_unlock(&lmgr_global_mutex);
}

/*
 * Replacement for pthread_mutex_lock()
 */
int lmgr_mutex_lock(pthread_mutex_t *m, const char *file, int line)
{
   int ret;
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->pre_P(m, file, line);
   ret = pthread_mutex_lock(m);
   self->post_P();   
   return ret;
}

/*
 * Replacement for pthread_mutex_unlock()
 */
int lmgr_mutex_unlock(pthread_mutex_t *m, const char *file, int line)
{
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->do_V(m, file, line);
   return pthread_mutex_unlock(m);
}

/* TODO: check this
 */
int lmgr_cond_wait(pthread_cond_t *cond,
                   pthread_mutex_t *mutex)
{
   int ret;
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->do_V(mutex);   
   ret = pthread_cond_wait(cond, mutex);
   self->pre_P(mutex);
   self->post_P();
   return ret;
}

/*
 * Use this function when the caller handle the mutex directly
 *
 * lmgr_pre_lock(m);
 * pthread_mutex_lock(m);
 * lmgr_post_lock(m);
 */
void lmgr_pre_lock(void *m)
{
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->pre_P(m);
}

/*
 * Use this function when the caller handle the mutex directly
 */
void lmgr_post_lock()
{
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->post_P();
}

/*
 * Do directly pre_P and post_P (used by trylock)
 */
void lmgr_do_lock(void *m)
{
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->pre_P(m);
   self->post_P();
}

/*
 * Use this function when the caller handle the mutex directly
 */
void lmgr_do_unlock(void *m)
{
   lmgr_thread_t *self = lmgr_get_thread_info();
   self->do_V(m);
}

typedef struct {
   void *(*start_routine)(void*);
   void *arg;
} lmgr_thread_arg_t;

extern "C" 
void *lmgr_thread_launcher(void *x)
{
   void *ret=NULL;
   lmgr_init_thread();
   pthread_cleanup_push(cln_hdl, NULL);

   lmgr_thread_arg_t arg;
   lmgr_thread_arg_t *a = (lmgr_thread_arg_t *)x;
   arg.start_routine = a->start_routine;
   arg.arg = a->arg;
   free(a);

   ret = arg.start_routine(arg.arg);
   pthread_cleanup_pop(1);
   return ret;
}

int lmgr_thread_create(pthread_t *thread,
                       const pthread_attr_t *attr,
                       void *(*start_routine)(void*), void *arg)
{
   /* Will be freed by the child */
   lmgr_thread_arg_t *a = (lmgr_thread_arg_t*) malloc(sizeof(lmgr_thread_arg_t));
   a->start_routine = start_routine;
   a->arg = arg;
   return pthread_create(thread, attr, lmgr_thread_launcher, a);   
}

#else  /* _USE_LOCKMGR */

/*
 * !!! WARNING !!! 
 * Use this function only after a fatal signal
 * We don't use any lock to display information
 */
void dbg_print_lock(FILE *fp)
{
   Pmsg0(000, "lockmgr disabled\n");
}

#endif  /* _USE_LOCKMGR */

#ifdef _TEST_IT

#include "lockmgr.h"
#define pthread_mutex_lock(x)   lmgr_mutex_lock(x)
#define pthread_mutex_unlock(x) lmgr_mutex_unlock(x)
#define pthread_cond_wait(x,y)  lmgr_cond_wait(x,y)
#define pthread_create(a, b, c, d)  lmgr_thread_create(a,b,c,d)
#undef P
#undef V
#define P(x) lmgr_mutex_lock(&(x), __FILE__, __LINE__)
#define V(x) lmgr_mutex_unlock(&(x), __FILE__, __LINE__)

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex5 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex6 = PTHREAD_MUTEX_INITIALIZER;

void *self_lock(void *temp)
{
   P(mutex1);
   P(mutex1);
   V(mutex1);
   
   return NULL;
}

void *nolock(void *temp)
{
   P(mutex2);
   sleep(5);
   V(mutex2);
   return NULL;
}

void *locker(void *temp)
{
   pthread_mutex_t *m = (pthread_mutex_t*) temp;
   pthread_mutex_lock(m);
   pthread_mutex_unlock(m);
   return NULL;
}

void *rwlocker(void *temp)
{
   brwlock_t *m = (brwlock_t*) temp;
   rwl_writelock(m);
   rwl_writelock(m);

   rwl_writeunlock(m);
   rwl_writeunlock(m);
   return NULL;
}

void *mix_rwl_mutex(void *temp)
{
   brwlock_t *m = (brwlock_t*) temp;
   P(mutex1);
   rwl_writelock(m);
   rwl_writeunlock(m);
   V(mutex1);
   return NULL;
}


void *th2(void *temp)
{
   P(mutex2);
   P(mutex1);
   
   lmgr_dump();

   sleep(10);

   V(mutex1);
   V(mutex2);

   lmgr_dump();
   return NULL;
}
void *th1(void *temp)
{
   P(mutex1);
   sleep(2);
   P(mutex2);
   
   lmgr_dump();

   sleep(10);

   V(mutex2);
   V(mutex1);

   lmgr_dump();
   return NULL;
}

void *thx(void *temp)
{
   int s= 1 + (int) (500.0 * (rand() / (RAND_MAX + 1.0))) + 200;
   P(mutex1);
   bmicrosleep(0,s);
   P(mutex2);
   bmicrosleep(0,s);

   V(mutex2);
   V(mutex1);
   return NULL;
}

void *th3(void *a) {
   while (1) {
      fprintf(stderr, "undertaker sleep()\n");
      sleep(10);
      lmgr_dump();
      if (lmgr_detect_deadlock()) {
         lmgr_dump();
         exit(1);
      }
   }
   return NULL;
}

int err=0;
int nb=0;
void _ok(const char *file, int l, const char *op, int value, const char *label)
{
   nb++;
   if (!value) {
      err++;
      printf("ERR %.30s %s:%i on %s\n", label, file, l, op);
   } else {
      printf("OK  %.30s\n", label);
   }
}

#define ok(x, label) _ok(__FILE__, __LINE__, #x, (x), label)

void _nok(const char *file, int l, const char *op, int value, const char *label)
{
   nb++;
   if (value) {
      err++;
      printf("ERR %.30s %s:%i on !%s\n", label, file, l, op);
   } else {
      printf("OK  %.30s\n", label);
   }
}

#define nok(x, label) _nok(__FILE__, __LINE__, #x, (x), label)

int report()
{
   printf("Result %i/%i OK\n", nb - err, nb);
   return err>0;
}

/* 
 * TODO:
 *  - Must detect multiple lock
 *  - lock/unlock in wrong order
 *  - deadlock with 2 or 3 threads
 */
int main()
{
   pthread_t id1, id2, id3, tab[200];
   lmgr_init_thread();

   pthread_create(&id1, NULL, self_lock, NULL);
   sleep(2);
   ok(lmgr_detect_deadlock(), "Check self deadlock");
   lmgr_v(&mutex1);             /* a bit dirty */
   pthread_join(id1, NULL);


   pthread_create(&id1, NULL, nolock, NULL);
   sleep(2);
   nok(lmgr_detect_deadlock(), "Check for nolock");
   pthread_join(id1, NULL);

   P(mutex1);
   pthread_create(&id1, NULL, locker, &mutex1);
   pthread_create(&id2, NULL, locker, &mutex1);
   pthread_create(&id3, NULL, locker, &mutex1);
   sleep(2);
   nok(lmgr_detect_deadlock(), "Check for multiple lock");
   V(mutex1);
   pthread_join(id1, NULL);
   pthread_join(id2, NULL);   
   pthread_join(id3, NULL);


   brwlock_t wr;
   rwl_init(&wr);
   rwl_writelock(&wr);
   rwl_writelock(&wr);
   pthread_create(&id1, NULL, rwlocker, &wr);
   pthread_create(&id2, NULL, rwlocker, &wr);
   pthread_create(&id3, NULL, rwlocker, &wr);
   nok(lmgr_detect_deadlock(), "Check for multiple rwlock");
   rwl_writeunlock(&wr);
   nok(lmgr_detect_deadlock(), "Check for simple rwlock");
   rwl_writeunlock(&wr);
   nok(lmgr_detect_deadlock(), "Check for multiple rwlock");

   pthread_join(id1, NULL);
   pthread_join(id2, NULL);   
   pthread_join(id3, NULL);   

   rwl_writelock(&wr);
   P(mutex1);
   pthread_create(&id1, NULL, mix_rwl_mutex, &wr);
   nok(lmgr_detect_deadlock(), "Check for mix rwlock/mutex");
   V(mutex1);
   nok(lmgr_detect_deadlock(), "Check for mix rwlock/mutex");
   rwl_writeunlock(&wr);
   nok(lmgr_detect_deadlock(), "Check for mix rwlock/mutex");
   pthread_join(id1, NULL);

   P(mutex5);
   P(mutex6);
   V(mutex5);
   V(mutex6);

   nok(lmgr_detect_deadlock(), "Check for wrong order");

   for(int j=0; j<200; j++) {
      pthread_create(&tab[j], NULL, thx, NULL);
   }
   for(int j=0; j<200; j++) {
      pthread_join(tab[j], NULL);
      if (j%3) { lmgr_detect_deadlock();}
   }
   nok(lmgr_detect_deadlock(), "Check 200 lockers");

   P(mutex4);
   P(mutex5);
   P(mutex6);
   V(mutex6);
   V(mutex5);
   V(mutex4);

   pthread_create(&id1, NULL, th1, NULL);
   sleep(1);
   pthread_create(&id2, NULL, th2, NULL);
   sleep(1);
   ok(lmgr_detect_deadlock(), "Check for deadlock");

//   lmgr_dump();
//
//   pthread_create(&id3, NULL, th3, NULL);
//
//   pthread_join(id1, NULL);
//   pthread_join(id2, NULL);
   lmgr_cleanup_main();
   sm_check(__FILE__, __LINE__, false);
   return report();
}

#endif
