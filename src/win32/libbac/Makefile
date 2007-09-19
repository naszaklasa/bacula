#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#  
#  Written for Bacula by Howard Thomson, April 2006
#    for building using cross-complilation.
# 

include ../Makefile.inc

INCLUDES = \
	$(INCLUDE_GCC) \
	$(INCLUDE_MINGW) \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_VSS) \
	$(INCLUDE_ICONS) \
	$(INCLUDE_OPENSSL)

DEFINES = \
	-DWIN32 \
	$(HAVES)

######################################################################

# Files files in src/lib

LIB_OBJS = \
	$(OBJDIR)/address_conf.o \
	$(OBJDIR)/alist.o \
	$(OBJDIR)/attr.o \
	$(OBJDIR)/base64.o \
	$(OBJDIR)/berrno.o \
	$(OBJDIR)/bget_msg.o \
	$(OBJDIR)/bnet.o \
	$(OBJDIR)/bnet_server.o \
	$(OBJDIR)/bpipe.o \
	$(OBJDIR)/bregex.o \
	$(OBJDIR)/bshm.o \
	$(OBJDIR)/bsnprintf.o \
	$(OBJDIR)/bsock.o \
	$(OBJDIR)/bsys.o \
	$(OBJDIR)/btime.o \
	$(OBJDIR)/btimers.o \
	$(OBJDIR)/cram-md5.o \
	$(OBJDIR)/crc32.o \
	$(OBJDIR)/crypto.o \
	$(OBJDIR)/daemon.o \
	$(OBJDIR)/dlist.o \
	$(OBJDIR)/edit.o \
	$(OBJDIR)/enh_fnmatch.o \
	$(OBJDIR)/fnmatch.o \
	$(OBJDIR)/hmac.o \
	$(OBJDIR)/htable.o \
	$(OBJDIR)/idcache.o \
	$(OBJDIR)/jcr.o \
	$(OBJDIR)/lex.o \
	$(OBJDIR)/md5.o \
	$(OBJDIR)/mem_pool.o \
	$(OBJDIR)/message.o \
	$(OBJDIR)/openssl.o \
	$(OBJDIR)/parse_conf.o \
	$(OBJDIR)/pythonlib.o \
	$(OBJDIR)/queue.o \
	$(OBJDIR)/rblist.o \
	$(OBJDIR)/res.o \
	$(OBJDIR)/runscript.o \
	$(OBJDIR)/rwlock.o \
	$(OBJDIR)/scan.o \
	$(OBJDIR)/serial.o \
	$(OBJDIR)/sha1.o \
	$(OBJDIR)/signal.o \
	$(OBJDIR)/smartall.o \
	$(OBJDIR)/tls.o \
	$(OBJDIR)/tree.o \
	$(OBJDIR)/util.o \
	$(OBJDIR)/var.o \
	$(OBJDIR)/watchdog.o \
	$(OBJDIR)/winapi.o \
	$(OBJDIR)/workq.o

#	$(LIBDIR)/events.o

######################################################################

# Targets

.PHONY: all clean

all: $(LIBDIR)/libbac.a

clean:
	@echo "Cleaning `pwd`"
	$(ECHO_CMD)rm -f $(OBJDIR)/*.[od] $(LIBDIR)/libbac.a

#
# Rules
# 

$(LIBDIR)/libbac.a: $(LIB_OBJS)
	@echo "Updating archive $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(AR) rs $@ $^

include ../Makefile.rules

ifneq ($(MAKECMDGOALS),clean)
include $(patsubst %.o,%.d,$(filter-out %.res,$(LIB_OBJS)))
endif
