#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libmowgli], [0.7.1], [bugs+libmowgli@atheme.org])
AC_CONFIG_SRCDIR([src/libmowgli/mowgli_alloc.c])
AC_CONFIG_HEADER([src/libmowgli/mowgli_config.h])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_ISC_POSIX

test x"$GCC" = x"yes" && CFLAGS="$CFLAGS -pipe -Wall"

# XXX workaround
PACKAGE="libmowgli"
AC_SUBST(PACKAGE)

# Checks for libraries.
BUILDSYS_SHARED_LIB
LIBS="$LIBS $DYNAMIC_LD_LIBS"

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h locale.h stdarg.h sys/types.h sys/stat.h errno.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_CHECK_FUNCS([memset setlocale strcasecmp strchr strdup strerror strtol strtod])
AC_CHECK_FUNCS([printf sprintf snprintf vsnprintf mmap gettimeofday strndup strlcpy strlcat epoll_ctl port_create])
AC_FUNC_STAT

AC_CHECK_LIB(dl, dlopen, [LIBS="$LIBS -ldl"])

dnl Detect which loader to use
AC_MSG_CHECKING(loader type)
case "$target" in
*-*-mingw32)
    MOWGLI_MODULE='mowgli_module_win32.c'
	;;
*)
    MOWGLI_MODULE='mowgli_module_posix.c'
	;;
esac
AC_MSG_RESULT($MOWGLI_MODULE);
AC_SUBST(MOWGLI_MODULE)

# Check for optional features.
EXAMPLES_BUILD=""
AC_ARG_ENABLE(examples,
    [  --enable-examples        build and install example programs],
    [enable_examples=$enableval],
    [enable_examples="no"]
)

if test "x$enable_examples" = "xyes"; then
	EXAMPLES_BUILD="examples"
fi

AC_SUBST(EXAMPLES_BUILD)

BUILDSYS_TOUCH_DEPS

AC_CONFIG_FILES([buildsys.mk extra.mk libmowgli.pc])
AC_OUTPUT

cat << _EOF_

Configuration:
	No special features yet.

Now type "make" to build, and "make install" to install.
Thank you for using libmowgli.

_EOF_
