/*
 * Miscellaneous Bacula memory and thread safe routines
 *   Generally, these are interfaces to system or standard
 *   library routines.
 *
 *  Bacula utility functions are in util.c
 *
 *   Version $Id: bsys.c,v 1.42.2.4 2005/12/22 21:35:24 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */


#include "bacula.h"
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t timer = PTHREAD_COND_INITIALIZER;

/*
 * This routine will sleep (sec, microsec).  Note, however, that if a
 *   signal occurs, it will return early.  It is up to the caller
 *   to recall this routine if he/she REALLY wants to sleep the
 *   requested time.
 */
int bmicrosleep(time_t sec, long usec)
{
   struct timespec timeout;
   struct timeval tv;
   struct timezone tz;
   int stat;

   timeout.tv_sec = sec;
   timeout.tv_nsec = usec * 1000;

#ifdef HAVE_NANOSLEEP
   stat = nanosleep(&timeout, NULL);
   if (!(stat < 0 && errno == ENOSYS)) {
      return stat;
   }
   /* If we reach here it is because nanosleep is not supported by the OS */
#endif

   /* Do it the old way */
   gettimeofday(&tv, &tz);
   timeout.tv_nsec += tv.tv_usec * 1000;
   timeout.tv_sec += tv.tv_sec;
   while (timeout.tv_nsec >= 1000000000) {
      timeout.tv_nsec -= 1000000000;
      timeout.tv_sec++;
   }

   Dmsg2(200, "pthread_cond_timedwait sec=%d usec=%d\n", sec, usec);
   /* Note, this unlocks mutex during the sleep */
   P(timer_mutex);
   stat = pthread_cond_timedwait(&timer, &timer_mutex, &timeout);
   if (stat != 0) {
      berrno be;
      Dmsg2(200, "pthread_cond_timedwait stat=%d ERR=%s\n", stat,
         be.strerror(stat));
   }
   V(timer_mutex);
   return stat;
}

/*
 * Guarantee that the string is properly terminated */
