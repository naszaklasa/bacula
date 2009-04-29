/*
 * This file is used to test simple I/O operation for accurate mode
 *
 *
 * 1) Loading
 *    fseek()   ?
 *    fwrite()
 *
 * 2) Fetch + Update
 *    fseek(pos1)
 *    fread()
 *    fseek(pos1)
 *    fwrite()
 *
 * 3) Read all
 *    fseek()
 *    fread()
 *    
 */

/*
 cd regress/build
 make
 cd patches/testing
 g++ -g -Wall -I../../src -I../../src/lib -L../../src/lib justdisk.c -lbac -lpthread -lssl -D_TEST_TREE
 find / > lst
 ./a.out
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "bacula.h"
#include "dird/dird.h"
typedef struct {
   char *path;
   char *fname;
   int32_t seen;
   int32_t pad;
   int64_t mtime;
   int64_t ctime;
} AccurateElt;

#define NB_ELT 500

typedef struct {
   char *buf;
   rblink link;
} MY;

typedef struct {
   char *path;
   char *fname;
   int64_t index;
   rblink link;
} MY_INDEX;

static int my_index_cmp(void *item1, void *item2)
{
   MY_INDEX *elt1, *elt2;
   elt1 = (MY_INDEX *) item1;
   elt2 = (MY_INDEX *) item2;
   int r = strcmp(elt1->fname, elt2->fname);
   if (r) {
      return r;
   }
   r = strcmp(elt1->path, elt2->path);
   return r;
}

static int my_cmp(void *item1, void *item2)
{
   MY *elt1, *elt2;
   elt1 = (MY *) item1;
   elt2 = (MY *) item2;
   return strcmp(elt1->buf, elt2->buf);
}

rblist *load_rb(const char *file)
{
   FILE *fp;
   char buffer[1024];
   MY *res;
   rblist *lst;
   lst = New(rblist(res, &res->link));

   fp = fopen(file, "r");
   if (!fp) {
      return NULL;
   }
   while (fgets(buffer, sizeof(buffer), fp)) {
      if (*buffer) {
         int len = strlen(buffer);
         if (len > 0) {
            buffer[len-1]=0;       /* zap \n */
         }
         MY *buf = (MY *)malloc(sizeof(MY));
         memset(buf, 0, sizeof(MY));
         buf->buf = bstrdup(buffer);
         res = (MY *)lst->insert(buf, my_cmp);
         if (res != buf) {
            free(buf->buf);
            free(buf);
         }
      }
   }
   fclose(fp);

   return lst;
}

/* buffer used for 4k io operations */
class AccurateBuffer
{
public:
   AccurateBuffer() { 
      _fp=NULL; 
      _nb=-1;
      _max_nb=-1;
      _dirty=0; 
      memset(_buf, 0, sizeof(_buf));
   };
   void *get_elt(int nb);
   void init();
   void destroy();
   void update_elt(int nb);
   ~AccurateBuffer() {
      destroy();
   }
private:
   char _buf[4096];
   int _nb;
   int _max_nb;
   char _dirty;
   FILE *_fp;
};

void AccurateBuffer::init()
{
   _fp = fopen("testfile", "w+");
   if (!_fp) {
      exit(1);
   }
   _nb=-1;
   _max_nb=-1;
}

void AccurateBuffer::destroy()
{
   if (_fp) {
      fclose(_fp);
      _fp = NULL;
   }
}

void *AccurateBuffer::get_elt(int nb)
{
   int page=nb*sizeof(AccurateElt)/sizeof(_buf);

   if (!_fp) {
      init();
   }

   if (page != _nb) {		/* not the same page */
      if (_dirty) {		/* have to sync on disk */
//	 printf("put dirty page on disk %i\n", _nb);
	 if (fseek(_fp, _nb*sizeof(_buf), SEEK_SET) == -1) {
	    perror("bad fseek");
	    exit(3);
	 }
	 if (fwrite(_buf, sizeof(_buf), 1, _fp) != 1) {
	    perror("writing...");
	    exit(2);
	 }
	 _dirty=0;
      }
      if (page <= _max_nb) {	/* we read it only if the page exists */
//       printf("read page from disk %i <= %i\n", page, _max_nb);
	 fseek(_fp, page*sizeof(_buf), SEEK_SET);
	 if (fread(_buf, sizeof(_buf), 1, _fp) != 1) {
//	    printf("memset to zero\n");
	    memset(_buf, 0, sizeof(_buf));
	 }
      } else {
	 memset(_buf, 0, sizeof(_buf));
      }
      _nb = page;
      _max_nb = MAX(_max_nb, page);
   }

   /* compute addr of the element in _buf */
   int addr=(nb%(sizeof(_buf)/sizeof(AccurateElt)))*sizeof(AccurateElt);
// printf("addr=%i\n", addr);
   return (void *) (_buf + addr);
}

void AccurateBuffer::update_elt(int nb)
{
   _dirty = 1;
}

