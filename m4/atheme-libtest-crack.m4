# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_CRACK], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBCRACK="No"

    LIBCRACK_CFLAGS=""
    LIBCRACK_LIBS=""

    AC_ARG_WITH([cracklib],
        [AS_HELP_STRING([--without-cracklib], [Do not attempt to detect cracklib (for modules/nickserv/pwquality -- checking password strength)])],
        [], [with_cracklib="auto"])

    case "x${with_cracklib}" in
        xno | xyes | xauto)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-cracklib])
            ;;
    esac

    AS_IF([test "${with_cracklib}" != "no"], [
        # If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
        AC_SEARCH_LIBS([FascistCheck], [crack], [
            AC_MSG_CHECKING([if cracklib appears to be usable])
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #include <crack.h>
                ]], [[
                    (void) FascistCheck(NULL, NULL);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBCRACK="Yes"
                AC_DEFINE([HAVE_CRACKLIB], [1], [Define to 1 if cracklib appears to be usable])
                AC_DEFINE([HAVE_ANY_PASSWORD_QUALITY_LIBRARY], [1], [Define to 1 if any password quality library appears to be usable])
                AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"], [
                    LIBCRACK_LIBS="${ac_cv_search_FascistCheck}"
                ])
            ], [
                AC_MSG_RESULT([no])
                LIBCRACK="No"
                AS_IF([test "${with_cracklib}" = "yes"], [
                    AC_MSG_FAILURE([--with-cracklib was given but cracklib appears to be unusable])
                ])
            ])
        ], [
            LIBCRACK="No"
            AS_IF([test "${with_cracklib}" = "yes"], [
                AC_MSG_ERROR([--with-cracklib was given but cracklib could not be found])
            ])
        ])
    ], [
        LIBCRACK="No"
    ])

    AS_IF([test "${LIBCRACK}" = "No"], [
        LIBCRACK_CFLAGS=""
        LIBCRACK_LIBS=""
    ])

    AC_SUBST([LIBCRACK_CFLAGS])
    AC_SUBST([LIBCRACK_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
