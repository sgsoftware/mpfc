bin_PROGRAMS = mpfc
mpfc_SOURCES = main.c types.h error.h error.c player.c player.h \
					plist.c plist.h song.c song.h util.h \
					inp.c inp.h outp.c outp.h pmng.c pmng.h \
					cfg.h song_info.h ep.h ep.c \
					window.c window.h editbox.c editbox.h \
					choice_ctrl.c choice_ctrl.h file_input.c file_input.h \
					dlgbox.c dlgbox.h menu.c menu.h codepages.c codepages.h \
					listbox.c listbox.h help_screen.c help_screen.h \
					sat.c sat.h eqwnd.c eqwnd.h key_bind.c key_bind.h \
					colors.c colors.h history.c history.h button.c button.h \
					combobox.c combobox.h undo.c undo.h
EXTRA_DIST = .mpfcrc

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" @DEFS@
mpfc_LDADD = glist/libglist.a util/libutil.a cfg/libcfg.a
SUBDIRS = util glist cfg
