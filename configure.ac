dnl Copyright (C) 2005-2018 Atheme Development Group
dnl Rights to this code are documented in doc/LICENSE.
dnl
dnl This file is the autoconf stub.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([atheme], [7.3.0-dev], [https://github.com/atheme/atheme/issues/])

AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/sysconf.h])
AC_PREFIX_DEFAULT([~/atheme])

dnl Buildsys module stuff
BUILDSYS_INIT
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="${LIBS} ${DYNAMIC_LD_LIBS}"

AC_MSG_CHECKING([if you read GIT-Access])
if ! test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"; then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([please read GIT-Access before trying to build from git])
fi
AC_MSG_RESULT([yes])

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

if test "x${prefix}" = "xNONE"; then
	prefix="${ac_default_prefix}"
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

case "${host}" in
*-*-mingw32)
    CFLAGS="-I/mingw/include ${CFLAGS}"
    LIBS="${LIBS} -lwsock32 -lws2_32 -lregex"
        ;;
esac

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
AS_IF([test "x${enable_profile}" = "xyes"], [
	CFLAGS_SAVED="${CFLAGS}"
	ATHEME_CC_TEST_CFLAGS([-pg])
	AS_IF([test "x${ATHEME_CC_TEST_CFLAGS_RESULT}" = "xyes"], [LDFLAGS="${LDFLAGS} -pg"])
	CFLAGS="${CFLAGS_SAVED}"
])

AC_ARG_ENABLE([relro],
	[AS_HELP_STRING([--enable-relro], [Enable -Wl,-z,relro to mark the relocation table read-only])],
	[], [enable_relro="yes"])

AC_ARG_ENABLE([nonlazy-bind],
	[AS_HELP_STRING([--enable-nonlazy-bind], [Enable -Wl,-z,now to resolve all symbols at startup time])],
	[], [enable_nonlazy_bind="yes"])

AC_ARG_ENABLE([as-needed],
	[AS_HELP_STRING([--enable-as-needed], [Enable -Wl,--as-needed to strip unnecessary libraries at link time])],
	[], [enable_as_needed="yes"])

AS_IF([test "x${enable_relro}" = "xyes"], [ATHEME_CC_TEST_CFLAGS([-Wl,-z,relro])])
AS_IF([test "x${enable_nonlazy_bind}" = "xyes"], [ATHEME_CC_TEST_CFLAGS([-Wl,-z,now])])
AS_IF([test "x${enable_as_needed}" = "xyes"], [ATHEME_CC_TEST_CFLAGS([-Wl,--as-needed])])

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

d="$(eval echo "${MODDIR}")"
d="$(eval echo "${d}")"
d="$(eval echo "${d}")"
d="$(eval echo "${d}")"

echo "
Configuration:

    Atheme Version ..........: ${PACKAGE_VERSION}

    Installation Prefix .....: ${prefix}
    Module Root Directory ...: ${d}
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
    OpenSSL Support .........: ${LIBCRYPTO}
    PCRE Support ............: ${LIBPCRE}
    Perl Support ............: ${LIBPERL}
    QR Code Support .........: ${LIBQRENCODE}

    Mowgli Installation .....: ${MOWGLI_SOURCE}

    CC ......................: ${CC}
    CFLAGS ..................: ${CFLAGS}
    CPPFLAGS ................: ${CPPFLAGS}

    LD ......................: ${LD}
    LDFLAGS .................: ${LDFLAGS}

Type make to build Atheme, and make install to install it."
