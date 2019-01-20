# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
# Copyright (C) 2015-2018 Atheme Development Group (https://atheme.github.io/)

AC_PREREQ([2.69])

AC_LANG([C])
AC_INIT([Atheme IRC Services], [7.3.0-rc2], [https://github.com/atheme/atheme/issues/], [atheme], [https://atheme.github.io/])

AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/sysconf.h])
AC_CONFIG_SRCDIR([src/services/main.c])
AC_PREFIX_DEFAULT([~/atheme])

AH_TOP([
#ifndef ATHEME_INC_SYSCONF_H
#define ATHEME_INC_SYSCONF_H 1

#define ATHEME_DIGEST_FRONTEND_INTERNAL 0x00U
#define ATHEME_DIGEST_FRONTEND_MBEDTLS  0x01U
#define ATHEME_DIGEST_FRONTEND_NETTLE   0x02U
#define ATHEME_DIGEST_FRONTEND_OPENSSL  0x03U

#define ATHEME_RANDOM_FRONTEND_INTERNAL 0x00U
#define ATHEME_RANDOM_FRONTEND_MBEDTLS  0x01U
#define ATHEME_RANDOM_FRONTEND_OPENBSD  0x02U
#define ATHEME_RANDOM_FRONTEND_SODIUM   0x03U
#define ATHEME_RANDOM_FRONTEND_OPENSSL  0x04U
])

AH_BOTTOM([
#ifndef ATHEME_DIGEST_FRONTEND
#  error "No digest frontend"
#endif

#ifndef ATHEME_RANDOM_FRONTEND
#  error "No RNG frontend"
#endif

#endif
])

AC_MSG_CHECKING([if you read GIT-Access.txt])
AS_IF([test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"], [
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([please read GIT-Access.txt before trying to build from git])
])

AS_IF([test "x${prefix}" = "xNONE"], [prefix="${ac_default_prefix}"])
AS_IF([test -z "${CFLAGS}"], [CFLAGS=""])
AS_IF([test -z "${CPPFLAGS}"], [CPPFLAGS=""])

dnl Buildsys module stuff
BUILDSYS_INIT
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="${LIBS} ${DYNAMIC_LD_LIBS}"

AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([MV], [mv])
AC_PATH_PROG([CP], [cp])
AC_PATH_PROG([LN], [ln])
AC_PROG_LN_S
AC_PATH_PROG([TAR], [tar])
AC_PATH_PROG([AR], [ar])
AC_PATH_PROG([RANLIB], [ranlib])

AC_PROG_CC_C99
AC_USE_SYSTEM_EXTENSIONS
AC_C_BIGENDIAN
AC_PROG_MAKE_SET

PKG_PROG_PKG_CONFIG
AS_IF([test -z "${PKG_CONFIG}"], [AC_MSG_ERROR([pkg-config not found])])



dnl Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_SUBST([VENDOR_STRING], ["Atheme Development Group <https://atheme.github.io/>"])
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])
AC_DEFINE_UNQUOTED([VENDOR_STRING], ["${VENDOR_STRING}"], [Vendor string for module headers])



dnl If we're building on Windows we need socket and regex libraries from mingw
case "${host}" in
	*-*-mingw32)
		CFLAGS="-I/mingw/include ${CFLAGS}"
		LIBS="-lwsock32 -lws2_32 -lregex ${LIBS}"
		;;
esac



dnl GNU gettext
mkdir_p="mkdir -p"
AC_SUBST([mkdir_p])
AM_GNU_GETTEXT([external])
LIBS="${LIBS} ${LIBINTL}"
PODIR_COND_D=""
USE_GETTEXT="No"
if test "x${USE_NLS}" = "xyes"; then
	USE_GETTEXT="Yes"
	PODIR_COND_D="po"
	AC_SUBST([PODIR_COND_D])
fi



dnl Checks for header files.
AC_CHECK_HEADERS([link.h], [], [], [])

dnl Checks for library functions.
AC_DEFINE([__STDC_WANT_LIB_EXT1__], [1], [We want ISO 9899:2011 Annex K memory functions if available])
AC_CHECK_FUNCS([execve explicit_bzero flock fork getpid getrlimit gettimeofday memset_s strcasestr strtok_r])
AC_CHECK_FUNC([socket], [], [AC_CHECK_LIB([socket], [socket])])
AC_CHECK_FUNC([gethostbyname], [], [AC_CHECK_LIB([nsl], [gethostbyname])])
HW_FUNC_SNPRINTF
HW_FUNC_ASPRINTF



