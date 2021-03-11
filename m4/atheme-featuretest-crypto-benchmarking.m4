# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_CRYPTO_BENCHMARKING], [

    LIBS_SAVED="${LIBS}"

    CLOCK_GETTIME_LIBS=""
    CRYPTO_BENCHMARKING="No"

    AC_ARG_ENABLE([crypto-benchmarking],
        [AS_HELP_STRING([--disable-crypto-benchmarking], [Don't build the crypto benchmarking utility])],
        [], [enable_crypto_benchmarking="auto"])

    AS_CASE(["x${enable_crypto_benchmarking}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --enable-crypto-benchmarking])
    ])

    AS_IF([test "${enable_crypto_benchmarking}" != "no"], [
        AC_SEARCH_LIBS([clock_gettime], [rt], [
            AS_IF([test "x${ac_cv_search_clock_gettime}" != "xnone required"], [
                CLOCK_GETTIME_LIBS="${ac_cv_search_clock_gettime}"
                LIBS="${CLOCK_GETTIME_LIBS} ${LIBS}"
            ])

            AC_MSG_CHECKING([if clock_gettime(2) appears to be usable])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDLIB_H
                    #  include <stdlib.h>
                    #endif
                    #ifdef HAVE_SYS_TIME_H
                    #  include <sys/time.h>
                    #endif
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
                CRYPTO_BENCHMARKING="Yes"
                ATHEME_COND_CRYPTO_BENCHMARK_ENABLE
            ], [
                AC_MSG_RESULT([no])
                CLOCK_GETTIME_LIBS=""
                AS_IF([test "${enable_crypto_benchmarking}" = "yes"], [
                    AC_MSG_FAILURE([--enable-crypto-benchmarking was given but clock_gettime(2) does not appear to be usable])
                ], [
                    AC_MSG_WARN([the crypto benchmarking utility will not be available])
                ])
            ])
        ], [
            CLOCK_GETTIME_LIBS=""
            AS_IF([test "${enable_crypto_benchmarking}" = "yes"], [
                AC_MSG_FAILURE([--enable-crypto-benchmarking was given but clock_gettime(2) is not available])
            ], [
                AC_MSG_WARN([the crypto benchmarking utility will not be available])
            ])
        ], [])
    ])

    AC_SUBST([CLOCK_GETTIME_LIBS])

    LIBS="${LIBS_SAVED}"

    unset LIBS_SAVED
])
