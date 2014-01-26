lib_LTLIBRARIES = libmpfcwnd.la
libmpfcwndhdrdir = $(includedir)/mpfc/libmpfcwnd
libmpfcwndhdr_HEADERS = wnd.h wnd_print.h wnd_msg.h wnd_kbd.h \
						wnd_def_handlers.h wnd_root.h wnd_mouse.h wnd_types.h \
						wnd_basic.h wnd_class.h wnd_dialog.h wnd_dlgitem.h \
						wnd_button.h wnd_editbox.h wnd_vbox.h wnd_hbox.h \
						wnd_label.h wnd_filebox.h wnd_checkbox.h wnd_radio.h \
						wnd_combobox.h wnd_scrollable.h wnd_kbind.h \
						wnd_views.h wnd_multiview_dialog.h wnd_listbox.h \
						wnd_repval.h
libmpfcwnd_la_SOURCES = wnd.c wnd_print.c wnd_msg.c wnd_kbd.c \
						wnd_def_handlers.c wnd_root.c wnd_mouse.c wnd_basic.c \
						wnd_class.c wnd_dialog.c wnd_dlgitem.c wnd_button.c \
						wnd_editbox.c wnd_vbox.c wnd_hbox.c wnd_label.c \
						wnd_filebox.c wnd_checkbox.c wnd_radio.c \
						wnd_combobox.c wnd_scrollable.c wnd_kbind.c \
						wnd_views.c wnd_multiview_dialog.c wnd_listbox.c \
						wnd_repval.c \
						$(libmpfcwndhdr_HEADERS)
libmpfcwnd_la_LIBADD = @COMMON_LIBS@ @GPM_LIBS@ @PTHREAD_LIBS@ @CURSES_LIBS@ 
libmpfcwnd_la_LDFLAGS = -version-info 2:0
AM_CPPFLAGS = -I$(top_srcdir)/src @CURSES_CFLAGS@
localedir = $(datadir)/locale
