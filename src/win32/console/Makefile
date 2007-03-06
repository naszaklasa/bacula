#
# Makefile to build the native Win32 console
# VC++ and tools must be on your path
#
#     Kern Sibbald, April 2004
#

NMAKE=nmake

first_rule: all

dummy:

all:  console


console:
	(env MAKEFLAGS= ${NMAKE} CFG="console - Win32 Release" /f console.mak)


clean:
	rm -rf Debug/*.obj Release/*.obj

distclean: clean
