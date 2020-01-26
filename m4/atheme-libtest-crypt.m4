# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_CRYPT], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBCRYPT="No"

    LIBCRYPT_CFLAGS=""
    LIBCRYPT_LIBS=""

    AC_ARG_WITH([crypt],
        [AS_HELP_STRING([--without-crypt], [Do not attempt to detect crypt(3) (for modules/crypto/crypt3-*)])],
        [], [with_crypt="auto"])

    case "x${with_crypt}" in
        xno | xyes | xauto)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-crypt])
            ;;
    esac

    AS_IF([test "${with_crypt}" != "no"], [
        AC_SEARCH_LIBS([crypt], [crypt], [
            AC_CHECK_HEADERS([crypt.h], [], [], [])
            AC_MSG_CHECKING([if crypt(3) appears to be usable])
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #ifdef HAVE_UNISTD_H
                    #  include <unistd.h>
                    #endif
                    #ifdef HAVE_CRYPT_H
                    #  include <crypt.h>
                    #endif
                ]], [[
                    (void) crypt(NULL, NULL);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBCRYPT="Yes"
                AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) appears to be usable])
                AS_IF([test "x${ac_cv_search_crypt}" != "xnone required"], [
                    LIBCRYPT_LIBS="${ac_cv_search_crypt}"
                ])
            ], [
                AC_MSG_RESULT([no])
                LIBCRYPT="No"
                AS_IF([test "${with_crypt}" = "yes"], [
                    AC_MSG_FAILURE([--with-crypt was given but crypt(3) appears to be unusable])
                ])
            ])
        ], [
            LIBCRYPT="No"
            AS_IF([test "${with_crypt}" = "yes"], [
                AC_MSG_ERROR([--with-crypt was given but crypt(3) could not be found])
            ])
        ])
    ], [
        LIBCRYPT="No"
    ])

    AS_IF([test "${LIBCRYPT}" = "No"], [
        LIBCRYPT_CFLAGS=""
        LIBCRYPT_LIBS=""
    ])

    AC_SUBST([LIBCRYPT_CFLAGS])
    AC_SUBST([LIBCRYPT_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
