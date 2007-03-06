# 
# bootstrap/linux Makefile.in
#


# autoconf/Make.common.in	 -*- Makefile -*-
# release date (man), LSM date, version number/name, current maintainer
DATE="22 April 2005"
LSMDATE=22Apr05
VERSION=1.36.3
VERNAME=bacula-$(VERSION)#
MAINT=Kern Sibbald#
MAINTEMAIL=<kern@sibbald.com>#
WEBMAINT=#
WEBMAINTEMAIL=#
WEBPAGE=#
FTPSITENAME=#
FTPSITEDIR=#
#-------------------------------------------------------------------------

SHELL = /bin/sh

# Installation target directories & other installation stuff
prefix = /usr
exec_prefix = /usr
binprefix =
manprefix =
sbindir = /usr/sbin
sysconfdir = /etc/bacula
scriptdir = /etc/bacula/scripts
mandir = ${prefix}/share/man/man1
manext = 1

# Tools & program stuff
CC = i486-linux-gnu-gcc
CPP = i486-linux-gnu-gcc -E
CXX = i486-linux-gnu-g++
MV = /bin/mv
RM = /bin/rm
RMF = /bin/rm -f
CP = /bin/cp
SED = /bin/sed
AWK = /usr/bin/mawk
ECHO = /bin/echo
CMP = /usr/bin/cmp
TBL = /usr/bin/tbl
AR = /usr/bin/ar
RANLIB = ranlib
INSTALL = /usr/bin/install -c
# add the -s to the following in PRODUCTION mode
INSTALL_PROGRAM = /usr/bin/install -c -m 0754
INSTALL_DATA = /usr/bin/install -c -m 644
INSTALL_SCRIPT = /usr/bin/install -c -m 0754
INSTALL_CONFIG = /usr/bin/install -c -m 640

# Flags & libs
CFLAGS = -g -O2 -Wall 
CPPFLAGS = 
LDFLAGS = 
TTOOL_LDFLAGS = 
#DEFS = -DHAVE_CONFIG_H
LIBS = -lpthread  -lwrap
DINCLUDE = 
DLIB = 
DB_LIBS = 

# Windows (cygwin) flags 
WCFLAGS = 
WLDFLAGS = 

# X Include directory
#XINC =  @XPM_CFLAGS@

# extra libraries needed by X on some systems, X library location
#XLIB =  -L/usr/X11R6/lib @XPM_LIBS@ -lX11 

# End of common section of the Makefile
#-------------------------------------------------------------------------

srcdir =	.
VPATH = 	.
.PATH:		.

# one up
basedir = ..

first_rule: all
dummy:

#-------------------------------------------------------------------------

all:
	./getdiskinfo
	./make_rescue_disk
	if test ! -e ../rpm_release ; then \
	   ./make_static_bacula ; \
	fi
	./copy_to_roottree

copy-static-fd:
	./getdiskinfo
	./make_rescue_disk
	./copy_static_bacula $(DESTDIR)$(sbindir)/static-bacula-fd $(DESTDIR)$(sysconfdir)/bacula-fd.conf
	./copy_to_roottree
    

depend:
	

#-------------------------------------------------------------------------


install:

uninstall:


clean:
	@$(RMF) *~ 1 2 3
	@$(RMF) format.* partition.* start_network mount_drives
	@$(RMF) sfdisk
	@$(RMF) -r diskinfo etc.gz
	@$(RMF) bin/bacula-fd

# clean for distribution
distclean: clean
	 


# ------------------------------------------------------------------------
