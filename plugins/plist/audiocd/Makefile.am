mylib_LTLIBRARIES = libaudiocd.la

mylibdir = $(libdir)/mpfc/plist

libaudiocd_la_SOURCES = audiocd.c 
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd @GSTREAMER_CFLAGS@
libaudiocd_la_LDFLAGS = -Xcompiler -nostartfiles 
libaudiocd_la_LIBADD = @COMMON_LIBS@ 
localedir = $(datadir)/locale
