ECHO_CMD=@

DIRS=	dll \
	cats \
	filed \
	dird \
	stored \
	console \
	wx-console \
	tools \
	scripts \
	installer

.PHONY: $(DIRS) clean all

all: Makefile.inc $(DIRS)

clean: $(DIRS)
	$(ECHO_CMD)-rm -rf release

$(DIRS):
	@if test -f Makefile.inc; then \
	   if $(MAKE) -C $@ $(MAKECMDGOALS); then \
		echo "\n===== Make of $@ succeeded =====\n\n" ; \
	   else  \
		echo "\n!!!!! Make of $@ failed !!!!!\n\n" ; \
	   fi ; \
	fi

Makefile.inc: Makefile.inc.in
	@echo Creating $@
	$(ECHO_CMD)TOPDIR=`(cd ../../..;pwd)`; \
	if test -e ../../../cross-tools/mingw32/bin/mingw32-gcc; then \
		BINDIR=$${TOPDIR}/cross-tools/mingw32/bin; \
		INCDIR=$${TOPDIR}/cross-tools/mingw32/mingw32/include; \
		DLLDIR=$${TOPDIR}/cross-tools/mingw32/mingw32/bin; \
	elif test -e /mingw/bin/mingw32-gcc; then \
		BINDIR=/mingw/bin; \
		INCDIR=/mingw/include; \
		DLLDIR=/mingw/bin; \
	else \
		echo "\nThe GCC cross compiler isn't installed."; \
		echo "You must run build-win32-cross-tools and build-dependencies first.\n"; \
		exit 1; \
	fi ; \
	$(ECHO_CMD)BUILDDIR=`(pwd)`; \
	sed \
		-e "s^@BUILDDIR@^$${BUILDDIR}^" \
		-e "s^@TOPDIR@^$${TOPDIR}^" \
		-e "s^@BINDIR@^$${BINDIR}^" \
		-e "s^@INCDIR@^$${INCDIR}^" \
		-e "s^@DLLDIR@^$${DLLDIR}^" < Makefile.inc.in > $@
