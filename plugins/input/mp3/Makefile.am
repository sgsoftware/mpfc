lib_LTLIBRARIES = libmp3.la

libdir = $(prefix)/lib/mpfc/input

libmp3_la_SOURCES = mp3.c mp3.h
INCLUDES = -I$(top_builddir)/src
libmp3_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/glist/libglist.a \
					$(top_builddir)/src/cfg/libcfg.a \
					$(top_builddir)/src/myid3/libmyid3.a \
					$(top_builddir)/src/file/libfile.a 
libmp3_la_LDFLAGS = -Xcompiler -nostartfiles 
