dnl Copyright (C) 2005-2018 Atheme Development Group
dnl Rights to this code are documented in doc/LICENSE.
dnl
dnl This file is the autoconf stub.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_LANG([C])
AC_INIT([atheme], [7.3.0-dev], [https://github.com/atheme/atheme/issues/])

AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/sysconf.h])
AC_PREFIX_DEFAULT([~/atheme])

AC_MSG_CHECKING([if you read GIT-Access.txt])
if ! test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"; then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([please read GIT-Access.txt before trying to build from git])
fi
AC_MSG_RESULT([yes])

if test "x${prefix}" = "xNONE"; then
	prefix="${ac_default_prefix}"
fi

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
if test -z "${PKG_CONFIG}"; then
	AC_MSG_ERROR([pkg-config not found])
fi

dnl Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])
AC_DEFINE([VENDOR_STRING], ["Atheme Development Group <http://atheme.github.io>"], [Vendor and URL for modules's "vendor" field])

dnl GNU gettext
AM_GNU_GETTEXT([external])
LIBS="${LIBS} ${LIBINTL}"

dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([PREFIX], "${prefix}", [Installation prefix.])

dnl Checks for header files.
AC_CHECK_HEADERS([link.h],,,[-])

dnl Checks for library functions.
AC_CHECK_FUNCS([inet_pton inet_ntop gettimeofday umask arc4random arc4random_buf arc4random_uniform explicit_bzero memset_s getrlimit fork getpid execve strtok_r inet_ntop strcasestr flock])
AC_CHECK_FUNC([socket], [], [AC_CHECK_LIB([socket], [socket])])
AC_CHECK_FUNC([gethostbyname], [], [AC_CHECK_LIB([nsl], [gethostbyname])])
AC_SEARCH_LIBS([crypt], [crypt], [AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) is available])])
HW_FUNC_SNPRINTF
HW_FUNC_ASPRINTF



dnl If we don't have explicit_bzero(3)/memset_s(3) then we need explicit_bzero.c which provides a fallback
EXPLICIT_BZERO_C=""
AS_IF([test "x${ac_cv_func_explicit_bzero}x${ac_cv_func_memset_s}" = "xnoxno"], [
	EXPLICIT_BZERO_C="explicit_bzero.c"
	AC_SUBST([EXPLICIT_BZERO_C])
])



dnl If we're building on Windows we need socket and regex libraries from mingw
case "${host}" in
*-*-mingw32)
    CFLAGS="-I/mingw/include ${CFLAGS}"
    LIBS="${LIBS} -lwsock32 -lws2_32 -lregex"
        ;;
esac



LIBCRACKLIB="No"
CRACKLIB_C=""
CRACKLIB_LIBS=""
AC_ARG_WITH([cracklib],
	[AS_HELP_STRING([--with-cracklib], [Compile NickServ cracklib module for checking password strength])],
	[], [with_cracklib="auto"])

case "${with_cracklib}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-cracklib])
		;;
esac

AS_IF([test "x${with_cracklib}" != "xno"], [
	LIBS_SAVED="${LIBS}"
	AC_SEARCH_LIBS([FascistCheck], [crack], [
		LIBCRACKLIB="Yes"
		CRACKLIB_C="cracklib.c"
		AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"],
			[CRACKLIB_LIBS="${ac_cv_search_FascistCheck}"])
	], [
		AS_IF([test "x${with_cracklib}" != "xauto"],
			[AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found])])
	])
	LIBS="${LIBS_SAVED}"
])
AC_SUBST([CRACKLIB_C])
AC_SUBST([CRACKLIB_LIBS])



LIBCRYPTO="No"
LIBCRYPTO_LIBS=""
AC_ARG_WITH([openssl],
	[AS_HELP_STRING([--with-openssl], [Compile with OpenSSL to provide more functionality.])],
	[], [with_openssl="auto"])

case "${with_openssl}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-openssl])
		;;
esac

