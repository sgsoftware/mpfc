lib_LTLIBRARIES = libmpfc.la

libdir = $(prefix)/lib/

libmpfc_la_SOURCES = cfg.c ../src/cfg.h charset.c ../src/charset.h file.c \
					 ../src/file.h file_http.c file_http.h file_reg.c \
					 file_reg.h genre_list.c \
					 ../src/genre_list.h id3.c ../src/myid3.h plugin_charset.c \
					 ../src/csp.h plugin_effect.c ../src/ep.h plugin_input.c \
					 ../src/inp.h plugin_output.c ../src/outp.h plugin_mng.c \
					 ../src/pmng.h util.c ../src/util.h ../src/song_info.h \
					 song_info.c ../src/mystring.h string.c ../src/vfs.h \
					 vfs.c ../src/logger.h logger.c cfg_rcfile.c \
					 ../src/plugin.h plugin.c
libmpfc_la_LIBADD = @COMMON_LIBS@ @RESOLV_LIBS@ 
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd
localedir = $(datadir)/locale
