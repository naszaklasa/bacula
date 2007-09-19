#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

# Configuration

include ../Makefile.inc

INCLUDES = \
	-I. \
	-I../../stored \
	$(INCLUDE_DDK) \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_ICONS)

DEFINES = \
	-DHAVE_TRAY_MONITOR \
	-DUSING_DLL \
	-DWINVER=0x500 \
	$(HAVES)

vpath %.c ../../stored ../libwin32
vpath %.cpp ../../stored ../libwin32

##########################################################################

COMMON_OBJS = \
	$(OBJDIR)/acquire.o \
	$(OBJDIR)/ansi_label.o \
	$(OBJDIR)/autochanger.o \
	$(OBJDIR)/block.o \
	$(OBJDIR)/butil.o \
	$(OBJDIR)/dev.o \
	$(OBJDIR)/device.o \
	$(OBJDIR)/dvd.o \
	$(OBJDIR)/ebcdic.o \
	$(OBJDIR)/label.o \
	$(OBJDIR)/lock.o \
	$(OBJDIR)/match_bsr.o \
	$(OBJDIR)/mount.o \
	$(OBJDIR)/parse_bsr.o \
	$(OBJDIR)/read_record.o \
	$(OBJDIR)/record.o \
	$(OBJDIR)/reserve.o \
	$(OBJDIR)/scan.o \
	$(OBJDIR)/spool.o \
	$(OBJDIR)/stored_conf.o \
	$(OBJDIR)/wait.o \
	$(OBJDIR)/mtops.o

# bacula-sd
SD_OBJS = \
	$(OBJDIR)/stored.o \
	$(OBJDIR)/append.o \
	$(OBJDIR)/askdir.o \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/dircmd.o \
	$(OBJDIR)/fd_cmds.o \
	$(OBJDIR)/job.o \
	$(OBJDIR)/mac.o \
	$(OBJDIR)/pythonsd.o \
	$(OBJDIR)/read.o \
	$(OBJDIR)/status.o

# bextract
BEXTRACT_OBJS = \
	$(OBJDIR)/bextract.o \
	$(OBJDIR)/pythonsd.o

OBJS_WIN = \
	$(OBJDIR)/aboutDialog.o \
	$(OBJDIR)/service.o \
	$(OBJDIR)/statusDialog.o \
	$(OBJDIR)/trayMonitor.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/bacula.res

ALL_OBJS = \
	$(COMMON_OBJS) \
	$(SD_OBJS) \
	$(BEXTRACT_OBJS) \
	$(OBJS_WIN) \
	$(OBJDIR)/bcopy.o \
	$(OBJDIR)/bls.o \
	$(OBJDIR)/btape.o \
	$(OBJDIR)/bscan.o

LIBS_STORED = \
	$(LIBS_PTHREADS) \
	-lole32 \
	-loleaut32 \
	-lwsock32 \
	-luuid \
	-lcomctl32

######################################################################

# Targets

.PHONY: all clean

all:	$(BINDIR)/bacula-sd.exe $(BINDIR)/bcopy.exe $(BINDIR)/bextract.exe \
	$(BINDIR)/bls.exe $(BINDIR)/bscan.exe $(BINDIR)/btape.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bacula-sd.exe)
	$(call clean_exe,$(BINDIR)/bcopy.exe)
	$(call clean_exe,$(BINDIR)/bextract.exe)
	$(call clean_exe,$(BINDIR)/bls.exe)
	$(call clean_exe,$(BINDIR)/bscan.exe)
	$(call clean_exe,$(BINDIR)/btape.exe)

realclean: clean
	rm -f tags bacula-sd.conf

#
# Rules
#

$(BINDIR)/bacula-sd.exe: $(SD_OBJS) $(COMMON_OBJS) $(OBJS_WIN) $(LIBS_BACULA)
	$(call link_winapp,$(LIBS_STORED))

$(BINDIR)/btape.exe: $(OBJDIR)/btape.o $(COMMON_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(LIBS_STORED))

$(BINDIR)/bls.exe: $(OBJDIR)/bls.o $(COMMON_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(LIBS_STORED))

$(BINDIR)/bextract.exe: $(BEXTRACT_OBJS) $(COMMON_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(LIBS_STORED) $(LIBS_ZLIB))

$(BINDIR)/bscan.exe: $(OBJDIR)/bscan.o $(COMMON_OBJS) $(LIBS_CATS) $(LIBS_BACULA)
	$(call link_conapp,$(LIBS_STORED))

$(BINDIR)/bcopy.exe: $(OBJDIR)/bcopy.o $(COMMON_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(LIBS_STORED))

$(OBJDIR)/mtops.o: mtops.cpp
	@echo "Compiling $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(CXX) $(CFLAGS) $(INCLUDE_DDK) -I../../stored -c $< -o $@

$(OBJDIR)/winres.res:	  baculasd/winres.rc
	@echo "Compiling $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(WINDRES) $(INCLUDE_ICONS) -I baculasd -O coff $< -o $@

include ../Makefile.rules
