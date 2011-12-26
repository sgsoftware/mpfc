mylib_LTLIBRARIES = libcue.la

mylibdir = $(libdir)/mpfc/plist

libcue_la_SOURCES = cue.c cue_sheet.c
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
libcue_la_LDFLAGS = -Xcompiler -nostartfiles 
libcue_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
