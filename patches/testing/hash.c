
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

int64_t get_current_time()
{
   struct timeval tv;
   if (gettimeofday(&tv, NULL) != 0) {
      tv.tv_sec = (long)time(NULL);   /* fall back to old method */
      tv.tv_usec = 0;
   }
   return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}

#ifdef USE_TCHDB
# include <tcutil.h>
# include <tchdb.h>

TCHDB *hdb;
TCXSTR *kstr;
TCXSTR *dstr;

bool hash_init(int nbelt)
{
  int opt=HDBTLARGE ;//| HDBTTCBS;
   
  /* create the object */
  hdb = tchdbnew();

  kstr = tcxstrnew();
  dstr = tcxstrnew();

  tchdbsetcache(hdb, nbelt);
  fprintf(stderr, "cache;%i\n", nbelt);
  tchdbtune(hdb, nbelt*4, -1, 16, 0);

  fprintf(stderr, "bucket;%lli\n", (int64_t) nbelt*4);
  fprintf(stderr, "option;%i\n", opt);
  unlink("casket.hdb");

  /* open the database */
  if(!tchdbopen(hdb, "casket.hdb", HDBOWRITER | HDBOCREAT)){
     fprintf(stderr, "open error\n");
     exit (1);
  }
  return 1;
}

bool hash_put(char *key, char *val)
{
   int ecode;

   if (!tchdbputasync2(hdb, key, val)) {
      ecode = tchdbecode(hdb);
      fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
      return 0;
   }
   return 1;
}

char *hash_get(char *key, char *ret, int len)
{
   return tchdbget2(hdb, key);
}

void hash_free(char *ret)
{
   free(ret);
}

void cleanup()
{
   tchdbclose(hdb);
   tchdbdel(hdb);
}

void hash_iterinit()
{
  /* traverse records */
  tchdbiterinit(hdb);
}

bool hash_next(char *line, int len1, char *data, int len2)
{
   return tchdbiternext3(hdb, kstr, dstr);
}

void hash_iterdone()
{
}

#endif


#ifdef USE_TCBDB
# include <tcutil.h>
# include <tcbdb.h>

TCBDB *hdb;
TCXSTR *kstr;
TCXSTR *dstr;

bool hash_init(int nbelt)
{
  int opt=HDBTLARGE ;//| HDBTTCBS;
   
  /* create the object */
  hdb = tcbdbnew();

  kstr = tcxstrnew();
  dstr = tcxstrnew();

  fprintf(stderr, "cache;%i\n", -1);
  tcbdbtune(hdb, 0, 0, nbelt*4, -1, -1, opt);

  fprintf(stderr, "bucket;%lli\n", (int64_t) nbelt*4);
  fprintf(stderr, "option;%i\n", opt);
  unlink("casket.hdb");

  /* open the database */
  if(!tcbdbopen(hdb, "casket.hdb", HDBOWRITER | HDBOCREAT)){
     fprintf(stderr, "open error\n");
     exit (1);
  }
  return 1;
}

bool hash_put(char *key, char *val)
{
   int ecode;

   if (!tcbdbput(hdb, key, strlen(key), val, strlen(val)+1)) {
      ecode = tcbdbecode(hdb);
      fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
      return 0;
   }
   return 1;
}

char *hash_get(char *key, char *ret, int len)
{
   return tcbdbget2(hdb, key);
}

void hash_free(char *ret)
{
   free(ret);
}

void cleanup()
{
   tcbdbclose(hdb);
   tcbdbdel(hdb);
}
BDBCUR *iter;
void hash_iterinit()
{
  /* traverse records */
   iter = tcbdbcurnew(hdb);
   tcbdbcurfirst(iter);
}

bool hash_next(char *line, int len1, char *data, int len2)
{
   bool ret =  tcbdbcurrec(iter, kstr, dstr);
   tcbdbcurnext(iter);
   return ret;
}

void hash_iterdone()
{
   tcbdbcurdel(iter);
}

#endif