AS_IF([test "x${with_openssl}" != "xno"], [
	LIBS_SAVED="${LIBS}"
	AC_SEARCH_LIBS([EVP_DigestUpdate], [crypto], [
		LIBCRYPTO="Yes"
		AS_IF([test "x${ac_cv_search_EVP_DigestUpdate}" != "xnone required"],
			[LIBCRYPTO_LIBS="${ac_cv_search_EVP_DigestUpdate}"])
	], [
		AS_IF([test "x${with_openssl}" != "xauto"],
			[AC_MSG_ERROR([--with-openssl was specified but OpenSSL could not be found])])
	])
	LIBS="${LIBS_SAVED}"
])
AS_IF([test "x${LIBCRYPTO}" = "xYes"], [
	AC_CHECK_HEADERS([openssl/ec.h], [], [], [])
	AC_CHECK_HEADERS([openssl/evp.h openssl/hmac.h openssl/sha.h], [], [
		LIBCRYPTO="No"
		AS_IF([test "x${with_openssl}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
	], [])
])
AS_IF([test "x${LIBCRYPTO}" = "xYes"], [
	AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if OpenSSL is available])
	AC_SUBST([LIBCRYPTO_LIBS])
])



LIBMBEDCRYPTO="No"
LIBMBEDCRYPTO_LIBS=""
AC_ARG_WITH([mbedtls],
	[AS_HELP_STRING([--with-mbedtls], [Compile with ARM mbedTLS support.])],
	[], [with_mbedtls="auto"])

case "${with_mbedtls}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-mbedtls])
		;;
esac

AS_IF([test "x${with_mbedtls}" != "xno"], [
	LIBS_SAVED="${LIBS}"

	AC_CHECK_HEADERS([mbedtls/md.h mbedtls/pkcs5.h], [], [
		AS_IF([test "x${with_mbedtls}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
	], [])

	AC_SEARCH_LIBS([mbedtls_md_setup], [mbedcrypto], [
		AC_MSG_CHECKING([if ARM mbedTLS is functional])

		AC_RUN_IFELSE([
			AC_LANG_PROGRAM([[
				#include <mbedtls/md.h>
				#include <mbedtls/pkcs5.h>
			]], [[
				const mbedtls_md_info_t *const md_md5 = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
				const mbedtls_md_info_t *const md_sha1 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
				const mbedtls_md_info_t *const md_sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
				const mbedtls_md_info_t *const md_sha512 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);

				if (mbedtls_md_get_size(md_md5) != 0x10U)
					return 1;

				if (mbedtls_md_get_size(md_sha1) != 0x14U)
					return 2;

				if (mbedtls_md_get_size(md_sha256) != 0x20U)
					return 3;

				if (mbedtls_md_get_size(md_sha512) != 0x40U)
					return 4;

				return 0;
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBMBEDCRYPTO="Yes"
		], [
			AC_MSG_RESULT([no])
			LIBMBEDCRYPTO="No"
			AS_IF([test "x${with_mbedtls}" != "xauto"],
				[AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS is unusable for this task])])
		], [
			AC_MSG_RESULT([maybe])
			LIBMBEDCRYPTO="Yes"
			AC_MSG_WARN([--with-mbedtls was specified but we are cross-compiling])
		])
	], [
		AS_IF([test "x${with_mbedtls}" != "xauto"],
			[AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS could not be found])])
	])

	AS_IF([test "x${LIBMBEDCRYPTO}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_mbedtls_md_setup}" != "xnone required"],
			[LIBMBEDCRYPTO_LIBS="$ac_cv_search_mbedtls_md_setup"])
		AC_DEFINE([HAVE_LIBMBEDCRYPTO], [1], [Define to 1 if we have ARM mbedTLS available.])
		AC_SUBST([LIBMBEDCRYPTO_LIBS])
	])

	LIBS="${LIBS_SAVED}"
])



LIBNETTLE="No"
LIBNETTLE_LIBS=""
AC_ARG_WITH([nettle],
	[AS_HELP_STRING([--with-nettle], [Compile with libnettle (cryptographic library) support.])],
	[], [with_nettle="auto"])

case "${with_nettle}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-nettle])
		;;
esac

