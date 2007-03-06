/*

        Definitions for the smart memory allocator
  
     Version $Id: smartall.h,v 1.10 2004/08/13 20:27:05 kerns Exp $

*/

/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

extern uint64_t sm_max_bytes;
extern uint64_t sm_bytes;
extern uint32_t sm_max_buffers;
extern uint32_t sm_buffers;

#ifdef  SMARTALLOC
#undef  SMARTALLOC
#define SMARTALLOC SMARTALLOC


extern void *sm_malloc(const char *fname, int lineno, unsigned int nbytes),
            *sm_calloc(const char *fname, int lineno,
                unsigned int nelem, unsigned int elsize),
            *sm_realloc(const char *fname, int lineno, void *ptr, unsigned int size),
            *actuallymalloc(unsigned int size),
            *actuallycalloc(unsigned int nelem, unsigned int elsize),
            *actuallyrealloc(void *ptr, unsigned int size);
extern void sm_free(const char *fname, int lineno, void *fp);
extern void actuallyfree(void *cp),
            sm_dump(bool bufdump), sm_static(int mode);
extern void sm_new_owner(const char *fname, int lineno, char *buf);

#ifdef SMCHECK
extern void sm_check(const char *fname, int lineno, bool bufdump);
extern int sm_check_rtn(const char *fname, int lineno, bool bufdump);
#else
#define sm_check(f, l, fl)
#define sm_check_rtn(f, l, fl) 1
#endif


/* Redefine standard memory allocator calls to use our routines
   instead. */

#define free(x)        sm_free(__FILE__, __LINE__, (x))
#define cfree(x)       sm_free(__FILE__, __LINE__, (x))
#define malloc(x)      sm_malloc(__FILE__, __LINE__, (x))
#define calloc(n,e)    sm_calloc(__FILE__, __LINE__, (n), (e))
#define realloc(p,x)   sm_realloc(__FILE__, __LINE__, (p), (x))

#else

/* If SMARTALLOC is disabled, define its special calls to default to
   the standard routines.  */

#define actuallyfree(x)      free(x)
#define actuallymalloc(x)    malloc(x)
#define actuallycalloc(x,y)  calloc(x,y)
#define actuallyrealloc(x,y) realloc(x,y)
#define sm_dump(x)
#define sm_static(x)
#define sm_new_owner(a, b, c)
#define sm_malloc(f, l, n) malloc(n)

#define sm_check(f, l, fl)
#define sm_check_rtn(f, l, fl) 1

extern void *b_malloc();
#define malloc(x) b_malloc(__FILE__, __LINE__, (x))                  


#endif

#ifdef SMARTALLOC
// #ifdef xxx

#define New(type) new(__FILE__, __LINE__) type

class SMARTALLOC
{
public:

void *operator new(size_t s, const char *fname, int line)
{
  void *p = sm_malloc(fname, line, s > sizeof(int) ? s : sizeof(int));
  return p;
}
void *operator new[](size_t s, const char *fname, int line)
{
   void *p = sm_malloc(fname, line, s > sizeof(int) ? s : sizeof(int));
   return p;
}
void  operator delete(void *ptr)
{
   free(ptr);
}
void  operator delete[](void *ptr, size_t i)
{
   free(ptr);
}

void  operator delete(void *ptr, const char *fname, int line)
{
   free(ptr);
}
void  operator delete[](void *ptr, size_t i, const char *fname, int line) 
{
   free(ptr);
}


private:
void *operator new(size_t s) throw() { return 0; }
void *operator new[](size_t s) throw() { return 0; }
};
 

#else

#define New(type) new type

class SMARTALLOC
{
   public:
      void *operator new(size_t s)
      {
          return malloc(s);
      }
      void *operator new[](size_t s)
      {
          return malloc(s);
      }
      void  operator delete(void *ptr)
      {
          free(ptr);
      }
      void  operator delete[](void *ptr, size_t i)
      {
          free(ptr);
      }
};
#endif