#ifdef USE_DB
# include <db.h>
DBT dbkey, dbdata;
DB *hdb;
bool hash_init(int nbelt)
{
   int ret;

   /* Create and initialize database object, open the database. */
   if ((ret = db_create(&hdb, NULL, 0)) != 0) {
      fprintf(stderr,
	      "db_create: %s\n", db_strerror(ret));
      exit (1);
   }
   hdb->set_errfile(hdb, stderr);
   hdb->set_errpfx(hdb, "hash");
   if ((ret = hdb->set_pagesize(hdb, 1024)) != 0) {
      hdb->err(hdb, ret, "set_pagesize");
      exit (1);
   }
   if ((ret = hdb->set_cachesize(hdb, 0, 32 * 1024 * 1024, 0)) != 0) {
      hdb->err(hdb, ret, "set_cachesize");
      exit (1);;
   }
   fprintf(stderr, "cache;%lli\n", (int64_t)32 * 1024 * 1024);
   fprintf(stderr, "bucket;0\n");
   fprintf(stderr, "option;0\n");

   remove("access.db");
   if ((ret = hdb->open(hdb,
			NULL, "access.db", NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
      hdb->err(hdb, ret, "access.db: open");
      exit (1);
   }
   memset(&dbkey, 0, sizeof(DBT));
   memset(&dbdata, 0, sizeof(DBT));

   return 1;
}

bool hash_put(char *key, char *val)
{
   int ret;
   dbkey.data = key;
   dbkey.size = (u_int32_t)strlen(key);
   dbdata.data = val;
   dbdata.size = strlen(val)+1;
   if ((ret = hdb->put(hdb, NULL, &dbkey, &dbdata, 0))) {
      hdb->err(hdb, ret, "DBcursor->put");
      return 0;
   }
   return 1;
}

void cleanup()
{
   hdb->close(hdb, 0);
}

char *hash_get(char *key, char *ret, int len)
{
/* Zero out the DBTs before using them. */
   memset(&dbkey, 0, sizeof(DBT));
   memset(&dbdata, 0, sizeof(DBT));
   
   dbkey.data = key;
   dbkey.ulen = strlen(key);
   
   dbdata.data = ret;
   dbdata.ulen = len;
   dbdata.flags = DB_DBT_USERMEM;

   if (hdb->get(hdb, NULL, &dbkey, &dbdata, 0) == 0) {
      return (char *)dbdata.data;
   } else {
      return NULL;
   }
}

void hash_free(char *ret)
{
}

DBC *cursorp=NULL;
void hash_iterinit()
{
   hdb->cursor(hdb, NULL, &cursorp, 0); 
}

bool hash_next(char *line, int len1, char *data, int len2)
{
   return cursorp->c_get(cursorp, &dbkey, &dbdata, DB_NEXT) == 0;
}

void hash_iterdone()
{
   if (cursorp != NULL) 
      cursorp->c_close(cursorp); 
}


#endif



int main(int argc, char **argv)
{
  char *start_heap = (char *)sbrk(0);

  char *value;
  int i=0;
  char save_key[200];
  char mkey[200];
  char data[200];
  int64_t ctime, ttime;

  if (argc != 4) {
      fprintf(stderr, "Usage: tt count file cache\n");
      exit(1);
  }
  
  hash_init(atoll(argv[1]));

  FILE *fp = fopen(argv[2], "r");
  if (!fp) {
     fprintf(stderr, "open %s file error\n", argv[2]);
     exit (1);
  }
  char line[2048];
  sprintf(data, "AOOODS SDOOQSD A BBBBB AAAAA ZEZEZQS SQDSQDQ DSQ DAZEA ZEA S");

  ctime = get_current_time();
  fprintf(stderr, "elt;time\n"); 
  while (fgets(line, sizeof(line), fp)) {
     i++;

     hash_put(line, data);

     if (i == 99) {
	strcpy(save_key, mkey);
     }

     if (i%100000 == 0) {
	ttime= get_current_time();
	fprintf(stderr, "%i;%i\n", 
	  i/100000, (int) ((ttime - ctime)/1000));
     }
  }
  ttime= get_current_time();
  fclose(fp);

  printf("loading %i file into hash database in %ims\n", 
	 i, (ttime - ctime)/1000);

  fprintf(stderr, "load;%i\n", (ttime - ctime)/1000);
  fprintf(stderr, "elt;%i\n", i);

  ////////////////////////////////////////////////////////////////

  fp = fopen(argv[2], "r");
  if (!fp) {
     fprintf(stderr, "open %s file error\n", argv[2]);
     exit (1);
  }
  i=0;
  ctime = get_current_time();
  char *p;
  fprintf(stderr, "elt;time\n"); 
  while (fgets(line, sizeof(line), fp)) {
     i++;
     p = hash_get(line, data, sizeof(data));
     if (p) {
	p[5]='H';
	hash_put(line, p);
     }

     if (i%100000 == 0) {
	ttime= get_current_time();
	fprintf(stderr, "%i;%i\n", 
	  i/100000, (int) ((ttime - ctime)/1000));
     }

     hash_free(p);
  }
  ttime= get_current_time();
  fprintf(stderr, "seen;%i\n", (ttime - ctime)/1000);

  fprintf(stderr, "elt;time\n"); 
  i=0;
  hash_iterinit();
  while(hash_next(line, sizeof(line), data, sizeof(data)))
  {
     i++;
     if (i%100000 == 0) {
	ttime= get_current_time();
	fprintf(stderr, "%i;%i\n", 
	  i/100000, (int) ((ttime - ctime)/1000));
     }
  }
  hash_iterdone();

  ttime = get_current_time();
  fprintf(stderr, "walk;%i\n", (ttime - ctime)/1000);

  fprintf(stderr, "heap;%lld\n", (long long)((char *)sbrk(0) - start_heap));
  
  cleanup();

  return 0;
}
