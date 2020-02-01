# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_OPENSSL], [

    DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_OPENSSL"
    DIGEST_FRONTEND="${LIBCRYPTO_NAME}"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_GCRYPT], [

    DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_GCRYPT"
    DIGEST_FRONTEND="GNU libgcrypt"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_MBEDTLS], [

    DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_MBEDTLS"
    DIGEST_FRONTEND="ARM mbedTLS"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_INTERNAL], [

    DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_INTERNAL"
    DIGEST_FRONTEND="Internal"
])

AC_DEFUN([ATHEME_DECIDE_DIGEST_FRONTEND], [

    DIGEST_FRONTEND_VAL=""
    DIGEST_FRONTEND=""

    AC_ARG_WITH([digest-api-frontend],
        [AS_HELP_STRING([--with-digest-api-frontend=@<:@frontend@:>@], [Digest API frontend to use (auto, openssl, libressl, gcrypt, mbedtls, internal). Default: auto])],
        [], [with_digest_api_frontend="auto"])

    case "x${with_digest_api_frontend}" in

        xauto)
            AS_IF([test "${LIBCRYPTO}${LIBCRYPTO_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_OPENSSL
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen automatically)])
            ], [test "${LIBGCRYPT}${LIBGCRYPT_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_GCRYPT
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen automatically)])
            ], [test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_MBEDTLS
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen automatically)])
            ], [
                ATHEME_DIGEST_FRONTEND_USE_INTERNAL
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen automatically)])
            ])
            ;;

        xopenssl | xlibressl)
            AS_IF([test "${LIBCRYPTO}${LIBCRYPTO_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_OPENSSL
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-digest-api-frontend=openssl/libressl requires --with-openssl and usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
            ])
            ;;

        xgcrypt)
            AS_IF([test "${LIBGCRYPT}${LIBGCRYPT_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_GCRYPT
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-digest-api-frontend=gcrypt requires --with-gcrypt and usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
            ])
            ;;

        xmbedtls)
            AS_IF([test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_DIGEST}" = "YesYes"], [
                ATHEME_DIGEST_FRONTEND_USE_MBEDTLS
                AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen by user)])
            ], [
                AC_MSG_ERROR([--with-digest-api-frontend=mbedtls requires --with-mbedtls and usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
            ])
            ;;

        xinternal)
            ATHEME_DIGEST_FRONTEND_USE_INTERNAL
            AC_MSG_NOTICE([using digest frontend: ${DIGEST_FRONTEND} (chosen by user)])
            ;;

        *)
            AC_MSG_ERROR([invalid option for --with-digest-api-frontend (auto, openssl, libressl, gcrypt, mbedtls, internal)])
            ;;
    esac

    AC_DEFINE_UNQUOTED([ATHEME_API_DIGEST_FRONTEND], [${DIGEST_FRONTEND_VAL}], [Atheme Digest API Frontend])
])
