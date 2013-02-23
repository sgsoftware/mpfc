AC_INIT([mpfc], [2.0-hg])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE
AC_PROG_CC_C99
AC_C_BIGENDIAN
AC_PROG_LIBTOOL

# For libcue
AM_PROG_LEX
AC_PROG_YACC

m4_include([m4/pkg.m4])
PKG_PROG_PKG_CONFIG

AC_CONFIG_HEADERS(mpfc-config.h)
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION(0.16.1)

# Set up common libraries
#COMMON_LIBS="-ldmalloc"
COMMON_LIBS=""
AC_SUBST(COMMON_LIBS)

# Check for ncurses
LIBS_save=$LIBS
AC_CHECK_HEADERS([ncursesw/curses.h],,[AC_MSG_ERROR(*** Can't find curses.h ***)])
AC_CHECK_LIB(ncursesw, waddch,,[AC_MSG_ERROR(*** Can't find ncurses library ***)])
CURSES_LIBS="-lncursesw"
AC_SUBST(CURSES_LIBS)
LIBS=$LIBS_save

# Check for pthread
LIBS_save=$LIBS
AC_CHECK_HEADERS([pthread.h],,[AC_MSG_ERROR(*** Can't find pthread.h ***)])
AC_CHECK_LIB(pthread, pthread_create,,[AC_MSG_ERROR(*** Can't find posix threads library ***)])
PTHREAD_LIBS="-lpthread"
AC_SUBST(PTHREAD_LIBS)
LIBS=$LIBS_save

# Check for libdl
LIBS_save=$LIBS
AC_CHECK_HEADERS([dlfcn.h],,[AC_MSG_ERROR(*** Can't find dlfcn.h ***)])
AC_CHECK_LIB(dl, dlopen,,[AC_MSG_ERROR(*** Can't find libdl ***)])
DL_LIBS="-ldl"
AC_SUBST(DL_LIBS)
LIBS=$LIBS_save

# Check for gpm library
AC_ARG_ENABLE(gpm, 
[	--disable-gpm		Disable gpm support [default=enabled]],,
		enable_gpm="yes")
if test "x$enable_gpm" = xyes; then
	LIBS_save=$LIBS
	AC_CHECK_HEADERS([gpm.h],,[AC_MSG_ERROR(*** Can't find gpm.h ***)])
	AC_CHECK_LIB(gpm, Gpm_Open,,[AC_MSG_ERROR(*** Can't find gpm library ***)])
	GPM_LIBS="-lgpm"
	AC_SUBST(GPM_LIBS)
	LIBS=$LIBS_save
else
	AC_MSG_RESULT([*** gpm support disabled per user request ***])
fi

# Check for soundcard header 
AC_CHECK_HEADERS([sys/soundcard.h],, [AC_MSG_ERROR(*** Can't find sys/soundcard.h ***)])

# Check for resolver library
LIBS_save=$LIBS
AC_CHECK_HEADERS([netdb.h],,[AC_MSG_ERROR(*** Can't find libresolv header ***)])
AC_CHECK_LIB(resolv, gethostbyname,,[AC_MSG_ERROR(*** Can't find libresolv ***)])
RESOLV_LIBS="-lresolv"
AC_SUBST(RESOLV_LIBS)
LIBS=$LIBS_save

# Check for ICU library
LIBS_save=$LIBS
AC_CHECK_HEADERS([unicode/ucnv.h],,[AC_MSG_ERROR(*** Can't find libicu header ***)])
#AC_CHECK_LIB(icuuc, ucnv_open,,[AC_MSG_ERROR(*** Can't find libicu ***)])
ICU_LIBS="-licuuc"
AC_SUBST(ICU_LIBS)
LIBS=$LIBS_save

# Check for json
PKG_CHECK_MODULES(JSON, json-glib-1.0,, [AC_MSG_ERROR(*** json-glib 1.0 not found ***)])
AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)

# Check for math lib
LIBS_save=$LIBS
AC_CHECK_HEADERS([math.h],,[AC_MSG_ERROR(*** Can't find math header ***)])
AC_CHECK_LIB(m, pow,,[AC_MSG_ERROR(*** Can't find libm ***)])
MATH_LIBS="-lm"
AC_SUBST(MATH_LIBS)
LIBS=$LIBS_save

# Stuff for cue plugin
AC_ARG_ENABLE(cue, 
[	--disable-cue		Disable cue playlist plugin [default=enabled]],,
		enable_cue="yes")
if test "x$enable_cue" = xyes; then
	LIBS_save=$LIBS
	LIBS=$LIBS_save
else
	AC_MSG_RESULT([*** cue plugin disabled per user request ***])
fi

# Stuff for m3u plugin
AC_ARG_ENABLE(m3u, 
[	--disable-m3u		Disable m3u playlist plugin [default=enabled]],,
		enable_m3u="yes")
if test "x$enable_m3u" = xyes; then
	LIBS_save=$LIBS
	LIBS=$LIBS_save
else
	AC_MSG_RESULT([*** m3u plugin disabled per user request ***])
fi

# Stuff for pls plugin
AC_ARG_ENABLE(pls, 
[	--disable-pls		Disable pls playlist plugin [default=enabled]],,
		enable_pls="yes")
if test "x$enable_pls" = xyes; then
	LIBS_save=$LIBS
	LIBS=$LIBS_save
else
	AC_MSG_RESULT([*** pls plugin disabled per user request ***])
fi

# Stuff for audiocd plugin
AC_ARG_ENABLE(audiocd, 
[	--disable-audiocd		Disable audiocd playlist plugin [default=enabled]],,
		enable_audiocd="yes")
if test "x$enable_audiocd" = xyes; then
	LIBS_save=$LIBS
	LIBS=$LIBS_save
else
	AC_MSG_RESULT([*** audiocd plugin disabled per user request ***])
fi

# Choose general plugins
ALL_GENERAL_PLUGINS=""
for plugin in $ALL_GENERAL_PLUGINS; do
	name="\$enable_general_$plugin"
	value=`eval echo $name`
	SUMMARY_MSG="No"
	if test "x${value}" = xyes; then
		GENERAL_PLUGINS="$GENERAL_PLUGINS $plugin"
		SUMMARY_MSG="Yes"
	fi
	GENERAL_SUMMARY="$GENERAL_SUMMARY\n$plugin: $SUMMARY_MSG"
done

# Choose playlist plugins
ALL_PLIST_PLUGINS="m3u pls cue audiocd"
for plugin in $ALL_PLIST_PLUGINS; do
	name="\$enable_$plugin"
	value=`eval echo $name`
	SUMMARY_MSG="No"
	if test "x${value}" = xyes; then
		PLIST_PLUGINS="$PLIST_PLUGINS $plugin"
		SUMMARY_MSG="Yes"
	fi
	PLIST_SUMMARY="$PLIST_SUMMARY\n$plugin: $SUMMARY_MSG"
done

AC_SUBST(GENERAL_PLUGINS)
AC_SUBST(PLIST_PLUGINS)

PKG_CHECK_MODULES(GSTREAMER, gstreamer-1.0,, [AC_MSG_ERROR(*** GStreamer 1.0 not found ***)])
AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

PKG_CHECK_MODULES(GSTREAMER_AUDIO, gstreamer-audio-1.0,, [AC_MSG_ERROR(*** GStreamer-audio 1.0 not found ***)])
AC_SUBST(GSTREAMER_AUDIO_CFLAGS)
AC_SUBST(GSTREAMER_AUDIO_LIBS)

PKG_CHECK_MODULES(TAGLIB, taglib_c,, [AC_MSG_ERROR(*** Taglib not found ***)])
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

AC_OUTPUT(Makefile \
		  doc/Makefile \
		  intl/Makefile  \
		  libmpfc/Makefile \
		  libmpfcwnd/Makefile \
		  libcue/Makefile \
		  libcue/src/libcue/Makefile \
		  po/Makefile.in  \
		  src/Makefile \
		  plugins/Makefile \
		  plugins/general/Makefile \
		  plugins/plist/Makefile \
		  plugins/plist/m3u/Makefile \
		  plugins/plist/pls/Makefile \
		  plugins/plist/cue/Makefile \
		  plugins/plist/audiocd/Makefile \
		  po/Makefile \
		  mpfc.pc)

# Print plugins summary
echo -e "\n****** Plugins summary ******\n"
echo -e "General plugins:$GENERAL_SUMMARY\n"
echo -e "Playlist plugins:$PLIST_SUMMARY\n"