AS_IF([test "x${with_nettle}" != "xno"], [
	LIBS_SAVED="${LIBS}"

	AC_CHECK_HEADERS([nettle/md5.h nettle/sha1.h nettle/sha2.h nettle/nettle-meta.h], [], [
		AS_IF([test "x${with_mbedtls}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
	], [])

	AC_SEARCH_LIBS([nettle_md5_init], [nettle], [
		AC_MSG_CHECKING([if libnettle is functional])

		AC_RUN_IFELSE([
			AC_LANG_PROGRAM([[
				#include <nettle/md5.h>
				#include <nettle/sha1.h>
				#include <nettle/sha2.h>
				#include <nettle/nettle-meta.h>
			]], [[
				const struct nettle_hash *md_md5 = &nettle_md5;
				const struct nettle_hash *md_sha1 = &nettle_sha1;
				const struct nettle_hash *md_sha256 = &nettle_sha256;
				const struct nettle_hash *md_sha512 = &nettle_sha512;

				if (md_md5->digest_size != MD5_DIGEST_SIZE)
					return 1;

				if (md_sha1->digest_size != SHA1_DIGEST_SIZE)
					return 1;

				if (md_sha256->digest_size != SHA256_DIGEST_SIZE)
					return 1;

				if (md_sha512->digest_size != SHA512_DIGEST_SIZE)
					return 1;
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBNETTLE="Yes"
		], [
			AC_MSG_RESULT([no])
			LIBNETTLE="No"
			AS_IF([test "x${with_nettle}" != "xauto"],
				[AC_MSG_ERROR([--with-nettle was specified but libnettle is unusable for this task])])
		], [
			AC_MSG_RESULT([maybe])
			LIBNETTLE="Yes"
			AC_MSG_WARN([--with-nettle was specified but we are cross-compiling])
		])
	], [
		AS_IF([test "x${with_nettle}" != "xauto"],
			[AC_MSG_ERROR([--with-nettle was specified but libnettle could not be found])])
	])

	AS_IF([test "x${LIBNETTLE}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_nettle_md5_init}" != "xnone required"],
			[LIBNETTLE_LIBS="$ac_cv_search_nettle_md5_init"])
		AC_DEFINE([HAVE_LIBNETTLE], [1], [Define to 1 if we have libnettle available.])
		AC_SUBST([LIBNETTLE_LIBS])
	])

	LIBS="${LIBS_SAVED}"
])



LIBIDN="No"
LIBIDN_LIBS=""
AC_ARG_WITH([libidn],
	[AS_HELP_STRING([--with-libidn], [Compile with GNU libidn for SASL SCRAM-SHA support.])],
	[], [with_libidn="auto"])

case "${with_libidn}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-libidn])
		;;
esac

AS_IF([test "x${with_libidn}" != "xno"], [
	LIBS_SAVED="${LIBS}"
	AC_SEARCH_LIBS([stringprep], [idn], [
		LIBIDN="Yes"
		AS_IF([test "x${ac_cv_search_stringprep}" != "xnone required"],
			[LIBIDN_LIBS="${ac_cv_search_stringprep}"])
		AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if we have GNU libidn available.])
		AC_SUBST([LIBIDN_LIBS])
	], [
		AS_IF([test "x${with_libidn}" != "xauto"],
			[AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])])
	])
	LIBS="${LIBS_SAVED}"
])



AC_ARG_WITH([ldap],
	[AS_HELP_STRING([--without-ldap],[Disable building ldap auth module.])],
	[], [with_ldap="auto"])

case "${with_ldap}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-ldap])
		;;
esac

AS_IF([test "x${with_ldap}" != "xno"], [
	# Thank autoconf and openldap developers for making me do all this stuff.
	LDAP_CFLAGS=""
	LDAP_LIBS=""
	LDAP_AUTHC=""
	AC_CHECK_LIB([ldap], [ldap_initialize], [
		LDAP_CFLAGS=""
		LDAP_LIBS="-lldap"
		LDAP_AUTHC="ldap.c"
	], [
		unset ac_cv_lib_ldap_ldap_initialize
		CFLAGS_SAVED="${CFLAGS}"
		LIBS_SAVED="${LIBS}"
		CFLAGS="${CFLAGS} -I/usr/local/include"
		LIBS="${LIBS} -L/usr/local/lib"
		AC_CHECK_LIB([ldap], [ldap_initialize], [
			LDAP_CFLAGS="-I/usr/local/include"
			LDAP_LIBS="-L/usr/local/lib -lldap"
			LDAP_AUTHC="ldap.c"
		])
		CFLAGS="${CFLAGS_SAVED}"
		LIBS="${LIBS_SAVED}"
	])
])