char *bstrncpy(char *dest, const char *src, int maxlen)
{
   strncpy(dest, src, maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}

/*
 * Guarantee that the string is properly terminated */
char *bstrncpy(char *dest, POOL_MEM &src, int maxlen)
{
   strncpy(dest, src.c_str(), maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}


char *bstrncat(char *dest, const char *src, int maxlen)
{
   strncat(dest, src, maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}

char *bstrncat(char *dest, POOL_MEM &src, int maxlen)
{
   strncat(dest, src.c_str(), maxlen-1);
   dest[maxlen-1] = 0;
   return dest;
}

/*
 * Get character length of UTF-8 string
 *
 * Valid UTF-8 codes
 * U-00000000 - U-0000007F: 0xxxxxxx 
 * U-00000080 - U-000007FF: 110xxxxx 10xxxxxx 
 * U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx 
 * U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
 * U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
int cstrlen(const char *str)
{
   uint8_t *p = (uint8_t *)str;
   int len = 0;
   while (*p) {
      if ((*p & 0xC0) != 0xC0) {
         p++;
         len++;
         continue;
      }
      if ((*p & 0xD0) == 0xC0) {
         p += 2;
         len++;
         continue;
      }
      if ((*p & 0xF0) == 0xD0) {
         p += 3;
         len++;
         continue;
      }
      if ((*p & 0xF8) == 0xF0) {
         p += 4;
         len++;
         continue;
      }
      if ((*p & 0xFC) == 0xF8) {
         p += 5;
         len++;
         continue;
      }
      if ((*p & 0xFE) == 0xFC) {
         p += 6;
         len++;
         continue;
      }
      p++;                      /* Shouln't get here but must advance */
   }
   return len;
}



#ifndef DEBUG
void *bmalloc(size_t size)
{
  void *buf;

  buf = malloc(size);
  if (buf == NULL) {
     Emsg1(M_ABORT, 0, _("Out of memory: ERR=%s\n"), strerror(errno));
  }
  return buf;
}
#endif

void *b_malloc(const char *file, int line, size_t size)
{
  void *buf;

#ifdef SMARTALLOC
  buf = sm_malloc(file, line, size);
#else
  buf = malloc(size);
#endif
  if (buf == NULL) {
     e_msg(file, line, M_ABORT, 0, _("Out of memory: ERR=%s\n"), strerror(errno));
  }
  return buf;
}


void *brealloc (void *buf, size_t size)
{
   buf = realloc(buf, size);
   if (buf == NULL) {
      Emsg1(M_ABORT, 0, _("Out of memory: ERR=%s\n"), strerror(errno));
   }
   return buf;
}


void *bcalloc (size_t size1, size_t size2)
{
  void *buf;

   buf = calloc(size1, size2);
   if (buf == NULL) {
      Emsg1(M_ABORT, 0, _("Out of memory: ERR=%s\n"), strerror(errno));
   }
   return buf;
}

/* Code now in src/lib/bsnprintf.c */
#ifndef USE_BSNPRINTF

#define BIG_BUF 5000
/*
 * Implement snprintf
 */
int bsnprintf(char *str, int32_t size, const char *fmt,  ...)
{
   va_list   arg_ptr;
   int len;

   va_start(arg_ptr, fmt);
   len = bvsnprintf(str, size, fmt, arg_ptr);
   va_end(arg_ptr);
   return len;
}

/*
 * Implement vsnprintf()
 */
int bvsnprintf(char *str, int32_t size, const char  *format, va_list ap)
{
#ifdef HAVE_VSNPRINTF
   int len;
   len = vsnprintf(str, size, format, ap);
   str[size-1] = 0;
   return len;

#else

   int len, buflen;
   char *buf;
   buflen = size > BIG_BUF ? size : BIG_BUF;
   buf = get_memory(buflen);
   len = vsprintf(buf, format, ap);
   if (len >= buflen) {
      Emsg0(M_ABORT, 0, _("Buffer overflow.\n"));
   }
   memcpy(str, buf, len);
   str[len] = 0;                /* len excludes the null */
   free_memory(buf);
   return len;
#endif
}
#endif /* USE_BSNPRINTF */

#ifndef HAVE_LOCALTIME_R

struct tm *localtime_r(const time_t *timep, struct tm *tm)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    struct tm *ltm,

    P(mutex);
    ltm = localtime(timep);
    if (ltm) {
       memcpy(tm, ltm, sizeof(struct tm));
    }
    V(mutex);
    return ltm ? tm : NULL;
}
#endif /* HAVE_LOCALTIME_R */

#ifndef HAVE_READDIR_R
#ifndef HAVE_WIN32
#include <dirent.h>

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    struct dirent *ndir;
    int stat;

    P(mutex);
    errno = 0;
    ndir = readdir(dirp);
    stat = errno;
    if (ndir) {
       memcpy(entry, ndir, sizeof(struct dirent));
       strcpy(entry->d_name, ndir->d_name);
       *result = entry;
    } else {
       *result = NULL;
    }
    V(mutex);
    return stat;

}
#endif
#endif /* HAVE_READDIR_R */


int bstrerror(int errnum, char *buf, size_t bufsiz)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int stat = 0;
    const char *msg;

    P(mutex);

    msg = strerror(errnum);
    if (!msg) {
       msg = _("Bad errno");
       stat = -1;
    }
    bstrncpy(buf, msg, bufsiz);
    V(mutex);
    return stat;
}

/*
 * These are mutex routines that do error checking
 *  for deadlock and such.  Normally not turned on.
 */
