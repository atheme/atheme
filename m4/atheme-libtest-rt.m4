# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_RT], [

    LIBS_SAVED="${LIBS}"

    LIBRT="No"
    LIBRT_LIBS=""

    AC_SEARCH_LIBS([clock_gettime], [rt], [
        AS_IF([test "x${ac_cv_search_clock_gettime}" != "xnone required"], [
            LIBRT_LIBS="${ac_cv_search_clock_gettime}"
        ])

        LIBRT="Yes"
        ATHEME_COND_CRYPTO_BENCHMARK_ENABLE
    ], [], [])

    AC_SUBST([LIBRT_LIBS])

    LIBS="${LIBS_SAVED}"
])
