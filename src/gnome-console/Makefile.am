## Process this file with automake to produce Makefile.in

INCLUDES = \
        -I$(top_srcdir)/../.. \
        -I$(top_srcdir)/intl \
        $(GNOME_INCLUDEDIR)

bin_PROGRAMS = bacula

bacula_SOURCES = \
        console.c \
        console_conf.c \
        support.c support.h \
        interface.c interface.h \
        callbacks.c callbacks.h

bacula_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) -L../../lib -lbac
