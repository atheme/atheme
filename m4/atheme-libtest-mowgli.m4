# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_MOWGLI], [

    MOWGLI_SOURCE=""

    AC_ARG_WITH([libmowgli],
        [AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@], [Specify location of system libmowgli install, "yes" to ask pkg-config, or "no" to force use of internal libmowgli submodule (default)])],
        [], [with_libmowgli="no"])

    case "x${with_libmowgli}" in
        xno | xyes)
            ;;
        x/*)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-libmowgli])
            ;;
    esac

    AS_IF([test "x${with_libmowgli}" = "xyes"], [
        AS_IF([test -n "${PKG_CONFIG}"], [
            PKG_CHECK_MODULES([MOWGLI], [libmowgli-2 >= 2.0.0], [], [with_libmowgli="no"])
        ], [
            with_libmowgli="no"
        ])
    ])

    AS_IF([test "x${with_libmowgli}" = "xyes"], [
        MOWGLI_SOURCE="System"
        CFLAGS="${CFLAGS} ${MOWGLI_CFLAGS}"
        LIBS="${LIBS} ${MOWGLI_LIBS}"
    ], [test "x${with_libmowgli}" = "xno"], [
        MOWGLI_SOURCE="Internal"
        CPPFLAGS="${CPPFLAGS} -I$(pwd)/libmowgli-2/src/libmowgli"
        LDFLAGS="${LDFLAGS} -L$(pwd)/libmowgli-2/src/libmowgli"
        LIBS="${LIBS} -lmowgli-2"
        ATHEME_COND_LIBMOWGLI_SUBMODULE_ENABLE
    ], [test -d "${with_libmowgli}/include/libmowgli-2" -a -d "${with_libmowgli}/lib"], [
        MOWGLI_SOURCE="System"
        CPPFLAGS="${CPPFLAGS} -I${with_libmowgli}/include/libmowgli-2"
        LDFLAGS="${LDFLAGS} -L${with_libmowgli}/lib"
        LIBS="${LIBS} -lmowgli-2"
    ], [
        AC_MSG_ERROR([${with_libmowgli} is not a suitable directory for libmowgli])
    ])
])
