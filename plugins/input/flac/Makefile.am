mylib_LTLIBRARIES = libflac.la

mylibdir = $(libdir)/mpfc/input

libflac_la_SOURCES = flac.c
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libflac_la_LDFLAGS = -Xcompiler -nostartfiles 
libflac_la_LIBADD = @COMMON_LIBS@ @FLAC_LIBS@ 
localedir = $(datadir)/locale
