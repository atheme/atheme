# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_CPPFLAGS], [

    AC_ARG_ENABLE([fortify-source],
        [AS_HELP_STRING([--disable-fortify-source], [Disable -D_FORTIFY_SOURCE=2 (hardening for many C library function invocations)])],
        [], [enable_fortify_source="yes"])

    AS_CASE(["x${enable_fortify_source}"], [xno], [], [xyes], [
        ATHEME_CPP_TEST_FLAGS([-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2])
    ], [
        AC_MSG_ERROR([invalid option for --enable-fortify-source])
    ])
])
