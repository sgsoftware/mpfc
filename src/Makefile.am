bin_PROGRAMS = mpfc
mpfc_SOURCES = main.c types.h error.h error.c player.c player.h \
					plist.c plist.h song.c song.h util.h \
					cfg.h cfg.c song_info.h \
					window.c window.h editbox.c editbox.h \
					choice_ctrl.c choice_ctrl.h file_input.c file_input.h \
					dlgbox.c dlgbox.h \
					listbox.c listbox.h help_screen.c help_screen.h \
					sat.c sat.h eqwnd.c eqwnd.h key_bind.c key_bind.h \
					colors.c colors.h history.c history.h button.c button.h \
					combobox.c combobox.h undo.c undo.h label.c label.h \
					finder.h dlg_ctrl.c dlg_ctrl.h iwt.c iwt.h \
					browser.c browser.h
EXTRA_DIST = .mpfcrc

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" @DEFS@
mpfc_LDADD = @CURSES_LIBS@ @PTHREAD_LIBS@ @DL_LIBS@ @GPM_LIBS@ \
			 $(top_builddir)/libmpfc/libmpfc.la
