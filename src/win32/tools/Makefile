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
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_ICONS)

DEFINES = \
	-DUSING_DLL \
	-DUSING_CATS \
	$(HAVES)

LDFLAGS:=$(LDFLAGS) -lstdc++

vpath %.c ../../tools ../../dird
vpath %.cpp ../../tools ../../dird

##########################################################################

DIRCONF_OBJS = \
	$(OBJDIR)/dird_conf.o \
	$(OBJDIR)/run_conf.o \
	$(OBJDIR)/inc_conf.o

ALL_OBJS = \
	$(DIRCONF_OBJS) \
	$(OBJDIR)/bsmtp.o \
	$(OBJDIR)/dbcheck.o \
	$(OBJDIR)/scsilist.o \
	$(OBJDIR)/ScsiDeviceList.o \
	$(OBJDIR)/fstype.o \
	$(OBJDIR)/drivetype.o \
	$(OBJDIR)/testfind.o \
	$(OBJDIR)/testls.o \
	$(OBJDIR)/bregex.o \
	$(OBJDIR)/bwild.o

##########################################################################

# Targets

.PHONY: all clean

all: \
	$(BINDIR)/bsmtp.exe $(BINDIR)/dbcheck.exe  $(BINDIR)/scsilist.exe \
	$(BINDIR)/drivetype.exe $(BINDIR)/fstype.exe \
	$(BINDIR)/testfind.exe $(BINDIR)/testls.exe \
	$(BINDIR)/bregex.exe $(BINDIR)/bwild.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bsmtp.exe)
	$(call clean_exe,$(BINDIR)/dbcheck.exe)
	$(call clean_exe,$(BINDIR)/scsilist.exe)
	$(call clean_exe,$(BINDIR)/drivetype.exe)
	$(call clean_exe,$(BINDIR)/fstype.exe)
	$(call clean_exe,$(BINDIR)/testfind.exe)
	$(call clean_exe,$(BINDIR)/testls.exe)
	$(call clean_exe,$(BINDIR)/bregex.exe)
	$(call clean_exe,$(BINDIR)/bwild.exe)

#
# Rules
#

$(BINDIR)/bsmtp.exe: $(OBJDIR)/bsmtp.o $(LIBS_BACULA)
	$(call link_conapp,-lws2_32)

$(BINDIR)/dbcheck.exe: $(OBJDIR)/dbcheck.o $(DIRCONF_OBJS) $(LIBS_BACULA) $(LIBS_CATS)
	$(call link_conapp,)

$(BINDIR)/scsilist.exe: $(OBJDIR)/scsilist.o $(OBJDIR)/ScsiDeviceList.o $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/drivetype.exe: $(OBJDIR)/drivetype.o $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/fstype.exe: $(OBJDIR)/fstype.o $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/testfind.exe: $(OBJDIR)/testfind.o $(DIRCONF_OBJS) $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/testls.exe: $(OBJDIR)/testls.o $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/bregex.exe: $(OBJDIR)/bregex.o $(LIBS_BACULA)
	$(call link_conapp,)

$(BINDIR)/bwild.exe: $(OBJDIR)/bwild.o $(LIBS_BACULA)
	$(call link_conapp,)

include ../Makefile.rules
