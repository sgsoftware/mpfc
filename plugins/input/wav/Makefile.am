lib_LTLIBRARIES = libwav.la

libdir = $(prefix)/lib/mpfc/input

libwav_la_SOURCES = wav.c wav.h
INCLUDES = -I$(top_builddir)/src
libwav_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/glist/libglist.a \
					$(top_builddir)/src/cfg/libcfg.a