AS_IF([test "x${with_ldap}" = "xyes" && test "x${LDAP_AUTHC}" = "x"],
	[AC_MSG_ERROR([LDAP support was explicitly requested but could not be found])])

AC_SUBST([LDAP_CFLAGS])
AC_SUBST([LDAP_LIBS])
AC_SUBST([LDAP_AUTHC])



LIBPERL="No"
AC_ARG_WITH([perl],
        [AS_HELP_STRING([--without-perl], [Disable building perl scripting module])],
        [], [with_perl="no"])


case "${with_perl}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-perl])
		;;
esac

AS_IF([test "x${with_perl}" != "xno"],
[
  AC_PATH_PROG([perlpath], [perl])

  if test -n "${perlpath}"; then
      PERL_ENABLE="perl"
      PERL_CFLAGS="$(perl -MExtUtils::Embed -e ccopts)"
      PERL_LIBS="$(perl -MExtUtils::Embed -e ldopts)"
      LIBPERL="Yes"
  fi

  dnl if Perl is built with threading support, we need to link atheme against libpthread
  if test -n "$(echo "${PERL_LIBS}" | grep pthread)"; then
     LIBS="${LIBS} -lpthread"
  fi
])

AS_IF([test "x${with_perl}" = "xyes" && test "x${PERL_CFLAGS}" = "x"],
	[AC_MSG_ERROR([Perl support was requested but Perl could not be found])])

AC_SUBST([PERL_CFLAGS])
AC_SUBST([PERL_LIBS])
AC_SUBST([PERL_ENABLE])



dnl gettextize's Makefile.in wants this
mkdir_p="mkdir -p"
AC_SUBST([mkdir_p])

DOCDIR='${prefix}/doc'
MODDIR='${exec_prefix}'
SHAREDIR='${prefix}'
LOGDIR='${prefix}/var'
DATADIR='${prefix}/etc'
RUNDIR='${prefix}/var'

FHSPATHS="no"
AC_MSG_CHECKING([if you want FHS-like pathnames])
AC_ARG_ENABLE([fhs-paths],
AS_HELP_STRING([--enable-fhs-paths], [ Use more FHS-like pathnames (for packagers)]),
[
  case "${enableval}" in
  yes)
    FHSPATHS="yes"
    DOCDIR='${datadir}/doc/atheme'
    MODDIR='${libdir}/atheme'
    SHAREDIR='${datadir}/atheme'
    LOGDIR='${localstatedir}/log/atheme'
    DATADIR='${localstatedir}/lib/atheme'
    RUNDIR='${localstatedir}/run/atheme'
    ;;
  no)
    FHSPATHS="no"
    ;;
  esac
])
AC_MSG_RESULT([${FHSPATHS}])
AC_SUBST([DOCDIR])
AC_SUBST([MODDIR])
AC_SUBST([SHAREDIR])
AC_SUBST([DATADIR])
AC_SUBST([LOGDIR])
AC_SUBST([RUNDIR])
AC_SUBST([ABSPATHS])
AC_SUBST([SRCDIR])
AC_SUBST([BUILDDIR])
dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([MODDIR], "${MODDIR}", [Module root directory.])

dnl gettext
LOCALEDIR='${datadir}/locale'
AC_SUBST([LOCALEDIR])



REPRODUCIBLE_BUILDS="No"
AC_MSG_CHECKING([if you want reproducible builds])
AC_ARG_ENABLE([reproducible-builds],
AS_HELP_STRING([--enable-reproducible-builds],[ Enable reproducible builds.]),
[
  case "${enableval}" in
  yes)
    REPRODUCIBLE_BUILDS="Yes"
    AC_DEFINE([REPRODUCIBLE_BUILDS], [1], [Uncomment to enable reproducible builds.])
    ;;
  no)
    REPRODUCIBLE_BUILDS="No"
    ;;
  esac
])
AC_MSG_RESULT([${REPRODUCIBLE_BUILDS}])



