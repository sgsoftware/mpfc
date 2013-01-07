lib_LTLIBRARIES = libmpfc.la

libmpfchdrdir = $(includedir)/mpfc
libmpfchdr_HEADERS = ../mpfc-config.h ../src/types.h ../src/cfg.h \
					 ../src/charset.h \
					 ../src/genre_list.h ../src/csp.h ../src/pmng.h \
					 ../src/util.h ../src/song_info.h ../src/mystring.h \
					 ../src/logger.h ../src/plugin.h \
					 ../src/genp.h ../src/command.h ../src/main_types.h \
					 ../src/plp.h

libmpfc_la_SOURCES = cfg.c charset.c \
					 genre_list.c plugin_charset.c \
					 plugin_mng.c util.c \
					 song_info.c string.c logger.c cfg_rcfile.c \
					 plugin.c plugin_general.c plugin_plist.c command.c \
					 $(libmpfchdr_HEADERS)
libmpfc_la_LIBADD = @COMMON_LIBS@ @RESOLV_LIBS@ @DL_LIBS@
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/libmpfcwnd @GSTREAMER_CFLAGS@ @TAGLIB_CFLAGS@
localedir = $(datadir)/locale
