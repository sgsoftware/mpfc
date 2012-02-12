mylib_LTLIBRARIES = libcueplist.la

mylibdir = $(libdir)/mpfc/plist

libcueplist_la_SOURCES = cue.c 
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd -I$(top_builddir)/libcue/src
libcueplist_la_LDFLAGS = -Xcompiler -nostartfiles 
libcueplist_la_LIBADD = @COMMON_LIBS@ $(top_builddir)/libcue/src/libcue/libmpfccue.la
localedir = $(datadir)/locale