#ifdef DEBUG_MUTEX
void _p(char *file, int line, pthread_mutex_t *m)
{
   int errstat;
   if ((errstat = pthread_mutex_trylock(m))) {
      e_msg(file, line, M_ERROR, 0, _("Possible mutex deadlock.\n"));
      /* We didn't get the lock, so do it definitely now */
      if ((errstat=pthread_mutex_lock(m))) {
         berrno be;
         e_msg(file, line, M_ABORT, 0, _("Mutex lock failure. ERR=%s\n"),
               be.strerror(errstat));
      } else {
         e_msg(file, line, M_ERROR, 0, _("Possible mutex deadlock resolved.\n"));
      }

   }
}

void _v(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat=pthread_mutex_trylock(m)) == 0) {
      berrno be;
      e_msg(file, line, M_ERROR, 0, _("Mutex unlock not locked. ERR=%s\n"),
           be.strerror(errstat));
    }
    if ((errstat=pthread_mutex_unlock(m))) {
       berrno be;
       e_msg(file, line, M_ABORT, 0, _("Mutex unlock failure. ERR=%s\n"),
              be.strerror(errstat));
    }
}

#else

void _p(pthread_mutex_t *m)
{
   int errstat;
   if ((errstat=pthread_mutex_lock(m))) {
      berrno be;
      e_msg(__FILE__, __LINE__, M_ABORT, 0, _("Mutex lock failure. ERR=%s\n"),
            be.strerror(errstat));
   }
}

void _v(pthread_mutex_t *m)
{
   int errstat;
   if ((errstat=pthread_mutex_unlock(m))) {
      berrno be;
      e_msg(__FILE__, __LINE__, M_ABORT, 0, _("Mutex unlock failure. ERR=%s\n"),
            be.strerror(errstat));
   }
}

#endif /* DEBUG_MUTEX */

#ifdef DEBUG_MEMSET
/* These routines are not normally turned on */
#undef memset
void b_memset(const char *file, int line, void *mem, int val, size_t num)
{
   /* Testing for 2000 byte zero at beginning of Volume block */
   if (num > 1900 && num < 3000) {
      Pmsg3(000, _("Memset for %d bytes at %s:%d\n"), (int)num, file, line);
   }
   memset(mem, val, num);
}
#endif

#if !defined(HAVE_CYGWIN) && !defined(HAVE_WIN32)
static int del_pid_file_ok = FALSE;
#endif

/*
 * Create a standard "Unix" pid file.
 */
void create_pid_file(char *dir, const char *progname, int port)
{
#if !defined(HAVE_CYGWIN) && !defined(HAVE_WIN32)
   int pidfd, len;
   int oldpid;
   char  pidbuf[20];
   POOLMEM *fname = get_pool_memory(PM_FNAME);
   struct stat statp;

   Mmsg(&fname, "%s/%s.%d.pid", dir, progname, port);
   if (stat(fname, &statp) == 0) {
      /* File exists, see what we have */
      *pidbuf = 0;
      if ((pidfd = open(fname, O_RDONLY|O_BINARY, 0)) < 0 ||
           read(pidfd, &pidbuf, sizeof(pidbuf)) < 0 ||
           sscanf(pidbuf, "%d", &oldpid) != 1) {
         Emsg2(M_ERROR_TERM, 0, _("Cannot open pid file. %s ERR=%s\n"), fname, strerror(errno));
      }
      /* See if other Bacula is still alive */
      if (kill(oldpid, 0) != -1 || errno != ESRCH) {
         Emsg3(M_ERROR_TERM, 0, _("%s is already running. pid=%d\nCheck file %s\n"),
               progname, oldpid, fname);
      }
      /* He is not alive, so take over file ownership */
      unlink(fname);                  /* remove stale pid file */
   }
   /* Create new pid file */
   if ((pidfd = open(fname, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0640)) >= 0) {
      len = sprintf(pidbuf, "%d\n", (int)getpid());
      write(pidfd, pidbuf, len);
      close(pidfd);
      del_pid_file_ok = TRUE;         /* we created it so we can delete it */
   } else {
      Emsg2(M_ERROR_TERM, 0, _("Could not open pid file. %s ERR=%s\n"), fname, strerror(errno));
   }
   free_pool_memory(fname);
#endif
}


