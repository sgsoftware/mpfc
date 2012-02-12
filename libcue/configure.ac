AC_INIT([libmpfccue], [1.4.0], [sergey.e.galanov@gmail.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([foreign 1.9 dist-bzip2])
AM_CONFIG_HEADER([config.h])

m4_define([package_major_version], [1])
m4_define([package_minor_version], [4])
m4_define([package_micro_version], [0])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])
AM_INIT_AUTOMAKE

# This makes sure the right substitution is done
[LIBMPFCCUE_MAJOR_VERSION]=package_major_version
[LIBMPFCCUE_MINOR_VERSION]=package_minor_version
[LIBMPFCCUE_MICRO_VERSION]=package_micro_version
AC_SUBST([LIBMPFCCUE_MAJOR_VERSION])
AC_SUBST([LIBMPFCCUE_MINOR_VERSION])
AC_SUBST([LIBMPFCCUE_MICRO_VERSION])

# CURRENT:REVISION:AGE
#
# CURRENT
# The most recent interface number that this library implements.
# REVISION
# The implementation number of the current interface.
# AGE
# The difference between the newest and oldest interfaces that this library
# implements. In other words, the library implements all the interface numbers in
# the range from number current - age to current.

LIBMPFCCUE_LIBRARY_NAME=libmpfccue
AC_SUBST(LIBMPFCCUE_LIBRARY_NAME)

LIBMPFCCUE_LIBTOOL_VERSION=$LIBMPFCCUE_MAJOR_VERSION:$LIBMPFCCUE_MINOR_VERSION:$LIBMPFCCUE_MICRO_VERSION
AC_SUBST(LIBMPFCCUE_LIBTOOL_VERSION)

LIBMPFCCUE_API_VERSION=$LIBMPFCCUE_MAJOR_VERSION.$LIBMPFCCUE_MINOR_VERSION
AC_SUBST(LIBMPFCCUE_API_VERSION)

AC_CHECK_HEADERS([libintl.h, malloc.c, stddef.h])

AC_TYPE_SIZE_T

AC_PROG_CC_C99
AM_PROG_LEX
AC_PROG_YACC
AM_PROG_LIBTOOL

AC_ARG_WITH([parser-buffer],
	    [AS_HELP_STRING([--with-parser-buffer=SIZE],
			    [Size of parser buffer. Default of 1024 should be enough])],
	    [parser_buffer="$withval"],
	    [parser_buffer=1024])

AC_DEFINE_UNQUOTED([PARSER_BUFFER], [$parser_buffer], [Size of parser buffer])

AC_OUTPUT([
Makefile
src/libcue/Makefile
])
