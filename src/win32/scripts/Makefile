#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

##########################################################################

BSLEEP_OBJS = \
	$(OBJDIR)/bsleep.o

ALL_OBJS = \
	$(BSLEEP_OBJS)

######################################################################

# Targets

.PHONY: all clean

all:	$(BINDIR)/bsleep.exe

clean:
	@echo "Cleaning `pwd`"
	$(call clean_obj,$(ALL_OBJS))
	$(call clean_exe,$(BINDIR)/bsleep.exe)

#
# Rules
#

$(BINDIR)/bsleep.exe: $(BSLEEP_OBJS)
	$(call link_conapp)

include ../Makefile.rules
