mylib_LTLIBRARIES = libcueplist.la

mylibdir = $(libdir)/mpfc/plist

libcueplist_la_SOURCES = cue.c 
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd -I$(top_srcdir)/libcue/src
libcueplist_la_LDFLAGS = -Xcompiler -nostartfiles 
libcueplist_la_LIBADD = $(top_builddir)/libcue/src/libcue/libmpfccue.la @COMMON_LIBS@
localedir = $(datadir)/locale
