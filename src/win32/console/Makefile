#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

INCLUDES = \
	$(INCLUDE_BACULA) \
	$(INCLUDE_PTHREADS) \
	$(INCLUDE_OPENSSL)

DEFINES = \
	-DUSING_DLL \
	$(HAVES)

vpath %.c ../../console
vpath %.cpp ../../console

##########################################################################

CONSOLE_OBJS = \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/console.o \
	$(OBJDIR)/console_conf.o

ALL_OBJS = $(CONSOLE_OBJS)

CONSOLE_LIBS = \
	-lwsock32

######################################################################

# Targets

.PHONY: all clean

all: $(BINDIR)/bconsole.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bconsole.exe)

#
# Rules
#

$(BINDIR)/bconsole.exe: $(CONSOLE_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(CONSOLE_LIBS))

include ../Makefile.rules
