AC_PREREQ(2.61)

AC_INIT([whistler], [0.0.1], [slomo@uni-paderborn.de])
AC_CONFIG_SRCDIR([whs/Makefile.am])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE([foreign 1.9 -Wall dist-bzip2])

AM_MAINTAINER_MODE

AM_DISABLE_STATIC
AC_DISABLE_STATIC
AC_GNU_SOURCE
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL
AC_PROG_LIBTOOL

AC_C_CONST
AC_C_INLINE
AC_ISC_POSIX
AC_HEADER_STDC
AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG

AC_CHECK_LIBM
AC_SUBST(LIBM)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16.0 gobject-2.0 >= 2.16.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 gstreamer-base-0.10 gstreamer-audio-0.10)
GSTREAMER_PLUGINS_DIR="`pkg-config --variable=pluginsdir gstreamer-0.10`"
AC_SUBST(GSTREAMER_LIBS)
AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_PLUGINS_DIR)

GLIB_CFLAGS_EXTRA="-DG_LOG_DOMAIN=\\\"whistler\\\""

AC_DEFINE(WHS_MAJOR, 0, [Major version number of libwhistler])
AC_DEFINE(WHS_MINOR, 0, [Minor version number of libwhistler])
AC_DEFINE(WHS_MICRO, 1, [Micro version number of libwhistler])
AC_SUBST(WHS_VERSION_INFO, "0:0:0")

AC_ARG_ENABLE(more-warnings,
AC_HELP_STRING([--enable-more-warnings], [Maximum compiler warnings]),
set_more_warnings="$enableval", set_more_warnings=no)

AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
	AC_MSG_RESULT(yes)

	for option in -Wall -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wcast-align ; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CFLAGS="$SAVE_CFLAGS"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
else
	AC_MSG_RESULT(no)
fi

AC_ARG_ENABLE(warning-errors,
AC_HELP_STRING([--enable-warning-errors], [Make warnings compiler errors]),
set_werror="$enableval", set_werror=no)

AC_MSG_CHECKING(for handling warnings as compiler errors)
if test "$GCC" = "yes" -a "$set_werror" != "no"; then
	AC_MSG_RESULT(yes)

	CFLAGS="$CFLAGS -Werror"
	GLIB_CFLAGS_EXTRA="$GLIB_CFLAGS_EXTRA -DG_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED"
else
	AC_MSG_RESULT(no)
fi

AC_SUBST(GLIB_CFLAGS_EXTRA)
	
AC_CONFIG_FILES([
Makefile
ext/Makefile
ext/gpfft/Makefile
whs/Makefile
programs/Makefile
gst/Makefile
])

AC_OUTPUT

