# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_NLS], [

    # This must be lowercase; gettext.m4 requires it so
    USE_NLS="no"

    AC_ARG_ENABLE([nls],
        [AS_HELP_STRING([--enable-nls], [Enable localization/translation support])],
        [], [enable_nls="no"])

    AS_CASE(["x${enable_nls}"], [xno], [], [xyes], [
        AM_PO_SUBDIRS
        AM_GNU_GETTEXT([external], [need-formatstring-macros])
        AS_IF([test "x${USE_NLS}" = "xyes"], [
            ATHEME_COND_NLS_ENABLE
            AS_IF([test "x${LIBINTL}" != "x"], [
                LIBS="${LIBINTL} ${LIBS}"
            ])
        ], [
            AC_MSG_WARN([NLS was requested but is unavailable])
        ])
    ], [
        AC_MSG_ERROR([invalid option for --enable-nls])
    ])

    AC_SUBST([USE_NLS])
])
