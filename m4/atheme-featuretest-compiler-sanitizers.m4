# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER], [

    AC_MSG_CHECKING([for C compiler *and* linker flag(s) $1 ])

    CFLAGS_SAVED="${CFLAGS}"
    CFLAGS="${CFLAGS} $1"

    LDFLAGS_SAVED="${LDFLAGS}"
    LDFLAGS="${LDFLAGS} $1"

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            static int
            retfoo(void)
            {
                return 0;
            }
        ]], [[
            return retfoo();
        ]])
    ], [
        AC_MSG_RESULT([yes])
        COMPILER_SANITIZERS="Yes"
    ], [
        AC_MSG_RESULT([no])
        CFLAGS="${CFLAGS_SAVED}"
        LDFLAGS="${LDFLAGS_SAVED}"
    ])
])

AC_DEFUN([ATHEME_FEATURETEST_COMPILER_SANITIZERS], [

    COMPILER_SANITIZERS="No"

    AC_ARG_ENABLE([compiler-sanitizers],
        [AS_HELP_STRING([--enable-compiler-sanitizers], [Enable various compiler run-time-instrumented sanitizers])],
        [], [enable_compiler_sanitizers="no"])

    # Test for this whether this option was given or not
    # This option is required for meaningful sanitizer output, but it is also useful without any sanitizers
    ATHEME_CC_TEST_CFLAGS([-g])

    CFLAGS_SAVED_FT="${CFLAGS}"
    LDFLAGS_SAVED_FT="${LDFLAGS}"

    case "x${enable_compiler_sanitizers}" in
        xyes)
            # -fsanitize= benefits from these, but they're not strictly necessary
            ATHEME_CC_TEST_CFLAGS([-fno-omit-frame-pointer])
            ATHEME_CC_TEST_CFLAGS([-fno-optimize-sibling-calls])

            # This, however, is necessary. This test will fail if you are using Clang and not using an LLVM
            # bitcode-parsing-capable linker. Clang in LTO mode compiles to LLVM bitcode, not machine code.
            #
            # The linker is responsible for translating that to machine code. If you want to use Clang, you
            # must set LDFLAGS="-fuse-ld=lld", to use the LLVM linker, or you can use the Gold linker with
            # the LLVM linker plugin.
            ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fvisibility=default -flto])

            AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [

                # However, just because that succeeded, don't enable this feature. We need at least one of the below.
                COMPILER_SANITIZERS="No"

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
            ])

            AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [
                # This causes pointless noise, turn it off.
                ATHEME_FEATURETEST_COMPILER_SANITIZERS_DRIVER([-fno-sanitize=unsigned-integer-overflow])
            ])

            ;;
        xno)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-compiler-sanitizers])
            ;;
    esac

    AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [
        AC_DEFINE([ATHEME_ENABLE_COMPILER_SANITIZERS], [1], [Define to 1 if --enable-compiler-sanitizers was given to ./configure])
    ], [
        # None of the sanitizers are available, give up ...
        CFLAGS="${CFLAGS_SAVED_FT}"
        LDFLAGS="${LDFLAGS_SAVED_FT}"
    ])
])
