AM_YFLAGS		= -d

EXTRA_DIST		= libcue.h cd.h cdtext.h rem.h time.h config.h # libcue.pc.in

#pkgconfigdir		= $(libdir)/pkgconfig
#pkgconfig_DATA		= libcue.pc


AM_CPPFLAGS =				\
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

noinst_LTLIBRARIES		= libmpfccue.la

#libmpfccue_la_LDFLAGS	= -version-info 2:0 
#libmpfccueincludedir	= $(includedir)/@LIBMPFCCUE_LIBRARY_NAME@
