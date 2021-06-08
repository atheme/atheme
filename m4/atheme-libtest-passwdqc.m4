# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Aaron Jones <me@aaronmdjones.net>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_PASSWDQC], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBPASSWDQC="No"

    LIBPASSWDQC_CFLAGS=""
    LIBPASSWDQC_LIBS=""

    AC_ARG_WITH([passwdqc],
        [AS_HELP_STRING([--without-passwdqc], [Do not attempt to detect libpasswdqc (for modules/nickserv/pwquality -- checking password strength)])],
        [], [with_passwdqc="auto"])

    AS_CASE(["x${with_passwdqc}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --with-passwdqc])
    ])

    AS_IF([test "${with_passwdqc}" != "no"], [
        # If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
        AC_SEARCH_LIBS([passwdqc_check], [passwdqc], [
            AC_MSG_CHECKING([if libpasswdqc appears to be usable])
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #include <passwdqc.h>
                ]], [[
                    passwdqc_params_qc_t qc_config = {
                        .min                    = { 0, 0, 0, 0, 0 },
                        .max                    = 0,
                        .passphrase_words       = 0,
                        .match_length           = 0,
                    };
                    (void) passwdqc_check(NULL, NULL, NULL, NULL);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBPASSWDQC="Yes"
                AC_DEFINE([HAVE_LIBPASSWDQC], [1], [Define to 1 if libpasswdqc appears to be usable])
                AC_DEFINE([HAVE_ANY_PASSWORD_QUALITY_LIBRARY], [1], [Define to 1 if any password quality library appears to be usable])
                AS_IF([test "x${ac_cv_search_passwdqc_check}" != "xnone required"], [
                    LIBPASSWDQC_LIBS="${ac_cv_search_passwdqc_check}"
                ])
            ], [
                AC_MSG_RESULT([no])
                LIBPASSWDQC="No"
                AS_IF([test "${with_passwdqc}" = "yes"], [
                    AC_MSG_FAILURE([--with-passwdqc was given but libpasswdqc appears to be unusable])
                ])
            ])
        ], [
            LIBPASSWDQC="No"
            AS_IF([test "${with_passwdqc}" = "yes"], [
                AC_MSG_ERROR([--with-passwdqc was given but libpasswdqc could not be found])
            ])
        ])
    ], [
        LIBPASSWDQC="No"
    ])

    AS_IF([test "${LIBPASSWDQC}" = "No"], [
        LIBPASSWDQC_CFLAGS=""
        LIBPASSWDQC_LIBS=""
    ])

    AC_SUBST([LIBPASSWDQC_CFLAGS])
    AC_SUBST([LIBPASSWDQC_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    unset CFLAGS_SAVED
    unset LIBS_SAVED
])
