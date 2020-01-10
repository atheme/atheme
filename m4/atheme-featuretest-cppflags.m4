# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

ATHEME_CPP_TEST_CPPFLAGS_RESULT="no"

AC_DEFUN([ATHEME_CPP_TEST_CPPFLAGS],
    [
        AC_MSG_CHECKING([for C preprocessor flag(s) $1 ])

        CPPFLAGS_SAVED="${CPPFLAGS}"
        CPPFLAGS="${CPPFLAGS} $1"

        AC_COMPILE_IFELSE(
            [
                AC_LANG_PROGRAM([[]], [[]])
            ], [
                ATHEME_CPP_TEST_CPPFLAGS_RESULT='yes'

                AC_MSG_RESULT([yes])
            ], [
                ATHEME_CPP_TEST_CPPFLAGS_RESULT='no'
                CPPFLAGS="${CPPFLAGS_SAVED}"

                AC_MSG_RESULT([no])
            ], [
                ATHEME_CPP_TEST_CPPFLAGS_RESULT='no'
                CPPFLAGS="${CPPFLAGS_SAVED}"

                AC_MSG_RESULT([skipped as we are cross-compiling])
            ]
        )

        unset CPPFLAGS_SAVED
    ]
)

AC_DEFUN([ATHEME_FEATURETEST_CPPFLAGS], [

    AC_ARG_ENABLE([fortify-source],
        [AS_HELP_STRING([--disable-fortify-source], [Disable -D_FORTIFY_SOURCE=2 (hardening for many C library function invocations)])],
        [], [enable_fortify_source="yes"])

    case "x${enable_fortify_source}" in
        xyes)
            ATHEME_CPP_TEST_CPPFLAGS([-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2])
            ;;
        xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-fortify-source])
            ;;
    esac
])
