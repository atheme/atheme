# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_CC_ENABLE_WARNINGS], [

    ATHEME_CC_TEST_CFLAGS([-Werror=unknown-warning-option])

    ATHEME_CC_TEST_CFLAGS([-Weverything])

    AS_IF([test "${ATHEME_CC_TEST_CFLAGS_RESULT}" = "no"], [

        # These two have to be consecutive and first for the most reliable
        # results. Do not alphabetise with the flags below.    -- amdj
        ATHEME_CC_TEST_CFLAGS([-Wall])
        ATHEME_CC_TEST_CFLAGS([-Wextra])

        ATHEME_CC_TEST_CFLAGS([-Waggregate-return])
        ATHEME_CC_TEST_CFLAGS([-Waggressive-loop-optimizations])
        ATHEME_CC_TEST_CFLAGS([-Walloc-zero])
        ATHEME_CC_TEST_CFLAGS([-Walloca])
        ATHEME_CC_TEST_CFLAGS([-Warray-bounds])
        ATHEME_CC_TEST_CFLAGS([-Wbad-function-cast])
        ATHEME_CC_TEST_CFLAGS([-Wc99-c11-compat])
        ATHEME_CC_TEST_CFLAGS([-Wcast-qual])
        ATHEME_CC_TEST_CFLAGS([-Wdangling-else])
        ATHEME_CC_TEST_CFLAGS([-Wdate-time])
        ATHEME_CC_TEST_CFLAGS([-Wdisabled-optimization])
        ATHEME_CC_TEST_CFLAGS([-Wdouble-promotion])
        ATHEME_CC_TEST_CFLAGS([-Wduplicated-branches])
        ATHEME_CC_TEST_CFLAGS([-Wduplicated-cond])
        ATHEME_CC_TEST_CFLAGS([-Wfatal-errors])
        ATHEME_CC_TEST_CFLAGS([-Wfloat-equal])
        ATHEME_CC_TEST_CFLAGS([-Wformat-nonliteral])
        ATHEME_CC_TEST_CFLAGS([-Wformat-overflow])
        ATHEME_CC_TEST_CFLAGS([-Wformat-security])
        ATHEME_CC_TEST_CFLAGS([-Wformat-signedness])
        ATHEME_CC_TEST_CFLAGS([-Wformat-truncation])
        ATHEME_CC_TEST_CFLAGS([-Wformat-y2k])
        ATHEME_CC_TEST_CFLAGS([-Winline])
        ATHEME_CC_TEST_CFLAGS([-Winit-self])
        ATHEME_CC_TEST_CFLAGS([-Winvalid-pch])
        ATHEME_CC_TEST_CFLAGS([-Wjump-misses-init])
        ATHEME_CC_TEST_CFLAGS([-Wlogical-op])
        ATHEME_CC_TEST_CFLAGS([-Wmissing-declarations])
        ATHEME_CC_TEST_CFLAGS([-Wmissing-format-attribute])
        ATHEME_CC_TEST_CFLAGS([-Wmissing-include-dirs])
        ATHEME_CC_TEST_CFLAGS([-Wmissing-prototypes])
        ATHEME_CC_TEST_CFLAGS([-Wmissing-variable-declarations])
        ATHEME_CC_TEST_CFLAGS([-Wmultistatement-macros])
        ATHEME_CC_TEST_CFLAGS([-Wnested-externs])
        ATHEME_CC_TEST_CFLAGS([-Wnormalized=nfkc])
        ATHEME_CC_TEST_CFLAGS([-Wnull-dereference])
        ATHEME_CC_TEST_CFLAGS([-Wold-style-definition])
        ATHEME_CC_TEST_CFLAGS([-Woverlength-strings])
        ATHEME_CC_TEST_CFLAGS([-Wpointer-arith])
        ATHEME_CC_TEST_CFLAGS([-Wpointer-compare])
        ATHEME_CC_TEST_CFLAGS([-Wredundant-decls])
        ATHEME_CC_TEST_CFLAGS([-Wrestrict])
        ATHEME_CC_TEST_CFLAGS([-Wshadow])
        ATHEME_CC_TEST_CFLAGS([-Wstack-protector])

        ATHEME_CC_TEST_CFLAGS([-Wstrict-overflow=3])
        AS_IF([test "${ATHEME_CC_TEST_CFLAGS_RESULT}" = "no"], [
            ATHEME_CC_TEST_CFLAGS([-Wstrict-overflow])
        ])

        ATHEME_CC_TEST_CFLAGS([-Wstrict-prototypes])
        ATHEME_CC_TEST_CFLAGS([-Wstringop-overflow=4])
        ATHEME_CC_TEST_CFLAGS([-Wstringop-truncation])
        ATHEME_CC_TEST_CFLAGS([-Wtrampolines])
        ATHEME_CC_TEST_CFLAGS([-Wundef])
        ATHEME_CC_TEST_CFLAGS([-Wunsafe-loop-optimizations])
        ATHEME_CC_TEST_CFLAGS([-Wunsuffixed-float-constants])
        ATHEME_CC_TEST_CFLAGS([-Wunused])
        ATHEME_CC_TEST_CFLAGS([-Wwrite-strings])
    ])

    ATHEME_CC_TEST_CFLAGS([-Wno-address-of-packed-member])
    ATHEME_CC_TEST_CFLAGS([-Wno-cast-align])
    ATHEME_CC_TEST_CFLAGS([-Wno-conversion])
    ATHEME_CC_TEST_CFLAGS([-Wno-disabled-macro-expansion])
    ATHEME_CC_TEST_CFLAGS([-Wno-documentation-deprecated-sync])
    ATHEME_CC_TEST_CFLAGS([-Wno-documentation-unknown-command])
    ATHEME_CC_TEST_CFLAGS([-Wno-extra-semi-stmt])
    ATHEME_CC_TEST_CFLAGS([-Wno-format-pedantic])
    ATHEME_CC_TEST_CFLAGS([-Wno-format-zero-length])
    ATHEME_CC_TEST_CFLAGS([-Wno-inline])
    ATHEME_CC_TEST_CFLAGS([-Wno-packed])
    ATHEME_CC_TEST_CFLAGS([-Wno-padded])
    ATHEME_CC_TEST_CFLAGS([-Wno-pedantic])
    ATHEME_CC_TEST_CFLAGS([-Wno-reserved-id-macro])
    ATHEME_CC_TEST_CFLAGS([-Wno-sign-conversion])
    ATHEME_CC_TEST_CFLAGS([-Wno-unused-parameter])
    ATHEME_CC_TEST_CFLAGS([-Wno-unused-variable])
    ATHEME_CC_TEST_CFLAGS([-Wno-vla])
])

AC_DEFUN([ATHEME_FEATURETEST_WARNINGS], [

    BUILD_WARNINGS="No"

    AC_ARG_ENABLE([warnings],
        [AS_HELP_STRING([--enable-warnings], [Enable compiler warnings])],
        [], [enable_warnings="no"])

    case "x${enable_warnings}" in
        xyes)
            BUILD_WARNINGS="Yes"
            ATHEME_CC_ENABLE_WARNINGS
            ;;
        xno)
            BUILD_WARNINGS="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-warnings])
            ;;
    esac
])
