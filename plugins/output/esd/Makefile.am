lib_LTLIBRARIES = libesdout.la

libdir = $(prefix)/lib/mpfc/output

libesdout_la_SOURCES = esd.c
INCLUDES = @ESD_CFLAGS@ -I$(top_builddir)/src
libesdout_la_LIBADD = @ESD_LIBS@
localedir = $(datadir)/locale
