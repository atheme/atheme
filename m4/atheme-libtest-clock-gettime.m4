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
            LIBS="${CLOCK_GETTIME_LIBS} ${LIBS}"
        ])

        AC_MSG_CHECKING([if clock_gettime(3) appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_TIME_H
                #  include <time.h>
                #endif
            ]], [[
                struct timespec begin;
                (void) begin.tv_sec;
                (void) begin.tv_nsec;
                (void) clock_gettime(CLOCK_MONOTONIC, &begin);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            HAVE_CLOCK_GETTIME="Yes"
            ATHEME_COND_CRYPTO_BENCHMARK_ENABLE
        ], [
            AC_MSG_RESULT([no])
            HAVE_CLOCK_GETTIME="No"
            CLOCK_GETTIME_LIBS=""
        ])
    ], [], [])

    AC_SUBST([CLOCK_GETTIME_LIBS])

    LIBS="${LIBS_SAVED}"
])
