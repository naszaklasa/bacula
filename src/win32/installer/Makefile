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
	-DOUT_DIR=..\\release \
	-DSRC_DIR=release \
	-DBUILD_TOOLS=MinGW \

INSTALL_EXE := ../release/winbacula-$(VERSION).exe

BACULA_BINARIES := \
	bacula.dll \
	cats_mysql.dll \
	cats_postgresql.dll \
	cats_sqlite3.dll \
	bacula-dir.exe \
	bacula-fd.exe \
	bacula-sd.exe \
	bconsole.exe \
	bcopy.exe \
	bextract.exe \
	bls.exe \
	bscan.exe \
	bsleep.exe \
	bsmtp.exe \
	btape.exe \
	dbcheck.exe \
	scsilist.exe \
	bwx-console.exe

DEPKGS_BINARIES := \
	libeay32.dll \
	pthreadGCE.dll \
	ssleay32.dll \
	zlib1.dll \
	sqlite3.exe \
	openssl.exe \
	loaderinfo.exe \
	mt.exe \
	mtx.exe \
	scsitape.exe \
	sed.exe \
	tapeinfo.exe \
	expr64.exe \
	snooze.exe \
	mkisofs.exe \
	growisofs.exe \
	dvd-ram-control.exe \
	dvd+rw-booktype.exe \
	dvd+rw-format.exe \
	dvd+rw-mediainfo.exe \
	wxbase28_gcc_bacula.dll \
	wxmsw28_core_gcc_bacula.dll

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
	$(CMD_ECHO)-rm -rf release

#
# Rules
#

define Convert_Binary
release/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@ ; \
	$(STAB2CV) $$@
endef

define Copy_Binary
release/$$(notdir $(1)): $(1)
	$$(call checkdir,$$@)
	$(ECHO_CMD)cp -f $$^ $$@
endef

define Copy_Docs
release/$(1): $(DOCDIR)/$(1)
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

$(INSTALL_EXE): winbacula.nsi $(addprefix release/,$(BACULA_BINARIES) $(SCRIPT_FILES) $(CAT_FILES) $(DEPKGS_BINARIES) $(NONGCC_BINARIES) $(NONGCC_LIBRARIES) $(MINGW_BINARIES) $(SSL_FILES) $(DIRD_FILES) $(DOC_FILES))
	NSISDIR=$(NSIS_DIR) \
	$(NSIS_DIR)/makensis -V3 $(DEFINES) winbacula.nsi

include ../Makefile.rules
