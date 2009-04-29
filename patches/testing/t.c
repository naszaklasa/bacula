#include <sys/mtio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef FTAPE
#include "faketape.h"

#define write	faketape_write
#define open    faketape_open
#define read    faketape_read
#define close   faketape_close
#define ioctl   faketape_ioctl
#define debug(x) faketape_debug(x)
#else
#define debug(x) 
#endif

static int fd;
void print_pos()
{
   struct mtget mt_get;
   ioctl(fd, MTIOCGET, &mt_get);
   printf("*** file:block %i:%i BOT=%u EOD=%u EOF=%u\n", 
	  mt_get.mt_fileno, mt_get.mt_blkno, 
	  GMT_BOT(mt_get.mt_gstat) != 0,
	  GMT_EOD(mt_get.mt_gstat) != 0,
	  GMT_EOF(mt_get.mt_gstat) != 0
      );
}

int main(int argc, char **argv)
{
   int r1;
   char c[200];
   struct mtop mt_com;
   
   if (argc > 1) {
      debug(atoi(argv[1]));
   }

   fd  = open("/dev/lto2", O_CREAT | O_RDWR);
   if (fd < 0) {
      perror("Can't open fd");
      exit(1);
   }

   print_pos();

   /* rewind */
   printf("\n*** rewind\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTREW;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   exit(0);

   /* write something */
   printf("\n*** write something (3 writes)\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();

   /* write EOF */
   printf("\n*** WEOF\n");
   mt_com.mt_op = MTWEOF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   write(fd, "12345", strlen("12345")+1);
   write(fd, "678910", strlen("678910")+1);

   /* write EOF */
   printf("\n*** WEOF\n");
   mt_com.mt_op = MTWEOF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   

   /* BSF */
   printf("\n*** bsf\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTBSF;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   /* BSR */
   printf("\n*** bsr\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTBSR;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();

   /* read something */
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   printf("\n*** read c=%s len=%i\n", c, r1);
   print_pos();
   exit(0);

   /* FSR */
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   /* FSR */
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   /* FSR */
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   /* FSR */
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   /* FSR */
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   exit(0);


   /* write a second file */
   printf("\n*** write something\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();

    /* BSF */
   printf("\n*** bsf\n");
   mt_com.mt_op = MTBSF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();
   
   /* rewind */
   printf("\n*** rewind\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTREW;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);

   /* read something with error */
   printf("\n*** read c=%s len=%i\n", c, r1);
   errno=0;
   r1 = read(fd, c, 2);
   c[r1] = 0;
   perror("");
   print_pos();

   /* read something */
   printf("\n*** read c=%s len=%i\n", c, r1);
   errno=0;
   r1 = read(fd, c, 200);
   c[r1] = 0;
   print_pos();

   /* write something */
   printf("\n*** write something\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();
   
   /* rewind */
   printf("\n*** rewind\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTREW;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   r1 = read(fd, c, 200);
   c[r1] = '\0';
   printf("\n*** read c=%s len=%i\n", c, r1);
   r1 = read(fd, c, 200);
   c[r1] = '\0';
   printf("\n*** read c=%s len=%i\n", c, r1);
 
   /* write EOF */
   printf("\n*** WEOF");
   mt_com.mt_op = MTWEOF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* FSF */
   printf("\n*** fsf x1");
   mt_com.mt_op = MTFSF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* write something */
   printf("\n*** write something\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();

   /* FSF */
   printf("\n*** fsf");
   mt_com.mt_op = MTFSF;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* WEOF */
   printf("\n*** weof 3");
   mt_com.mt_op = MTWEOF;
   mt_com.mt_count = 3;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* rewind */
   printf("\n*** rewind\n");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTREW;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   print_pos();

   /* FSR */
   printf("\n*** fsr x10");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 10;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* eom */
   printf("\n*** goto eom");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTEOM;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* fsr */
   printf("\n*** fsr");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTFSR;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* write something */
   printf("\n*** write something\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();
   
   /* rewind */
   printf("\n*** rewind");
   mt_com.mt_count = 1;
   mt_com.mt_op = MTREW;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);

   /* FSF */
   printf("\n*** fsf x2");
   mt_com.mt_op = MTFSF;
   mt_com.mt_count = 2;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   /* write something */
   printf("\n*** write something\n");
   write(fd, "abcdefghijklmnopqrstuvwyz", strlen("abcdefghijklmnopqrstuvwyz")+1);
   print_pos();

   printf("\n*** bsf x2");
   mt_com.mt_op = MTBSF;
   mt_com.mt_count = 2;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   printf("\n*** bsf x10");
   mt_com.mt_op = MTBSF;
   mt_com.mt_count = 10;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   printf("\n*** eom");
   mt_com.mt_op = MTEOM;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   printf("\n*** bsr x10");
   mt_com.mt_op = MTBSR;
   mt_com.mt_count = 10;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   printf("\n*** eom");
   mt_com.mt_op = MTEOM;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();
   
   printf("\n*** fsr");
   mt_com.mt_op = MTFSR;
   mt_com.mt_count = 1;
   r1 = ioctl(fd, MTIOCTOP, &mt_com);
   printf(" r=%i\n", r1);
   print_pos();

   close(fd);
   return(0);
}
