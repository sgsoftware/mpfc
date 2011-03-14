mylib_LTLIBRARIES = libmp3.la

mylibdir = $(libdir)/mpfc/input

libmp3_la_SOURCES = mp3.c mp3.h 
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libmp3_la_LDFLAGS = -Xcompiler -nostartfiles 
libmp3_la_LIBADD = @COMMON_LIBS@ @MAD_LIBS@ @MATH_LIBS@
localedir = $(datadir)/locale
