# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_CLOCK_GETTIME], [

    LIBS_SAVED="${LIBS}"

    HAVE_CLOCK_GETTIME="No"
    CLOCK_GETTIME_LIBS=""

    AC_SEARCH_LIBS([clock_gettime], [rt], [
        AS_IF([test "x${ac_cv_search_clock_gettime}" != "xnone required"], [
            CLOCK_GETTIME_LIBS="${ac_cv_search_clock_gettime}"
        ])

        HAVE_CLOCK_GETTIME="Yes"
        ATHEME_COND_CRYPTO_BENCHMARK_ENABLE
    ], [], [])

    AC_SUBST([CLOCK_GETTIME_LIBS])

    LIBS="${LIBS_SAVED}"
])