LARGE_NET="No"
AC_MSG_CHECKING([if you want large network support])
AC_ARG_ENABLE([large-net],
AS_HELP_STRING([--enable-large-net],[ Enable large network support.]),
[
  case "${enableval}" in
  yes)
    LARGE_NET="Yes"
    AC_DEFINE([LARGE_NETWORK], [1], [Uncomment to enable large network support.])
    ;;
  no)
    LARGE_NET="No"
    ;;
  esac
])
AC_MSG_RESULT([${LARGE_NET}])



CONTRIB="No"
CONTRIB_ENABLE=""
AC_MSG_CHECKING([if you want contrib modules])
AC_ARG_ENABLE([contrib],
AS_HELP_STRING([--enable-contrib],[ Enable contrib modules.]),
[
  case "${enableval}" in
  yes)
    CONTRIB="Yes"
    ;;
  no)
    CONTRIB="No"
    ;;
  esac
])
AC_MSG_RESULT([${CONTRIB}])

if test "x${CONTRIB}" = "xYes"; then
	CONTRIB_ENABLE="contrib"
fi

AC_SUBST([CONTRIB_ENABLE])



BUILD_WARNINGS="No"
AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],[ Enable compiler warnings])],
	[], [enable_warnings="no"])
AS_IF([test "x${enable_warnings}" = "xyes"], [
	ATHEME_CC_ENABLE_WARNINGS
	BUILD_WARNINGS="Yes"
])



AC_ARG_ENABLE([propolice],
	[AS_HELP_STRING([--disable-propolice],[ Disable propolice protections (for debugging.)])],
	[], [enable_propolice=no])
AS_IF([test "x${enable_propolice}" = "xyes"],
	[
	ATHEME_CC_TEST_CFLAGS([-fno-stack-protector-all])
	ATHEME_CC_TEST_CFLAGS([-fno-pie])
	ATHEME_CC_TEST_CFLAGS([-nonow])
	ATHEME_CC_TEST_CFLAGS([-norelro])
	]
)



AC_ARG_ENABLE([profile],
	[AS_HELP_STRING([--enable-profile],[ Enable profiling extensions])],
	[], [enable_profile=no])

AC_ARG_ENABLE([relro],
	[AS_HELP_STRING([--enable-relro], [Enable -Wl,-z,relro to mark the relocation table read-only])],
	[], [enable_relro="yes"])

AC_ARG_ENABLE([nonlazy-bind],
	[AS_HELP_STRING([--enable-nonlazy-bind], [Enable -Wl,-z,now to resolve all symbols at startup time])],
	[], [enable_nonlazy_bind="yes"])

AC_ARG_ENABLE([as-needed],
	[AS_HELP_STRING([--enable-as-needed], [Enable -Wl,--as-needed to strip unnecessary libraries at link time])],
	[], [enable_as_needed="yes"])

AS_IF([test "x${enable_profile}" = "xyes"], [ATHEME_LD_TEST_LDFLAGS([-pg])])
AS_IF([test "x${enable_relro}" = "xyes"], [ATHEME_LD_TEST_LDFLAGS([-Wl,-z,relro])])
AS_IF([test "x${enable_nonlazy_bind}" = "xyes"], [ATHEME_LD_TEST_LDFLAGS([-Wl,-z,now])])
AS_IF([test "x${enable_as_needed}" = "xyes"], [ATHEME_LD_TEST_LDFLAGS([-Wl,--as-needed])])



enablerpath="yes"
AC_ARG_ENABLE([rpath],
AS_HELP_STRING([--disable-rpath],[Disable use of -Wl,-rpath= during linking]),
   [enablerpath="${enableval}"], [enablerpath="yes"])

if test "x${enablerpath}" != "xyes"; then
	LDFLAGS_RPATH=""
fi



AC_ARG_WITH([libmowgli],
  AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@],[ Specify location of system libmowgli install, or "no" to force use of internal libmowgli]),
  [], [with_libmowgli=yes])

if test "x${with_libmowgli}" = "xyes"; then
  PKG_CHECK_MODULES([MOWGLI], [libmowgli-2 >= 2.0.0],
    [MOWGLI_SOURCE="System"],
    [with_libmowgli="no"])
