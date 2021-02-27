# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

ATHEME_TEST_CC_FLAGS_RESULT="no"
ATHEME_CPP_TEST_FLAGS_RESULT="no"
ATHEME_TEST_LD_FLAGS_RESULT="no"
ATHEME_TEST_CCLD_FLAGS_RESULT="no"

AC_DEFUN([ATHEME_TEST_CC_FLAGS], [

    AC_MSG_CHECKING([for C compiler flag(s) $1 ])

    CFLAGS_SAVED="${CFLAGS:-}"
    CFLAGS="${CFLAGS:-}${CFLAGS:+ }$1"

    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            static int
            return_zero(void)
            {
                return 0;
            }
        ]], [[
            int zero = return_zero();
        ]])
    ], [
        AC_MSG_RESULT([yes])
        ATHEME_TEST_CC_FLAGS_RESULT="yes"
    ], [
        AC_MSG_RESULT([no])
        ATHEME_TEST_CC_FLAGS_RESULT="no"
        CFLAGS="${CFLAGS_SAVED}"
    ])

    unset CFLAGS_SAVED
])

AC_DEFUN([ATHEME_CPP_TEST_FLAGS], [

    AC_MSG_CHECKING([for C preprocessor flag(s) $1 ])

    CPPFLAGS_SAVED="${CPPFLAGS:-}"
    CPPFLAGS="${CPPFLAGS:-}${CPPFLAGS:+ }$1"

    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            static int
            return_zero(void)
            {
                return 0;
            }
        ]], [[
            int zero = return_zero();
        ]])
    ], [
        AC_MSG_RESULT([yes])
        ATHEME_CPP_TEST_FLAGS_RESULT="yes"
    ], [
        AC_MSG_RESULT([no])
        ATHEME_CPP_TEST_FLAGS_RESULT="no"
        CPPFLAGS="${CPPFLAGS_SAVED}"
    ])

    unset CPPFLAGS_SAVED
])

AC_DEFUN([ATHEME_TEST_LD_FLAGS], [

    AC_MSG_CHECKING([for C linker flag(s) $1 ])

    LDFLAGS_SAVED="${LDFLAGS:-}"
    LDFLAGS="${LDFLAGS:-}${LDFLAGS:+ }$1"

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            static int
            return_zero(void)
            {
                return 0;
            }
        ]], [[
            int zero = return_zero();
        ]])
    ], [
        AC_MSG_RESULT([yes])
        ATHEME_TEST_LD_FLAGS_RESULT="yes"
    ], [
        AC_MSG_RESULT([no])
        ATHEME_TEST_LD_FLAGS_RESULT="no"
        LDFLAGS="${LDFLAGS_SAVED}"
    ])

    unset LDFLAGS_SAVED
])

AC_DEFUN([ATHEME_TEST_CCLD_FLAGS], [

    AC_MSG_CHECKING([for C compiler *and* linker flag(s) $1 ])

    CFLAGS_SAVED="${CFLAGS:-}"
    LDFLAGS_SAVED="${LDFLAGS:-}"

    CFLAGS="${CFLAGS:-}${CFLAGS:+ }$1"
    LDFLAGS="${LDFLAGS:-}${LDFLAGS:+ }$1"

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            static int
            return_zero(void)
            {
                return 0;
            }
        ]], [[
            int zero = return_zero();
        ]])
    ], [
        AC_MSG_RESULT([yes])
        ATHEME_TEST_CCLD_FLAGS_RESULT="yes"
    ], [
        AC_MSG_RESULT([no])
        ATHEME_TEST_CCLD_FLAGS_RESULT="no"
        CFLAGS="${CFLAGS_SAVED}"
        LDFLAGS="${LDFLAGS_SAVED}"
    ])

    unset CFLAGS_SAVED
    unset LDFLAGS_SAVED
])
