mylib_LTLIBRARIES = libaudiocd.la

mylibdir = $(libdir)/mpfc/input

libaudiocd_la_SOURCES = audiocd.c audiocd.h cddb.c cddb.h
libaudiocd_la_LIBADD = @COMMON_LIBS@ @RESOLV_LIBS@
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
