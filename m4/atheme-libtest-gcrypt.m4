# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2020 Aaron Jones <me@aaronmdjones.net>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_GCRYPT], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBGCRYPT="No"
    LIBGCRYPT_USABLE="No"
    LIBGCRYPT_DIGEST="No"

    LIBGCRYPT_CFLAGS=""
    LIBGCRYPT_LIBS=""

    AC_ARG_WITH([gcrypt],
        [AS_HELP_STRING([--without-gcrypt], [Do not attempt to detect GNU libgcrypt (cryptographic library)])],
        [], [with_gcrypt="auto"])

    AS_CASE(["x${with_gcrypt}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --with-gcrypt])
    ])

    AS_IF([test "${with_gcrypt}" != "no"], [
        # If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
        AC_SEARCH_LIBS([gcry_check_version], [gcrypt], [
            AC_MSG_CHECKING([if GNU libgcrypt appears to be usable])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #define GCRYPT_NO_DEPRECATED 1
                    #define GCRYPT_NO_MPI_MACROS 1
                    #include <gcrypt.h>
                ]], [[
                    (void) gcry_check_version(GCRYPT_VERSION);
                    (void) gcry_control(GCRYCTL_DISABLE_SECMEM_WARN, 0);
                    (void) gcry_control(GCRYCTL_INIT_SECMEM, 0);
                    (void) gcry_control(GCRYCTL_SET_VERBOSITY, 0);
                    (void) gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
                    (void) gcry_control(GCRYCTL_SELFTEST, 0);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBGCRYPT="Yes"
            ], [
                AC_MSG_RESULT([no])
                LIBGCRYPT="No"
                AS_IF([test "${with_gcrypt}" = "yes"], [
                    AC_MSG_FAILURE([--with-gcrypt was given but GNU libgcrypt appears to be unusable])
                ])
            ])
        ], [
            LIBGCRYPT="No"
            AS_IF([test "${with_gcrypt}" = "yes"], [
                AC_MSG_ERROR([--with-gcrypt was given but GNU libgcrypt could not be found])
            ])
        ])
    ], [
        LIBGCRYPT="No"
    ])

    AS_IF([test "${LIBGCRYPT}" = "Yes"], [
        AC_MSG_CHECKING([if GNU libgcrypt has usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #define GCRYPT_NO_DEPRECATED 1
                #define GCRYPT_NO_MPI_MACROS 1
                #include <gcrypt.h>
            ]], [[
                gcry_md_hd_t ctx;
                (void) gcry_md_test_algo(GCRY_MD_MD5);
                (void) gcry_md_test_algo(GCRY_MD_SHA1);
                (void) gcry_md_test_algo(GCRY_MD_SHA256);
                (void) gcry_md_test_algo(GCRY_MD_SHA512);
                (void) gcry_md_open(&ctx, GCRY_MD_NONE, GCRY_MD_FLAG_HMAC | GCRY_MD_FLAG_SECURE);
                (void) gcry_md_setkey(ctx, NULL, 0);
                (void) gcry_md_write(ctx, NULL, 0);
                (void) gcry_md_read(ctx, GCRY_MD_NONE);
                (void) gcry_md_close(ctx);
                (void) gcry_kdf_derive(NULL, 0, GCRY_KDF_PBKDF2, GCRY_MD_NONE, NULL, 0, 0, 0, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBGCRYPT_USABLE="Yes"
            LIBGCRYPT_DIGEST="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])

    AS_IF([test "${LIBGCRYPT_USABLE}" = "Yes"], [
        AC_DEFINE([HAVE_LIBGCRYPT], [1], [Define to 1 if GNU libgcrypt appears to be usable])
        AS_IF([test "x${ac_cv_search_gcry_check_version}" != "xnone required"], [
            LIBGCRYPT_LIBS="${ac_cv_search_gcry_check_version}"
        ])
    ], [
        LIBGCRYPT="No"
        AS_IF([test "${with_gcrypt}" = "yes"], [
            AC_MSG_FAILURE([--with-gcrypt was given but GNU libgcrypt appears to be unusable])
        ])
    ])

    AS_IF([test "${LIBGCRYPT}" = "No"], [
        LIBGCRYPT_CFLAGS=""
        LIBGCRYPT_LIBS=""
    ])

    AC_SUBST([LIBGCRYPT_CFLAGS])
    AC_SUBST([LIBGCRYPT_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    unset CFLAGS_SAVED
    unset LIBS_SAVED
])
