# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_MOWGLI], [

    LIBMOWGLI_SOURCE=""
    LIBMOWGLI_CFLAGS=""
    LIBMOWGLI_LIBS=""

    AC_ARG_WITH([libmowgli],
        [AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@], [Specify location of system libmowgli install, "yes" to ask pkg-config, or "no" to force use of internal libmowgli submodule (default)])],
        [], [with_libmowgli="no"])

    AS_CASE(["x${with_libmowgli}"], [xno], [
        LIBMOWGLI_SOURCE="Internal"
        LIBMOWGLI_CFLAGS="-I$(pwd)/libmowgli-2/src/libmowgli"
        LIBMOWGLI_LIBS="-L$(pwd)/libmowgli-2/src/libmowgli -lmowgli-2"
    ], [xyes], [
        AS_IF([test -z "${PKG_CONFIG}"], [
            AC_MSG_ERROR([--with-libmowgli=yes was given, but pkg-config is required and is not available])
        ])
        PKG_CHECK_MODULES([LIBMOWGLI], [libmowgli-2 >= 2.0.0], [], [
            AC_MSG_ERROR([--with-libmowgli=yes was given, but pkg-config could not locate libmowgli])
        ])
        LIBMOWGLI_SOURCE="System (pkg-config)"
    ], [x/*], [
        AS_IF([test -d "${with_libmowgli}/include/libmowgli-2" -a -d "${with_libmowgli}/lib"], [], [
            AC_MSG_ERROR([${with_libmowgli} is not a suitable directory for libmowgli])
        ])
        LIBMOWGLI_SOURCE="System (${with_libmowgli})"
        LIBMOWGLI_CFLAGS="-I${with_libmowgli}/include/libmowgli-2"
        LIBMOWGLI_LIBS="-L${with_libmowgli}/lib -lmowgli-2"
    ], [
        AC_MSG_ERROR([invalid option for --with-libmowgli])
    ])

    AC_SUBST([LIBMOWGLI_CFLAGS])
    AC_SUBST([LIBMOWGLI_LIBS])
])
