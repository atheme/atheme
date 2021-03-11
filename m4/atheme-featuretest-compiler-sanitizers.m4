# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020-2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER], [

    ATHEME_TEST_CCLD_FLAGS([-fsanitize=$1])
    AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "yes"], [
        COMPILER_SANITIZERS="Yes"
        COMPILER_SANITIZERS_ENABLED="${COMPILER_SANITIZERS_ENABLED:-}${COMPILER_SANITIZERS_ENABLED:+, }$1"
    ])
])

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS], [

    COMPILER_SANITIZERS="No"
    COMPILER_SANITIZERS_ENABLED=""

    AC_ARG_ENABLE([compiler-sanitizers],
        [AS_HELP_STRING([--enable-compiler-sanitizers], [Enable various compiler run-time-instrumented sanitizers])],
        [], [enable_compiler_sanitizers="no"])

    AS_CASE(["x${enable_compiler_sanitizers}"], [xno], [
        # Enable compiler optimisations (what autoconf would have done if we didn't explicitly overrule it)
        ATHEME_TEST_CC_FLAGS([-O2])
    ], [xyes], [
        AS_IF([test "${HEAP_ALLOCATOR}" = "Yes"], [
            AC_MSG_ERROR([To use --enable-compiler-sanitizers you must pass --disable-heap-allocator])
        ])

        # Disable compiler optimisations for more accurate stack-traces
        ATHEME_TEST_CC_FLAGS([-O0])

        # -fsanitize= benefits from these, but they're not strictly necessary
        ATHEME_TEST_CC_FLAGS([-fno-omit-frame-pointer])
        ATHEME_TEST_CC_FLAGS([-fno-optimize-sibling-calls])

        # Some compilers (like Clang) require LTO to be enabled for some of their sanitizers to function.
        # This test will fail if you are using Clang and not using an LLVM bitcode-parsing-capable linker.
        # Clang in LTO mode compiles to LLVM bitcode, not machine code.
        #
        # The linker is responsible for translating that to machine code. If you want to use Clang, you
        # must set LDFLAGS="-fuse-ld=lld", to use the LLVM linker, or you can use the Gold linker with
        # the LLVM linker plugin (an exercise for the reader).
        ATHEME_TEST_CCLD_FLAGS([-fvisibility=default -flto])

        AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
            AC_MSG_FAILURE([--enable-compiler-sanitizers was given, but LTO (a common pre-requisite) could not be enabled])
        ])

        # Now on to the good stuff ...
        ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([address])
        ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([bounds])
        AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([array-bounds])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([local-bounds])
        ])
        ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([float-divide-by-zero])
        ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([leak])
        ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([undefined])
        AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([alignment])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([bool])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([builtin])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([enum])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([float-cast-overflow])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([function])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([integer])
            AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([implicit-conversion])
                AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
                    ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([implicit-integer-sign-change])
                    ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([implicit-signed-integer-truncation])
                    ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([implicit-unsigned-integer-truncation])
                ])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([integer-divide-by-zero])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([shift])
                AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
                    ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([shift-base])
                    ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([shift-exponent])
                ])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([signed-integer-overflow])
            ])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([nonnull-attribute])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([null])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([nullability])
            AS_IF([test "${ATHEME_TEST_CCLD_FLAGS_RESULT}" = "no"], [
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([nullability-arg])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([nullability-assign])
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([nullability-return])
            ])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([object-size])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([pointer-overflow])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([return])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([returns-nonnull-attribute])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([unreachable])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([unsigned-shift-base])
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([vla-bound])
        ])

        AS_IF([test "${COMPILER_SANITIZERS}" = "No"], [
            AC_MSG_FAILURE([--enable-compiler-sanitizers was given, but no sanitizers could be enabled])
        ])

        # This causes pointless noise, turn it off.
        ATHEME_TEST_CCLD_FLAGS([-fno-sanitize=unsigned-integer-overflow])

        AC_DEFINE([ATHEME_ENABLE_COMPILER_SANITIZERS], [1], [Define to 1 if compiler sanitizers are enabled])
    ], [
        AC_MSG_ERROR([invalid option for --enable-compiler-sanitizers])
    ])
])
