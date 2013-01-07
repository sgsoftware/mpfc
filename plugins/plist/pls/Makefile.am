mylib_LTLIBRARIES = libpls.la

mylibdir = $(libdir)/mpfc/plist

libpls_la_SOURCES = pls.c 
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd
libpls_la_LDFLAGS = -Xcompiler -nostartfiles 
libpls_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