fi

AS_IF(
  [test "x${with_libmowgli}" = "xyes"], [ ],
  [test "x${with_libmowgli}" = "xno"], [
    MOWGLI_CFLAGS="-I../libmowgli-2/src/libmowgli -I../../libmowgli-2/src/libmowgli -I../../../libmowgli-2/src/libmowgli"
    MOWGLI_LIBS="-L../libmowgli-2/src/libmowgli -L../../libmowgli-2/src/libmowgli -L../../../libmowgli-2/src/libmowgli -lmowgli-2"
    LIBMOWGLI="libmowgli-2"
    MOWGLI_SOURCE="Internal"
    AC_CONFIG_SUBDIRS([libmowgli-2])],
  [ MOWGLI_SOURCE="System"
    MOWGLI_CFLAGS="-I${with_libmowgli}/include/libmowgli-2"
    MOWGLI_LIBS="-L${with_libmowgli}/lib -lmowgli-2" ])

CPPFLAGS="${CPPFLAGS} ${MOWGLI_CFLAGS}"
LIBS="${LIBS} ${MOWGLI_LIBS}"

AC_SUBST([MOWGLI_CFLAGS])
AC_SUBST([MOWGLI_LIBS])
AC_SUBST([LIBMOWGLI])



LIBPCRE="No"
AC_ARG_WITH([pcre],
	[AS_HELP_STRING([--with-pcre], [Enable Perl-Compatible Regular Expression support])],
	[], [with_pcre="auto"])

case "${with_pcre}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-pcre])
		;;
esac

AS_IF([test "x${with_pcre}" != "xno"], [
	PKG_CHECK_MODULES([PCRE], [libpcre], [
		LIBPCRE="Yes"
		AC_DEFINE([HAVE_PCRE], [1], [Define to 1 if PCRE is available.])
		AC_SUBST([PCRE_CFLAGS])
		AC_SUBST([PCRE_LIBS])
	], [
		AS_IF([test "x${with_pcre}" != "xauto"], [
		      AC_MSG_ERROR([--with-pcre was specified but the PCRE library could not be found])
		])
	])
])



LIBQRENCODE="No"
QRCODE_C=""
AC_ARG_WITH([qrencode],
	[AS_HELP_STRING([--with-qrencode], [Compile with libqrencode for generating QR codes])],
	[], [with_qrencode="auto"])

case "${with_qrencode}" in
	no | yes | auto)
		;;
	*)
		AC_MSG_ERROR([invalid option for --with-qrencode])
		;;
esac

AS_IF([test "x${with_qrencode}" != "xno"], [
	PKG_CHECK_MODULES([LIBQRENCODE], [libqrencode], [
		LIBQRENCODE="Yes"
		QRCODE_C="qrcode.c"
		AC_SUBST([QRCODE_C])
		AC_DEFINE([HAVE_LIBQRENCODE], [1], [Define to 1 if libqrencode is available])
	], [
		LIBQRENCODE="No"
		AS_IF([test "x${with_qrencode}" = "xyes"], [
			AC_MSG_ERROR([--with-qrencode was specified but libqrencode could not be found])
		])
	])
])



BUILDROOT=`pwd`
AC_SUBST([BUILDROOT])

PODIR=""
USE_GETTEXT="No"
if test "x${USE_NLS}" = "xyes"; then
	PODIR="po"
	USE_GETTEXT="Yes"
fi
AC_SUBST([PODIR])



dnl Digest interface to use
DIGEST_API_LIBS=""
DIGEST_FRONTEND_H="digest_fe_internal.h"
DIGEST_FRONTEND_C="digest_fe_internal.c"
DIGEST_FRONTEND_C="${DIGEST_FRONTEND_C} digest_be_md5.c"
DIGEST_FRONTEND_C="${DIGEST_FRONTEND_C} digest_be_sha1.c"
DIGEST_FRONTEND_C="${DIGEST_FRONTEND_C} digest_be_sha2_256.c"
DIGEST_FRONTEND_C="${DIGEST_FRONTEND_C} digest_be_sha2_512.c"
DIGEST_FRONTEND="Internal"

