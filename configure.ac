dnl Copyright (c) 2005 Atheme Development Group
dnl Rights to this code are documented in doc/LICENSE.
dnl
dnl This file is the autoconf stub.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([atheme], [7.2.12], [https://github.com/atheme/atheme/issues/])

AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/sysconf.h])
AC_PREFIX_DEFAULT([~/atheme])

dnl Buildsys module stuff
BUILDSYS_INIT
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="${LIBS} ${DYNAMIC_LD_LIBS}"

if test -d ".git"; then
	AC_MSG_CHECKING([if you read GIT-Access.txt])
	if ! test -f "libmowgli-2/Makefile" -a -f "modules/contrib/Makefile"; then
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([please read GIT-Access.txt before trying to build from git])
	fi
	AC_MSG_RESULT([yes])
else
	AC_MSG_CHECKING([if you downloaded the correct source code tarball])
	if ! test -f "include/serno.h"; then
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([please read the Obtaining Atheme section of README.md])
	fi
	AC_MSG_RESULT([yes])
fi

AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([MV], [mv])
AC_PATH_PROG([CP], [cp])
AC_PATH_PROG([LN], [ln])
AC_PROG_LN_S
AC_PATH_PROG([TAR], [tar])
AC_PATH_PROG([AR], [ar])
AC_PATH_PROG([RANLIB], [ranlib])
AC_PROG_SED

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
AC_DEFINE_UNQUOTED([PACKAGE], ["$PACKAGE"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["$VERSION"], [Version number of package])
AC_DEFINE([VENDOR_STRING], ["Atheme Development Group <http://atheme.github.io>"], [Vendor and URL for modules's "vendor" field])

dnl GNU gettext

dnl Uncomment this if we choose to provide our own libintl.
AM_GNU_GETTEXT([external])
LIBS="${LIBS} ${LIBINTL}"

if test "x${prefix}" = "xNONE"; then
	prefix="${ac_default_prefix}"
fi

dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([PREFIX], "${prefix}", [Installation prefix.])

dnl Checks for header files.
AC_CHECK_HEADERS([link.h],,,[-])

dnl Checks for library functions.
AC_CHECK_FUNCS([inet_pton inet_ntop gettimeofday umask arc4random arc4random_buf arc4random_uniform explicit_bzero memset_s getrlimit fork getpid execve strtok_r inet_ntop strcasestr flock])
AC_CHECK_FUNC([socket], [], AC_CHECK_LIB([socket], [socket]))
AC_CHECK_FUNC([gethostbyname], [], AC_CHECK_LIB([nsl], [gethostbyname]))
AC_SEARCH_LIBS([crypt], [crypt], [AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) is available])])
HW_FUNC_SNPRINTF
HW_FUNC_ASPRINTF

CRACKLIB_C=""
CRACKLIB_LIBS=""
AC_ARG_WITH([cracklib],
	[AS_HELP_STRING([--with-cracklib], [Compile NickServ cracklib module for checking password strength])],
	[], [with_cracklib="auto"])
AS_IF([test "x${with_cracklib}" != "xno"], [
	LIBS_save="${LIBS}"
	AC_SEARCH_LIBS([FascistCheck], [crack], [
		CRACKLIB_C="cracklib.c"
		AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"],
			[CRACKLIB_LIBS="$ac_cv_search_FascistCheck"])
	], [
		AS_IF([test "x${with_cracklib}" != "xauto"],
			[AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found])])
	])
	LIBS="${LIBS_save}"
])
AC_SUBST([CRACKLIB_C])
AC_SUBST([CRACKLIB_LIBS])

LIBIDN="no"
LIBIDN_LIBS=""
AC_ARG_WITH([libidn],
	[AS_HELP_STRING([--with-libidn], [Compile with GNU libidn for SASL SCRAM-SHA support.])],
	[], [with_libidn="auto"])
AS_IF([test "x${with_libidn}" != "xno"], [
	LIBS_save="${LIBS}"
	AC_SEARCH_LIBS([stringprep], [idn], [
		LIBIDN="yes"
		AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if we have GNU libidn available.])
		AS_IF([test "x${ac_cv_search_stringprep}" != "xnone required"],
			[LIBIDN_LIBS="$ac_cv_search_stringprep"])
	], [
		AS_IF([test "x${with_libidn}" != "xauto"],
			[AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])])
	])
	LIBS="${LIBS_save}"
])
AC_SUBST([LIBIDN_LIBS])

