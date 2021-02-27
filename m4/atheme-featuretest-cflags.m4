# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_CFLAGS], [

    AC_ARG_ENABLE([async-unwind-tables],
        [AS_HELP_STRING([--disable-async-unwind-tables], [Disable -fasynchronous-unwind-tables (Generate precise unwind tables for more reliable backtraces)])],
        [], [enable_async_unwind_tables="yes"])

    AC_ARG_ENABLE([debugging-symbols],
        [AS_HELP_STRING([--disable-debugging-symbols], [Don't build with debugging symbols (-g) enabled])],
        [], [enable_debugging_symbols="yes"])

    AC_ARG_ENABLE([stack-clash-protection],
        [AS_HELP_STRING([--disable-stack-clash-protection], [Disable -fstack-clash-protection (Prevents skipping over VMM guard pages)])],
        [], [enable_stack_clash_protection="yes"])

    AC_ARG_ENABLE([stack-protector],
        [AS_HELP_STRING([--disable-stack-protector], [Disable -fstack-protector{-all,-strong,} (Stack smashing protection)])],
        [], [enable_stack_protector="yes"])



    case "x${enable_async_unwind_tables}" in

        xyes)
            ATHEME_TEST_CC_FLAGS([-fasynchronous-unwind-tables])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-async-unwind-tables])
            ;;
    esac

    case "x${enable_debugging_symbols}" in

        xyes)
            ATHEME_TEST_CC_FLAGS([-gdwarf-4])
            AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
                ATHEME_TEST_CC_FLAGS([-ggdb3])
                AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
                    ATHEME_TEST_CC_FLAGS([-g])
                    AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
                        AC_MSG_WARN([--enable-debugging-symbols was given, but they could not be enabled])
                    ])
                ])
            ])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-debugging-symbols])
            ;;
    esac

    case "x${enable_stack_clash_protection}" in

        xyes)
            ATHEME_TEST_CC_FLAGS([-fstack-clash-protection])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-stack-clash-protection])
            ;;
    esac

    case "x${enable_stack_protector}" in

        xyes)
            ATHEME_TEST_CC_FLAGS([-fstack-protector-all])
            AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
                ATHEME_TEST_CC_FLAGS([-fstack-protector-strong])
                AS_IF([test "${ATHEME_TEST_CC_FLAGS_RESULT}" = "no"], [
                    ATHEME_TEST_CC_FLAGS([-fstack-protector])
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
