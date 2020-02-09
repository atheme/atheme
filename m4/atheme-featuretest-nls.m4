# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_NLS], [

    USE_NLS="no"

    AC_ARG_ENABLE([nls],
        [AS_HELP_STRING([--enable-nls], [Enable localization/translation support])],
        [], [enable_nls="no"])

    case "x${enable_nls}" in
        xyes | xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-nls])
            ;;
    esac

    AS_IF([test "${enable_nls}" = "yes"], [
        USE_NLS="yes"
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
    ])

    AC_SUBST([USE_NLS])
])
