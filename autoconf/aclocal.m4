dnl
dnl =========  Large File Support ==============
dnl By default, many hosts won't let programs access large files;
dnl one must use special compiler options to get large-file access to work.
dnl For more details about this brain damage please see:
dnl http://www.sas.com/standards/large.file/x_open.20Mar96.html

dnl Written by Paul Eggert <eggert@twinsun.com>.
dnl 
dnl Modified by Kern Sibbald to turn on the large file
dnl   flags on all machines. Otherwise functions such as
dnl   fseek are not large file capable.
dnl

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_FLAGS(FLAGSNAME)
AC_DEFUN(AC_SYS_LARGEFILE_FLAGS,
  [AC_CACHE_CHECK([for $1 value to request large file support],
     ac_cv_sys_largefile_$1,
     [ac_cv_sys_largefile_$1=`($GETCONF LFS_$1) 2>/dev/null` || {
        ac_cv_sys_largefile_$1=no
        ifelse($1, CFLAGS,
          [case "$host_os" in
           # IRIX 6.2 and later require cc -n32.
changequote(, )dnl
           irix6.[2-9]* | irix6.1[0-9]* | irix[7-9].* | irix[1-9][0-9]*)
changequote([, ])dnl
             if test "$GCC" != yes; then
               ac_cv_sys_largefile_CFLAGS=-n32
             fi
             ac_save_CC="$CC"
             CC="$CC $ac_cv_sys_largefile_CFLAGS"
             AC_TRY_LINK(, , , ac_cv_sys_largefile_CFLAGS=no)
             CC="$ac_save_CC"
           esac])
      }])])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_SPACE_APPEND(VAR, VAL)
AC_DEFUN(AC_SYS_LARGEFILE_SPACE_APPEND,
  [case $2 in
   no) ;;
   ?*)
     case "[$]$1" in
     '') $1=$2 ;;
     *) $1=[$]$1' '$2 ;;
     esac ;;
   esac])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_MACRO_VALUE(C-MACRO, CACHE-VAR, COMMENT, CODE-TO-SET-DEFAULT)
AC_DEFUN(AC_SYS_LARGEFILE_MACRO_VALUE,
  [AC_CACHE_CHECK([for $1], $2,
     [$2=no
changequote(, )dnl
      $4
      for ac_flag in $ac_cv_sys_largefile_CFLAGS no; do
        case "$ac_flag" in
        -D$1)
          $2=1 ;;
        -D$1=*)
          $2=`expr " $ac_flag" : '[^=]*=\(.*\)'` ;;
        esac
      done
changequote([, ])dnl
      ])
   if test "[$]$2" != no; then
     AC_DEFINE_UNQUOTED([$1], [$]$2, [$3])
   fi])

AC_DEFUN(AC_BAC_LARGEFILE,
  [AC_REQUIRE([AC_CANONICAL_HOST])
   AC_ARG_ENABLE(largefile,
     [  --disable-largefile     omit support for large files])
   if test "$enable_largefile" != no; then
     AC_CHECK_TOOL(GETCONF, getconf)
     AC_SYS_LARGEFILE_FLAGS(CFLAGS)
     AC_SYS_LARGEFILE_FLAGS(LDFLAGS)
     AC_SYS_LARGEFILE_FLAGS(LIBS)

     for ac_flag in $ac_cv_sys_largefile_CFLAGS no; do
       case "$ac_flag" in
       no) ;;
       -D_FILE_OFFSET_BITS=*) ;;
       -D_LARGEFILE_SOURCE | -D_LARGEFILE_SOURCE=*) ;;
       -D_LARGE_FILES | -D_LARGE_FILES=*) ;;
       -D?* | -I?*)
         AC_SYS_LARGEFILE_SPACE_APPEND(CPPFLAGS, "$ac_flag") ;;
       *)
         AC_SYS_LARGEFILE_SPACE_APPEND(CFLAGS, "$ac_flag") ;;
       esac
     done
     AC_SYS_LARGEFILE_SPACE_APPEND(LDFLAGS, "$ac_cv_sys_largefile_LDFLAGS")
     AC_SYS_LARGEFILE_SPACE_APPEND(LIBS, "$ac_cv_sys_largefile_LIBS")
     AC_SYS_LARGEFILE_MACRO_VALUE(_FILE_OFFSET_BITS,
       ac_cv_sys_file_offset_bits,
       [Number of bits in a file offset, on hosts where this is settable.],
       [ac_cv_sys_file_offset_bits=64])
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE,
       ac_cv_sys_largefile_source,
       [Define to make fseeko etc. visible, on some hosts.],
       [ac_cv_sys_largefile_source=1]) 
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGE_FILES,
       ac_cv_sys_large_files,
       [Define for large files, on AIX-style hosts.],
          [ac_cv_sys_large_files=1])
   fi
  ])
dnl ==========================================================

dnl Check type of signal routines (posix, 4.2bsd, 4.1bsd or v7)
AC_DEFUN(SIGNAL_CHECK,
[AC_REQUIRE([AC_TYPE_SIGNAL])
AC_MSG_CHECKING(for type of signal functions)
AC_CACHE_VAL(bash_cv_signal_vintage,
[
  AC_TRY_LINK([#include <signal.h>],[
    sigset_t ss;
    struct sigaction sa;
    sigemptyset(&ss); sigsuspend(&ss);
    sigaction(SIGINT, &sa, (struct sigaction *) 0);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
  ], bash_cv_signal_vintage=posix,
  [
    AC_TRY_LINK([#include <signal.h>], [
        int mask = sigmask(SIGINT);
        sigsetmask(mask); sigblock(mask); sigpause(mask);
    ], bash_cv_signal_vintage=4.2bsd,
    [
      AC_TRY_LINK([
        #include <signal.h>
        RETSIGTYPE foo() { }], [
                int mask = sigmask(SIGINT);
                sigset(SIGINT, foo); sigrelse(SIGINT);
                sighold(SIGINT); sigpause(SIGINT);
        ], bash_cv_signal_vintage=svr3, bash_cv_signal_vintage=v7
    )]
  )]
)
])
AC_MSG_RESULT($bash_cv_signal_vintage)
if test "$bash_cv_signal_vintage" = posix; then
AC_DEFINE(HAVE_POSIX_SIGNALS)
elif test "$bash_cv_signal_vintage" = "4.2bsd"; then
AC_DEFINE(HAVE_BSD_SIGNALS)
elif test "$bash_cv_signal_vintage" = svr3; then
AC_DEFINE(HAVE_USG_SIGHOLD)
fi
])

AC_DEFUN(BA_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])


