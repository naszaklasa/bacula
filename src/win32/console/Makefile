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

vpath %.c $(MAINDIR)/src/console
vpath %.cpp $(MAINDIR)/src/console

##########################################################################

CONSOLE_OBJS = \
	$(OBJDIR)/authenticate.o \
	$(OBJDIR)/console.o \
	$(OBJDIR)/console_conf.o

ALL_OBJS = $(CONSOLE_OBJS)

CONSOLE_LIBS = $(LIBS_NETWORK) 

######################################################################

# Targets

.PHONY: all clean

all: $(BINDIR)/bconsole.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bconsole.exe)
	$(ECHO_CMD)rm -rf $(OBJDIRS)

#
# Rules
#

$(BINDIR)/bconsole.exe: $(CONSOLE_OBJS) $(LIBS_BACULA)
	$(call link_conapp,$(CONSOLE_LIBS))

include ../Makefile.rules
