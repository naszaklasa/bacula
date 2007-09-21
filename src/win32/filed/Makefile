#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

INCLUDES = \
	-I. \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_VSS) \
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_ICONS)

DEFINES = \
	-DHAVE_TRAY_MONITOR \
	-DUSING_DLL \
	-DWIN32_VSS \
	-DWINVER=0x500 \
	$(HAVES)

vpath %.c ../../filed ../libwin32
vpath %.cpp ../../filed ../libwin32

##########################################################################

FILED_OBJS = \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/backup.o \
	$(OBJDIR)/estimate.o \
	$(OBJDIR)/filed.o \
	$(OBJDIR)/filed_conf.o \
	$(OBJDIR)/heartbeat.o \
	$(OBJDIR)/job.o \
	$(OBJDIR)/restore.o \
	$(OBJDIR)/status.o \
	$(OBJDIR)/verify.o \
	$(OBJDIR)/verify_vol.o \
	$(OBJDIR)/vss.o \
	$(OBJDIR)/vss_XP.o \
	$(OBJDIR)/vss_W2K3.o \
	$(OBJDIR)/vss_Vista.o \
	$(OBJDIR)/aboutDialog.o \
	$(OBJDIR)/service.o \
	$(OBJDIR)/statusDialog.o \
	$(OBJDIR)/trayMonitor.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/bacula.res

ALL_OBJS = $(FILED_OBJS)

FILED_LIBS = \
	$(LIBS_PTHREADS) \
	$(LIBS_ZLIB) \
	-lole32 \
	-loleaut32 \
	-lwsock32 \
	-luuid \
	-lcomctl32

######################################################################

# Targets

.PHONY: all clean

all: $(BINDIR)/bacula-fd.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bacula-fd.exe)

#
# Rules
#

$(BINDIR)/bacula-fd.exe: $(FILED_OBJS) $(LIBS_BACULA)
	$(call link_winapp,$(FILED_LIBS))

$(OBJDIR)/winres.res: ../libwin32/winres.rc
	@echo "Compiling $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(WINDRES) $(INCLUDE_ICONS) -O coff $< -o $@

include ../Makefile.rules
