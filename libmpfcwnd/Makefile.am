lib_LTLIBRARIES = libmpfcwnd.la
libdir = $(prefix)/lib/
libmpfcwnd_la_SOURCES = wnd.c wnd.h wnd_print.c wnd_print.h \
						wnd_msg.c wnd_msg.h wnd_kbd.c wnd_kbd.h \
						wnd_def_handlers.h wnd_def_handlers.c \
						wnd_root.h wnd_root.c wnd_common_msg.c \
						wnd_common_msg.h wnd_mouse.c wnd_mouse.h
libmpfcwnd_la_LIBADD = @GPM_LIBS@ @PTHREAD_LIBS@ @CURSES_LIBS@
INCLUDES = -I$(top_builddir)/src
localedir = $(datadir)/locale
