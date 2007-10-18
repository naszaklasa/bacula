#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

INCLUDES = \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_OPENSSL)

DEFINES = \
	$(HAVES)

vpath %.c ../compat ../../findlib ../../lib
vpath %.cpp ../compat ../../findlib ../../lib

######################################################################

# Files files in src/lib

COMPAT_OBJS = \
	$(OBJDIR)/compat.o \
	$(OBJDIR)/print.o \
	$(OBJDIR)/winapi.o

#	$(OBJDIR)/getopt.o \

FIND_OBJS = \
	$(OBJDIR)/attribs.o \
	$(OBJDIR)/bfile.o \
	$(OBJDIR)/create_file.o \
	$(OBJDIR)/drivetype.o \
	$(OBJDIR)/enable_priv.o \
	$(OBJDIR)/find.o \
	$(OBJDIR)/find_one.o \
	$(OBJDIR)/fstype.o \
	$(OBJDIR)/makepath.o \
	$(OBJDIR)/match.o \
	$(OBJDIR)/save-cwd.o

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
	$(OBJDIR)/breg.o \
	$(OBJDIR)/bregex.o \
	$(OBJDIR)/bsock.o \
	$(OBJDIR)/bsnprintf.o \
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
	$(OBJDIR)/guid_to_name.o \
	$(OBJDIR)/hmac.o \
	$(OBJDIR)/htable.o \
	$(OBJDIR)/jcr.o \
	$(OBJDIR)/lex.o \
	$(OBJDIR)/md5.o \
	$(OBJDIR)/mem_pool.o \
	$(OBJDIR)/message.o \
	$(OBJDIR)/openssl.o \
	$(OBJDIR)/pythonlib.o \
	$(OBJDIR)/queue.o \
	$(OBJDIR)/rblist.o \
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
	$(OBJDIR)/workq.o

DLL_OBJS = \
	$(COMPAT_OBJS) $(FIND_OBJS) $(LIB_OBJS)

STATIC_OBJS = \
	$(OBJDIR)/parse_conf.o \
	$(OBJDIR)/res.o

ALL_OBJS = \
	$(DLL_OBJS) $(STATIC_OBJS)

LIBS_DLL = \
	$(LIBS_SSL) \
	$(LIBS_CRYPTO) \
	$(LIBS_PTHREADS) \
	$(LIBS_ZLIB) \
	-lwsock32 \
	-lole32 \
	-loleaut32 \
	-luuid

######################################################################

# Targets

.PHONY: all clean

all: $(BINDIR)/bacula.dll $(LIBDIR)/libbacula.a

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bacula.dll)
	$(ECHO_CMD)rm -f $(OBJDIR)/bacula.a $(LIBDIR)/libbacula.a

#
# Rules for generating from ../lib
#

$(LIBDIR)/libbacula.a: DLL_DEFINE=USING_DLL

$(LIBDIR)/libbacula.a: $(BINDIR)/bacula.dll $(STATIC_OBJS)
	@echo "Updating archive $@"
	$(call checkdir,$@)
	$(ECHO_CMD)cp $(OBJDIR)/bacula.a $@
	$(ECHO_CMD)$(AR) rsv $@ $(filter %.o,$^)

$(BINDIR)/bacula.dll: DLL_DEFINE=BUILDING_DLL

$(BINDIR)/bacula.dll: $(DLL_OBJS) bacula.def
	@echo "Linking $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(LDFLAGS) -mdll -mwindows -Wl,--out-implib,$(OBJDIR)/bacula.a $^ $(LIBS_DLL) -o $@

include ../Makefile.rules

$(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) -D$(DLL_DEFINE) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@echo "Compiling $<"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) -D$(DLL_DEFINE) $(CFLAGS) -c $< -o $@
