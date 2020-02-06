# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_MBEDCRYPTO], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBMBEDCRYPTO="No"
    LIBMBEDCRYPTO_USABLE="No"
    LIBMBEDCRYPTO_DIGEST="No"
    LIBMBEDCRYPTO_RANDOM="No"

    LIBMBEDCRYPTO_CFLAGS=""
    LIBMBEDCRYPTO_LIBS=""

    AC_ARG_WITH([mbedtls],
        [AS_HELP_STRING([--without-mbedtls], [Do not attempt to detect ARM mbedTLS (cryptographic library)])],
        [], [with_mbedtls="auto"])

    case "x${with_mbedtls}" in
        xno | xyes | xauto)
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-mbedtls])
            ;;
    esac

    AS_IF([test "${with_mbedtls}" != "no"], [
        # If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
        AC_SEARCH_LIBS([mbedtls_version_get_string], [mbedcrypto], [
            AC_MSG_CHECKING([if libmbedcrypto appears to be usable])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #ifdef MBEDTLS_CONFIG_FILE
                    #  include MBEDTLS_CONFIG_FILE
                    #else
                    #  include <mbedtls/config.h>
                    #endif
                    #include <mbedtls/error.h>
                    #include <mbedtls/version.h>
                    #ifndef MBEDTLS_ERROR_C
                    #  error "MBEDTLS_ERROR_C is not enabled"
                    #endif
                    #ifndef MBEDTLS_VERSION_C
                    #  error "MBEDTLS_VERSION_C is not enabled"
                    #endif
                ]], [[
                    (void) mbedtls_strerror(0, NULL, 0);
                    (void) mbedtls_version_get_string(NULL);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBMBEDCRYPTO="Yes"
            ], [
                AC_MSG_RESULT([no])
                LIBMBEDCRYPTO="No"
                AS_IF([test "${with_mbedtls}" = "yes"], [
                    AC_MSG_FAILURE([--with-mbedtls was given but libmbedcrypto appears to be unusable])
                ])
            ])
        ], [
            LIBMBEDCRYPTO="No"
            AS_IF([test "${with_mbedtls}" = "yes"], [
                AC_MSG_ERROR([--with-mbedtls was given but libmbedcrypto could not be found])
            ])
        ])
    ], [
        LIBMBEDCRYPTO="No"
    ])

    AS_IF([test "${LIBMBEDCRYPTO}" = "Yes"], [
        AC_MSG_CHECKING([if libmbedcrypto has usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef MBEDTLS_CONFIG_FILE
                #  include MBEDTLS_CONFIG_FILE
                #else
                #  include <mbedtls/config.h>
                #endif
                #include <mbedtls/md.h>
                #include <mbedtls/pkcs5.h>
                #ifndef MBEDTLS_MD_C
                #  error "MBEDTLS_MD_C is not enabled"
                #endif
                #ifndef MBEDTLS_PKCS5_C
                #  error "MBEDTLS_PKCS5_C is not enabled"
                #endif
                #ifndef MBEDTLS_MD5_C
                #  error "MBEDTLS_MD5_C is not enabled"
                #endif
                #ifndef MBEDTLS_SHA1_C
                #  error "MBEDTLS_SHA1_C is not enabled"
                #endif
                #ifndef MBEDTLS_SHA256_C
                #  error "MBEDTLS_SHA256_C is not enabled"
                #endif
                #ifndef MBEDTLS_SHA512_C
                #  error "MBEDTLS_SHA512_C is not enabled"
                #endif
            ]], [[
                (void) mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
                (void) mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
                (void) mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
                (void) mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
                (void) mbedtls_md_init(NULL);
                (void) mbedtls_md_setup(NULL, NULL, 0);
                (void) mbedtls_md_starts(NULL);
                (void) mbedtls_md_update(NULL, NULL, 0);
                (void) mbedtls_md_finish(NULL, NULL);
                (void) mbedtls_md_hmac_starts(NULL, NULL, 0);
                (void) mbedtls_md_hmac_update(NULL, NULL, 0);
                (void) mbedtls_md_hmac_finish(NULL, NULL);
                (void) mbedtls_md_free(NULL);
                (void) mbedtls_pkcs5_pbkdf2_hmac(NULL, NULL, 0, NULL, 0, 0, 0, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBMBEDCRYPTO_USABLE="Yes"
            LIBMBEDCRYPTO_DIGEST="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libmbedcrypto has a usable HMAC-DRBG random number generator])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef MBEDTLS_CONFIG_FILE
                #  include MBEDTLS_CONFIG_FILE
                #else
                #  include <mbedtls/config.h>
                #endif
                #include <mbedtls/entropy.h>
                #include <mbedtls/hmac_drbg.h>
                #include <mbedtls/md.h>
                #ifndef MBEDTLS_ENTROPY_C
                #  error "MBEDTLS_ENTROPY_C is not enabled"
                #endif
                #ifndef MBEDTLS_HMAC_DRBG_C
                #  error "MBEDTLS_HMAC_DRBG_C is not enabled"
                #endif
                #ifndef MBEDTLS_MD_C
                #  error "MBEDTLS_MD_C is not enabled"
                #endif
                #if !defined(MBEDTLS_SHA256_C) && !defined(MBEDTLS_SHA512_C)
                #  error "Neither MBEDTLS_SHA256_C nor MBEDTLS_SHA512_C are enabled"
                #endif
            ]], [[
                (void) mbedtls_entropy_init(NULL);
                (void) mbedtls_entropy_func(NULL, NULL, 0);
                (void) mbedtls_md_info_from_type(MBEDTLS_MD_NONE);
                (void) mbedtls_hmac_drbg_init(NULL);
                (void) mbedtls_hmac_drbg_seed(NULL, NULL, NULL, NULL, NULL, 0);
                (void) mbedtls_hmac_drbg_reseed(NULL, NULL, 0);
                (void) mbedtls_hmac_drbg_random(NULL, NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBMBEDCRYPTO_USABLE="Yes"
            LIBMBEDCRYPTO_RANDOM="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libmbedcrypto can provide SASL ECDH-X25519-CHALLENGE])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef MBEDTLS_CONFIG_FILE
                #  include MBEDTLS_CONFIG_FILE
                #else
                #  include <mbedtls/config.h>
                #endif
                #include <mbedtls/bignum.h>
                #include <mbedtls/ecdh.h>
                #include <mbedtls/ecp.h>
                #ifndef MBEDTLS_BIGNUM_C
                #  error "MBEDTLS_BIGNUM_C is not enabled"
                #endif
                #ifndef MBEDTLS_ECDH_C
                #  error "MBEDTLS_ECDH_C is not enabled"
                #endif
                #ifndef MBEDTLS_ECP_C
                #  error "MBEDTLS_ECP_C is not enabled"
                #endif
            ]], [[
                (void) mbedtls_ecdh_compute_shared(NULL, NULL, NULL, NULL, NULL, NULL);
                (void) mbedtls_ecdh_gen_public(NULL, NULL, NULL, NULL, NULL);
                (void) mbedtls_ecp_check_pubkey(NULL, NULL);
                (void) mbedtls_ecp_group_free(NULL);
                (void) mbedtls_ecp_group_init(NULL);
                (void) mbedtls_ecp_group_load(NULL, 0);
                (void) mbedtls_ecp_point_free(NULL);
                (void) mbedtls_ecp_point_init(NULL);
                (void) mbedtls_mpi_free(NULL);
                (void) mbedtls_mpi_init(NULL);
                (void) mbedtls_mpi_lset(NULL, 0);
                (void) mbedtls_mpi_read_binary(NULL, NULL, 0);
                (void) mbedtls_mpi_write_binary(NULL, NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBMBEDCRYPTO_ECDH_X25519], [1], [Define to 1 if libmbedcrypto can provide SASL ECDH-X25519-CHALLENGE])
            FEATURE_SASL_ECDH_X25519_CHALLENGE="Yes"
            LIBMBEDCRYPTO_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])

    AS_IF([test "${LIBMBEDCRYPTO_USABLE}" = "Yes"], [
        AC_DEFINE([HAVE_LIBMBEDCRYPTO], [1], [Define to 1 if libmbedcrypto appears to be usable])
        AS_IF([test "x${ac_cv_search_mbedtls_version_get_string}" != "xnone required"], [
            LIBMBEDCRYPTO_LIBS="${ac_cv_search_mbedtls_version_get_string}"
        ])
    ], [
        LIBMBEDCRYPTO="No"
        AS_IF([test "${with_mbedtls}" = "yes"], [
            AC_MSG_FAILURE([--with-mbedtls was given but libmbedcrypto appears to be unusable])
        ])
    ])

    AS_IF([test "${LIBMBEDCRYPTO}" = "No"], [
        LIBMBEDCRYPTO_CFLAGS=""
        LIBMBEDCRYPTO_LIBS=""
    ])

    AC_SUBST([LIBMBEDCRYPTO_CFLAGS])
    AC_SUBST([LIBMBEDCRYPTO_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
