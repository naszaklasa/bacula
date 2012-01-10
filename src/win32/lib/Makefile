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

vpath %.c $(BUILDDIR)/compat $(MAINDIR)/src/findlib $(MAINDIR)/src/lib
vpath %.cpp $(BUILDDIR)/compat $(MAINDIR)/src/findlib $(MAINDIR)/src/lib

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
	$(OBJDIR)/mkpath.o \
	$(OBJDIR)/match.o

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
	$(OBJDIR)/fnmatch.o \
	$(OBJDIR)/guid_to_name.o \
	$(OBJDIR)/hmac.o \
	$(OBJDIR)/htable.o \
	$(OBJDIR)/jcr.o \
	$(OBJDIR)/lex.o \
	$(OBJDIR)/md5.o \
	$(OBJDIR)/mem_pool.o \
	$(OBJDIR)/message.o \
	$(OBJDIR)/mntent_cache.o \
	$(OBJDIR)/openssl.o \
	$(OBJDIR)/plugins.o \
	$(OBJDIR)/priv.o \
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
	$(OBJDIR)/workq.o \
	$(OBJDIR)/binflate.o \
	$(OBJDIR)/lockmgr.o

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
	$(LIBS_NETWORK) \
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
	$(ECHO_CMD)rm -rf $(OBJDIRS)

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

$(BINDIR)/bacula.dll: $(DLL_OBJS) bacula$(WIN_VERSION).def
	@echo "Linking $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(LDFLAGS) -mdll -mwindows -Wl,--out-implib,$(OBJDIR)/bacula.a $^ $(LIBS_DLL) -o $@

#bacula$(WIN_VERSION).def: $(DLL_OBJS)
#	 ./make_def $(DLL_OBJS) >bacula$(WIN_VERSION).def.new && \
#	 mv bacula$(WIN_VERSION).def.new bacula$(WIN_VERSION).def


include ../Makefile.rules

$(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) -D$(DLL_DEFINE) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@echo "Compiling $<"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) -D$(DLL_DEFINE) $(CFLAGS) -c $< -o $@
