lib_LTLIBRARIES = libogg_vorbis.la

libdir = $(prefix)/lib/mpfc/input

libogg_vorbis_la_SOURCES = ogg.c vcedit.c vcedit.h
INCLUDES = -I$(top_builddir)/src
libogg_vorbis_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a \
					$(top_builddir)/src/glist/libglist.a
libogg_vorbis_la_LDFLAGS = -Xcompiler -nostartfiles
