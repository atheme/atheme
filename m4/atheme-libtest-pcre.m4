# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_PCRE], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBPCRE="No"
    LIBPCRE_PATH=""

    AC_ARG_WITH([pcre],
        [AS_HELP_STRING([--without-pcre], [Do not attempt to detect libpcre (Perl-Compatible Regular Expressions)])],
        [], [with_pcre="auto"])

    case "x${with_pcre}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBPCRE_PATH="${with_pcre}"
            with_pcre="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-pcre])
            ;;
    esac

    AS_IF([test "${with_pcre}" != "no"], [
        AS_IF([test -n "${LIBPCRE_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBPCRE_PATH}/include" -a -d "${LIBPCRE_PATH}/lib"], [
                LIBPCRE_CFLAGS="-I${LIBPCRE_PATH}/include"
                LIBPCRE_LIBS="-L${LIBPCRE_PATH}/lib -lpcre"
            ], [
                AC_MSG_ERROR([${LIBPCRE_PATH} is not a suitable directory for libpcre])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBPCRE], [libpcre], [], [])
        ])
        AS_IF([test -n "${LIBPCRE_CFLAGS+set}" -a -n "${LIBPCRE_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBPCRE="Yes"
        ], [
            LIBPCRE="No"
            AS_IF([test "${with_pcre}" != "no" && test "${with_pcre}" != "auto"], [
                AC_MSG_FAILURE([--with-pcre was given but libpcre could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBPCRE}" = "Yes"], [
        CFLAGS="${LIBPCRE_CFLAGS} ${CFLAGS}"
        LIBS="${LIBPCRE_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if libpcre appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <pcre.h>
            ]], [[
                (void) pcre_compile(NULL, 0, NULL, NULL, NULL);
                (void) pcre_exec(NULL, NULL, NULL, 0, 0, 0, NULL, 0);
                (void) pcre_free(NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBPCRE="Yes"
            AC_DEFINE([HAVE_LIBPCRE], [1], [Define to 1 if libpcre appears to be usable])
        ], [
            AC_MSG_RESULT([no])
            LIBPCRE="No"
            AS_IF([test "${with_pcre}" != "no" && test "${with_pcre}" != "auto"], [
                AC_MSG_FAILURE([--with-pcre was given but libpcre does not appear to be usable])
            ])
        ])
    ])

    AS_IF([test "${LIBPCRE}" = "No"], [
        LIBPCRE_CFLAGS=""
        LIBPCRE_LIBS=""
    ])

    AC_SUBST([LIBPCRE_CFLAGS])
    AC_SUBST([LIBPCRE_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
