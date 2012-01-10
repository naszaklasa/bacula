#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Robert Nelson, June 2006
#

include ../Makefile.inc

VERSION := $(shell sed -ne 's/.*[ \t]VERSION[ \t][ \t]*"\(.*\)"/\1/p' < ../../version.h)

DEFINES := \
	-DVERSION=$(VERSION) \
	-DOUT_DIR=..\\release32 \
	-DSRC_DIR=release32 \
	-DBUILD_TOOLS=MinGW32 \
	-DWINVER=32 \
	-DBUILD_BAT=$(BUILD_BAT)

INSTALL_EXE := $(BUILDDIR)/release32/winbacula-$(VERSION).exe

BACULABINARIES := \
	bacula.dll \
	exchange-fd.dll \
	bacula-fd.exe \
	bconsole.exe \
	bsleep.exe \
	bsmtp.exe \

ifeq ($(bat),no)
	BACULA_BINARIES=$(BACULABINARIES)
	HELP=
else
	BACULA_BINARIES=$(BACULABINARIES) bat.exe bacula-tray-monitor.exe
	HELP=help
endif

DEPKGS_BINARIES := \
	libeay32.dll \
	pthreadGCE.dll \
	ssleay32.dll \
	zlib1.dll \
	openssl.exe \
	sed.exe \
	expr64.exe \
	snooze.exe \
	mkisofs.exe \
	growisofs.exe \
	QtCore4.dll \
	QtGui4.dll \
	libgcc_s_dw2-1.dll

NONGCC_BINARIES :=

NONGCC_LIBRARIES := 

MINGW_BINARIES := \
	mingwm10.dll

SCRIPT_FILES := 

CAT_FILES := 

DIRD_FILES := 

SSL_FILES := \
	openssl.cnf

DOC_FILES := \
	docs/manuals/en/console/console.pdf \
	docs/manuals/en/main/main.pdf \
	docs/manuals/en/misc/misc.pdf \
	docs/manuals/en/problems/problems.pdf \
	docs/manuals/en/utility/utility.pdf

##########################################################################

# Targets

.PHONY: all clean installer

all:	      $(HELP) $(INSTALL_EXE)

installer:    $(HELP) $(INSTALL_EXE)

clean:
	@echo "Cleaning `pwd`"
	$(CMD_ECHO)-rm -f $(INSTALL_EXE)
	$(CMD_ECHO)-rm -rf release32

help:
	rm -rf release32/help
	mkdir -p release32/help
	cp -f $(BINDIR)/help/* release32/help/


#
# Rules
#

define Convert_Binary
release32/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@ ; \
	$(STAB2CV) $$@
endef

define Copy_Binary
release32/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@
endef

define Copy_Docs
release32/$(1): $(DOCDIR)/$(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$(dir $$@)
endef

$(foreach file,$(addprefix $(DEPKGS)/bin/, $(DEPKGS_BINARIES)),$(eval $(call Convert_Binary,$(file))))

$(foreach file,$(addprefix $(DEPKGS)/bin/, $(NONGCC_BINARIES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix $(DEPKGS)/lib/, $(NONGCC_LIBRARIES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix $(BINDIR)/, $(BACULA_BINARIES)),$(eval $(call Convert_Binary,$(file))))

$(foreach file,$(addprefix $(MINGW_DLLDIR)/, $(MINGW_BINARIES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix $(DEPKGS)/ssl/, $(SSL_FILES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix ../scripts/, $(SCRIPT_FILES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix ../cats/, $(CAT_FILES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix ../../dird/, $(DIRD_FILES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(DOC_FILES),$(eval $(call Copy_Docs,$(file))))



$(INSTALL_EXE): winbacula.nsi $(addprefix release32/,$(BACULA_BINARIES) $(SCRIPT_FILES) $(CAT_FILES) $(DEPKGS_BINARIES) $(NONGCC_BINARIES) $(NONGCC_LIBRARIES) $(MINGW_BINARIES) $(SSL_FILES) $(DIRD_FILES) $(DOC_FILES))
	NSISDIR=$(NSIS_DIR) \
	echo "$(NSIS_DIR)/makensis -V3 $(DEFINES) winbacula.nsi"
	$(NSIS_DIR)/makensis -V3 $(DEFINES) winbacula.nsi

include $(BUILDDIR)/Makefile.rules
