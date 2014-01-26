mylib_LTLIBRARIES = libm3u.la

mylibdir = $(libdir)/mpfc/plist

libm3u_la_SOURCES = m3u.c 
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd
libm3u_la_LDFLAGS = -Xcompiler -nostartfiles -version-info 2:0
libm3u_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
