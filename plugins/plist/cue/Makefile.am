mylib_LTLIBRARIES = libcueplist.la

mylibdir = $(libdir)/mpfc/plist

libcueplist_la_SOURCES = cue.c 
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd @LIBCUE_CFLAGS@
libcueplist_la_LDFLAGS = -Xcompiler -nostartfiles 
libcueplist_la_LIBADD = @COMMON_LIBS@ @LIBCUE_LIBS@
localedir = $(datadir)/locale
