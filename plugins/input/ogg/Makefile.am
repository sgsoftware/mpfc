lib_LTLIBRARIES = libogg_vorbis.la

libdir = $(prefix)/lib/mpfc/input

libogg_vorbis_la_SOURCES = ogg.c vcedit.c vcedit.h
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libogg_vorbis_la_LDFLAGS = -Xcompiler -nostartfiles
libogg_vorbis_la_LIBADD = @OGG_LIBS@
