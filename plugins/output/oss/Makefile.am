lib_LTLIBRARIES = liboss.la

libdir = $(prefix)/lib/mpfc/output

liboss_la_SOURCES = oss.c
INCLUDES = -I$(top_builddir)/src
liboss_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a
