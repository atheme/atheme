# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_NETTLE], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBNETTLE="No"
    LIBNETTLE_PATH=""
    LIBNETTLE_USABLE="No"

    AC_ARG_WITH([nettle],
        [AS_HELP_STRING([--without-nettle], [Do not attempt to detect nettle (cryptographic library)])],
        [], [with_nettle="auto"])

    case "x${with_nettle}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBNETTLE_PATH="${with_nettle}"
            with_nettle="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-nettle])
            ;;
    esac

    AS_IF([test "${with_nettle}" != "no"], [
        AS_IF([test -n "${LIBNETTLE_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBNETTLE_PATH}/include" -a -d "${LIBNETTLE_PATH}/lib"], [
                LIBNETTLE_CFLAGS="-I${LIBNETTLE_PATH}/include"
                LIBNETTLE_LIBS="-L${LIBNETTLE_PATH}/lib -lnettle"
            ], [
                AC_MSG_ERROR([${LIBNETTLE_PATH} is not a suitable directory for GNU Nettle])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBNETTLE], [nettle], [], [])
        ])
        AS_IF([test -n "${LIBNETTLE_CFLAGS+set}" -a -n "${LIBNETTLE_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBNETTLE="Yes"
        ], [
            LIBNETTLE="No"
            AS_IF([test "${with_nettle}" != "no" && test "${with_nettle}" != "auto"], [
                AC_MSG_FAILURE([--with-nettle was given but GNU Nettle could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBNETTLE}" = "Yes"], [
        CFLAGS="${LIBNETTLE_CFLAGS} ${CFLAGS}"
        LIBS="${LIBNETTLE_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if GNU Nettle has a usable constant-time memory comparison function])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <nettle/memops.h>
            ]], [[
                (void) nettle_memeql_sec(NULL, NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBNETTLE_MEMEQL], [1], [Define to 1 if GNU Nettle has a usable constant-time memory comparison function])
            LIBNETTLE_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if GNU Nettle can provide SASL ECDH-X25519-CHALLENGE])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <nettle/curve25519.h>
                #ifndef NETTLE_CURVE25519_RFC7748
                #  error "NETTLE_CURVE25519_RFC7748 is not set"
                #endif
            ]], [[
                (void) nettle_curve25519_mul_g(NULL, NULL);
                (void) nettle_curve25519_mul(NULL, NULL, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBNETTLE_ECDH_X25519], [1], [Define to 1 if GNU Nettle can provide SASL ECDH-X25519-CHALLENGE])
            FEATURE_SASL_ECDH_X25519_CHALLENGE="Yes"
            LIBNETTLE_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])

    AS_IF([test "${LIBNETTLE_USABLE}" = "Yes"], [
        AC_DEFINE([HAVE_LIBNETTLE], [1], [Define to 1 if GNU Nettle appears to be usable])
        AC_CHECK_HEADERS([nettle/version.h], [], [], [])
    ], [
        LIBNETTLE="No"
        AS_IF([test "${with_nettle}" != "no" && test "${with_nettle}" != "auto"], [
            AC_MSG_FAILURE([--with-nettle was given but GNU Nettle appears to be unusable])
        ])
    ])

    AS_IF([test "${LIBNETTLE}" = "No"], [
        LIBNETTLE_CFLAGS=""
        LIBNETTLE_LIBS=""
    ])

    AC_SUBST([LIBNETTLE_CFLAGS])
    AC_SUBST([LIBNETTLE_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