dnl This should be before any library tests so that insufficient header
dnl inclusions in those tests are caught before they become a runtime
dnl problem
ATHEME_CC_TEST_CFLAGS([-Werror=implicit])



dnl Libraries that are autodetected (alphabetical)
ATHEME_LIBTEST_CRACK
ATHEME_LIBTEST_CRYPT
ATHEME_LIBTEST_CRYPTO
ATHEME_LIBTEST_IDN
ATHEME_LIBTEST_LDAP
ATHEME_LIBTEST_MBEDCRYPTO
ATHEME_LIBTEST_NETTLE
ATHEME_LIBTEST_PASSWDQC
ATHEME_LIBTEST_PCRE
ATHEME_LIBTEST_QRENCODE
ATHEME_LIBTEST_SODIUM

dnl Libraries that need to be explicitly enabled (alphabetical)
ATHEME_LIBTEST_PERL

dnl Optional features
ATHEME_FEATURETEST_CONTRIB
ATHEME_FEATURETEST_FHSPATHS
ATHEME_FEATURETEST_LARGENET
ATHEME_FEATURETEST_LEGACY_PWCRYPTO
ATHEME_FEATURETEST_REPROBUILDS
ATHEME_FEATURETEST_SODIUM_MALLOC

dnl Digest and RNG frontend to use in libathemecore
dnl Avoid duplicate -lcrypto or -lmbedcrypto or the like when digest API frontend == random API frontend
ATHEME_DECIDE_DIGEST_FRONTEND
ATHEME_DECIDE_RANDOM_FRONTEND
AS_IF([test "x${DIGEST_API_CFLAGS}" = "x${RANDOM_API_CFLAGS}"], [RANDOM_API_CFLAGS=""])
AS_IF([test "x${DIGEST_API_LIBS}" = "x${RANDOM_API_LIBS}"], [RANDOM_API_LIBS=""])
AC_SUBST([DIGEST_API_CFLAGS])
AC_SUBST([RANDOM_API_CFLAGS])
AC_SUBST([DIGEST_API_LIBS])
AC_SUBST([RANDOM_API_LIBS])

dnl These must be here, not above, or they might interfere with library and feature tests
dnl Explanation is that these modify one or more of CFLAGS, CPPFLAGS, LDFLAGS, and LIBS
dnl Warnings should be second-to-last so that they don't clutter the contents of config.log
dnl Mowgli test must be last or it will interfere with linker tests (the library may not yet exist)
ATHEME_FEATURETEST_CPPFLAGS
ATHEME_FEATURETEST_CFLAGS
ATHEME_FEATURETEST_DEBUGGING
ATHEME_FEATURETEST_LDFLAGS
ATHEME_FEATURETEST_WARNINGS
ATHEME_LIBTEST_MOWGLI



dnl Paths that source code files might want to reference
CPPFLAGS="${CPPFLAGS} -DPREFIX=\\\"\${prefix}\\\" -DBINDIR=\\\"\${bindir}\\\" -DDATADIR=\\\"\${DATADIR}\\\""
CPPFLAGS="${CPPFLAGS} -DLOCALEDIR=\\\"\${LOCALEDIR}\\\" -DLOGDIR=\\\"\${LOGDIR}\\\" -DMODDIR=\\\"\${MODDIR}\\\""
CPPFLAGS="${CPPFLAGS} -DRUNDIR=\\\"\${RUNDIR}\\\" -DSHAREDIR=\\\"\${SHAREDIR}\\\" -DSYSCONFDIR=\\\"\${sysconfdir}\\\""



AC_SUBST([CFLAGS])
AC_SUBST([CPPFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([LIBS])

BUILDSYS_TOUCH_DEPS

AS_IF([test "x${MOWGLI_SOURCE}" = "xInternal"], [

	dnl Run ./configure in the libmowgli submodule directory but without OpenSSL support
	dnl Atheme does not use the OpenSSL VIO interface in Mowgli and this avoids an unnecessary -lssl
	dnl
	dnl I can't figure out M4 lists to figure out how to do this optional --disable-heap-allocator
	dnl   argument in a nicer manner.  -- amdj
	dnl

	AS_IF([test "${enable_sodium_malloc}" = "yes"], [
		AX_SUBDIRS_CONFIGURE([libmowgli-2], [[--without-openssl], [--disable-heap-allocator]], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
	], [
		AX_SUBDIRS_CONFIGURE([libmowgli-2], [[--without-openssl]], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
	])
])

AC_CONFIG_FILES([
	atheme-services.pc
	buildsys.mk
	extra.mk
])

AC_OUTPUT

ATHEME_PRINT_CONFIGURATION
