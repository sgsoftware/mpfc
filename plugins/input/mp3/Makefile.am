lib_LTLIBRARIES = libmp3.la

libdir = $(prefix)/lib/mpfc/input

libmp3_la_SOURCES = mp3.c mp3.h 
INCLUDES = -I$(top_builddir)/src
libmp3_la_LDFLAGS = -Xcompiler -nostartfiles 
libmp3_la_LIBADD = @MAD_LIBS@ @MATH_LIBS@
localedir = $(datadir)/locale
