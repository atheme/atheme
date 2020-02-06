# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_CRYPTO], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBCRYPTO="No"
    LIBCRYPTO_PATH=""
    LIBCRYPTO_NAME=""
    LIBCRYPTO_USABLE="No"
    LIBCRYPTO_DIGEST="No"
    LIBCRYPTO_RANDOM="No"

    AC_ARG_WITH([openssl],
        [AS_HELP_STRING([--without-openssl], [Do not attempt to detect OpenSSL (cryptographic library)])],
        [], [with_openssl="auto"])

    case "x${with_openssl}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBCRYPTO_PATH="${with_openssl}"
            with_openssl="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-openssl])
            ;;
    esac

    AS_IF([test "${with_openssl}" != "no"], [
        AS_IF([test -n "${LIBCRYPTO_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBCRYPTO_PATH}/include" -a -d "${LIBCRYPTO_PATH}/lib"], [
                LIBCRYPTO_CFLAGS="-I${LIBCRYPTO_PATH}/include"
                LIBCRYPTO_LIBS="-L${LIBCRYPTO_PATH}/lib -lcrypto"
            ], [
                AC_MSG_ERROR([${LIBCRYPTO_PATH} is not a suitable directory for OpenSSL])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [], [])
        ])
        AS_IF([test -n "${LIBCRYPTO_CFLAGS+set}" -a -n "${LIBCRYPTO_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBCRYPTO="Yes"
        ], [
            LIBCRYPTO="No"
            AS_IF([test "${with_openssl}" != "no" && test "${with_openssl}" != "auto"], [
                AC_MSG_FAILURE([--with-openssl was given but OpenSSL could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBCRYPTO}" = "Yes"], [
        CFLAGS="${LIBCRYPTO_CFLAGS} ${CFLAGS}"
        LIBS="${LIBCRYPTO_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if libcrypto has usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/evp.h>
                #include <openssl/hmac.h>
                #include <openssl/opensslv.h>
            ]], [[
                (void) EVP_MD_size(NULL);
                (void) EVP_DigestInit_ex(NULL, EVP_md5(), NULL);
                (void) EVP_DigestInit_ex(NULL, EVP_sha1(), NULL);
                (void) EVP_DigestInit_ex(NULL, EVP_sha256(), NULL);
                (void) EVP_DigestInit_ex(NULL, EVP_sha512(), NULL);
                (void) EVP_DigestUpdate(NULL, NULL, 0);
                (void) EVP_DigestFinal_ex(NULL, NULL, NULL);
                (void) HMAC_Init_ex(NULL, NULL, 0, EVP_md5(), NULL);
                (void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha1(), NULL);
                (void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha256(), NULL);
                (void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha512(), NULL);
                (void) HMAC_Update(NULL, NULL, 0);
                (void) HMAC_Final(NULL, NULL, NULL);
                (void) PKCS5_PBKDF2_HMAC(NULL, 0, NULL, 0, 0, NULL, 0, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBCRYPTO_USABLE="Yes"
            LIBCRYPTO_DIGEST="Yes"

            AC_MSG_CHECKING([if libcrypto has HMAC_CTX_new() & HMAC_CTX_free()])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #include <openssl/hmac.h>
                    #include <openssl/opensslv.h>
                ]], [[
                    (void) HMAC_CTX_new();
                    (void) HMAC_CTX_free(NULL);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                AC_DEFINE([HAVE_LIBCRYPTO_HMAC_CTX_DYNAMIC], [1], [Define to 1 if libcrypto has HMAC_CTX_new() & HMAC_CTX_free()])
            ], [
                AC_MSG_RESULT([no])
            ])
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libcrypto has a usable random number generator])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/err.h>
                #include <openssl/rand.h>
                #include <openssl/opensslv.h>
            ]], [[
                (void) ERR_get_error();
                (void) ERR_get_error_line_data(NULL, NULL, NULL, NULL);
                (void) RAND_add(NULL, 0, (double) 0);
                (void) RAND_bytes(NULL, 0);
                (void) RAND_status();
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBCRYPTO_USABLE="Yes"
            LIBCRYPTO_RANDOM="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libcrypto has a usable constant-time memory comparison function])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/crypto.h>
                #include <openssl/opensslv.h>
            ]], [[
                (void) CRYPTO_memcmp(NULL, NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBCRYPTO_MEMCMP], [1], [Define to 1 if libcrypto has a usable constant-time memory comparison function])
            LIBCRYPTO_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libcrypto has a usable memory zeroing function])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/crypto.h>
                #include <openssl/opensslv.h>
            ]], [[
                (void) OPENSSL_cleanse(NULL, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBCRYPTO_CLEANSE], [1], [Define to 1 if libcrypto has a usable memory zeroing function])
            LIBCRYPTO_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libcrypto can provide SASL ECDH-X25519-CHALLENGE])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/curve25519.h>
                #include <openssl/opensslv.h>
            ]], [[
                (void) X25519_keypair(NULL, NULL);
                (void) X25519(NULL, NULL, NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBCRYPTO_ECDH_X25519], [1], [Define to 1 if libcrypto can provide SASL ECDH-X25519-CHALLENGE])
            FEATURE_SASL_ECDH_X25519_CHALLENGE="Yes"
            LIBCRYPTO_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])

        AC_MSG_CHECKING([if libcrypto can provide SASL ECDSA-NIST256P-CHALLENGE])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <openssl/ec.h>
                #include <openssl/ecdsa.h>
                #include <openssl/evp.h>
                #include <openssl/opensslv.h>
            ]], [[
                EC_KEY *pubkey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
                (void) EC_KEY_set_conv_form(pubkey, POINT_CONVERSION_COMPRESSED);
                (void) o2i_ECPublicKey(&pubkey, NULL, 0);
                (void) ECDSA_verify(0, NULL, 0, NULL, 0, pubkey);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_LIBCRYPTO_ECDSA], [1], [Define to 1 if libcrypto can provide SASL ECDSA-NIST256P-CHALLENGE])
            FEATURE_SASL_ECDSA_NIST256P_CHALLENGE="Yes"
            LIBCRYPTO_USABLE="Yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])

    AS_IF([test "${LIBCRYPTO_USABLE}" = "Yes"], [
        AC_MSG_CHECKING([if OpenSSL is really LibreSSL in disguise])
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([[
                #include <openssl/opensslv.h>
                #ifdef LIBRESSL_VERSION_NUMBER
                #  error "Detected LibreSSL"
                #endif
            ]], [[
                return 0;
            ]])
        ], [
            AC_MSG_RESULT([no])
            LIBCRYPTO_NAME="OpenSSL"
        ], [
            AC_MSG_RESULT([yes])
            LIBCRYPTO_NAME="LibreSSL"
        ])

        AC_DEFINE([HAVE_LIBCRYPTO], [1], [Define to 1 if libcrypto appears to be usable])
    ], [
        LIBCRYPTO="No"
        AS_IF([test "${with_openssl}" != "no" && test "${with_openssl}" != "auto"], [
            AC_MSG_FAILURE([--with-openssl was given but libcrypto appears to be unusable])
        ])
    ])

    AS_IF([test "${LIBCRYPTO}" = "No"], [
        LIBCRYPTO_CFLAGS=""
        LIBCRYPTO_LIBS=""
    ])

    AC_SUBST([LIBCRYPTO_CFLAGS])
    AC_SUBST([LIBCRYPTO_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
