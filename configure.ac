# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
# Copyright (C) 2015-2019 Atheme Development Group (https://atheme.github.io/)

AC_PREREQ([2.69])

AC_LANG([C])
AC_INIT([Atheme IRC Services], [7.3.0-rc2], [https://github.com/atheme/atheme/issues/], [atheme], [https://atheme.github.io/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([buildsys.mk extra.mk])
AC_CONFIG_HEADERS([include/atheme/sysconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/services/main.c])
AC_PREFIX_DEFAULT([~/atheme])
AS_IF([test "x${prefix}" = "xNONE"], [prefix="${ac_default_prefix}"])

AS_IF([test -d ".git"], [
    AC_MSG_CHECKING([if you read GIT-Access.txt])
    AS_IF([test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"], [
        AC_MSG_RESULT([yes])
    ], [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([please read GIT-Access.txt before trying to build from git])
    ])
], [
    AC_MSG_CHECKING([if you downloaded the correct source code tarball])
    AS_IF([test -f "include/atheme/serno.h"], [
        AC_MSG_RESULT([yes])
    ], [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([please read the Obtaining Atheme section of README.md])
    ])
])

AH_TOP([
#ifndef ATHEME_INC_SYSCONF_H
#define ATHEME_INC_SYSCONF_H 1

#define ATHEME_API_DIGEST_FRONTEND_INTERNAL     0x00U
#define ATHEME_API_DIGEST_FRONTEND_MBEDTLS      0x01U
#define ATHEME_API_DIGEST_FRONTEND_OPENSSL      0x02U
#define ATHEME_API_DIGEST_FRONTEND_GCRYPT       0x03U

#define ATHEME_API_RANDOM_FRONTEND_INTERNAL     0x00U
#define ATHEME_API_RANDOM_FRONTEND_MBEDTLS      0x01U
#define ATHEME_API_RANDOM_FRONTEND_ARC4RANDOM   0x02U
#define ATHEME_API_RANDOM_FRONTEND_SODIUM       0x03U
#define ATHEME_API_RANDOM_FRONTEND_OPENSSL      0x04U
])

AH_BOTTOM([
#ifndef ATHEME_API_DIGEST_FRONTEND
#  error "No Digest API frontend was selected by the build system"
#endif /* !ATHEME_API_DIGEST_FRONTEND */

#ifndef ATHEME_API_RANDOM_FRONTEND
#  error "No RNG API frontend was selected by the build system"
#endif /* !ATHEME_API_RANDOM_FRONTEND */

#endif /* !ATHEME_INC_SYSCONF_H */
])

# If CFLAGS is empty or unset, explicitly set it to the empty string, so that
# AC_PROG_CC (below) does not add "-O2 -g", as this will conflict with the
# optimisation flags that the compiler sanitizers feature uses, and the
# more advanced debugging flags detection that the CFLAGS feature test uses
AS_IF([test "x${CFLAGS:-}" = "x"], [CFLAGS=""])

# Programs required by the build system
AC_PROG_CC
AC_PROG_CC_C99
AS_IF([test "x${ac_cv_prog_cc_c99}" = "xno"], [
    AC_MSG_FAILURE([This program requires a C99 compiler])
])
AC_CHECK_TOOL([AR], [ar])
AC_PATH_PROG([CP], [cp])
AC_PROG_INSTALL
AC_PATH_PROG([LN], [ln])
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PATH_PROG([MV], [mv])
AC_PROG_RANLIB
AC_PATH_PROG([RM], [rm])
AC_PROG_SED
AC_PATH_PROG([TAR], [tar])
PKG_PROG_PKG_CONFIG
AS_IF([test -z "${PKG_CONFIG}"], [
    AC_MSG_WARN([please consider installing pkg-config])
    AC_MSG_WARN([many of this script's library and feature detection tests rely upon it being available])
    AC_MSG_WARN([if you choose to do so, please re-run this script again])
])

# Does what its name implies. The above block can't be put into this function
# because of autotools black magic voodoo crap that no-one understands ...
ATHEME_CHECK_BUILD_REQUIREMENTS

# Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_SUBST([VENDOR_STRING], ["Atheme Development Group <https://atheme.github.io/>"])
AC_SUBST([PACKAGE_BUGREPORT_I18N], ["ircs://irc.libera.chat:6697/atheme-i18n"])
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])
AC_DEFINE_UNQUOTED([VENDOR_STRING], ["${VENDOR_STRING}"], [Vendor string for in-tree module headers])
AC_DEFINE_UNQUOTED([PACKAGE_BUGREPORT_I18N], ["${PACKAGE_BUGREPORT_I18N}"], [Where to report translation bugs])

# This should be before any library tests so that insufficient header
# inclusions in those tests are caught before they become a runtime
# problem
ATHEME_TEST_CC_FLAGS([-Werror=implicit])

# Conditional libraries for standard functions (no option to control detection)
ATHEME_LIBTEST_DL
ATHEME_LIBTEST_MATH
ATHEME_LIBTEST_SOCKET

# Libraries that are autodetected (alphabetical)
ATHEME_LIBTEST_ARGON2
ATHEME_LIBTEST_CRACK
ATHEME_LIBTEST_CRYPT
ATHEME_LIBTEST_CRYPTO
ATHEME_LIBTEST_GCRYPT
ATHEME_LIBTEST_IDN
ATHEME_LIBTEST_LDAP
ATHEME_LIBTEST_MBEDCRYPTO
ATHEME_LIBTEST_MOWGLI
ATHEME_LIBTEST_NETTLE
ATHEME_LIBTEST_PASSWDQC
ATHEME_LIBTEST_PCRE
ATHEME_LIBTEST_QRENCODE
ATHEME_LIBTEST_SODIUM

# Libraries that need to be explicitly enabled (alphabetical)
ATHEME_LIBTEST_PERL

# Digest and RNG frontend to use in libathemecore
ATHEME_DECIDE_DIGEST_FRONTEND
ATHEME_DECIDE_RANDOM_FRONTEND

# Optional features
# These must be after library tests, because some of them depend on the
# library detection logic in the tests to set appropriate variables
ATHEME_FEATURETEST_CONTRIB
ATHEME_FEATURETEST_CRYPTO_BENCHMARKING
ATHEME_FEATURETEST_ECDH_X25519_TOOL
ATHEME_FEATURETEST_ECDSA_NIST256P_TOOLS
ATHEME_FEATURETEST_FHSPATHS
ATHEME_FEATURETEST_HEAP_ALLOCATOR
ATHEME_FEATURETEST_LARGENET
ATHEME_FEATURETEST_LEGACY_PWCRYPTO
ATHEME_FEATURETEST_REPROBUILDS

# These must be here, not above, or they might interfere with library and feature tests
# Explanation is that these modify one or more of CFLAGS, CPPFLAGS, LDFLAGS, and LIBS
# Warnings should be last so that diagnostics don't clutter the contents of config.log
# Sanitizers must be before CFLAGS and LDFLAGS
ATHEME_FEATURETEST_COMPILER_SANITIZERS
ATHEME_FEATURETEST_CPPFLAGS
ATHEME_FEATURETEST_CFLAGS
ATHEME_FEATURETEST_LDFLAGS
ATHEME_FEATURETEST_NLS
ATHEME_FEATURETEST_WARNINGS

# Handle enabling and configuration of submodules
ATHEME_HANDLE_SUBMODULES

# Now add libmowgli and libpcre to CFLAGS and LIBS because the whole codebase uses them
AS_IF([test "x${LIBMOWGLI_CFLAGS}" != "x"], [CFLAGS="${LIBMOWGLI_CFLAGS} ${CFLAGS}"])
AS_IF([test "x${LIBMOWGLI_LIBS}" != "x"], [LIBS="${LIBMOWGLI_LIBS} ${LIBS}"])
AS_IF([test "x${LIBPCRE_CFLAGS}" != "x"], [CFLAGS="${LIBPCRE_CFLAGS} ${CFLAGS}"])
AS_IF([test "x${LIBPCRE_LIBS}" != "x"], [LIBS="${LIBPCRE_LIBS} ${LIBS}"])



AC_OUTPUT
BUILDSYS_TOUCH_DEPS
ATHEME_PRINT_CONFIGURATION
