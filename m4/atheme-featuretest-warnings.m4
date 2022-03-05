# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <me@aaronmdjones.net>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_CC_ENABLE_WARNINGS], [

    ATHEME_TEST_CC_FLAGS([-Werror=unknown-warning-option])

    ATHEME_TEST_CC_FLAGS([-Weverything])

    AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [

        # These two have to be consecutive and first for the most reliable
        # results. Do not alphabetise with the flags below.    -- amdj
        ATHEME_TEST_CC_FLAGS([-Wall])
        ATHEME_TEST_CC_FLAGS([-Wextra])

        ATHEME_TEST_CC_FLAGS([-Waggregate-return])
        ATHEME_TEST_CC_FLAGS([-Waggressive-loop-optimizations])
        ATHEME_TEST_CC_FLAGS([-Walloc-zero])
        ATHEME_TEST_CC_FLAGS([-Walloca])
        ATHEME_TEST_CC_FLAGS([-Warray-bounds])
        ATHEME_TEST_CC_FLAGS([-Wbad-function-cast])
        ATHEME_TEST_CC_FLAGS([-Wc99-c11-compat])
        ATHEME_TEST_CC_FLAGS([-Wcast-qual])
        ATHEME_TEST_CC_FLAGS([-Wdangling-else])
        ATHEME_TEST_CC_FLAGS([-Wdate-time])
        ATHEME_TEST_CC_FLAGS([-Wdisabled-optimization])
        ATHEME_TEST_CC_FLAGS([-Wdouble-promotion])
        ATHEME_TEST_CC_FLAGS([-Wduplicated-branches])
        ATHEME_TEST_CC_FLAGS([-Wduplicated-cond])
        ATHEME_TEST_CC_FLAGS([-Wfatal-errors])
        ATHEME_TEST_CC_FLAGS([-Wfloat-equal])
        ATHEME_TEST_CC_FLAGS([-Wformat-nonliteral])
        ATHEME_TEST_CC_FLAGS([-Wformat-overflow])
        ATHEME_TEST_CC_FLAGS([-Wformat-security])
        ATHEME_TEST_CC_FLAGS([-Wformat-signedness])
        ATHEME_TEST_CC_FLAGS([-Wformat-truncation])
        ATHEME_TEST_CC_FLAGS([-Wformat-y2k])
        ATHEME_TEST_CC_FLAGS([-Winline])
        ATHEME_TEST_CC_FLAGS([-Winit-self])
        ATHEME_TEST_CC_FLAGS([-Winvalid-pch])
        ATHEME_TEST_CC_FLAGS([-Wjump-misses-init])
        ATHEME_TEST_CC_FLAGS([-Wlogical-op])
        ATHEME_TEST_CC_FLAGS([-Wmissing-declarations])
        ATHEME_TEST_CC_FLAGS([-Wmissing-format-attribute])
        ATHEME_TEST_CC_FLAGS([-Wmissing-include-dirs])
        ATHEME_TEST_CC_FLAGS([-Wmissing-prototypes])
        ATHEME_TEST_CC_FLAGS([-Wmissing-variable-declarations])
        ATHEME_TEST_CC_FLAGS([-Wmultistatement-macros])
        ATHEME_TEST_CC_FLAGS([-Wnested-externs])
        ATHEME_TEST_CC_FLAGS([-Wnormalized=nfkc])
        ATHEME_TEST_CC_FLAGS([-Wnull-dereference])
        ATHEME_TEST_CC_FLAGS([-Wold-style-definition])
        ATHEME_TEST_CC_FLAGS([-Woverlength-strings])
        ATHEME_TEST_CC_FLAGS([-Wpointer-arith])
        ATHEME_TEST_CC_FLAGS([-Wpointer-compare])
        ATHEME_TEST_CC_FLAGS([-Wredundant-decls])
        ATHEME_TEST_CC_FLAGS([-Wrestrict])
        ATHEME_TEST_CC_FLAGS([-Wshadow])
        ATHEME_TEST_CC_FLAGS([-Wstack-protector])

        ATHEME_TEST_CC_FLAGS([-Wstrict-overflow=3])
        AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
            ATHEME_TEST_CC_FLAGS([-Wstrict-overflow])
        ])

        ATHEME_TEST_CC_FLAGS([-Wstrict-prototypes])
        ATHEME_TEST_CC_FLAGS([-Wstringop-overflow=4])
        ATHEME_TEST_CC_FLAGS([-Wstringop-truncation])
        ATHEME_TEST_CC_FLAGS([-Wtrampolines])
        ATHEME_TEST_CC_FLAGS([-Wundef])
        ATHEME_TEST_CC_FLAGS([-Wunsafe-loop-optimizations])
        ATHEME_TEST_CC_FLAGS([-Wunsuffixed-float-constants])
        ATHEME_TEST_CC_FLAGS([-Wunused])
        ATHEME_TEST_CC_FLAGS([-Wwrite-strings])
    ])

    ATHEME_TEST_CC_FLAGS([-Wno-conversion])
    ATHEME_TEST_CC_FLAGS([-Wno-declaration-after-statement])
    ATHEME_TEST_CC_FLAGS([-Wno-disabled-macro-expansion])
    ATHEME_TEST_CC_FLAGS([-Wno-documentation-deprecated-sync])
    ATHEME_TEST_CC_FLAGS([-Wno-documentation-unknown-command])
    ATHEME_TEST_CC_FLAGS([-Wno-extra-semi-stmt])
    ATHEME_TEST_CC_FLAGS([-Wno-format-pedantic])
    ATHEME_TEST_CC_FLAGS([-Wno-format-zero-length])
    ATHEME_TEST_CC_FLAGS([-Wno-packed])
    ATHEME_TEST_CC_FLAGS([-Wno-padded])
    ATHEME_TEST_CC_FLAGS([-Wno-pedantic])
    ATHEME_TEST_CC_FLAGS([-Wno-reserved-id-macro])
    ATHEME_TEST_CC_FLAGS([-Wno-reserved-identifier])
    ATHEME_TEST_CC_FLAGS([-Wno-sign-conversion])
    ATHEME_TEST_CC_FLAGS([-Wno-unused-parameter])
    ATHEME_TEST_CC_FLAGS([-Wno-unused-variable])
    ATHEME_TEST_CC_FLAGS([-Wno-vla])
])

AC_DEFUN([ATHEME_FEATURETEST_WARNINGS], [

    BUILD_WARNINGS="No"

    AC_ARG_ENABLE([warnings],
        [AS_HELP_STRING([--enable-warnings], [Enable compiler warnings])],
        [], [enable_warnings="no"])

    AS_CASE(["x${enable_warnings}"], [xno], [], [xyes], [
        BUILD_WARNINGS="Yes"
        ATHEME_CC_ENABLE_WARNINGS
    ], [
        AC_MSG_ERROR([invalid option for --enable-warnings])
    ])
])
