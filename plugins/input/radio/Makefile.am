lib_LTLIBRARIES = libradio.la

libdir = $(prefix)/lib/mpfc/input

libradio_la_SOURCES = radio.c 
INCLUDES = -I$(top_builddir)/src
libradio_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a
