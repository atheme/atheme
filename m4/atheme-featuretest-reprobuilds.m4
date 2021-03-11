# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_REPROBUILDS], [

    REPRODUCIBLE_BUILDS="No"

    AC_ARG_ENABLE([reproducible-builds],
        [AS_HELP_STRING([--enable-reproducible-builds], [Enable reproducible builds])],
        [], [enable_reproducible_builds="no"])

    AS_CASE(["x${enable_reproducible_builds}"], [xno], [], [xyes], [
        REPRODUCIBLE_BUILDS="Yes"
        AC_DEFINE([ATHEME_ENABLE_REPRODUCIBLE_BUILDS], [1], [Define to 1 if --enable-reproducible-builds was given to ./configure])
        AS_IF([test "x${SOURCE_DATE_EPOCH}" != "x"], [
            AC_DEFINE_UNQUOTED([SOURCE_DATE_EPOCH], [${SOURCE_DATE_EPOCH}], [Reproducible build timestamp])
        ])
    ], [
        AC_MSG_ERROR([invalid option for --enable-reproducible-builds])
    ])
])
