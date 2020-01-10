# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

ATHEME_CC_TEST_CFLAGS_RESULT="no"

AC_DEFUN([ATHEME_CC_TEST_CFLAGS],
    [
        AC_MSG_CHECKING([for C compiler flag(s) $1 ])

        CFLAGS_SAVED="${CFLAGS}"
        CFLAGS="${CFLAGS} $1"

        AC_COMPILE_IFELSE(
            [
                AC_LANG_PROGRAM([[]], [[]])
            ], [
                ATHEME_CC_TEST_CFLAGS_RESULT='yes'

                AC_MSG_RESULT([yes])
            ], [
                ATHEME_CC_TEST_CFLAGS_RESULT='no'
                CFLAGS="${CFLAGS_SAVED}"

                AC_MSG_RESULT([no])
            ], [
                ATHEME_CC_TEST_CFLAGS_RESULT='no'
                CFLAGS="${CFLAGS_SAVED}"

                AC_MSG_RESULT([skipped as we are cross-compiling])
            ]
        )

        unset CFLAGS_SAVED
    ]
)

AC_DEFUN([ATHEME_FEATURETEST_CFLAGS], [

    AC_ARG_ENABLE([async-unwind-tables],
        [AS_HELP_STRING([--disable-async-unwind-tables], [Disable -fasynchronous-unwind-tables (Generate precise unwind tables for more reliable backtraces)])],
        [], [enable_async_unwind_tables="yes"])

    AC_ARG_ENABLE([stack-clash-protection],
        [AS_HELP_STRING([--disable-stack-clash-protection], [Disable -fstack-clash-protection (Prevents skipping over VMM guard pages)])],
        [], [enable_stack_clash_protection="yes"])

    AC_ARG_ENABLE([stack-protector],
        [AS_HELP_STRING([--disable-stack-protector], [Disable -fstack-protector{-all,-strong,} (Stack smashing protection)])],
        [], [enable_stack_protector="yes"])

    case "x${enable_async_unwind_tables}" in
        xyes)
            ATHEME_CC_TEST_CFLAGS([-fasynchronous-unwind-tables])
            ;;
        xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-async-unwind-tables])
            ;;
    esac

    case "x${enable_stack_clash_protection}" in
        xyes)
            ATHEME_CC_TEST_CFLAGS([-fstack-clash-protection])
            ;;
        xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-stack-clash-protection])
            ;;
    esac

    case "x${enable_stack_protector}" in
        xyes)
            ATHEME_CC_TEST_CFLAGS([-fstack-protector-all])

            AS_IF([test "${ATHEME_CC_TEST_CFLAGS_RESULT}" = "no"], [

                ATHEME_CC_TEST_CFLAGS([-fstack-protector-strong])

                AS_IF([test "${ATHEME_CC_TEST_CFLAGS_RESULT}" = "no"], [

                    ATHEME_CC_TEST_CFLAGS([-fstack-protector])
                ])
            ])

            ;;
        xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-stack-protector])
            ;;
    esac
])