typedef struct B_DBx
{
   POOLMEM *fname;
   int fnl;
   POOLMEM *path;
   int pnl;
} B_DBx;

/*
 * Given a full filename, split it into its path
 *  and filename parts. They are returned in pool memory
 *  in the mdb structure.
 */
void split_path_and_file(B_DBx *mdb, const char *fname)
{
   const char *p, *f;

   /* Find path without the filename.
    * I.e. everything after the last / is a "filename".
    * OK, maybe it is a directory name, but we treat it like
    * a filename. If we don't find a / then the whole name
    * must be a path name (e.g. c:).
    */
   for (p=f=fname; *p; p++) {
      if (IsPathSeparator(*p)) {
         f = p;                       /* set pos of last slash */
      }
   }
   if (IsPathSeparator(*f)) {                   /* did we find a slash? */
      f++;                            /* yes, point to filename */
   } else {                           /* no, whole thing must be path name */
      f = p;
   }

   /* If filename doesn't exist (i.e. root directory), we
    * simply create a blank name consisting of a single
    * space. This makes handling zero length filenames
    * easier.
    */
   mdb->fnl = p - f;
   if (mdb->fnl > 0) {
      mdb->fname = check_pool_memory_size(mdb->fname, mdb->fnl+1);
      memcpy(mdb->fname, f, mdb->fnl);    /* copy filename */
      mdb->fname[mdb->fnl] = 0;
   } else {
      mdb->fname[0] = 0;
      mdb->fnl = 0;
   }

   mdb->pnl = f - fname;
   if (mdb->pnl > 0) {
      mdb->path = check_pool_memory_size(mdb->path, mdb->pnl+1);
      memcpy(mdb->path, fname, mdb->pnl);
      mdb->path[mdb->pnl] = 0;
   } else {
      mdb->path[0] = 0;
      mdb->pnl = 0;
   }

   Dmsg2(500, "split path=%s file=%s\n", mdb->path, mdb->fname);
}

