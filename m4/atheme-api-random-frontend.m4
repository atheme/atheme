# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_RANDOM_FRONTEND_USE_OPENBSD], [

    RANDOM_FRONTEND_VAL="ATHEME_API_RANDOM_FRONTEND_OPENBSD"
    RANDOM_FRONTEND="OpenBSD arc4random(3)"
])

AC_DEFUN([ATHEME_RANDOM_FRONTEND_USE_SODIUM], [

    RANDOM_FRONTEND_VAL="ATHEME_API_RANDOM_FRONTEND_SODIUM"
    RANDOM_FRONTEND="Sodium"
])

AC_DEFUN([ATHEME_RANDOM_FRONTEND_USE_OPENSSL], [

    RANDOM_FRONTEND_VAL="ATHEME_API_RANDOM_FRONTEND_OPENSSL"
    RANDOM_FRONTEND="${LIBCRYPTO_NAME}"
])

AC_DEFUN([ATHEME_RANDOM_FRONTEND_USE_MBEDTLS], [

    RANDOM_FRONTEND_VAL="ATHEME_API_RANDOM_FRONTEND_MBEDTLS"
    RANDOM_FRONTEND="ARM mbedTLS"
])

AC_DEFUN([ATHEME_RANDOM_FRONTEND_USE_INTERNAL], [

    AC_MSG_CHECKING([if getentropy(3) is available])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            #ifdef HAVE_STDLIB_H
            #  include <stdlib.h>
            #endif
            #ifdef HAVE_UNISTD_H
            #  include <unistd.h>
            #endif
        ]], [[
            (void) getentropy(NULL, 0);
        ]])
    ], [
        AC_MSG_RESULT([yes])
        AC_DEFINE([HAVE_USABLE_GETENTROPY], [1], [Define to 1 if getentropy(3) appears to be usable])
    ], [
        AC_MSG_RESULT([no])
        AC_MSG_CHECKING([if getrandom(2) is available])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sys/random.h>
            ]], [[
                (void) getrandom(NULL, 0, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_USABLE_GETRANDOM], [1], [Define to 1 if getrandom(2) appears to be usable])
        ], [
            AC_MSG_RESULT([no])
            AC_MSG_WARN([this program will need access to the urandom(4) device at run-time])
        ])
    ])

    RANDOM_FRONTEND_VAL="ATHEME_API_RANDOM_FRONTEND_INTERNAL"
    RANDOM_FRONTEND="Internal"
])

AC_DEFUN([ATHEME_DECIDE_RANDOM_FRONTEND], [

    OPENBSD="No"
    OPENBSD_ARC4RANDOM="No"

    RANDOM_FRONTEND_VAL=""
    RANDOM_FRONTEND=""

    AC_ARG_WITH([rng-api-frontend],
        [AS_HELP_STRING([--with-rng-api-frontend=@<:@frontend@:>@], [RNG API frontend to use (auto, openbsd, sodium, openssl, libressl, mbedtls, internal). Default: auto])],
        [], [with_rng_api_frontend="auto"])

    AC_MSG_CHECKING([if we are building for OpenBSD])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
            #ifndef __OpenBSD__
            #  error "Not building on OpenBSD"
            #endif
        ]], [[
            return 0;
        ]])
    ], [
        AC_MSG_RESULT([yes])
        OPENBSD="Yes"

        AC_MSG_CHECKING([if arc4random(3) appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef HAVE_STDLIB_H
                #  include <stdlib.h>
                #endif
            ]], [[
                (void) arc4random();
                (void) arc4random_uniform(0);
                (void) arc4random_buf(NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            OPENBSD_ARC4RANDOM="Yes"
        ], [
            AC_MSG_RESULT([no])
            OPENBSD_ARC4RANDOM="No"
        ])
    ], [
        AC_MSG_RESULT([no])
        OPENBSD="No"
        OPENBSD_ARC4RANDOM="No"
    ])

    case "x${with_rng_api_frontend}" in

        xauto)
            AS_IF([test "${OPENBSD}${OPENBSD_ARC4RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_OPENBSD
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen automatically)])
            ], [test "${LIBSODIUM}${LIBSODIUM_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_SODIUM
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen automatically)])
            ], [test "${LIBCRYPTO}${LIBCRYPTO_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_OPENSSL
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen automatically)])
            ], [test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_MBEDTLS
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen automatically)])
            ], [
                ATHEME_RANDOM_FRONTEND_USE_INTERNAL
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen automatically)])
            ])
            ;;

        xopenbsd)
            AS_IF([test "${OPENBSD}${OPENBSD_ARC4RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_OPENBSD
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-rng-api-frontend=openbsd requires OpenBSD and a usable OpenBSD arc4random(3)])
            ])
            ;;

        xsodium)
            AS_IF([test "${LIBSODIUM}${LIBSODIUM_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_SODIUM
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-rng-api-frontend=sodium requires --with-sodium and a usable random number generator])
            ])
            ;;

        xopenssl | xlibressl)
            AS_IF([test "${LIBCRYPTO}${LIBCRYPTO_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_OPENSSL
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-rng-api-frontend=openssl/libressl requires --with-openssl and a usable random number generator])
            ])
            ;;

        xmbedtls)
            AS_IF([test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_RANDOM}" = "YesYes"], [
                ATHEME_RANDOM_FRONTEND_USE_MBEDTLS
                AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-rng-api-frontend=mbedtls requires --with-mbedtls and a usable random number generator])
            ])
            ;;

        xinternal)
            ATHEME_RANDOM_FRONTEND_USE_INTERNAL
            AC_MSG_NOTICE([using RNG frontend: ${RANDOM_FRONTEND} (chosen by user)])
            ;;

        *)
            AC_MSG_ERROR([invalid option for --with-rng-api-frontend (auto, openbsd, sodium, openssl, libressl, mbedtls, internal)])
            ;;
    esac

    AC_DEFINE_UNQUOTED([ATHEME_API_RANDOM_FRONTEND], [${RANDOM_FRONTEND_VAL}], [Atheme RNG API Frontend])
])
