lib_LTLIBRARIES = libdisk_writer.la

libdir = $(prefix)/lib/mpfc/output

libdisk_writer_la_SOURCES = writer.c
INCLUDES = -I$(top_builddir)/src
libdisk_writer_la_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a