AC_DEFUN(BA_CHECK_OPSYS,
[
AC_CYGWIN
if test $HAVE_UNAME=yes -a x`uname -s` = xSunOS
then
        BA_CONDITIONAL(HAVE_SUN_OS, $TRUEPRG)
        AC_DEFINE(HAVE_SUN_OS)
else
        BA_CONDITIONAL(HAVE_SUN_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xOSF1
then
        BA_CONDITIONAL(HAVE_OSF1_OS, $TRUEPRG)
        AC_DEFINE(HAVE_OSF1_OS)
else
        BA_CONDITIONAL(HAVE_OSF1_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xAIX
then
        BA_CONDITIONAL(HAVE_AIX_OS, $TRUEPRG)
        AC_DEFINE(HAVE_AIX_OS)
else
        BA_CONDITIONAL(HAVE_AIX_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xHP-UX
then
        BA_CONDITIONAL(HAVE_HPUX_OS, $TRUEPRG)
        AC_DEFINE(HAVE_HPUX_OS)
else
        BA_CONDITIONAL(HAVE_HPUX_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xLinux
then
        BA_CONDITIONAL(HAVE_LINUX_OS, $TRUEPRG)
        AC_DEFINE(HAVE_LINUX_OS)
else
        BA_CONDITIONAL(HAVE_LINUX_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xFreeBSD
then
        BA_CONDITIONAL(HAVE_FREEBSD_OS, $TRUEPRG)
        AC_DEFINE(HAVE_FREEBSD_OS)
else
        BA_CONDITIONAL(HAVE_FREEBSD_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xNetBSD
then
        BA_CONDITIONAL(HAVE_NETBSD_OS, $TRUEPRG)
        AC_DEFINE(HAVE_NETBSD_OS)
else
        BA_CONDITIONAL(HAVE_NETBSD_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xOpenBSD
then
        BA_CONDITIONAL(HAVE_OPENBSD_OS, $TRUEPRG)
        AC_DEFINE(HAVE_OPENBSD_OS)
else
        BA_CONDITIONAL(HAVE_OPENBSD_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xBSD/OS
then
        BA_CONDITIONAL(HAVE_BSDI_OS, $TRUEPRG)
        AC_DEFINE(HAVE_BSDI_OS)
else
        BA_CONDITIONAL(HAVE_BSDI_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xSGI
then
        BA_CONDITIONAL(HAVE_SGI_OS, $TRUEPRG)
        AC_DEFINE(HAVE_SGI_OS)
else
        BA_CONDITIONAL(HAVE_SGI_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xIRIX
then
        BA_CONDITIONAL(HAVE_IRIX_OS, $TRUEPRG)
        AC_DEFINE(HAVE_IRIX_OS)
else
        BA_CONDITIONAL(HAVE_IRIX_OS, $FALSEPRG)
fi

if test $HAVE_UNAME=yes -a x`uname -s` = xDarwin
then
    AM_CONDITIONAL(HAVE_DARWIN_OS, $TRUEPRG)
    AC_DEFINE(HAVE_DARWIN_OS)
else
    AM_CONDITIONAL(HAVE_DARWIN_OS, $FALSEPRG)
fi
])

AC_DEFUN(BA_CHECK_OPSYS_DISTNAME,
[AC_MSG_CHECKING(for Operating System Distribution)
if test "x$DISTNAME" != "x"
then
        echo "distname set to $DISTNAME"
elif test $HAVE_UNAME=yes -a x`uname -s` = xOSF1
then
        DISTNAME=alpha
elif test $HAVE_UNAME=yes -a x`uname -s` = xAIX
then
        DISTNAME=aix
elif test $HAVE_UNAME=yes -a x`uname -s` = xHP-UX
then
        DISTNAME=hpux
elif test $HAVE_UNAME=yes -a x`uname -s` = xSunOS
then
        DISTNAME=solaris
elif test $HAVE_UNAME=yes -a x`uname -s` = xFreeBSD
then
        DISTNAME=freebsd
elif test $HAVE_UNAME=yes -a x`uname -s` = xNetBSD
then
        DISTNAME=netbsd
elif test $HAVE_UNAME=yes -a x`uname -s` = xOpenBSD
then
        DISTNAME=openbsd
elif test $HAVE_UNAME=yes -a x`uname -s` = xIRIX
then
        DISTNAME=irix
elif test $HAVE_UNAME=yes -a x`uname -s` = xBSD/OS
then
        DISTNAME=bsdi
elif test -f /etc/SuSE-release
then
        DISTNAME=suse
elif test -d /etc/SuSEconfig
then
        DISTNAME=suse5
elif test -f /etc/mandrake-release
then
        DISTNAME=mandrake
elif test -f /etc/whitebox-release
then
       DISTNAME=redhat
elif test -f /etc/redhat-release
then
        DISTNAME=redhat
elif test -f /etc/gentoo-release
then
        DISTNAME=gentoo
elif test -f /etc/debian_version
then
        DISTNAME=debian
elif test -f /etc/slackware-version
then
        DISTNAME=slackware
elif test $HAVE_UNAME=yes -a x`uname -s` = xDarwin
then
    DISTNAME=darwin
elif test -f /etc/engarde-version
then
        DISTNAME=engarde
elif test "$CYGWIN" = yes
then
        DISTNAME=cygwin
        AC_DEFINE(HAVE_CYGWIN)
else
        DISTNAME=unknown
fi
AC_MSG_RESULT(done)
])

AC_DEFUN(BA_CHECK_MYSQL_DB,
[
db_found=no
AC_MSG_CHECKING(for MySQL support)
AC_ARG_WITH(mysql,
[
  --with-mysql[=DIR]      Include MySQL support.  DIR is the MySQL base
                          install directory, default is to search through
                          a number of common places for the MySQL files.],
[
  if test "$withval" != "no"; then
        if test "$withval" = "yes"; then
                if test -f /usr/local/mysql/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/local/mysql/include/mysql
                        MYSQL_LIBDIR=/usr/local/mysql/lib/mysql
                        MYSQL_BINDIR=/usr/local/mysql/bin
                elif test -f /usr/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/include/mysql
                        MYSQL_LIBDIR=/usr/lib/mysql
                        MYSQL_BINDIR=/usr/bin      
                elif test -f /usr/include/mysql.h; then
                        MYSQL_INCDIR=/usr/include
                        MYSQL_LIBDIR=/usr/lib
                        MYSQL_BINDIR=/usr/bin
                elif test -f /usr/local/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/local/include/mysql
                        MYSQL_LIBDIR=/usr/local/lib/mysql
                        MYSQL_BINDIR=/usr/local/bin
                elif test -f /usr/local/include/mysql.h; then
                        MYSQL_INCDIR=/usr/local/include
                        MYSQL_LIBDIR=/usr/local/lib
                        MYSQL_BINDIR=/usr/local/bin
                else
                   AC_MSG_RESULT(no)
                   AC_MSG_ERROR(Unable to find mysql.h in standard locations)
                fi
        else
                if test -f $withval/include/mysql/mysql.h; then
                        MYSQL_INCDIR=$withval/include/mysql
                        MYSQL_LIBDIR=$withval/lib/mysql
                        MYSQL_BINDIR=$withval/bin
                elif test -f $withval/include/mysql.h; then
                        MYSQL_INCDIR=$withval/include
                        MYSQL_LIBDIR=$withval/lib
                        MYSQL_BINDIR=$withval/bin
                else
                   AC_MSG_RESULT(no)
                   AC_MSG_ERROR(Invalid MySQL directory $withval - unable to find mysql.h under $withval)
                fi
        fi
    SQL_INCLUDE=-I$MYSQL_INCDIR
    if test -f $MYSQL_LIBDIR/libmysqlclient_r.a; then
       SQL_LFLAGS="-L$MYSQL_LIBDIR -lmysqlclient_r -lz"
       AC_DEFINE(HAVE_THREAD_SAFE_MYSQL)
    else
       SQL_LFLAGS="-L$MYSQL_LIBDIR -lmysqlclient -lz"
    fi
    SQL_BINDIR=$MYSQL_BINDIR

    AC_DEFINE(HAVE_MYSQL)
    AC_MSG_RESULT(yes)
    db_found=yes
    support_mysql=yes
    db_name=MySQL
    DB_NAME=mysql

  else
        AC_MSG_RESULT(no)
  fi
]
)

AC_ARG_WITH(embedded-mysql,
[
  --with-embedded-mysql[=DIR] Include MySQL support.  DIR is the MySQL base
                          install directory, default is to search through
                          a number of common places for the MySQL files.],
[
  if test "$withval" != "no"; then
        if test "$withval" = "yes"; then
                if test -f /usr/local/mysql/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/local/mysql/include/mysql
                        MYSQL_LIBDIR=/usr/local/mysql/lib/mysql
                        MYSQL_BINDIR=/usr/local/mysql/bin
                elif test -f /usr/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/include/mysql
                        MYSQL_LIBDIR=/usr/lib/mysql
                        MYSQL_BINDIR=/usr/bin      
                elif test -f /usr/include/mysql.h; then
                        MYSQL_INCDIR=/usr/include
                        MYSQL_LIBDIR=/usr/lib
                        MYSQL_BINDIR=/usr/bin
                elif test -f /usr/local/include/mysql/mysql.h; then
                        MYSQL_INCDIR=/usr/local/include/mysql
                        MYSQL_LIBDIR=/usr/local/lib/mysql
                        MYSQL_BINDIR=/usr/local/bin
                elif test -f /usr/local/include/mysql.h; then
                        MYSQL_INCDIR=/usr/local/include
                        MYSQL_LIBDIR=/usr/local/lib
                        MYSQL_BINDIR=/usr/local/bin
                else
                   AC_MSG_RESULT(no)
                   AC_MSG_ERROR(Unable to find mysql.h in standard locations)
                fi
        else
                if test -f $withval/include/mysql/mysql.h; then
                        MYSQL_INCDIR=$withval/include/mysql
                        MYSQL_LIBDIR=$withval/lib/mysql
                        MYSQL_BINDIR=$withval/bin
                elif test -f $withval/include/mysql.h; then
                        MYSQL_INCDIR=$withval/include
                        MYSQL_LIBDIR=$withval/lib
                        MYSQL_BINDIR=$withval/bin
                else
                   AC_MSG_RESULT(no)
                   AC_MSG_ERROR(Invalid MySQL directory $withval - unable to find mysql.h under $withval)
                fi
        fi
    SQL_INCLUDE=-I$MYSQL_INCDIR
    SQL_LFLAGS="-L$MYSQL_LIBDIR -lmysqld -lz -lm -lcrypt"
    SQL_BINDIR=$MYSQL_BINDIR

    AC_DEFINE(HAVE_MYSQL)
    AC_DEFINE(HAVE_EMBEDDED_MYSQL)
    AC_MSG_RESULT(yes)
    db_found=yes
    support_mysql=yes
    db_name=MySQL
    DB_NAME=mysql

  else
        AC_MSG_RESULT(no)
  fi
]
)


AC_SUBST(SQL_LFLAGS)
AC_SUBST(SQL_INCLUDE)
AC_SUBST(SQL_BINDIR)
  
])


AC_DEFUN(BA_CHECK_SQLITE_DB,
[
db_found=no
AC_MSG_CHECKING(for SQLite support)
AC_ARG_WITH(sqlite,
[
  --with-sqlite[=DIR]     Include SQLite support.  DIR is the SQLite base
                          install directory, default is to search through
                          a number of common places for the SQLite files.],
[
  if test "$withval" != "no"; then
     if test "$withval" = "yes"; then
        if test -f /usr/local/include/sqlite.h; then
           SQLITE_INCDIR=/usr/local/include
           SQLITE_LIBDIR=/usr/local/lib
           SQLITE_BINDIR=/usr/local/bin
        elif test -f /usr/include/sqlite.h; then
           SQLITE_INCDIR=/usr/include
           SQLITE_LIBDIR=/usr/lib
           SQLITE_BINDIR=/usr/bin      
        elif test -f $prefix/include/sqlite.h; then
           SQLITE_INCDIR=$prefix/include
           SQLITE_LIBDIR=$prefix/lib
           SQLITE_BINDIR=$prefix/bin      
        else
           AC_MSG_RESULT(no)
           AC_MSG_ERROR(Unable to find sqlite.h in standard locations)
        fi
     else
        if test -f $withval/sqlite.h; then
           SQLITE_INCDIR=$withval
           SQLITE_LIBDIR=$withval
           SQLITE_BINDIR=$withval
        elif test -f $withval/include/sqlite.h; then
           SQLITE_INCDIR=$withval/include
           SQLITE_LIBDIR=$withval/lib
           SQLITE_BINDIR=$withval/bin
        else
           AC_MSG_RESULT(no)
           AC_MSG_ERROR(Invalid SQLite directory $withval - unable to find sqlite.h under $withval)
        fi
     fi
     SQL_INCLUDE=-I$SQLITE_INCDIR
     SQL_LFLAGS="-L$SQLITE_LIBDIR -lsqlite"
     SQL_BINDIR=$SQLITE_BINDIR

     AC_DEFINE(HAVE_SQLITE)
     AC_MSG_RESULT(yes)
     db_found=yes
     support_sqlite=yes
     db_name=SQLite
     DB_NAME=sqlite

  else
     AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(SQL_LFLAGS)
AC_SUBST(SQL_INCLUDE)
AC_SUBST(SQL_BINDIR)
  
])


AC_DEFUN(BA_CHECK_POSTGRESQL_DB,
[
db_found=no
AC_MSG_CHECKING(for PostgreSQL support)
AC_ARG_WITH(postgresql,
[  --with-postgresql[=DIR]      Include PostgreSQL support.  DIR is the PostgreSQL
                          base install directory, defaults to /usr/local/pgsql],
[
  if test "$withval" != "no"; then
      if test "$db_found" = "yes"; then
          AC_MSG_RESULT(error)
          AC_MSG_ERROR("You can configure for only one database.");
      fi
      if test "$withval" = "yes"; then
          if test -f /usr/local/include/libpq-fe.h; then
              POSTGRESQL_INCDIR=/usr/local/include
              POSTGRESQL_LIBDIR=/usr/local/lib
              POSTGRESQL_BINDIR=/usr/local/bin
          elif test -f /usr/include/libpq-fe.h; then
              POSTGRESQL_INCDIR=/usr/include
              POSTGRESQL_LIBDIR=/usr/lib
              POSTGRESQL_BINDIR=/usr/bin
          elif test -f /usr/include/pgsql/libpq-fe.h; then
              POSTGRESQL_INCDIR=/usr/include/pgsql
              POSTGRESQL_LIBDIR=/usr/lib/pgsql
              POSTGRESQL_BINDIR=/usr/bin
          elif test -f /usr/include/postgresql/libpq-fe.h; then
              POSTGRESQL_INCDIR=/usr/include/postgresql
              POSTGRESQL_LIBDIR=/usr/lib/postgresql
              POSTGRESQL_BINDIR=/usr/bin
          else
              AC_MSG_RESULT(no)
              AC_MSG_ERROR(Unable to find libpq-fe.h in standard locations)
          fi
      elif test -f $withval/include/libpq-fe.h; then
          POSTGRESQL_INCDIR=$withval/include
          POSTGRESQL_LIBDIR=$withval/lib
          POSTGRESQL_BINDIR=$withval/bin
      elif test -f $withval/include/postgresql/libpq-fe.h; then
          POSTGRESQL_INCDIR=$withval/include/postgresql
          POSTGRESQL_LIBDIR=$withval/lib
          POSTGRESQL_BINDIR=$withval/bin
      else
          AC_MSG_RESULT(no)
          AC_MSG_ERROR(Invalid PostgreSQL directory $withval - unable to find libpq-fe.h under $withval)
      fi
      POSTGRESQL_LFLAGS="-L$POSTGRESQL_LIBDIR -lpq"
      AC_CHECK_FUNC(crypt, , AC_CHECK_LIB(crypt, crypt, [POSTGRESQL_LFLAGS="-lcrypt $POSTGRESQL_LFLAGS"]))
      SQL_INCLUDE=-I$POSTGRESQL_INCDIR
      SQL_LFLAGS=$POSTGRESQL_LFLAGS
      SQL_BINDIR=$POSTGRESQL_BINDIR
      AC_DEFINE(HAVE_POSTGRESQL)
      AC_MSG_RESULT(yes)
      db_found=yes
      support_postgresql=yes
      db_name=PostgreSQL
      DB_NAME=postgresql
  else
      AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(SQL_LFLAGS)
AC_SUBST(SQL_INCLUDE)
AC_SUBST(SQL_BINDIR)

])



AC_DEFUN(BA_CHECK_SQL_DB, 
[AC_MSG_CHECKING(Checking for various databases)
dnl# --------------------------------------------------------------------------
dnl# CHECKING FOR VARIOUS DATABASES (thanks to UdmSearch team)
dnl# --------------------------------------------------------------------------
dnl Check for some DBMS backend
dnl NOTE: we can use only one backend at a time
db_found=no
db_name=none

if test x$support_mysql = xyes; then
   cats=cats
fi

AC_MSG_CHECKING(for Berkeley DB support)
AC_ARG_WITH(berkeleydb,
[
  --with-berkeleydb[=DIR] Include Berkeley DB support.  DIR is the Berkeley DB base
                          install directory, default is to search through
                          a number of common places for the DB files.],
[
  if test "$withval" != "no"; then
        if test "$withval" = "yes"; then
                if test -f /usr/include/db.h; then
                        BERKELEYDB_INCDIR=/usr/include
                        BERKELEYDB_LIBDIR=/usr/lib
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid Berkeley DB directory - unable to find db.h)
                fi
        else
                if test -f $withval/include/db.h; then
                        BERKELEYDB_INCDIR=$withval/include
                        BERKELEYDBL_LIBDIR=$withval/lib
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid Berkeley DB directory - unable to find db.h under $withval)
                fi
        fi
    SQL_INCLUDE=-I$BERKELEYDB_INCDIR
    SQL_LFLAGS="-L$BERKELEYDB_LIBDIR -ldb"

    AC_DEFINE(HAVE_BERKELEY_DB)
    AC_MSG_RESULT(yes)
    have_db=yes
    support_mysql=yes
    db_name=BerkelyDB

  else
        AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(SQL_LFLAGS)
AC_SUBST(SQL_INCLUDE)

if test x$support_berkleydb = xyes; then
   cats=cats
fi




AC_MSG_CHECKING(for mSQL support)
AC_ARG_WITH(msql,
[  --with-msql[=DIR]       Include mSQL support.  DIR is the mSQL base
                          install directory, defaults to /usr/local/Hughes.],
[
  if test "$withval" != "no"; then
    if test "$have_db" = "yes"; then
        AC_MSG_RESULT(error)
        AC_MSG_ERROR("You can configure for only one database.");
    fi

    if test "$withval" = "yes"; then
        MSQL_INCDIR=/usr/local/Hughes/include
        MSQL_LIBDIR=/usr/local/Hughes/lib
    else
        MSQL_INCDIR=$withval/include
        MSQL_LIBDIR=$withval/lib
    fi
    MSQL_INCLUDE=-I$MSQL_INCDIR
    MSQL_LFLAGS="-L$MSQL_LIBDIR -lmsql"

    AC_DEFINE(HAVE_MSQL)
    AC_MSG_RESULT(yes)
    have_db=yes
  else
        AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(MSQL_LFLAGS)
AC_SUBST(MSQL_INCLUDE)


AC_MSG_CHECKING(for iODBC support)
AC_ARG_WITH(iodbc,
[  --with-iodbc[=DIR]      Include iODBC support.  DIR is the iODBC base
                          install directory, defaults to /usr/local.],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi
        
        if test "$withval" = "yes"; then
                withval=/usr/local
        fi

        if test "$withval" != "no"; then
                if test -f $withval/include/isql.h; then
                        IODBC_INCDIR=$withval/include
                        IODBC_LIBDIR=$withval/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid iODBC directory - unable to find isql.h)
                fi
                IODBC_LFLAGS="-L$IODBC_LIBDIR -liodbc"
                IODBC_INCLUDE=-I$IODBC_INCDIR
                AC_DEFINE(HAVE_IODBC)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(IODBC_LFLAGS)
AC_SUBST(IODBC_INCLUDE)


AC_MSG_CHECKING(for unixODBC support)
AC_ARG_WITH(unixODBC,
[  --with-unixODBC[=DIR]   Include unixODBC support.  DIR is the unixODBC base
                          install directory, defaults to /usr/local.],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi
        
        if test "$withval" = "yes"; then
                withval=/usr/local
        fi

        if test "$withval" != "no"; then
                if test -f $withval/include/sql.h; then
                        UNIXODBC_INCDIR=$withval/include
                        UNIXODBC_LIBDIR=$withval/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid unixODBC directory - unable to find sql.h)
                fi
                UNIXODBC_LFLAGS="-L$UNIXODBC_LIBDIR -lodbc"
                UNIXODBC_INCLUDE=-I$UNIXODBC_INCDIR
                AC_DEFINE(HAVE_UNIXODBC)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(UNIXODBC_LFLAGS)
AC_SUBST(UNIXODBC_INCLUDE)


AC_MSG_CHECKING(for Solid support)
AC_ARG_WITH(solid,
[  --with-solid[=DIR]      Include Solid support.  DIR is the Solid base
                          install directory, defaults to /usr/local.],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi

        if test "$withval" = "yes"; then
                withval=/usr/local
        fi

        if test "$withval" != "no"; then
                if test -f $withval/include/cli0cli.h; then
                        SOLID_INCDIR=$withval/include
                        SOLID_LIBDIR=$withval/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid Solid directory - unable to find cli0cli.h)
                fi
                SOLID_LFLAGS="-L$SOLID_LIBDIR -lsolcli"
                SOLID_INCLUDE="-I$SOLID_INCDIR"
                AC_DEFINE(HAVE_SOLID)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(SOLID_LFLAGS)
AC_SUBST(SOLID_INCLUDE)

AC_MSG_CHECKING(for OpenLink ODBC support)
AC_ARG_WITH(openlink,
[  --with-openlink[=DIR]   Include OpenLink ODBC support. 
                          DIR is the base OpenLink ODBC install directory],
[
  if test "$withval" != "no"; then
        if test "$withval" = "yes"; then

                if test "$have_db" = "yes"; then
                        AC_MSG_RESULT(error)
                        AC_MSG_ERROR("You can configure for only one database.");
                fi

                if test -f /usr/local/virtuoso-ent/odbcsdk/include/isql.h; then
                        VIRT_INCDIR=/usr/local/virtuoso-ent/odbcsdk/include/
                        VIRT_LIBDIR=/usr/local/virtuoso-ent/odbcsdk/lib/
                elif test -f /usr/local/virtuoso-lite/odbcsdk/include/isql.h; then
                        VIRT_INCDIR=/usr/local/virtuoso-lite/odbcsdk/include/
                        VIRT_LIBDIR=/usr/local/virtuoso-lite/odbcsdk/lib/
                elif test -f /usr/local/virtuoso/odbcsdk/include/isql.h; then
                        VIRT_INCDIR=/usr/local/virtuoso/odbcsdk/include/
                        VIRT_LIBDIR=/usr/local/virtuoso/odbcsdk/lib/
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid OpenLink ODBC directory - unable to find isql.h)
                fi
        else
                if test -f $withval/odbcsdk/include/isql.h; then
                        VIRT_INCDIR=$withval/odbcsdk/include/
                        VIRT_LIBDIR=$withval/odbcsdk/lib/
                elif test -f $withval/include/isql.h; then
                        VIRT_INCDIR=$withval/include/
                        VIRT_LIBDIR=$withval/lib/
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid OpenLink ODBC directory - unable to find isql.h under $withval)
                fi
        fi
    VIRT_INCLUDE=-I$VIRT_INCDIR
    VIRT_LFLAGS="-L$VIRT_LIBDIR -liodbc"

    AC_DEFINE(HAVE_VIRT)
    AC_MSG_RESULT(yes)
    have_db=yes

  else
        AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(VIRT_LFLAGS)
AC_SUBST(VIRT_INCLUDE)


AC_MSG_CHECKING(for EasySoft ODBC support)
AC_ARG_WITH(easysoft,
[  --with-easysoft[=DIR]   Include EasySoft ODBC support. 
                          DIR is the base EasySoft ODBC install directory],
[
  if test "$withval" != "no"; then
        if test "$withval" = "yes"; then

                if test "$have_db" = "yes"; then
                        AC_MSG_RESULT(error)
                        AC_MSG_ERROR("You can configure for only one database.");
                fi

                if test -f /usr/local/easysoft/oob/client/include/sql.h; then
                        EASYSOFT_INCDIR=/usr/local/easysoft/oob/client/include/
                        EASYSOFT_LFLAGS="-L/usr/local/easysoft/oob/client/lib/ -L/usr/local/easysoft/lib"
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid EasySoft ODBC directory - unable to find sql.h)
                fi
        else
                if test -f $withval/easysoft/oob/client/include/sql.h; then
                        EASYSOFT_INCDIR=$withval/easysoft/oob/client/include/
                        EASYSOFT_LFLAGS="-L$withval/easysoft/oob/client/lib/ -L$withval/easysoft/lib"
                else
                AC_MSG_RESULT(no)
                AC_MSG_ERROR(Invalid EasySoft ODBC directory - unable to find sql.h under $withval)
                fi
        fi
    EASYSOFT_INCLUDE=-I$EASYSOFT_INCDIR
    EASYSOFT_LFLAGS="$EASYSOFT_LFLAGS -lesoobclient -lesrpc -lsupport -lextra"

    AC_DEFINE(HAVE_EASYSOFT)
    AC_MSG_RESULT(yes)
    have_db=yes

  else
        AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
AC_SUBST(EASYSOFT_LFLAGS)
AC_SUBST(EASYSOFT_INCLUDE)



AC_MSG_CHECKING(for InterBase support)
AC_ARG_WITH(ibase,
[  --with-ibase[=DIR]      Include InterBase support.  DIR is the InterBase
                          install directory, defaults to /usr/interbase.],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi
        
        if test "$withval" = "yes"; then
                withval=/usr/interbase
        fi

        if test "$withval" != "no"; then
                if test -f $withval/include/ibase.h; then
                        IBASE_INCDIR=$withval/include
                        IBASE_LIBDIR=$withval/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid InterBase directory - unable to find ibase.h)
                fi
                IBASE_LFLAGS="-L$IBASE_LIBDIR -lgds"
                IBASE_INCLUDE=-I$IBASE_INCDIR
                AC_DEFINE(HAVE_IBASE)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(IBASE_LFLAGS)
AC_SUBST(IBASE_INCLUDE)

AC_MSG_CHECKING(for Oracle8 support)
AC_ARG_WITH(oracle8,
[  --with-oracle8[=DIR]    Include Oracle8 support.  DIR is the Oracle
                          home directory, defaults to $ORACLE_HOME or
                          /oracle8/app/oracle/product/8.0.5.],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi

        if test "$withval" = "yes"; then
                withval="$ORACLE_HOME"
                if test "$withval" = ""; then
                        withval=/oracle8/app/oracle/product/8.0.5
                fi
        fi

        if test "$withval" != "no"; then
                if test -f $withval/rdbms/demo/oci.h; then
                        ORACLE8_INCDIR1=$withval/rdbms/demo/
                        ORACLE8_INCDIR2=$withval/rdbms/public/:
                        ORACLE8_INCDIR3=$withval/network/public/
                        ORACLE8_INCDIR4=$withval/plsql/public/
                        ORACLE8_LIBDIR1=$withval/lib
                        ORACLE8_LIBDIR2=$withval/rdbms/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid ORACLE directory - unable to find oci.h)
                fi
                ORACLE8_LFLAGS="-L$ORACLE8_LIBDIR1 -L$ORACLE8_LIBDIR2 $withval/lib/libclntsh.so -lmm -lepc -lclient -lvsn -lcommon -lgeneric -lcore4 -lnlsrtl3 -lnsl -lm -ldl -lnetv2 -lnttcp -lnetwork -lncr -lsql"
                ORACLE8_INCLUDE="-I$ORACLE8_INCDIR1 -I$ORACLE8_INCDIR2 -I$ORACLE8_INCDIR3 -I$ORACLE8_INCDIR4"
                AC_DEFINE(HAVE_ORACLE8)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(ORACLE8_LFLAGS)
AC_SUBST(ORACLE8_INCLUDE)


AC_MSG_CHECKING(for Oracle7 support)
AC_ARG_WITH(oracle7,
[  --with-oracle7[=DIR]    Include Oracle 7.3 support.  DIR is the Oracle
                          home directory, defaults to 
                          ORACLE_HOME [$ORACLE_HOME]],
[
        if test "$withval" != "no"; then
            if test "$have_db" = "yes"; then
                AC_MSG_RESULT(error)
                AC_MSG_ERROR("You can configure for only one database.");
            fi
        fi

        if test "$withval" = "yes"; then
                withval="$ORACLE_HOME"
        fi

        if test "$withval" != "no"; then
                if test -f $withval/rdbms/demo/ocidfn.h; then
                        ORACLE7_INCDIR=$withval/rdbms/demo/
                        ORACLE7_LIBDIR1=$withval/lib
                        ORACLE7_LIBDIR2=$withval/rdbms/lib
                else
                        AC_MSG_RESULT(no)
                        AC_MSG_ERROR(Invalid ORACLE directory - unable to find ocidfn.h)
                fi

        ORACLEINST_TOP=$withval
        if test -f "$ORACLEINST_TOP/rdbms/lib/sysliblist"
        then
          ORA_SYSLIB="`cat $ORACLEINST_TOP/rdbms/lib/sysliblist`"
        elif test -f "$ORACLEINST_TOP/lib/sysliblist"
            then
          ORA_SYSLIB="`cat $ORACLEINST_TOP/lib/sysliblist`"
        else
          ORA_SYSLIB="-lm"
        fi
          
                ORACLE7_LFLAGS="-L$ORACLE7_LIBDIR1 -L$ORACLE7_LIBDIR2 \
            -lclient -lsqlnet -lncr -lsqlnet -lclient -lcommon \
            -lgeneric -lsqlnet -lncr -lsqlnet -lclient -lcommon -lgeneric \
            -lepc -lnlsrtl3 -lc3v6 -lcore3 -lnlsrtl3 -lcore3 -lnlsrtl3 \
            $ORA_SYSLIB -lcore3 $ORA_SYSLIB"
                ORACLE7_INCLUDE="-I$ORACLE7_INCDIR "
                AC_DEFINE(HAVE_ORACLE7)
                AC_MSG_RESULT(yes)
                have_db=yes
        fi
],[
        AC_MSG_RESULT(no)
])
AC_SUBST(ORACLE7_LFLAGS)
AC_SUBST(ORACLE7_INCLUDE)
])
  

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])



dnl
dnl ========================================================================
dnl
dnl      Old Gnome 1.4 detection code -- deprecated, but still used
dnl
dnl ========================================================================

dnl AM_ACLOCAL_INCLUDE(macrodir)
AC_DEFUN([AM_ACLOCAL_INCLUDE],
[
        AM_CONDITIONAL(INSIDE_GNOME_COMMON, false)

        test -n "$ACLOCAL_FLAGS" && ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

        for k in $1 ; do ACLOCAL="$ACLOCAL -I $k" ; done
])

dnl
dnl GNOME_INIT_HOOK (script-if-gnome-enabled, [failflag], [additional-inits])
dnl
dnl if failflag is "fail" then GNOME_INIT_HOOK will abort if gnomeConf.sh
dnl is not found. 
dnl

AC_DEFUN([GNOME_INIT_HOOK],[
        AC_SUBST(GNOME_LIBS)
        AC_SUBST(GNOMEUI_LIBS)
        AC_SUBST(GNOMEGNORBA_LIBS)
        AC_SUBST(GTKXMHTML_LIBS)
        AC_SUBST(ZVT_LIBS)
        AC_SUBST(GNOME_LIBDIR)
        AC_SUBST(GNOME_INCLUDEDIR)

        AC_ARG_WITH(gnome-includes,
        [  --with-gnome-includes   Specify location of GNOME headers],[
        CFLAGS="$CFLAGS -I$withval"
        ])
        
        AC_ARG_WITH(gnome-libs,
        [  --with-gnome-libs       Specify location of GNOME libs],[
        LDFLAGS="$LDFLAGS -L$withval"
        gnome_prefix=$withval
        ])

        AC_ARG_WITH(gnome,
        [  --with-gnome            Specify prefix for GNOME files],
                if test x$withval = xyes; then
                        want_gnome=yes
                        dnl Note that an empty true branch is not
                        dnl valid sh syntax.
                        ifelse([$1], [], :, [$1])
                else
                        if test "x$withval" = xno; then
                                want_gnome=no
                        else
                                want_gnome=yes
                                LDFLAGS="$LDFLAGS -L$withval/lib"
                                CFLAGS="$CFLAGS -I$withval/include"
                                gnome_prefix=$withval/lib
                        fi
                fi,
                want_gnome=yes)

        if test "x$want_gnome" = xyes; then

            AC_PATH_PROG(GNOME_CONFIG,gnome-config,no)
            if test "$GNOME_CONFIG" = "no"; then
              no_gnome_config="yes"
            else
              AC_MSG_CHECKING(if $GNOME_CONFIG works)
              if $GNOME_CONFIG --libs-only-l gnome >/dev/null 2>&1; then
                AC_MSG_RESULT(yes)
                GNOME_GNORBA_HOOK([],$2)
                GNOME_LIBS="`$GNOME_CONFIG --libs-only-l gnome`"
                GNOMEUI_LIBS="`$GNOME_CONFIG --libs-only-l gnomeui`"
                GNOMEGNORBA_LIBS="`$GNOME_CONFIG --libs-only-l gnorba gnomeui`"
                GTKXMHTML_LIBS="`$GNOME_CONFIG --libs-only-l gtkxmhtml`"
                ZVT_LIBS="`$GNOME_CONFIG --libs-only-l zvt`"
                GNOME_LIBDIR="`$GNOME_CONFIG --libs-only-L gnorba gnomeui`"
                GNOME_INCLUDEDIR="`$GNOME_CONFIG --cflags gnorba gnomeui`"
                $1
              else
                AC_MSG_RESULT(no)
                no_gnome_config="yes"
              fi
            fi

            if test x$exec_prefix = xNONE; then
                if test x$prefix = xNONE; then
                    gnome_prefix=$ac_default_prefix/lib
                else
                    gnome_prefix=$prefix/lib
                fi
            else
                gnome_prefix=`eval echo \`echo $libdir\``
            fi
        
            if test "$no_gnome_config" = "yes"; then
              AC_MSG_CHECKING(for gnomeConf.sh file in $gnome_prefix)
              if test -f $gnome_prefix/gnomeConf.sh; then
                AC_MSG_RESULT(found)
                echo "loading gnome configuration from" \
                     "$gnome_prefix/gnomeConf.sh"
                . $gnome_prefix/gnomeConf.sh
                $1
              else
                AC_MSG_RESULT(not found)
                if test x$2 = xfail; then
                  AC_MSG_ERROR(Could not find the gnomeConf.sh file that is generated by gnome-libs install)
                fi
              fi
            fi
        fi

        if test -n "$3"; then
          n="$3"
          for i in $n; do
            AC_MSG_CHECKING(extra library \"$i\")
            case $i in 
              applets)
                AC_SUBST(GNOME_APPLETS_LIBS)
                GNOME_APPLETS_LIBS=`$GNOME_CONFIG --libs-only-l applets`
                AC_MSG_RESULT($GNOME_APPLETS_LIBS);;
              docklets)
                AC_SUBST(GNOME_DOCKLETS_LIBS)
                GNOME_DOCKLETS_LIBS=`$GNOME_CONFIG --libs-only-l docklets`
                AC_MSG_RESULT($GNOME_DOCKLETS_LIBS);;
              capplet)
                AC_SUBST(GNOME_CAPPLET_LIBS)
                GNOME_CAPPLET_LIBS=`$GNOME_CONFIG --libs-only-l capplet`
                AC_MSG_RESULT($GNOME_CAPPLET_LIBS);;
              *)
                AC_MSG_RESULT(unknown library)
            esac
            EXTRA_INCLUDEDIR=`$GNOME_CONFIG --cflags $i`
            GNOME_INCLUDEDIR="$GNOME_INCLUDEDIR $EXTRA_INCLUDEDIR"
          done
        fi
])

dnl
dnl GNOME_INIT ([additional-inits])
dnl

AC_DEFUN([GNOME_INIT],[
        GNOME_INIT_HOOK([],fail,$1)
])

dnl GNOME_X_CHECKS
dnl
dnl Basic X11 related checks for X11.  At the end, the following will be
dnl defined/changed:
dnl   GTK_{CFLAGS,LIBS}      From AM_PATH_GTK
dnl   CPPFLAGS               Will include $X_CFLAGS
dnl   GNOME_HAVE_SM          `true' or `false' depending on whether session
dnl                          management is available.  It is available if
dnl                          both -lSM and X11/SM/SMlib.h exist.  (Some
dnl                          Solaris boxes have the library but not the header)
dnl   XPM_LIBS               -lXpm if Xpm library is present, otherwise ""
dnl
dnl The following configure cache variables are defined (but not used):
dnl   gnome_cv_passdown_{x_libs,X_LIBS,X_CFLAGS}
dnl
AC_DEFUN([GNOME_X_CHECKS],
[
        AM_PATH_GTK(1.2.0,,AC_MSG_ERROR(GTK not installed, or gtk-config not in path))
        dnl Hope that GTK_CFLAGS have only -I and -D.  Otherwise, we could
        dnl   test -z "$x_includes" || CPPFLAGS="$CPPFLAGS -I$x_includes"
        dnl
        dnl Use CPPFLAGS instead of CFLAGS because AC_CHECK_HEADERS uses
        dnl CPPFLAGS, not CFLAGS
        CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"

        saved_ldflags="$LDFLAGS"
        LDFLAGS="$LDFLAGS $GTK_LIBS"

        gnome_cv_passdown_x_libs="$GTK_LIBS"
        gnome_cv_passdown_X_LIBS="$GTK_LIBS"
        gnome_cv_passdown_X_CFLAGS="$GTK_CFLAGS"
        gnome_cv_passdown_GTK_LIBS="$GTK_LIBS"

        LDFLAGS="$saved_ldflags $GTK_LIBS"

dnl We are requiring GTK >= 1.1.1, which means this will be fine anyhow.
        USE_DEVGTK=true

dnl     AC_MSG_CHECKING([whether to use features from (unstable) GTK+ 1.1.x])
dnl     AC_EGREP_CPP(answer_affirmatively,
dnl     [#include <gtk/gtkfeatures.h>
dnl     #ifdef GTK_HAVE_FEATURES_1_1_0
dnl        answer_affirmatively
dnl     #endif
dnl     ], dev_gtk=yes, dev_gtk=no)
dnl     if test "$dev_gtk" = "yes"; then
dnl        USE_DEVGTK=true
dnl     fi
dnl     AC_MSG_RESULT("$dev_gtk")

        GNOME_HAVE_SM=true
        case "$GTK_LIBS" in
         *-lSM*)
            dnl Already found it.
            ;;
         *)
            dnl Assume that if we have -lSM then we also have -lICE.
            AC_CHECK_LIB(SM, SmcSaveYourselfDone,
                [GTK_LIBS="-lSM -lICE $GTK_LIBS"],GNOME_HAVE_SM=false,
                $x_libs -lICE)
            ;;
        esac

        if test "$GNOME_HAVE_SM" = true; then
           AC_CHECK_HEADERS(X11/SM/SMlib.h,,GNOME_HAVE_SM=false)
        fi

        if test "$GNOME_HAVE_SM" = true; then
           AC_DEFINE(HAVE_LIBSM)
        fi

        XPM_LIBS=""
        AC_CHECK_LIB(Xpm, XpmFreeXpmImage, [XPM_LIBS="-lXpm"], , $x_libs)
        AC_SUBST(XPM_LIBS)

        AC_REQUIRE([GNOME_PTHREAD_CHECK])
        LDFLAGS="$saved_ldflags"

        AC_PROVIDE([GNOME_X_CHECKS])
])

AC_DEFUN([GNOME_PTHREAD_CHECK],[
        PTHREAD_LIB=""
        AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIB="-lpthread",
                [AC_CHECK_LIB(pthreads, pthread_create, PTHREAD_LIB="-lpthreads",
                    [AC_CHECK_LIB(c_r, pthread_create, PTHREAD_LIB="-lc_r",
                        [AC_CHECK_LIB(pthread, __pthread_attr_init_system, PTHREAD_LIB="-lpthread",
                                [AC_CHECK_FUNC(pthread_create)]
                        )]
                    )]
                )]
        )
        AC_SUBST(PTHREAD_LIB)
        AC_PROVIDE([GNOME_PTHREAD_CHECK])
])

dnl
dnl GNOME_GNORBA_HOOK (script-if-gnorba-found, failflag)
dnl
dnl if failflag is "failure" it aborts if gnorba is not found.
dnl

AC_DEFUN([GNOME_GNORBA_HOOK],[
        GNOME_ORBIT_HOOK([],$2)
        AC_CACHE_CHECK([for gnorba libraries],gnome_cv_gnorba_found,[
                gnome_cv_gnorba_found=no
                if test x$gnome_cv_orbit_found = xyes; then
                        GNORBA_CFLAGS="`gnome-config --cflags gnorba gnomeui`"
                        GNORBA_LIBS="`gnome-config --libs gnorba gnomeui`"
                        if test -n "$GNORBA_LIBS"; then
                                gnome_cv_gnorba_found=yes
                        fi
                fi
        ])
        AM_CONDITIONAL(HAVE_GNORBA, test x$gnome_cv_gnorba_found = xyes)
        if test x$gnome_cv_orbit_found = xyes; then
                $1
                GNORBA_CFLAGS="`gnome-config --cflags gnorba gnomeui`"
                GNORBA_LIBS="`gnome-config --libs gnorba gnomeui`"
                AC_SUBST(GNORBA_CFLAGS)
                AC_SUBST(GNORBA_LIBS)
        else
                if test x$2 = xfailure; then
                        AC_MSG_ERROR(gnorba library not installed or installation problem)
                fi
        fi
])

AC_DEFUN([GNOME_GNORBA_CHECK], [
        GNOME_GNORBA_HOOK([],failure)
])

dnl
dnl GNOME_ORBIT_HOOK (script-if-orbit-found, failflag)
dnl
dnl if failflag is "failure" it aborts if orbit is not found.
dnl

AC_DEFUN([GNOME_ORBIT_HOOK],[
        AC_PATH_PROG(ORBIT_CONFIG,orbit-config,no)
        AC_PATH_PROG(ORBIT_IDL,orbit-idl,no)
        AC_CACHE_CHECK([for working ORBit environment],gnome_cv_orbit_found,[
                if test x$ORBIT_CONFIG = xno -o x$ORBIT_IDL = xno; then
                        gnome_cv_orbit_found=no
                else
                        gnome_cv_orbit_found=yes
                fi
        ])
        AM_CONDITIONAL(HAVE_ORBIT, test x$gnome_cv_orbit_found = xyes)
        if test x$gnome_cv_orbit_found = xyes; then
                $1
                ORBIT_CFLAGS=`orbit-config --cflags client server`
                ORBIT_LIBS=`orbit-config --use-service=name --libs client server`
                AC_SUBST(ORBIT_CFLAGS)
                AC_SUBST(ORBIT_LIBS)
        else
                if test x$2 = xfailure; then
                        AC_MSG_ERROR(ORBit not installed or installation problem)
                fi
        fi
])

AC_DEFUN([GNOME_ORBIT_CHECK], [
        GNOME_ORBIT_HOOK([],failure)
])
dnl
dnl =========================================================================
dnl


dnl
dnl =========================================================================
dnl
dnl   New package checking stuff
dnl
dnl =========================================================================  
dnl
dnl PKG_CHECK_MODULES(GSTUFF, gtk+-2.0 >= 1.3 glib = 1.3.4, action-if, action-not)
dnl defines GSTUFF_LIBS, GSTUFF_CFLAGS, see pkg-config man page
dnl also defines GSTUFF_PKG_ERRORS on error
AC_DEFUN(PKG_CHECK_MODULES, [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  if test "$PKG_CONFIG" = "no" ; then
     echo "*** The pkg-config script could not be found. Make sure it is"
     echo "*** in your path, or set the PKG_CONFIG environment variable"
     echo "*** to the full path to pkg-config."
     echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get pkg-config."
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for $2)

        if $PKG_CONFIG --exists "$2" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "$2"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "$2"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            ## If we have a custom action on failure, don't print errors, but 
            ## do set a variable so people can do so.
            $1_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$2"`
            ifelse([$4], ,echo $$1_PKG_ERRORS,)
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version $PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig"
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     ifelse([$4], , AC_MSG_ERROR([Library requirements ($2) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.]), [$4])
  fi
])