AC_ARG_WITH([ldap],
	[AS_HELP_STRING([--without-ldap],[Disable building ldap auth module.])],
	[], [with_ldap="auto"])
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
		saveCFLAGS="${CFLAGS}"
		saveLIBS="${LIBS}"
		CFLAGS="${CFLAGS} -I/usr/local/include"
		LIBS="${LIBS} -L/usr/local/lib"
		AC_CHECK_LIB([ldap], [ldap_initialize], [
			LDAP_CFLAGS="-I/usr/local/include"
			LDAP_LIBS="-L/usr/local/lib -lldap"
			LDAP_AUTHC="ldap.c"
		])
		CFLAGS="${saveCFLAGS}"
		LIBS="${saveLIBS}"
	])
])

AS_IF([test "x${with_ldap}" = "xyes" && test "x${LDAP_AUTHC}" = "x"],
	[AC_MSG_ERROR([LDAP support was explicitly requested but could not be found])])

AC_SUBST(LDAP_CFLAGS)
AC_SUBST(LDAP_LIBS)
AC_SUBST(LDAP_AUTHC)

AC_ARG_WITH([perl],
        [AS_HELP_STRING([--without-perl], [Disable building perl scripting module])],
        [], [with_perl="no"])

AS_IF([test "x${with_perl}" != "xno"],
[
  AC_PATH_PROG(perlpath, perl)
  if test -n "${perlpath}"; then
      PERL_ENABLE=perl
      PERL_CFLAGS=`perl -MExtUtils::Embed -e ccopts`
      PERL_LIBS=`perl -MExtUtils::Embed -e ldopts`
  fi

  dnl if Perl is built with threading support, we need to link atheme against libpthread
  if test -n "`echo "$PERL_LIBS" | grep pthread`"; then
     LIBS="$LIBS -lpthread"
  fi
])

AS_IF([test "x${with_perl}" = "xyes" && test "x${PERL_CFLAGS}" = "x"],
	[AC_MSG_ERROR([Perl support was requested but Perl could not be found])])

AC_SUBST(PERL_CFLAGS)
AC_SUBST(PERL_LIBS)
AC_SUBST(PERL_ENABLE)

dnl gettextize's Makefile.in wants this
mkdir_p="mkdir -p"
AC_SUBST(mkdir_p)

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
  case "$enableval" in
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
AC_MSG_RESULT([$FHSPATHS])
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

REPRODUCIBLEBUILDS="no"
AC_MSG_CHECKING([if you want reproducible builds])
AC_ARG_ENABLE([reproducible-builds],
AS_HELP_STRING([--enable-reproducible-builds],[ Enable reproducible builds.]),
[
  case "$enableval" in
  yes)
    REPRODUCIBLEBUILDS="yes"
    AC_DEFINE([REPRODUCIBLE_BUILDS], [1], [Uncomment to enable reproducible builds.])
    ;;
  no)
    REPRODUCIBLEBUILDS="no"
    ;;
  esac
])
AC_MSG_RESULT([$REPRODUCIBLEBUILDS])

LARGENET="no"
AC_MSG_CHECKING([if you want large network support])
AC_ARG_ENABLE([large-net],
AS_HELP_STRING([--enable-large-net],[ Enable large network support.]),
[
  case "$enableval" in
  yes)
    LARGENET="yes"
    AC_DEFINE([LARGE_NETWORK], [1], [Uncomment to enable large network support.])
    ;;
  no)
    LARGENET="no"
    ;;
  esac
])
AC_MSG_RESULT([$LARGENET])

CONTRIB="no"
AC_MSG_CHECKING([if you want contrib modules])
AC_ARG_ENABLE([contrib],
AS_HELP_STRING([--enable-contrib],[ Enable contrib modules.]),
[
  case "$enableval" in
  yes)
    CONTRIB="yes"
    ;;
  no)
    CONTRIB="no"
    ;;
  esac
])
AC_MSG_RESULT([$CONTRIB])