/*
 * Delete the pid file if we created it
 */
int delete_pid_file(char *dir, const char *progname, int port)
{
#if !defined(HAVE_CYGWIN)  && !defined(HAVE_WIN32)
   POOLMEM *fname = get_pool_memory(PM_FNAME);

   if (!del_pid_file_ok) {
      free_pool_memory(fname);
      return 0;
   }
   del_pid_file_ok = FALSE;
   Mmsg(&fname, "%s/%s.%d.pid", dir, progname, port);
   unlink(fname);
   free_pool_memory(fname);
#endif
   return 1;
}

struct s_state_hdr {
   char id[14];
   int32_t version;
   uint64_t last_jobs_addr;
   uint64_t reserved[20];
};

static struct s_state_hdr state_hdr = {
   "Bacula State\n",
   3,
   0
};

#ifdef HAVE_WIN32
#undef open
#undef read
#undef write
#undef lseek
#undef close
#undef O_BINARY 
#define open _open
#define read _read
#define write _write
#define lseek _lseeki64
#define close _close
#define O_BINARY _O_BINARY
#endif

/*
 * Open and read the state file for the daemon
 */
void read_state_file(char *dir, const char *progname, int port)
{
   int sfd;
   ssize_t stat;
   bool ok = false;
   POOLMEM *fname = get_pool_memory(PM_FNAME);
   struct s_state_hdr hdr;
   int hdr_size = sizeof(hdr);

   Mmsg(&fname, "%s/%s.%d.state", dir, progname, port);
   /* If file exists, see what we have */
// Dmsg1(10, "O_BINARY=%d\n", O_BINARY);
   if ((sfd = open(fname, O_RDONLY|O_BINARY)) < 0) {
      Dmsg3(010, "Could not open state file. sfd=%d size=%d: ERR=%s\n",
                    sfd, sizeof(hdr), strerror(errno));
      goto bail_out;
   }
   if ((stat=read(sfd, &hdr, hdr_size)) != hdr_size) {
      Dmsg4(010, "Could not read state file. sfd=%d stat=%d size=%d: ERR=%s\n",
                    sfd, (int)stat, hdr_size, strerror(errno));
      goto bail_out;
   }
   if (hdr.version != state_hdr.version) {
      Dmsg2(010, "Bad hdr version. Wanted %d got %d\n",
         state_hdr.version, hdr.version);
      goto bail_out;
   }
   hdr.id[13] = 0;
   if (strcmp(hdr.id, state_hdr.id) != 0) {
      Dmsg0(000, "State file header id invalid.\n");
      goto bail_out;
   }
// Dmsg1(010, "Read header of %d bytes.\n", sizeof(hdr));
   if (!read_last_jobs_list(sfd, hdr.last_jobs_addr)) {
      goto bail_out;
   }
   ok = true;
bail_out:
   if (sfd >= 0) {
      close(sfd);
   }
   if (!ok) {
      unlink(fname);
    }
   free_pool_memory(fname);
}

/*
 * Write the state file
 */
