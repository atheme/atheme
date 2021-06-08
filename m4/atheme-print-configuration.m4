# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <me@aaronmdjones.net>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_PRINT_CONFIGURATION], [

    AS_IF([test "${DIGEST_FRONTEND}" = "Internal"], [DIGEST_FRONTEND="None (Internal MD5/SHA/HMAC/PBKDF2 Fallback)"])
    AS_IF([test "${RANDOM_FRONTEND}" = "Internal"], [RANDOM_FRONTEND="None (Internal ChaCha20-based Fallback RNG)"])
    AS_IF([test "${BUILD_WARNINGS}" = "Yes"], [BUILD_WARNINGS="Yes (NOT INTENDED FOR PRODUCTION USAGE)"])
    AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [COMPILER_SANITIZERS="Yes (${COMPILER_SANITIZERS_ENABLED})"])
    AS_IF([test "x${USE_NLS}" = "xyes"], [USE_NLS="Yes"], [USE_NLS="No"])

    AS_IF([test "x${FEATURE_SASL_ECDH_X25519_CHALLENGE}" != "xYes"], [FEATURE_SASL_ECDH_X25519_CHALLENGE="No"])
    AS_IF([test "x${FEATURE_SASL_ECDSA_NIST256P_CHALLENGE}" != "xYes"], [FEATURE_SASL_ECDSA_NIST256P_CHALLENGE="No"])
    AS_IF([test "x${FEATURE_SASL_SCRAM}" != "xYes"], [FEATURE_SASL_SCRAM="No"])

    AS_IF([test "${LIBARGON2}${LIBARGON2_TYPE_ID}" = "YesNo"], [
        LIBARGON2="Partial (Please consider upgrading your Argon2 library)"
    ])

    echo "
Configuration of ${PACKAGE_STRING}:

  Directories:
    Installation Prefix .....: ${prefix}
    Binary Directory ........: ${bindir}
    Module Directory ........: ${MODDIR}/modules
    Config Directory ........: ${sysconfdir}
    Data Directory ..........: ${DATADIR}
    Logfile Directory .......: ${LOGDIR}
    PID File Directory ......: ${RUNDIR}

  Libraries:
    ARM mbedTLS support .....: ${LIBMBEDCRYPTO}
    CrackLib support ........: ${LIBCRACK}
    GNU libgcrypt support ...: ${LIBGCRYPT}
    GNU libidn support ......: ${LIBIDN}
    GNU Nettle support ......: ${LIBNETTLE}
    LDAP support ............: ${LIBLDAP}
    OpenSSL support .........: ${LIBCRYPTO}
    passwdqc support ........: ${LIBPASSWDQC}
    PCRE support ............: ${LIBPCRE}
    Perl support ............: ${LIBPERL}
    QR Code support .........: ${LIBQRENCODE}
    Sodium support ..........: ${LIBSODIUM}

  Password Cryptography:
    Argon2 support ..........: ${LIBARGON2}
    crypt(3) support ........: ${LIBCRYPT}
    scrypt support ..........: ${LIBSODIUM_SCRYPT}

  SASL Mechanisms:
    AUTHCOOKIE ..............: Yes
    ECDH-X25519-CHALLENGE ...: ${FEATURE_SASL_ECDH_X25519_CHALLENGE}
    ECDSA-NIST256P-CHALLENGE : ${FEATURE_SASL_ECDSA_NIST256P_CHALLENGE}
    EXTERNAL ................: Yes
    PLAIN ...................: Yes
    SCRAM ...................: ${FEATURE_SASL_SCRAM}

  Program Features:
    Contrib Modules .........: ${CONTRIB_MODULES}
    Crypto Benchmarking .....: ${CRYPTO_BENCHMARKING}
    Digest Frontend .........: ${DIGEST_FRONTEND}
    ECDH-X25519 Tool ........: ${ECDH_X25519_TOOL}
    ECDSA-NIST256P Tools ....: ${ECDSA_NIST256P_TOOLS}
    Heap Allocator ..........: ${HEAP_ALLOCATOR}
    Internationalization ....: ${USE_NLS}
    Large Network Support ...: ${LARGE_NET}
    Legacy Crypto Modules ...: ${LEGACY_PWCRYPTO}
    Reproducible Builds .....: ${REPRODUCIBLE_BUILDS}
    RNG Frontend ............: ${RANDOM_FRONTEND}

  Build Features:
    Build Warnings ..........: ${BUILD_WARNINGS}
    Compiler Sanitizers .....: ${COMPILER_SANITIZERS}
    Mowgli Installation .....: ${LIBMOWGLI_SOURCE}

  Build Variables:
    CC ......................: ${CC}
    CFLAGS ..................: ${CFLAGS}
    CPPFLAGS ................: ${CPPFLAGS}
    LDFLAGS .................: ${LDFLAGS}
    LIBS ....................: ${LIBS}

Type 'make' to build ${PACKAGE_TARNAME}, and 'make install' to install it.
"

])
