bin_PROGRAMS = mpfc
WND_SRC = ../libmpfcwnd/wnd.c ../libmpfcwnd/wnd_basic.c \
		  ../libmpfcwnd/wnd_kbd.c ../libmpfcwnd/wnd_mouse.c \
		  ../libmpfcwnd/wnd_root.c ../libmpfcwnd/wnd_print.c \
		  ../libmpfcwnd/wnd_dialog.c ../libmpfcwnd/wnd_dlgitem.c \
		  ../libmpfcwnd/wnd_button.c ../libmpfcwnd/wnd_editbox.c \
		  ../libmpfcwnd/wnd_class.c ../libmpfcwnd/wnd_def_handlers.c \
		  ../libmpfcwnd/wnd_msg.c
mpfc_SOURCES = main.c types.h error.h error.c player.c player.h \
					plist.c plist.h song.c song.h util.h \
					cfg.h song_info.h sat.c sat.h key_bind.c key_bind.h \
					colors.c colors.h history.c history.h undo.c undo.h \
					finder.h iwt.c iwt.h help_screen.h help_screen.c \
					eqwnd.c eqwnd.h browser.c browser.h 
EXTRA_DIST = .mpfcrc

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" @DEFS@
INCLUDES = -I$(top_builddir)/libmpfcwnd/
mpfc_LDADD = @PTHREAD_LIBS@ @DL_LIBS@ \
			 $(top_builddir)/libmpfc/libmpfc.la \
			 $(top_builddir)/libmpfcwnd/libmpfcwnd.la
