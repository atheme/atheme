# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_ECDSA_NIST256P_TOOLS], [

    ECDSA_NIST256P_TOOLS="No"

    AC_ARG_ENABLE([ecdsa-nist256p-tools],
        [AS_HELP_STRING([--disable-ecdsa-nist256p-tools], [Don't build the SASL ECDSA-NIST256P-CHALLENGE utilities])],
        [], [enable_ecdsa_nist256p_tools="auto"])

    AS_CASE(["x${enable_ecdsa_nist256p_tools}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --enable-ecdsa-nist256p-tools])
    ])

    AS_IF([test "${enable_ecdsa_nist256p_tools}" != "no"], [
        AS_IF([test "x${FEATURE_SASL_ECDSA_NIST256P_CHALLENGE}" = "xYes"], [
            ECDSA_NIST256P_TOOLS="Yes"
            ATHEME_COND_ECDSA_NIST256P_TOOLS_ENABLE
        ], [
            AS_IF([test "${enable_ecdsa_nist256p_tools}" = "yes"], [
                AC_MSG_ERROR([--enable-ecdsa-nist256p-tools requires a crypto library capable of ECDSA with NIST P-256])
            ])
        ])
    ])
])
