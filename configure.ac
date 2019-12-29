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
AC_CONFIG_HEADER([include/atheme/sysconf.h])
AC_CONFIG_SRCDIR([src/services/main.c])
AC_PREFIX_DEFAULT([~/atheme])
AS_IF([test "x${prefix}" = "xNONE"], [prefix="${ac_default_prefix}"])

AH_TOP([
#ifndef ATHEME_INC_SYSCONF_H
#define ATHEME_INC_SYSCONF_H 1

#define ATHEME_API_DIGEST_FRONTEND_INTERNAL 0x00U
#define ATHEME_API_DIGEST_FRONTEND_MBEDTLS  0x01U
#define ATHEME_API_DIGEST_FRONTEND_NETTLE   0x02U
#define ATHEME_API_DIGEST_FRONTEND_OPENSSL  0x03U

#define ATHEME_API_RANDOM_FRONTEND_INTERNAL 0x00U
#define ATHEME_API_RANDOM_FRONTEND_MBEDTLS  0x01U
#define ATHEME_API_RANDOM_FRONTEND_OPENBSD  0x02U
#define ATHEME_API_RANDOM_FRONTEND_SODIUM   0x03U
#define ATHEME_API_RANDOM_FRONTEND_OPENSSL  0x04U
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

AC_MSG_CHECKING([if you read GIT-Access.txt])
AS_IF([test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"], [
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([please read GIT-Access.txt before trying to build from git])
])

AS_IF([test "x${CFLAGS+set}" = "xset"], [
	CFLAGS_WAS_SET="yes"
], [
	CFLAGS_WAS_SET="no"
	CFLAGS=""
])

dnl Programs required by the build system
AC_PROG_CC
AC_PROG_CC_C99
AC_PATH_PROG([AR], [ar])
AC_PROG_AWK
AC_PATH_PROG([CP], [cp])
AC_PROG_GREP
AC_PROG_EGREP
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
AS_IF([test -z "${PKG_CONFIG}"], [AC_MSG_ERROR([pkg-config not found])])

dnl Initialise our build system
BUILDSYS_INIT
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="${LIBS} ${DYNAMIC_LD_LIBS}"

dnl Checks for headers, libraries, and library functions
LIBS_SAVED="${LIBS}"
AC_DEFINE([__STDC_WANT_LIB_EXT1__], [1], [We want ISO 9899:2011 Annex K memory functions if available])
AC_USE_SYSTEM_EXTENSIONS
AC_CHECK_HEADERS([ctype.h dirent.h errno.h inttypes.h libintl.h limits.h locale.h netdb.h netinet/in.h], [], [], [])
AC_CHECK_HEADERS([regex.h signal.h stdarg.h stdbool.h stddef.h stdint.h stdio.h stdlib.h string.h], [], [], [])
AC_CHECK_HEADERS([strings.h sys/file.h sys/resource.h sys/stat.h sys/time.h sys/types.h sys/wait.h], [], [], [])
AC_CHECK_HEADERS([time.h unistd.h], [], [], [])
AC_CHECK_FUNCS([crypt_checkpass crypt_newhash explicit_bzero fork getrlimit memset_s timingsafe_bcmp], [], [])
AC_CHECK_FUNCS([timingsafe_memcmp], [], [])
AC_CHECK_FUNCS([dup2 execve flock gethostbyname getpid], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([gettimeofday localeconv memchr memmove memset], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([regcomp regerror regexec regfree setenv], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([setlocale strcasecmp strcasestr strchr], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([strerror strlen strncasecmp strnlen strpbrk], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([strrchr strstr strtod strtok strtok_r strtol], [], [AC_MSG_ERROR([required function not available])])
AC_CHECK_FUNCS([strtold strtoul strtoull], [], [AC_MSG_ERROR([required function not available])])
HW_FUNC_ASPRINTF
HW_FUNC_SNPRINTF
LIBS="${LIBS_SAVED}"

dnl Checks for features required of our toolchain
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_LONG_LONG_INT
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_CHECK_TYPES([ptrdiff_t])
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

dnl Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_SUBST([VENDOR_STRING], ["Atheme Development Group <https://atheme.github.io/>"])
AC_SUBST([PACKAGE_BUGREPORT_I18N], ["ircs://chat.freenode.net:6697/atheme-i18n"])
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])
AC_DEFINE_UNQUOTED([VENDOR_STRING], ["${VENDOR_STRING}"], [Vendor string for module headers])
AC_DEFINE_UNQUOTED([PACKAGE_BUGREPORT_I18N], ["${PACKAGE_BUGREPORT_I18N}"], [Where to report translation bugs])



dnl If we're building on Windows we need socket and regex libraries from mingw
case "${host}" in
	*-*-mingw32)
		CPPFLAGS="-I/mingw/include ${CPPFLAGS}"
		LIBS="-lwsock32 -lws2_32 -lregex ${LIBS}"
		;;
esac



dnl GNU gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_NEED([need-ngettext])
AM_GNU_GETTEXT_NEED([need-formatstring-macros])
AS_IF([test "x${USE_NLS}" = "xyes"], [
	ATHEME_COND_NLS_ENABLE
	LIBS="${LIBINTL} ${LIBS}"
])



dnl This should be before any library tests so that insufficient header
dnl inclusions in those tests are caught before they become a runtime
dnl problem
ATHEME_CC_TEST_CFLAGS([-Werror=implicit])

dnl Conditional libraries for standard functions (no option to control detection)
ATHEME_LIBTEST_DL
ATHEME_LIBTEST_MATH
ATHEME_LIBTEST_RT
ATHEME_LIBTEST_SOCKET

dnl Libraries that are autodetected (alphabetical)
ATHEME_LIBTEST_ARGON2
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
ATHEME_FEATURETEST_HEAP_ALLOCATOR
ATHEME_FEATURETEST_LARGENET
ATHEME_FEATURETEST_LEGACY_PWCRYPTO
ATHEME_FEATURETEST_REPROBUILDS
ATHEME_FEATURETEST_SODIUM_MALLOC

dnl Digest and RNG frontend to use in libathemecore
ATHEME_DECIDE_DIGEST_FRONTEND
ATHEME_DECIDE_RANDOM_FRONTEND

dnl These must be here, not above, or they might interfere with library and feature tests
dnl Explanation is that these modify one or more of CFLAGS, CPPFLAGS, LDFLAGS, and LIBS
dnl Warnings should be last so that diagnostics don't clutter the contents of config.log
ATHEME_FEATURETEST_CPPFLAGS
ATHEME_FEATURETEST_CFLAGS
ATHEME_FEATURETEST_DEBUGGING
ATHEME_FEATURETEST_LDFLAGS
ATHEME_FEATURETEST_WARNINGS

dnl Save toolchain variables in case we will be configuring submodules
SUBCONF_CC="${CC}"
SUBCONF_LD="${LD}"
SUBCONF_CFLAGS="${CFLAGS}"
SUBCONF_CPPFLAGS="${CPPFLAGS}"
SUBCONF_LDFLAGS="${LDFLAGS}"
SUBCONF_LIBS="${LIBS}"

dnl Mowgli test must be last or it will interfere with linker tests (the library may not yet exist)
dnl It would also pollute CPPFLAGS, LDFLAGS and/or LIBS above
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

	AX_SUBDIRS_CONFIGURE([libmowgli-2], [
		[--without-openssl], [CC="\${SUBCONF_CC}"], [LD="\${SUBCONF_LD}"], [CFLAGS="\${SUBCONF_CFLAGS}"],
		[CPPFLAGS="\${SUBCONF_CPPFLAGS}"], [LDFLAGS="\${SUBCONF_LDFLAGS}"], [LIBS="\${SUBCONF_LIBS}"]
	], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
])

AC_CONFIG_FILES([
	buildsys.mk
	extra.mk
])

AC_OUTPUT

ATHEME_PRINT_CONFIGURATION