AC_ARG_ENABLE([ssl],
	AS_HELP_STRING([--disable-ssl],[don't use OpenSSL to provide more SASL mechanisms]),
	[], [enable_ssl=yes])

SSL=no
if test "x${enable_ssl}" = "xyes"; then
	AC_CHECK_LIB([ssl], [SSL_read], [
		AC_CHECK_HEADERS([openssl/ssl.h openssl/err.h], [
			SSL_LIBS="-lssl -lcrypto"
			AC_SUBST([SSL_LIBS])
			SSL=yes
			],[SSL=no],[-])
		], [
			AC_CHECK_LIB([eay32], [DH_generate_key], [
				AC_CHECK_HEADERS(openssl/ssl.h openssl/err.h, [
					# Ugh.
					SSL_LIBS="-leay32 /mingw/lib/ssleay32.a"
					AC_SUBST([SSL_LIBS])
					SSL=yes
					],[SSL=no],[-])
			], [], [])
		], [-lcrypto])
	if test "x${SSL}" = "xyes"; then
		AC_CHECK_HEADERS([openssl/ec.h], [], [], []) # Defines HAVE_OPENSSL_EC_H
		AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if OpenSSL is available])
	fi
fi

case "$host" in
*-*-mingw32)
    CFLAGS="-I/mingw/include ${CFLAGS}"
    LIBS="${LIBS} -lwsock32 -lws2_32 -lregex"
        ;;
esac

AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],[ Enable compiler warnings])],
	[], [enable_warnings=no])
AS_IF([test "x${enable_warnings}" = "xyes"], [ATHEME_CC_ENABLE_WARNINGS])

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
AS_IF([test "x${enable_profile}" = "xyes"],
	[
	ATHEME_CC_TEST_CFLAGS([-pg])
	ac_save_LDFLAGS="${LDFLAGS} -pg"
	LDFLAGS="${ac_save_LDFLAGS}"
	]
)

enablerpath="yes"
AC_ARG_ENABLE([rpath],
AS_HELP_STRING([--disable-rpath],[Disable use of -Wl,-rpath= during linking]),
   [enablerpath="${enableval}"], [enablerpath="yes"])

if test "x${enablerpath}" != "xyes"; then
	LDFLAGS_RPATH=""
fi

PKG_CHECK_MODULES(LIBQRENCODE, [libqrencode], [LIBQRENCODE="yes"], [LIBQRENCODE="no"])

if test "x${LIBQRENCODE}" = "xyes"; then
	AC_DEFINE([HAVE_LIBQRENCODE], [1], [Define to 1 if libqrencode is available])
fi

AC_ARG_WITH([libmowgli],
    AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@], [Specify location of system libmowgli install, "yes" to ask pkg-config (default), or "no" to force use of internal libmowgli submodule]),
    [], [with_libmowgli=yes])

AS_IF([test "x${with_libmowgli}" = "xyes"], [
    AS_IF([test -n "${PKG_CONFIG}"], [
        PKG_CHECK_MODULES([LIBMOWGLI], [libmowgli-2 >= 2.0.0], [], [with_libmowgli="no"])
    ], [
        AC_MSG_WARN([pkg-config is not available, falling back to internal libmowgli submodule])
        with_libmowgli="no"
    ])
])

AS_IF([test "x${with_libmowgli}" = "xyes"], [
    MOWGLI_SOURCE="System (pkg-config)"
], [test "x${with_libmowgli}" = "xno"], [
    MOWGLI_SOURCE="Internal"
    LIBMOWGLI_CFLAGS="-I$(pwd)/libmowgli-2/src/libmowgli"
    LIBMOWGLI_LIBS="-L$(pwd)/libmowgli-2/src/libmowgli -lmowgli-2"
], [test -d "${with_libmowgli}/include/libmowgli-2" -a -d "${with_libmowgli}/lib"], [
    MOWGLI_SOURCE="System (${with_libmowgli})"
    LIBMOWGLI_CFLAGS="-I${with_libmowgli}/include/libmowgli-2"
    LIBMOWGLI_LIBS="-L${with_libmowgli}/lib -lmowgli-2"
], [
    AC_MSG_ERROR([${with_libmowgli} is not a suitable directory for libmowgli])
])

