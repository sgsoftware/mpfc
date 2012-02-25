bin_PROGRAMS = mpfc
mpfc_SOURCES = main.c types.h player.c player.h \
					server.c server.h server_client.c server_client.h \
			        rd_with_notify.c rd_with_notify.h \
					plist.c plist.h song.c song.h util.h \
					cfg.h song_info.h history.c history.h undo.c undo.h \
					info_rw_thread.h info_rw_thread.c \
					help_screen.h help_screen.c \
					eqwnd.c eqwnd.h browser.c browser.h test.c test.h \
					logger.h logger_view.c logger_view.h plugin.h \
					command.h xconvert.c xconvert.h main_types.h \
					file_utils.c file_utils.h
EXTRA_DIST = .mpfcrc

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" \
	   -DSYSCONFDIR=\"$(sysconfdir)\" @DEFS@
INCLUDES = -I$(top_builddir)/libmpfcwnd/ @GSTREAMER_CFLAGS@
mpfc_LDADD = $(top_builddir)/libmpfc/libmpfc.la \
			 $(top_builddir)/libmpfcwnd/libmpfcwnd.la \
			 @GSTREAMER_LIBS@ @JSON_LIBS@ @COMMON_LIBS@ @PTHREAD_LIBS@ @DL_LIBS@ @MATH_LIBS@
