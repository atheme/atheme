dnl Copyright (C) 2005-2018 Atheme Development Group
dnl Rights to this code are as documented in doc/LICENSE.

AC_PREREQ([2.69])

AC_LANG([C])
AC_INIT([atheme], [7.3.0-rc1], [https://github.com/atheme/atheme/issues/])

AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/sysconf.h])
AC_CONFIG_SRCDIR([src/services/main.c])
AC_PREFIX_DEFAULT([~/atheme])

AH_TOP([
#ifndef ATHEME_INC_SYSCONF_H
#define ATHEME_INC_SYSCONF_H 1
])

AH_BOTTOM([
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
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])
AC_DEFINE([VENDOR_STRING], ["Atheme Development Group <https://atheme.github.io/>"], [Vendor string for module headers])



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
AC_CHECK_FUNCS([arc4random arc4random_buf arc4random_uniform explicit_bzero memset_s])
AC_CHECK_FUNCS([execve flock fork getpid getrlimit gettimeofday strcasestr strtok_r])
AC_CHECK_FUNC([socket], [], [AC_CHECK_LIB([socket], [socket])])
AC_CHECK_FUNC([gethostbyname], [], [AC_CHECK_LIB([nsl], [gethostbyname])])
HW_FUNC_SNPRINTF
HW_FUNC_ASPRINTF



dnl If we don't have all of arc4random*(3) then we need arc4random.c which provides implementations
ARC4RANDOM_BUILDING="No"
ARC4RANDOM_COND_C=""
AS_IF([test "x${ac_cv_func_arc4random}x${ac_cv_func_arc4random_buf}x${ac_cv_func_arc4random_uniform}" != "xyesxyesxyes"], [
	ARC4RANDOM_BUILDING="Yes"
	ARC4RANDOM_COND_C="arc4random.c"
	AC_SUBST([ARC4RANDOM_COND_C])

	AC_CHECK_HEADERS([sys/random.h], [], [], [])
	AC_CHECK_FUNCS([getentropy getrandom])
])



dnl Libraries that are autodetected (alphabetical)
ATHEME_LIBTEST_CRACKLIB
ATHEME_LIBTEST_CRYPT
ATHEME_LIBTEST_IDN
ATHEME_LIBTEST_LDAP
ATHEME_LIBTEST_MBEDTLS
ATHEME_LIBTEST_NETTLE
ATHEME_LIBTEST_OPENSSL
ATHEME_LIBTEST_PCRE
ATHEME_LIBTEST_QRENCODE
ATHEME_LIBTEST_SODIUM

dnl Libraries that need to be explicitly enabled (alphabetical)
ATHEME_LIBTEST_PERL

dnl Digest frontend to use in libathemecore
ATHEME_DECIDE_DIGEST_FRONTEND

dnl Optional features
ATHEME_FEATURETEST_CONTRIB
ATHEME_FEATURETEST_FHSPATHS
ATHEME_FEATURETEST_LARGENET
ATHEME_FEATURETEST_LEGACY_PWCRYPTO
ATHEME_FEATURETEST_REPROBUILDS
ATHEME_FEATURETEST_SODIUM_MALLOC

dnl These must be here, not above, or they might interfere with library and feature tests
dnl Explanation is that these modify one or more of CFLAGS, CPPFLAGS, LDFLAGS, and LIBS
dnl Linker tests should be first so that warning flags don't clutter the contents of config.log
dnl Mowgli test must be last or it will interfere with linker tests (the library may not yet exist)
ATHEME_FEATURETEST_LDFLAGS
ATHEME_FEATURETEST_CPPFLAGS
ATHEME_FEATURETEST_CFLAGS
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

	AX_SUBDIRS_CONFIGURE([libmowgli-2], [[--without-openssl]], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
])

AC_CONFIG_FILES([
	atheme-services.pc
	buildsys.mk
	extra.mk
])

AC_OUTPUT

ATHEME_PRINT_CONFIGURATION
