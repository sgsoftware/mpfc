lib_LTLIBRARIES = libmpfc.la

libmpfchdrdir = $(includedir)/mpfc
libmpfchdr_HEADERS = ../mpfc-config.h ../src/types.h ../src/cfg.h \
					 ../src/charset.h \
					 ../src/file.h file_http.h file_reg.h \
					 ../src/genre_list.h ../src/csp.h \
					 ../src/inp.h ../src/pmng.h \
					 ../src/util.h ../src/song_info.h ../src/mystring.h \
					 ../src/logger.h ../src/plugin.h \
					 ../src/genp.h ../src/command.h ../src/main_types.h \
					 ../src/plp.h

libmpfc_la_SOURCES = cfg.c charset.c file.c file_http.c file_reg.c \
					 genre_list.c plugin_charset.c \
					 plugin_input.c plugin_mng.c util.c \
					 song_info.c string.c logger.c cfg_rcfile.c \
					 plugin.c plugin_general.c plugin_plist.c command.c \
					 $(libmpfchdr_HEADERS)
libmpfc_la_LIBADD = @COMMON_LIBS@ @RESOLV_LIBS@ @DL_LIBS@
INCLUDES = -I$(top_builddir)/src -I$(top_builddir)/libmpfcwnd @GSTREAMER_CFLAGS@ @TAGLIB_CFLAGS@
localedir = $(datadir)/locale
