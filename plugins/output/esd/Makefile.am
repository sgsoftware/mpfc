lib_LTLIBRARIES = libesdout.la

libdir = $(prefix)/lib/mpfc/output

libesdout_la_SOURCES = esd.c
INCLUDES = @ESD_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libesdout_la_LIBADD = @COMMON_LIBS@ @ESD_LIBS@
localedir = $(datadir)/locale
