lib_LTLIBRARIES = libmp3.la

libdir = $(prefix)/lib/mpfc/input

libmp3_la_SOURCES = mp3.c
INCLUDES = -I$(top_builddir)/src
libmp3_la_LIBADD = $(top_builddir)/src/error/liberror.a \
					$(top_builddir)/src/glist/libglist.a
libmp3_la_LDFLAGS = -Xcompiler -nostartfiles
