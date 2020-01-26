# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_MATH], [

    LIBS_SAVED="${LIBS}"

    LIBMATH_LIBS=""

    AC_CHECK_HEADERS([math.h], [], [], [])

    AC_SEARCH_LIBS([floor], [m], [
        AS_IF([test "x${ac_cv_search_floor}" != "xnone required"], [
            LIBMATH_LIBS="${LIBMATH_LIBS:-} ${ac_cv_search_floor}"
        ])
    ], [
        AC_MSG_ERROR([math functions appear to be unusable])
    ])

    LIBS="${LIBS_SAVED}"

    AC_SEARCH_LIBS([pow], [m], [
        AS_IF([test "x${ac_cv_search_pow}" != "xnone required"], [
            LIBMATH_LIBS="${LIBMATH_LIBS:-} ${ac_cv_search_pow}"
        ])
    ], [
        AC_MSG_ERROR([math functions appear to be unusable])
    ])

    LIBS="${LIBMATH_LIBS} ${LIBS_SAVED}"

    AC_MSG_CHECKING([if math functions appear to be usable])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_MATH_H
            #  include <math.h>
            #endif
        ]], [[
            const double tmp = 16.125;
            const double retf = floor(tmp);
            const double retp = pow(tmp, tmp);
        ]])
    ], [
        AC_MSG_RESULT([yes])
    ], [
        AC_MSG_RESULT([no])
        AC_MSG_FAILURE([math functions appear to be unusable])
    ])

    AC_SUBST([LIBMATH_LIBS])

    LIBS="${LIBS_SAVED}"
])
