lib_LTLIBRARIES = libtimidi.la

libdir = $(prefix)/lib/mpfc/input

libtimidi_la_SOURCES = timidi.c timidi.h
INCLUDES = -I$(top_builddir)/src
libtimidi_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/glist/libglist.a \
					$(top_builddir)/src/cfg/libcfg.a \
					$(top_builddir)/src/file/libfile.a
