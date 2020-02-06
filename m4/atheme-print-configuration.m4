# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_PRINT_CONFIGURATION], [

    AS_IF([test "${DIGEST_FRONTEND}" = "Internal"], [DIGEST_FRONTEND="None (Internal MD5/SHA/HMAC/PBKDF2 Fallback)"])
    AS_IF([test "${RANDOM_FRONTEND}" = "Internal"], [RANDOM_FRONTEND="None (Internal ChaCha20-based Fallback RNG)"])
    AS_IF([test "${SODIUM_MALLOC}" = "Yes"], [SODIUM_MALLOC="Yes (NOT INTENDED FOR PRODUCTION USAGE)"])
    AS_IF([test "${BUILD_WARNINGS}" = "Yes"], [BUILD_WARNINGS="Yes (NOT INTENDED FOR PRODUCTION USAGE)"])
    AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [COMPILER_SANITIZERS="Yes (NOT INTENDED FOR PRODUCTION USAGE)"])
    AS_IF([test "x${USE_NLS}" = "xyes"], [USE_NLS="Yes"], [USE_NLS="No"])

    prefix="$(eval echo "${prefix}")"
    prefix="$(eval echo "${prefix}")"

    bindir="$(eval echo "${bindir}")"
    bindir="$(eval echo "${bindir}")"

    MODDIR="$(eval echo "${MODDIR}")"
    MODDIR="$(eval echo "${MODDIR}")"

    sysconfdir="$(eval echo "${sysconfdir}")"
    sysconfdir="$(eval echo "${sysconfdir}")"

    LOGDIR="$(eval echo "${LOGDIR}")"
    LOGDIR="$(eval echo "${LOGDIR}")"

    DATADIR="$(eval echo "${DATADIR}")"
    DATADIR="$(eval echo "${DATADIR}")"

    RUNDIR="$(eval echo "${RUNDIR}")"
    RUNDIR="$(eval echo "${RUNDIR}")"

    echo "
Configuration of ${PACKAGE_STRING}:

    Installation Prefix .....: ${prefix}
    Binary Directory ........: ${bindir}
    Config Directory ........: ${sysconfdir}
    Data Directory ..........: ${DATADIR}
    Logfile Directory .......: ${LOGDIR}
    Module Directory ........: ${MODDIR}/modules
    PID Directory ...........: ${RUNDIR}

    Argon2 support ..........: ${LIBARGON2}
    crypt(3) support ........: ${LIBCRYPT}
    scrypt support ..........: ${LIBSODIUM_SCRYPT}

    CrackLib support ........: ${LIBCRACK}
    LDAP support ............: ${LIBLDAP}
    GNU libgcrypt support ...: ${LIBGCRYPT}
    GNU libidn support ......: ${LIBIDN}
    GNU Nettle support ......: ${LIBNETTLE}
    ARM mbedTLS support .....: ${LIBMBEDCRYPTO}
    OpenSSL support .........: ${LIBCRYPTO}
    passwdqc support ........: ${LIBPASSWDQC}
    PCRE support ............: ${LIBPCRE}
    Perl support ............: ${LIBPERL}
    QR Code support .........: ${LIBQRENCODE}
    Sodium support ..........: ${LIBSODIUM}

    Contrib Modules .........: ${CONTRIB_MODULES}
    Heap Allocator ..........: ${HEAP_ALLOCATOR}
    Internationalization ....: ${USE_NLS}
    Large Network Support ...: ${LARGE_NET}
    Reproducible Builds .....: ${REPRODUCIBLE_BUILDS}
    Sodium Memory Allocator .: ${SODIUM_MALLOC}

    Build Warnings ..........: ${BUILD_WARNINGS}
    Compiler sanitizers .....: ${COMPILER_SANITIZERS}

    Crypto Benchmarking .....: ${HAVE_CLOCK_GETTIME}
    Digest Frontend .........: ${DIGEST_FRONTEND}
    RNG Frontend ............: ${RANDOM_FRONTEND}

    Legacy Crypto Modules ...: ${LEGACY_PWCRYPTO}
    Mowgli Installation .....: ${MOWGLI_SOURCE}

    CC ......................: ${CC}
    CFLAGS ..................: ${CFLAGS}
    CPPFLAGS ................: ${CPPFLAGS}
    LDFLAGS .................: ${LDFLAGS}
    LIBS ....................: ${LIBS}

Type 'make' to build ${PACKAGE_TARNAME}, and 'make install' to install it.
"

])
