mylib_LTLIBRARIES = libogg_vorbis.la

mylibdir = $(libdir)/mpfc/input

libogg_vorbis_la_SOURCES = ogg.c vcedit.c vcedit.h
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libogg_vorbis_la_LDFLAGS = -Xcompiler -nostartfiles
libogg_vorbis_la_LIBADD = @COMMON_LIBS@ @OGG_LIBS@
