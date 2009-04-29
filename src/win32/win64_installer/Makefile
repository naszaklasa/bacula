#
# Makefile for win32 bacula executables
# Using MinGW cross-compiler on GNU/Linux
#
#  Written by Eric Bollengier, March 2009 
#    based on work by Robert Nelson
#

include ../Makefile.inc

VERSION := $(shell sed -ne 's/.*[ \t]VERSION[ \t][ \t]*"\(.*\)"/\1/p' < ../../version.h)

DEFINES := \
	-DVERSION=$(VERSION) \
	-DOUT_DIR=..\\release64 \
	-DSRC_DIR=release64 \
	-DWINVER=64 \
	-DBUILD_TOOLS=MinGW64 \

INSTALL_EXE := $(BUILDDIR)/release64/winbacula-$(VERSION).exe

BACULA_BINARIES := \
	bacula.dll \
	exchange-fd.dll \
	bsleep.exe \
	bsmtp.exe \
	bconsole.exe \
	bacula-fd.exe

DEPKGS32_BINARIES := \
	sed.exe \
	expr64.exe \
	snooze.exe

DEPKGS_BINARIES := \
	openssl.exe \
	cryptoeay32-0.9.8.dll \
	ssleay32-0.9.8.dll \
	pthreadGCE.dll \
	zlib1.dll

NONGCC_BINARIES := \
	libmysql.dll

NONGCC_LIBRARIES := \
	libpq.dll

MINGW_BINARIES := \
	mingwm10.dll

SCRIPT_FILES := \
	mtx-changer.cmd \
	disk-changer.cmd \
	dvd-handler.cmd

CAT_FILES := \
	create_mysql_database.cmd \
	drop_mysql_database.cmd \
	make_mysql_tables.cmd \
	make_mysql_tables.sql \
	drop_mysql_tables.cmd \
	drop_mysql_tables.sql \
	grant_mysql_privileges.cmd \
	grant_mysql_privileges.sql \
	make_mysql_catalog_backup.cmd \
	create_postgresql_database.cmd \
	create_postgresql_database.sql \
	drop_postgresql_database.cmd \
	make_postgresql_tables.cmd \
	make_postgresql_tables.sql \
	drop_postgresql_tables.cmd \
	drop_postgresql_tables.sql \
	grant_postgresql_privileges.cmd \
	grant_postgresql_privileges.sql \
	make_postgresql_catalog_backup.cmd \
	create_sqlite3_database.cmd \
	drop_sqlite3_database.cmd \
	make_sqlite3_tables.cmd \
	make_sqlite3_tables.sql \
	drop_sqlite3_tables.cmd \
	grant_sqlite3_privileges.cmd \
	make_sqlite3_catalog_backup.cmd \
	delete_catalog_backup.cmd

DIRD_FILES := \
	query.sql

SSL_FILES := \
	openssl.cnf

DOC_FILES := \
	manual/bacula.pdf \
	manual/bacula/*.html \
	manual/bacula/*.png  \
	manual/bacula/*.css

##########################################################################

# Targets

.PHONY: all clean installer

all:		$(INSTALL_EXE)

installer:	$(INSTALL_EXE)

clean:
	@echo "Cleaning `pwd`"
	$(CMD_ECHO)-rm -f $(INSTALL_EXE)
	$(CMD_ECHO)-rm -rf release64

#
# Rules
#

define Convert_Binary
release64/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@ ; \
	$(STAB2CV) $$@
endef

define Copy_Binary
release64/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@
endef

define Copy_Docs
release64/$(1): $(DOCDIR)/$(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$(dir $$@)
endef

$(foreach file,$(addprefix $(DEPKGS32)/bin/, $(DEPKGS32_BINARIES)),$(eval $(call Convert_Binary,$(file))))

$(foreach file,$(addprefix $(DEPKGS)/bin/, $(DEPKGS_BINARIES)),$(eval $(call Convert_Binary,$(file))))

#$(foreach file,$(addprefix $(DEPKGS)/bin/, $(NONGCC_BINARIES)),$(eval $(call Copy_Binary,$(file))))

#$(foreach file,$(addprefix $(DEPKGS)/lib/, $(NONGCC_LIBRARIES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix $(BINDIR)/, $(BACULA_BINARIES)),$(eval $(call Convert_Binary,$(file))))

#$(foreach file,$(addprefix $(MINGW_DLLDIR)/, $(MINGW_BINARIES)),$(eval $(call Copy_Binary,$(file))))

$(foreach file,$(addprefix $(DEPKGS32)/ssl/, $(SSL_FILES)),$(eval $(call Copy_Binary,$(file))))

#$(foreach file,$(addprefix ../scripts/, $(SCRIPT_FILES)),$(eval $(call Copy_Binary,$(file))))

#$(foreach file,$(addprefix ../cats/, $(CAT_FILES)),$(eval $(call Copy_Binary,$(file))))

#$(foreach file,$(addprefix ../../dird/, $(DIRD_FILES)),$(eval $(call Copy_Binary,$(file))))

#$(foreach file,$(DOC_FILES),$(eval $(call Copy_Docs,$(file))))

$(INSTALL_EXE): winbacula.nsi $(addprefix release64/,$(BACULA_BINARIES) $(DEPKGS_BINARIES) $(DEPKGS32_BINARIES) $(SSL_FILES) )
	NSISDIR=$(NSIS_DIR) \
	$(NSIS_DIR)/makensis -V3 $(DEFINES) winbacula.nsi

include $(BUILDDIR)/Makefile.rules
