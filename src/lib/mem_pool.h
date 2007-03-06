/*
 * Memory Pool prototypes
 *
 *  Kern Sibbald, MM
 *
 *   Version $Id: mem_pool.h,v 1.11.4.1 2006/01/15 11:18:45 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifndef __MEM_POOL_H_
#define __MEM_POOL_H_


#ifdef SMARTALLOC

#define get_pool_memory(pool) sm_get_pool_memory(__FILE__, __LINE__, pool)
extern POOLMEM *sm_get_pool_memory(const char *file, int line, int pool);

#define get_memory(size) sm_get_memory(__FILE__, __LINE__, size)
extern POOLMEM *sm_get_memory(const char *fname, int line, int32_t size);

#define sizeof_pool_memory(buf) sm_sizeof_pool_memory(__FILE__, __LINE__, buf)
extern int32_t sm_sizeof_pool_memory(const char *fname, int line, POOLMEM *buf);

#define realloc_pool_memory(buf,size) sm_realloc_pool_memory(__FILE__, __LINE__, buf, size)
extern POOLMEM  *sm_realloc_pool_memory(const char *fname, int line, POOLMEM *buf, int32_t size);

#define check_pool_memory_size(buf,size) sm_check_pool_memory_size(__FILE__, __LINE__, buf, size)
extern POOLMEM  *sm_check_pool_memory_size(const char *fname, int line, POOLMEM *buf, int32_t size);

#define free_pool_memory(x) sm_free_pool_memory(__FILE__, __LINE__, x)
#define free_memory(x) sm_free_pool_memory(__FILE__, __LINE__, x)
extern void sm_free_pool_memory(const char *fname, int line, POOLMEM *buf);


#else

extern POOLMEM *get_pool_memory(int pool);
extern POOLMEM *get_memory(int32_t size);
extern int32_t sizeof_pool_memory(POOLMEM *buf);
extern POOLMEM  *realloc_pool_memory(POOLMEM *buf, int32_t size);
extern POOLMEM  *check_pool_memory_size(POOLMEM *buf, int32_t size);
#define free_memory(x) free_pool_memory(x)
extern void   free_pool_memory(POOLMEM *buf);

#endif

extern void garbage_collect_memory_pool();
extern void  close_memory_pool();
extern void  print_memory_pool_stats();



#define PM_NOPOOL  0                  /* nonpooled memory */
#define PM_NAME    1                  /* Bacula name */
#define PM_FNAME   2                  /* file name buffer */
#define PM_MESSAGE 3                  /* daemon message */
#define PM_EMSG    4                  /* error message */
#define PM_MAX     PM_EMSG            /* Number of types */

class POOL_MEM {
   char *mem;
public:
   POOL_MEM() { mem = get_pool_memory(PM_NAME); *mem = 0; }
   POOL_MEM(int pool) { mem = get_pool_memory(pool); *mem = 0; }
   ~POOL_MEM() { free_pool_memory(mem); mem = NULL; }
   char *c_str() const { return mem; }
   int size() const { return sizeof_pool_memory(mem); }
   char *check_size(int32_t size) {
      mem = check_pool_memory_size(mem, size);
      return mem;
   }
   int32_t max_size();
   void realloc_pm(int32_t size);
   int strcpy(const char *str);
   int strcat(const char *str);
};

int pm_strcat (POOLMEM **pm, const char *str);
int pm_strcat (POOLMEM *&pm, const char *str);
int pm_strcat (POOL_MEM &pm, const char *str);
int pm_strcat (POOLMEM *&pm, POOL_MEM &str);
int pm_strcpy (POOLMEM **pm, const char *str);
int pm_strcpy (POOLMEM *&pm, const char *str);
int pm_strcpy (POOL_MEM &pm, const char *str);
int pm_strcpy (POOLMEM *&pm, POOL_MEM &str);


#endif