#ifdef _TEST_TREE
int main()
{
   AccurateElt *elt;
   char *start_heap = (char *)sbrk(0);
   MY *myp, *res;
   MY_INDEX *idx;

   FILE *fp;
   fp = fopen("lst", "r");
   if (!fp) {
      exit (5);
   }
   rblist *index, *rb_file, *rb_path;
   index   = New(rblist(idx, &idx->link)); /* (pathid, filenameid) => index */
   rb_path = New(rblist(myp, &myp->link)); /* pathid => path */
   rb_file = New(rblist(myp, &myp->link)); /* filenameid => filename */

   B_DBx mdb;
   mdb.fname = get_pool_memory(PM_FNAME);
   mdb.path = get_pool_memory(PM_FNAME);

   AccurateBuffer *accbuf = new AccurateBuffer;
   char buf[4096];
   int64_t count=0;
   printf("loading...\n");
   while (fgets(buf, sizeof(buf), fp)) {
      int len = strlen(buf);
      if (len > 0) {
	 buf[len-1]=0;       /* zap \n */
      }
      split_path_and_file(&mdb, buf);

      MY_INDEX *idx = (MY_INDEX *)malloc(sizeof(MY_INDEX));
      memset(idx, 0, sizeof(MY_INDEX));

      myp = (MY *)malloc(sizeof(MY));
      memset(myp, 0, sizeof(MY));
      myp->buf = bstrdup(mdb.path);
      res = (MY *)rb_path->insert(myp, my_cmp);
      if (res != myp) {
	 free(myp->buf);
	 free(myp);
      }

      idx->path = res->buf;

      myp = (MY *)malloc(sizeof(MY));
      memset(myp, 0, sizeof(MY));
      myp->buf = bstrdup(mdb.fname);
      res = (MY *)rb_file->insert(myp, my_cmp);
      if (res != myp) {
	 free(myp->buf);
	 free(myp);
      }

      idx->fname = res->buf;

      idx->index = count;
      index->insert(idx, my_index_cmp);

      elt = (AccurateElt *)accbuf->get_elt(count);
      elt->mtime = count;
      elt->path = idx->path;
      elt->fname = idx->fname;
      accbuf->update_elt(count);

      count++;
   }

   printf("fetch and update...\n");
   fprintf(stderr, "count;%lli\n", count);
   fprintf(stderr, "heap;%lld\n", (long long)((char *)sbrk(0) - start_heap));
      
   fseek(fp, 0, SEEK_SET);
   int toggle=0;
   while (fgets(buf, sizeof(buf), fp)) {
      int len = strlen(buf);
      if (len > 0) {
	 buf[len-1]=0;       /* zap \n */
      }
      split_path_and_file(&mdb, buf);
      
      MY search;
      search.buf = mdb.fname;
      MY *f = (MY *)rb_file->search(&search, my_cmp);

      if (!f) {
	 continue;			/* not found, skip */
      }

      search.buf = mdb.path;
      MY *p = (MY *)rb_path->search(&search, my_cmp);
      if (!p) {
	 continue;			/* not found, skip */
      }

      MY_INDEX idx;
      idx.path = p->buf;
      idx.fname = f->buf;
      MY_INDEX *res = (MY_INDEX *)index->search(&idx, my_index_cmp);
      
      if (!res) {
	 continue;			/* not found skip */
      }

      AccurateElt *elt = (AccurateElt *)accbuf->get_elt(res->index);
      elt->seen=toggle;
      accbuf->update_elt(res->index);
      toggle = (toggle+1)%100000;
   }

   for(int j=0;j<=count; j++) {
      AccurateElt *elt = (AccurateElt *)accbuf->get_elt(j);
      if (elt->path && elt->fname) {
	 if (!elt->seen) {
	    printf("%s%s\n", elt->path, elt->fname);
	 }
      }
   }
}
#else
#ifdef _TEST_OPEN
int main()
{
   int fd;
   int i;
   AccurateElt elt;
   char *start_heap = (char *)sbrk(0);

   fd = open("testfile", O_CREAT | O_RDWR, 0600);
   if (fd<0) {
      perror("E: Can't open testfile ");
      return(1);
   }

   memset(&elt, 0, sizeof(elt));

   /* 1) Loading */
   for (i=0; i<NB_ELT; i++) {
      write(fd, &elt, sizeof(elt));
   }

   lseek(fd, 0, SEEK_SET);	/* rewind */

   /* 2) load and update */
   for (i=0; i<NB_ELT; i++) {
      lseek(fd, i*sizeof(AccurateElt), SEEK_SET);
      read(fd, &elt, sizeof(elt));
      lseek(fd, i*sizeof(AccurateElt), SEEK_SET);
      write(fd, &elt, sizeof(elt));
   }

   lseek(fd, 0, SEEK_SET);	/* rewind */

   /* 3) Fetch all of them */
   for (i=0; i<NB_ELT; i++) {
      read(fd, &elt, sizeof(elt));
   }

   close(fd);

   fprintf(stderr, "heap;%lld\n", (long long)((char *)sbrk(0) - start_heap));
   sleep(50);
   return (0);
}
#else  /* _TEST_OPEN */
#ifdef _TEST_BUF
int main()
{
   char *start_heap = (char *)sbrk(0);

   int i;
   AccurateElt *elt;
   AccurateBuffer *buf = new AccurateBuffer;

   /* 1) Loading */
   printf("Loading...\n");
   for (i=0; i<NB_ELT; i++) {
      elt = (AccurateElt *) buf->get_elt(i);
      elt->mtime = i;
      buf->update_elt(i);
   }

   /* 2) load and update */
   printf("Load and update...\n");
   for (i=0; i<NB_ELT; i++) {
      elt = (AccurateElt *) buf->get_elt(i);
      if (elt->mtime != i) {
	 printf("Something is wrong with elt %i mtime=%lli\n", i, elt->mtime);	 
	 exit (0);
      }
      elt->seen = i;
      buf->update_elt(i);
   }

   /* 3) Fetch all of them */
   printf("Fetch them...\n");
   for (i=0; i<NB_ELT; i++) {
      elt = (AccurateElt *) buf->get_elt(i);
      if (elt->seen != i || elt->mtime != i) {
	 printf("Something is wrong with elt %i mtime=%lli seen=%i\n", i, elt->mtime, elt->seen);
	 exit (0);
      }
   }
   fprintf(stderr, "heap;%lld\n", (long long)((char *)sbrk(0) - start_heap));
   delete buf;

   return(0);
}

#else
int main()
{
   FILE *fd;
   int i;
   AccurateElt elt;
   fd = fopen("testfile", "w+");
   if (!fd) {
      perror("E: Can't open testfile ");
      return(1);
   }

   memset(&elt, 0, sizeof(elt));

   /* 1) Loading */
   printf("Loading...\n");
   for (i=0; i<NB_ELT; i++) {
      fwrite(&elt, sizeof(elt), 1, fd);
   }

   fseek(fd, 0, SEEK_SET);	/* rewind */

   /* 2) load and update */
   printf("Load and update...\n");
   for (i=0; i<NB_ELT; i++) {
      fseek(fd, i*sizeof(AccurateElt), SEEK_SET);
      fread(&elt, sizeof(elt), 1, fd);
      fseek(fd, i*sizeof(AccurateElt), SEEK_SET);
      fwrite(&elt, sizeof(elt), 1, fd);
   }

   fseek(fd, 0, SEEK_SET);	/* rewind */

   /* 3) Fetch all of them */
   printf("Fetch them...\n");
   for (i=0; i<NB_ELT; i++) {
      fread(&elt, sizeof(elt), 1, fd);
   }

   fclose(fd);
   return(0);
}
#endif	/* _TEST_BUF */
#endif	/* _TEST_OPEN */
#endif  /* _TEST_TREE */
