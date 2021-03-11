# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_ECDH_X25519_TOOL], [

    ECDH_X25519_TOOL="No"

    AC_ARG_ENABLE([ecdh-x25519-tool],
        [AS_HELP_STRING([--disable-ecdh-x25519-tool], [Don't build the SASL ECDH-X25519-CHALLENGE utility])],
        [], [enable_ecdh_x25519_tool="auto"])

    AS_CASE(["x${enable_ecdh_x25519_tool}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --enable-ecdh-x25519-tool])
    ])

    AS_IF([test "${enable_ecdh_x25519_tool}" != "no"], [
        AS_IF([test "x${FEATURE_SASL_ECDH_X25519_CHALLENGE}" = "xYes"], [
            ECDH_X25519_TOOL="Yes"
            ATHEME_COND_ECDH_X25519_TOOL_ENABLE
        ], [
            AS_IF([test "${enable_ecdh_x25519_tool}" = "yes"], [
                AC_MSG_ERROR([--enable-ecdh-x25519-tool requires a crypto library capable of X25519 ECDH])
            ])
        ])
    ])
])
