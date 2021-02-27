# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020-2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER], [

    ATHEME_TEST_CCLD_FLAGS([$1])
    AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "yes"], [COMPILER_SANITIZERS="Yes"])
])

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS], [

    COMPILER_SANITIZERS="No"

    AC_ARG_ENABLE([compiler-sanitizers],
        [AS_HELP_STRING([--enable-compiler-sanitizers], [Enable various compiler run-time-instrumented sanitizers])],
        [], [enable_compiler_sanitizers="no"])

    case "x${enable_compiler_sanitizers}" in

        xyes)
            AS_IF([test "${HEAP_ALLOCATOR}" = "Yes"], [
                AC_MSG_ERROR([To use --enable-compiler-sanitizers you must pass --disable-heap-allocator])
            ])

            # Disable compiler optimisations for more accurate stack-traces
            ATHEME_TEST_CC_FLAGS([-O0])

            # -fsanitize= benefits from these, but they're not strictly necessary
            ATHEME_TEST_CC_FLAGS([-fno-omit-frame-pointer])
            ATHEME_TEST_CC_FLAGS([-fno-optimize-sibling-calls])

            # Some compilers (like Clang) require LTO to be enabled for their sanitizers to function. This
            # test will fail if you are using Clang and not using an LLVM bitcode-parsing-capable linker.
            # Clang in LTO mode compiles to LLVM bitcode, not machine code.
            #
            # The linker is responsible for translating that to machine code. If you want to use Clang, you
            # must set LDFLAGS="-fuse-ld=lld", to use the LLVM linker, or you can use the Gold linker with
            # the LLVM linker plugin (an exercise for the reader).
            ATHEME_TEST_CCLD_FLAGS([-fvisibility=default -flto])

            AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "yes"], [

                # Now on to the good stuff ...
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=address])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=bool])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=function])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=implicit-conversion])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=integer])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=leak])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=nullability])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=shift])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fsanitize=undefined])

                AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [

                    AC_DEFINE([ATHEME_ENABLE_COMPILER_SANITIZERS], [1], [Define to 1 if compiler sanitizers are enabled])

                    # This causes pointless noise, turn it off.
                    ATHEME_TEST_CCLD_FLAGS([-fno-sanitize=unsigned-integer-overflow])
                ], [
                    AC_MSG_FAILURE([--enable-compiler-sanitizers was given, but no sanitizers could be enabled])
                ])
            ], [
                AC_MSG_FAILURE([--enable-compiler-sanitizers was given, but LTO (a common pre-requisite) could not be enabled])
            ])
            ;;

        xno)
            # Enable compiler optimisations (what autoconf would have done if we didn't explicitly overrule it)
            ATHEME_TEST_CC_FLAGS([-O2])
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-compiler-sanitizers])
            ;;
    esac
])
