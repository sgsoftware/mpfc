lib_LTLIBRARIES = libecho.la

libdir = $(prefix)/lib/mpfc/effect

libecho_la_SOURCES = echo.c
INCLUDES = -I$(top_builddir)/src
libecho_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a
