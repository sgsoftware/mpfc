lib_LTLIBRARIES = libaudiocd.la

libdir = $(prefix)/lib/mpfc/input

libaudiocd_la_SOURCES = audiocd.c audiocd.h cddb.c cddb.h
INCLUDES = -I$(top_builddir)/src
libaudiocd_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a
