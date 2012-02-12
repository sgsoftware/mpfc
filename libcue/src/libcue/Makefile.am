AM_YFLAGS		= -d

EXTRA_DIST		= # libcue.pc.in

#pkgconfigdir		= $(libdir)/pkgconfig
#pkgconfig_DATA		= libcue.pc


INCLUDES		=				\
			-I.				\
			-Wall -Wmissing-declarations

libmpfccue_la_SOURCES	=				\
			cd.c				\
			cdtext.c			\
			rem.c				\
			time.c				\
			cue_parser.y			\
			cue_scanner.l

#libmpfccueinclude_HEADERS	= 				\
#			libcue.h			\
#			cd.h				\
#			cdtext.h			\
#			rem.h				\
#			time.h

lib_LTLIBRARIES		= libmpfccue.la

#libmpfccue_la_LDFLAGS	= -version-info @LIBMPFCCUE_LIBTOOL_VERSION@
#libmpfccueincludedir	= $(includedir)/@LIBMPFCCUE_LIBRARY_NAME@
