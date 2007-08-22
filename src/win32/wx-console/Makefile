#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
# 

include ../Makefile.inc

INCLUDES = \
	-I../../wx-console \
	$(INCLUDE_BACULA) \
	$(INCLUDE_WX) \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_OPENSSL) \
	$(INCLUDE_ICONS)

DEFINES = \
	-DUSING_DLL \
	-DHAVE_WXCONSOLE \
	-D__WXMSW__ \
	-DWXUSINGDLL \
	-D_STAT_H \
	-D_STAT_DEFINED \
	$(HAVES)

vpath %.c ../../wx-console
vpath %.cpp ../../wx-console
vpath %.rc ../../wx-console

##########################################################################

# Files in ../../wx-console

CONSOLE_OBJS = \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/console_conf.o \
	$(OBJDIR)/console_thread.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/wxbconfigfileeditor.o \
	$(OBJDIR)/wxbconfigpanel.o \
	$(OBJDIR)/wxbhistorytextctrl.o \
	$(OBJDIR)/wxblistctrl.o \
	$(OBJDIR)/wxbmainframe.o \
	$(OBJDIR)/wxbrestorepanel.o \
	$(OBJDIR)/wxbtableparser.o \
	$(OBJDIR)/wxbtreectrl.o \
	$(OBJDIR)/wxbutils.o \
	$(OBJDIR)/wx-console_private.res

ALL_OBJS = $(CONSOLE_OBJS)

CONSOLE_LIBS = \
	$(LIBS_WX) \
	-lcomctl32 \
	-lwsock32 \
	-lole32 \
	-loleaut32 \
	-luuid

######################################################################

# Targets

.PHONY: all clean

all: $(BINDIR)/bwx-console.exe

clean:
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bwx-console.exe)
	$(call clean_exe,$(BINDIR)/wx-console.exe)

#
# Rules
#

$(BINDIR)/bwx-console.exe: $(CONSOLE_OBJS) $(LIBS_BACULA)
	$(call link_winapp,$(CONSOLE_LIBS))

$(OBJDIR)/wx-console_private.res: wx-console_private.rc
	@echo "Compiling $@"
	$(call checkdir,$@)
	$(ECHO_CMD)$(WINDRES) $(INCLUDE_ICONS) -I../../wx-console -O coff $< -o $@

include ../Makefile.rules
