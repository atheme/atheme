# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_SODIUM_MALLOC], [

    SODIUM_MALLOC="No"

    AC_ARG_ENABLE([sodium-malloc],
        [AS_HELP_STRING([--enable-sodium-malloc], [Enable libsodium hardened memory allocator])],
        [], [enable_sodium_malloc="no"])

    case "x${enable_sodium_malloc}" in
        xyes)
            AS_IF([test "${HEAP_ALLOCATOR}" = "Yes"], [
                AC_MSG_ERROR([--enable-sodium-malloc requires --disable-heap-allocator])
            ])
            AS_IF([test "${LIBSODIUM_MEMORY}" = "No"], [
                AC_MSG_ERROR([--enable-sodium-malloc requires usable libsodium memory allocation and manipulation functions])
            ])

            SODIUM_MALLOC="Yes"
            AC_DEFINE([ATHEME_ENABLE_SODIUM_MALLOC], [1], [Define to 1 if --enable-sodium-malloc was given to ./configure])
            AC_MSG_WARN([The sodium hardened memory allocator is not intended for production usage])
            ;;
        xno)
            SODIUM_MALLOC="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-sodium-malloc])
            ;;
    esac
])