AC_SUBST([LIBMOWGLI_CFLAGS])
AC_SUBST([LIBMOWGLI_LIBS])



AC_ARG_WITH([pcre],
AS_HELP_STRING([--with-pcre],[ Enable PCRE regular expression support]),
[],
[with_pcre=no])

if test "x${with_pcre}" != "xno"; then
	PKG_CHECK_MODULES([LIBPCRE], [libpcre], [], [AC_MSG_ERROR(PCRE requested, but not found)])
	AC_SUBST([LIBPCRE_CFLAGS])
	AC_SUBST([LIBPCRE_LIBS])
	AC_DEFINE([HAVE_PCRE], [1], [Define if you want to use PCRE])
fi



AS_IF([test "${MOWGLI_SOURCE}" = "Internal"], [

    AC_SUBST([SUBMODULE_LIBMOWGLI], [libmowgli-2])

    # Run ./configure in the libmowgli submodule directory but without OpenSSL support
    # Atheme does not use the OpenSSL VIO interface in Mowgli and this avoids an unnecessary -lssl

    AX_SUBDIRS_CONFIGURE([libmowgli-2], [
        [--without-openssl],
        [CC="\${CC}"],
        [LD="\${LD}"],
        [CFLAGS="\${CFLAGS}"],
        [CPPFLAGS="\${CPPFLAGS}"],
        [LDFLAGS="\${LDFLAGS}"],
        [LIBS="\${LIBS}"]
    ], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
])

AS_IF([test "x${CONTRIB}" = "xyes"], [

    AC_SUBST([SUBMODULE_CONTRIB], [contrib])

    AX_SUBDIRS_CONFIGURE([modules/contrib], [
        [CC="\${CC}"],
        [LD="\${LD}"],
        [CFLAGS="\${CFLAGS}"],
        [CPPFLAGS="\${CPPFLAGS}"],
        [LDFLAGS="\${LDFLAGS}"],
        [LIBS="\${LIBS}"]
    ], [[]], [[]], [[]])
])



# Now add libmowgli and libpcre to CFLAGS and LIBS because the entire codebase uses them
AS_IF([test "x${LIBMOWGLI_CFLAGS}" != "x"], [CFLAGS="${LIBMOWGLI_CFLAGS} ${CFLAGS}"])
AS_IF([test "x${LIBMOWGLI_LIBS}" != "x"], [LIBS="${LIBMOWGLI_LIBS} ${LIBS}"])
AS_IF([test "x${LIBPCRE_CFLAGS}" != "x"], [CFLAGS="${LIBPCRE_CFLAGS} ${CFLAGS}"])
AS_IF([test "x${LIBPCRE_LIBS}" != "x"], [LIBS="${LIBPCRE_LIBS} ${LIBS}"])



PODIR=""
if test "x${USE_NLS}" = "xyes"; then
	PODIR="po"
fi
AC_SUBST([PODIR])

BUILDSYS_TOUCH_DEPS

AC_CONFIG_FILES([
	buildsys.mk
	extra.mk
	atheme-services.pc
])
AC_OUTPUT

echo "
Configuration:
	Atheme version       : ${PACKAGE_VERSION}
	Installation prefix  : ${prefix}
	Module root directory: ${MODDIR}/modules
	Config directory     : ${sysconfdir}
	Logfile directory    : ${LOGDIR}
	Data directory       : ${DATADIR}
	PID directory        : ${RUNDIR}
	Reproducible builds  : ${REPRODUCIBLEBUILDS}
	Large network support: ${LARGENET}
	OpenSSL support      : ${SSL}
	GNU libidn support   : ${LIBIDN}
	Contrib modules      : ${CONTRIB}
	Mowgli installation  : ${MOWGLI_SOURCE}
	PCRE support         : ${with_pcre}
	Perl support         : ${with_perl}
	QR Code support      : ${LIBQRENCODE}
	CFLAGS               : ${CFLAGS}
	Internationalization : ${USE_NLS}

Type make to build Atheme, and make install to install it.
"
