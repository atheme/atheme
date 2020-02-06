# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_SODIUM], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBSODIUM="No"
    LIBSODIUM_PATH=""
    LIBSODIUM_USABLE="No"
    LIBSODIUM_MEMORY="No"
    LIBSODIUM_RANDOM="No"
    LIBSODIUM_SCRYPT="No"

    AC_ARG_WITH([sodium],
        [AS_HELP_STRING([--without-sodium], [Do not attempt to detect libsodium (cryptographic library)])],
        [], [with_sodium="auto"])

    case "x${with_sodium}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBSODIUM_PATH="${with_sodium}"
            with_sodium="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-sodium])
            ;;
    esac

    AS_IF([test "${with_sodium}" != "no"], [
        AS_IF([test -n "${LIBSODIUM_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBSODIUM_PATH}/include" -a -d "${LIBSODIUM_PATH}/lib"], [
                LIBSODIUM_CFLAGS="-I${LIBSODIUM_PATH}/include"
                LIBSODIUM_LIBS="-L${LIBSODIUM_PATH}/lib -lsodium"
            ], [
                AC_MSG_ERROR([${LIBSODIUM_PATH} is not a suitable directory for libsodium])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBSODIUM], [libsodium], [], [])
        ])
        AS_IF([test -n "${LIBSODIUM_CFLAGS+set}" -a -n "${LIBSODIUM_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBSODIUM="Yes"
        ], [
            LIBSODIUM="No"
            AS_IF([test "${with_sodium}" != "no" && test "${with_sodium}" != "auto"], [
                AC_MSG_FAILURE([--with-sodium was given but libsodium could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBSODIUM}" = "Yes"], [
        CFLAGS="${LIBSODIUM_CFLAGS} ${CFLAGS}"
        LIBS="${LIBSODIUM_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if libsodium appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #include <sodium/core.h>
                #include <sodium/utils.h>
                #include <sodium/version.h>
            ]], [[
                (void) sodium_init();
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBSODIUM="Yes"
        ], [
            AC_MSG_RESULT([no])
            LIBSODIUM="No"
            AS_IF([test "${with_sodium}" != "no" && test "${with_sodium}" != "auto"], [
                AC_MSG_FAILURE([--with-sodium was given but libsodium appears to be unusable])
            ])
        ])
    ])

    AS_IF([test "${LIBSODIUM}" = "Yes"], [

        AC_MSG_CHECKING([if libsodium has usable memory allocation and manipulation functions])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/core.h>
                #include <sodium/utils.h>
            ]], [[
                (void) sodium_malloc(0);
                (void) sodium_allocarray(0, 0);
                (void) sodium_mprotect_readonly(NULL);
                (void) sodium_mprotect_readwrite(NULL);
                (void) sodium_free(NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBSODIUM_USABLE="Yes"
            LIBSODIUM_MEMORY="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libsodium has a usable constant-time memory comparison function])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/core.h>
                #include <sodium/utils.h>
            ]], [[
                (void) sodium_memcmp(NULL, NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBSODIUM_MEMCMP], [1], [Define to 1 if libsodium has a usable constant-time memory comparison function])
            LIBSODIUM_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libsodium has a usable memory zeroing function])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/core.h>
                #include <sodium/utils.h>
            ]], [[
                (void) sodium_memzero(NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBSODIUM_MEMZERO], [1], [Define to 1 if libsodium has a usable memory zeroing function])
            LIBSODIUM_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libsodium has a usable random number generator])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/core.h>
                #include <sodium/utils.h>
                #include <sodium/randombytes.h>
            ]], [[
                (void) randombytes_random();
                (void) randombytes_uniform(0);
                (void) randombytes_buf(NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBSODIUM_USABLE="Yes"
            LIBSODIUM_RANDOM="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libsodium has a usable scrypt password hash generator])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
            ]], [[
                (void) crypto_pwhash_scryptsalsa208sha256_str_needs_rehash(NULL, 0, 0);
                (void) crypto_pwhash_scryptsalsa208sha256_str_verify(NULL, NULL, 0);
                (void) crypto_pwhash_scryptsalsa208sha256_str(NULL, NULL, 0, 0, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBSODIUM_SCRYPT], [1], [Define to 1 if libsodium has a usable scrypt password hash generator])
            LIBSODIUM_USABLE="Yes"
            LIBSODIUM_SCRYPT="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libsodium can provide SASL ECDH-X25519-CHALLENGE])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <sodium/crypto_scalarmult_curve25519.h>
            ]], [[
                (void) crypto_scalarmult_curve25519_base(NULL, NULL);
                (void) crypto_scalarmult_curve25519(NULL, NULL, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBSODIUM_ECDH_X25519], [1], [Define to 1 if libsodium can provide SASL ECDH-X25519-CHALLENGE])
            FEATURE_SASL_ECDH_X25519_CHALLENGE="Yes"
            LIBSODIUM_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])

    AS_IF([test "${LIBSODIUM_USABLE}" = "Yes"], [
        AC_DEFINE([HAVE_LIBSODIUM], [1], [Define to 1 if libsodium appears to be usable])
    ], [
        LIBSODIUM="No"
        AS_IF([test "${with_sodium}" != "no" && test "${with_sodium}" != "auto"], [
            AC_MSG_FAILURE([--with-sodium was given but libsodium appears to be unusable])
        ])
    ])

    AS_IF([test "${LIBSODIUM}" = "No"], [
        LIBSODIUM_CFLAGS=""
        LIBSODIUM_LIBS=""
    ])

    AC_SUBST([LIBSODIUM_CFLAGS])
    AC_SUBST([LIBSODIUM_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