AS_IF([test "x${DIGEST_FRONTEND}x${LIBMBEDCRYPTO}" = "xInternalxYes"], [
	DIGEST_API_LIBS="${LIBMBEDCRYPTO_LIBS}"
	DIGEST_FRONTEND_H="digest_fe_mbedtls.h"
	DIGEST_FRONTEND_C="digest_fe_mbedtls.c"
	DIGEST_FRONTEND="ARM mbedTLS"
])

AS_IF([test "x${DIGEST_FRONTEND}x${LIBNETTLE}" = "xInternalxYes"], [
	DIGEST_API_LIBS="${LIBNETTLE_LIBS}"
	DIGEST_FRONTEND_H="digest_fe_nettle.h"
	DIGEST_FRONTEND_C="digest_fe_nettle.c"
	DIGEST_FRONTEND="Nettle"
])

AS_IF([test "x${DIGEST_FRONTEND}x${LIBCRYPTO}" = "xInternalxYes"], [
	DIGEST_API_LIBS="${LIBCRYPTO_LIBS}"
	DIGEST_FRONTEND_H="digest_fe_openssl.h"
	DIGEST_FRONTEND_C="digest_fe_openssl.c"
	DIGEST_FRONTEND="OpenSSL"
])

AC_DEFINE_UNQUOTED([DIGEST_FE_HEADER], ["${DIGEST_FRONTEND_H}"], [Front-end header for digest interface])
AC_SUBST([DIGEST_FRONTEND_C])
AC_SUBST([DIGEST_API_LIBS])



CPPFLAGS="${CPPFLAGS} -DPREFIX=\\\"\${prefix}\\\" -DBINDIR=\\\"\${bindir}\\\" -DLOCALEDIR=\\\"\${LOCALEDIR}\\\" -DMODDIR=\\\"\${MODDIR}\\\" -DSHAREDIR=\\\"${SHAREDIR}\\\" -DSYSCONFDIR=\\\"${sysconfdir}\\\" -DLOGDIR=\\\"${LOGDIR}\\\" -DRUNDIR=\\\"${RUNDIR}\\\" -DDATADIR=\\\"${DATADIR}\\\""
LDFLAGS="${LDFLAGS} ${LDFLAGS_RPATH}"

AC_SUBST([CPPFLAGS])
AC_SUBST([LDFLAGS])



BUILDSYS_TOUCH_DEPS

AC_CONFIG_FILES([
	buildsys.mk
	extra.mk
	atheme-services.pc
])
AC_OUTPUT



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
Configuration:

    Atheme Version ..........: ${PACKAGE_VERSION}

    Installation Prefix .....: ${prefix}
    Binary Directory ........: ${bindir}
    Module Directory ........: ${MODDIR}/modules
    Config Directory ........: ${sysconfdir}
    Logfile Directory .......: ${LOGDIR}
    Data Directory ..........: ${DATADIR}
    PID Directory ...........: ${RUNDIR}

    Contrib Modules .........: ${CONTRIB}
    Internationalization ....: ${USE_GETTEXT}
    Large Network Support ...: ${LARGE_NET}
    Reproducible Builds .....: ${REPRODUCIBLE_BUILDS}
    Build Warnings ..........: ${BUILD_WARNINGS}

    CrackLib Support ........: ${LIBCRACKLIB}
    GNU libidn Support ......: ${LIBIDN}
    ARM mbedTLS Support .....: ${LIBMBEDCRYPTO}
    Nettle Support ..........: ${LIBNETTLE}
    OpenSSL Support .........: ${LIBCRYPTO}
    PCRE Support ............: ${LIBPCRE}
    Perl Support ............: ${LIBPERL}
    QR Code Support .........: ${LIBQRENCODE}

    Digest Frontend .........: ${DIGEST_FRONTEND}
    Mowgli Installation .....: ${MOWGLI_SOURCE}

    CC ......................: ${CC}
    CFLAGS ..................: ${CFLAGS}
    CPPFLAGS ................: ${CPPFLAGS}

    LD ......................: ${LD}
    LDFLAGS .................: ${LDFLAGS}

Type make to build Atheme, and make install to install it."
