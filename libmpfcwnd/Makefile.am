lib_LTLIBRARIES = libmpfcwnd.la
libdir = $(prefix)/lib/
libmpfcwnd_la_SOURCES = wnd.c wnd.h wnd_print.c wnd_print.h \
						wnd_msg.c wnd_msg.h wnd_kbd.c wnd_kbd.h \
						wnd_def_handlers.h wnd_def_handlers.c \
						wnd_root.h wnd_root.c wnd_mouse.c wnd_mouse.h \
						wnd_types.h wnd_basic.c wnd_basic.h wnd_class.c \
						wnd_class.h wnd_dialog.c wnd_dialog.h \
						wnd_dlgitem.c wnd_dlgitem.h \
						wnd_button.c wnd_button.h wnd_editbox.c \
						wnd_editbox.h wnd_vbox.c wnd_vbox.h wnd_hbox.c \
						wnd_hbox.h wnd_label.c wnd_label.h wnd_filebox.c \
						wnd_filebox.h wnd_checkbox.c wnd_checkbox.h \
						wnd_radio.c wnd_radio.h wnd_combobox.c wnd_combobox.h \
						wnd_scrollable.c wnd_scrollable.h wnd_kbind.c \
						wnd_kbind.h
libmpfcwnd_la_LIBADD = @GPM_LIBS@ @PTHREAD_LIBS@ @CURSES_LIBS@
INCLUDES = -I$(top_builddir)/src
localedir = $(datadir)/locale
