noinst_LIBRARIES = libplugin.a
libplugin_a_SOURCES = ../pmng.h pmng.c ../inp.h inp.c ../outp.h outp.c ../ep.h \
					ep.c ../finder.c
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
INCLUDES = -I$(top_builddir)/src
libplugin_a_LIBADD = $(top_builddir)/src/util/libutil.a \
					$(top_builddir)/src/cfg/libcfg.a 
