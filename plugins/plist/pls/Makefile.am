mylib_LTLIBRARIES = libpls.la

mylibdir = $(libdir)/mpfc/plist

libpls_la_SOURCES = pls.c 
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd
libpls_la_LDFLAGS = -Xcompiler -nostartfiles -version-info 2:0
libpls_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
