mylib_LTLIBRARIES = libm3u.la

mylibdir = $(libdir)/mpfc/plist

libm3u_la_SOURCES = m3u.c 
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd
libm3u_la_LDFLAGS = -Xcompiler -nostartfiles 
libm3u_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
