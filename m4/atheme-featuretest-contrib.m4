# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_CONTRIB], [

    CONTRIB_MODULES="No"

    AC_ARG_ENABLE([contrib],
        [AS_HELP_STRING([--enable-contrib], [Enable contrib modules])],
        [], [enable_contrib="no"])

    AS_CASE(["x${enable_contrib}"], [xno], [], [xyes], [
        CONTRIB_MODULES="Yes"
        AC_DEFINE([ATHEME_ENABLE_CONTRIB], [1], [Define to 1 if --enable-contrib was given to ./configure])
    ], [
        AC_MSG_ERROR([invalid option for --enable-contrib])
    ])
])