void write_state_file(char *dir, const char *progname, int port)
{
   int sfd;
   bool ok = false;
   POOLMEM *fname = get_pool_memory(PM_FNAME);

   Mmsg(&fname, "%s/%s.%d.state", dir, progname, port);
   /* Create new state file */
   unlink(fname);
   if ((sfd = open(fname, O_CREAT|O_WRONLY|O_BINARY, 0640)) < 0) {
      berrno be;
      Dmsg2(000, "Could not create state file. %s ERR=%s\n", fname, be.strerror());
      Emsg2(M_ERROR, 0, _("Could not create state file. %s ERR=%s\n"), fname, be.strerror());
      goto bail_out;
   }
   if (write(sfd, &state_hdr, sizeof(state_hdr)) != sizeof(state_hdr)) {
      berrno be;
      Dmsg1(000, "Write hdr error: ERR=%s\n", be.strerror());
      goto bail_out;
   }
// Dmsg1(010, "Wrote header of %d bytes\n", sizeof(state_hdr));
   state_hdr.last_jobs_addr = sizeof(state_hdr);
   state_hdr.reserved[0] = write_last_jobs_list(sfd, state_hdr.last_jobs_addr);
// Dmsg1(010, "write last job end = %d\n", (int)state_hdr.reserved[0]);
   if (lseek(sfd, 0, SEEK_SET) < 0) {
      berrno be;
      Dmsg1(000, "lseek error: ERR=%s\n", be.strerror());
      goto bail_out;
   }
   if (write(sfd, &state_hdr, sizeof(state_hdr)) != sizeof(state_hdr)) {
      berrno be;
      Pmsg1(000, _("Write final hdr error: ERR=%s\n"), be.strerror());
      goto bail_out;
   }
   ok = true;
// Dmsg1(010, "rewrote header = %d\n", sizeof(state_hdr));
bail_out:
   if (sfd >= 0) {
      close(sfd);
   }
   if (!ok) {
      unlink(fname);
   }
   free_pool_memory(fname);
}


/*
 * Drop to privilege new userid and new gid if non-NULL
 */
void drop(char *uid, char *gid)
{
#ifdef HAVE_GRP_H
   if (gid) {
      struct group *group;
      gid_t gr_list[1];

      if ((group = getgrnam(gid)) == NULL) {
         Emsg1(M_ERROR_TERM, 0, _("Could not find specified group: %s\n"), gid);
      }
      if (setgid(group->gr_gid)) {
         Emsg1(M_ERROR_TERM, 0, _("Could not set specified group: %s\n"), gid);
      }
      gr_list[0] = group->gr_gid;
      if (setgroups(1, gr_list)) {
         Emsg1(M_ERROR_TERM, 0, _("Could not set specified group: %s\n"), gid);
      }
   }
#endif

#ifdef HAVE_PWD_H
   if (uid) {
      struct passwd *passw;
      if ((passw = getpwnam(uid)) == NULL) {
         Emsg1(M_ERROR_TERM, 0, _("Could not find specified userid: %s\n"), uid);
      }
      if (setuid(passw->pw_uid)) {
         Emsg1(M_ERROR_TERM, 0, _("Could not set specified userid: %s\n"), uid);
      }
   }
#endif

}


/* BSDI does not have this.  This is a *poor* simulation */
#ifndef HAVE_STRTOLL
long long int
strtoll(const char *ptr, char **endptr, int base)
{
   return (long long int)strtod(ptr, endptr);
}
#endif

/*
 * Bacula's implementation of fgets(). The difference is that it handles
 *   being interrupted by a signal (e.g. a SIGCHLD).
 */
#undef fgetc
char *bfgets(char *s, int size, FILE *fd)
{
   char *p = s;
   int ch;
   *p = 0;
   for (int i=0; i < size-1; i++) {
      do {
         errno = 0;
         ch = fgetc(fd);
      } while (ch == -1 && (errno == EINTR || errno == EAGAIN));
      if (ch == -1) {
         if (i == 0) {
            return NULL;
         } else {
            return s;
         }
      }
      *p++ = ch;
      *p = 0;
      if (ch == '\r') { /* Support for Mac/Windows file format */
         ch = fgetc(fd);
         if (ch == '\n') { /* Windows (\r\n) */
            *p++ = ch;
            *p = 0;
         }
         else { /* Mac (\r only) */
            (void)ungetc(ch, fd); /* Push next character back to fd */
         }
         break;
      }
      if (ch == '\n') {
         break;
      }
   }
   return s;
}

/*
 * Make a "unique" filename.  It is important that if
 *   called again with the same "what" that the result
 *   will be identical. This allows us to use the file
 *   without saving its name, and re-generate the name
 *   so that it can be deleted.
 */
void make_unique_filename(POOLMEM **name, int Id, char *what)
{
   Mmsg(name, "%s/%s.%s.%d.tmp", working_directory, my_name, what, Id);
}
