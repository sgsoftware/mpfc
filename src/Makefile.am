bin_PROGRAMS = mpfc
mpfc_SOURCES = main.c types.h error.h player.c player.h \
					plist.c plist.h song.c song.h util.c util.h \
					inp.c inp.h outp.c outp.h pmng.c pmng.h \
					options.c options.h song_info.h \
					window.c window.h editbox.c editbox.h \
					choice_ctrl.c choice_ctrl.h file_input.c file_input.h \
					dlgbox.c dlgbox.h menu.c menu.h codepages.c codepages.h \
					listbox.c listbox.h help_screen.c help_screen.h

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
mpfc_LDADD = glist/libglist.a error/liberror.a
SUBDIRS = error glist
