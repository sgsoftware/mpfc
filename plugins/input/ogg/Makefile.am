lib_LTLIBRARIES = libogg.la

libdir = $(prefix)/lib/mpfc/input

libogg_la_SOURCES = ogg.c 
INCLUDES = -I$(top_builddir)/src
libogg_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a \
					$(top_builddir)/src/glist/libglist.a
libogg_la_LDFLAGS = -Xcompiler -nostartfiles
