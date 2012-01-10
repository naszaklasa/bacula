#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

#
# Change the following depending on where this directory is located
#  in the Bacula tree.	It should point to the src/win32 directory.
BUILDDIR = ..


include $(BUILDDIR)/Makefile.inc

INCLUDES = \
	-I. \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_BACULA) \
	$(INCLUDE_ZLIB) \
	$(INCLUDE_VSS) \
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_ICONS)

DEFINES = \
	-DUSING_DLL \
	-DWIN32_VSS \
	-DWINVER=0x500 \
	$(HAVES)

#	-DHAVE_TRAY_MONITOR \


vpath %.c $(MAINDIR)/src/filed $(BUILDDIR)/libwin32
vpath %.cpp $(MAINDIR)/src/filed $(BUILDDIR)/libwin32

##########################################################################

FILED_OBJS = \
	$(OBJDIR)/accurate.o \
	$(OBJDIR)/acl.o \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/backup.o \
	$(OBJDIR)/estimate.o \
	$(OBJDIR)/fd_plugins.o \
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
	$(OBJDIR)/service.o \
	$(OBJDIR)/xattr.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/bacula.res

#	$(OBJDIR)/trayMonitor.o \
#	$(OBJDIR)/aboutDialog.o \
#	$(OBJDIR)/statusDialog.o \


ALL_OBJS = $(FILED_OBJS)

FILED_LIBS = \
	$(LIBS_PTHREADS) \
	$(LIBS_ZLIB) \
	$(LIBS_NETWORK) \
	-lole32 \
	-loleaut32 \
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
	$(ECHO_CMD)rm -rf $(OBJDIRS)

#
# Rules
#

$(BINDIR)/bacula-fd.exe: $(FILED_OBJS) $(LIBS_BACULA)
	$(call link_winapp,$(FILED_LIBS))

$(OBJDIR)/winres.res: $(BUILDDIR)/libwin32/winres.rc
	@echo "Compiling $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(WINDRES) $(INCLUDE_ICONS) -O coff $< -o $@

include $(BUILDDIR)/Makefile.rules
