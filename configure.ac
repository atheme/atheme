dnl Copyright (c) 2005 Atheme Development Group
dnl Rights to this code are documented in doc/LICENSE.
dnl
dnl This file is the autoconf stub.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(atheme, 7.1.0, bugs@atheme.org)

AC_CONFIG_AUX_DIR(autoconf)

AC_CONFIG_HEADER(include/sysconf.h)
AC_PREFIX_DEFAULT(~/atheme)

dnl Buildsys module stuff
BUILDSYS_INIT
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="$LIBS $DYNAMIC_LD_LIBS"

AC_PATH_PROG(RM, rm)
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(LN, ln)
AC_PROG_LN_S
AC_PATH_PROG(TAR, tar)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(RANLIB, ranlib)

AC_PROG_CC_C99
AC_GNU_SOURCE
AC_C_BIGENDIAN
AC_PROG_MAKE_SET

PKG_PROG_PKG_CONFIG
if test -z "$PKG_CONFIG"; then
	AC_ERROR([pkg-config not found])
fi

dnl Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])

dnl GNU gettext

dnl Uncomment this if we choose to provide our own libintl.
AM_GNU_GETTEXT([external])
LIBS="${LIBS} ${LIBINTL}"

if test "$prefix" = "NONE"; then
	prefix="${ac_default_prefix}"
fi

dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([PREFIX], "${prefix}", [Installation prefix.])

dnl Checks for header files.
AC_CHECK_HEADERS(link.h,,,[-])

dnl Checks for library functions.
AC_CHECK_FUNCS([inet_pton inet_ntop gettimeofday umask arc4random getrlimit fork getpid execve strtok_r inet_ntop strcasestr])
AC_CHECK_FUNC(socket,, AC_CHECK_LIB(socket, socket))
AC_CHECK_FUNC(gethostbyname,, AC_CHECK_LIB(nsl, gethostbyname))
AC_SEARCH_LIBS(crypt, crypt, [AC_DEFINE([HAVE_CRYPT], [], [Define if crypt() is available])])
HW_FUNC_SNPRINTF
HW_FUNC_ASPRINTF

CRACKLIB_C=""
CRACKLIB_LIBS=""
AC_ARG_WITH([cracklib],
	[AS_HELP_STRING([--with-cracklib], [Compile cracklib nickserv submodule for checking password strength.])],
	[],
	[with_cracklib="auto"])
AS_IF([test "x$with_cracklib" != "xno"],
	[LIBS_save="$LIBS"
	AC_SEARCH_LIBS([FascistCheck], [crack],
		[CRACKLIB_C="cracklib.c"
		AS_IF([test "x$ac_cv_search_FascistCheck" != "xnone required"],
			[CRACKLIB_LIBS="$ac_cv_search_FascistCheck"])],
		[AS_IF([test "x$with_cracklib" != "xauto"],
			[AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found.])])])
	LIBS="$LIBS_save"])
AC_SUBST(CRACKLIB_C)
AC_SUBST([CRACKLIB_LIBS])

AC_ARG_WITH([ldap],
	[AS_HELP_STRING([--without-ldap],[Disable building ldap auth module.])],
	[],
	[with_ldap="auto"])

AS_IF([test "x$with_ldap" != "xno"],
[
# Thank autoconf and openldap developers for making me do all this stuff.
LDAP_CFLAGS=""
LDAP_LIBS=""
LDAP_AUTHC=""
AC_CHECK_LIB(ldap, ldap_initialize, [
	LDAP_CFLAGS=""
	LDAP_LIBS="-lldap"
	LDAP_AUTHC="ldap.c"
], [
	unset ac_cv_lib_ldap_ldap_initialize
	saveCFLAGS="$CFLAGS"
	saveLIBS="$LIBS"
	CFLAGS="$CFLAGS -I/usr/local/include"
	LIBS="$LIBS -L/usr/local/lib"
	AC_CHECK_LIB(ldap, ldap_initialize, [
		LDAP_CFLAGS="-I/usr/local/include"
		LDAP_LIBS="-L/usr/local/lib -lldap"
		LDAP_AUTHC="ldap.c"
	])
	CFLAGS="$saveCFLAGS"
	LIBS="$saveLIBS"
])
dnl with_ldap != no
])

AS_IF([test "x$with_ldap" = "xyes" && test "x$LDAP_AUTHC" = "x"],
[AC_MSG_ERROR([LDAP support was explicitly requested but could not be found.])])

AC_SUBST(LDAP_CFLAGS)
AC_SUBST(LDAP_LIBS)
AC_SUBST(LDAP_AUTHC)

AC_ARG_WITH([perl],
        [AS_HELP_STRING([--without-perl],[Disable building perl scripting module.])],
        [],
        [with_perl="no"])

AS_IF([test "x$with_perl" != "xno"],
[
  AC_PATH_PROG(perlpath, perl)
  if test -n "$perlpath"; then
      PERL_ENABLE=perl
      PERL_CFLAGS=`perl -MExtUtils::Embed -e ccopts`
      PERL_LIBS=`perl -MExtUtils::Embed -e ldopts`
  fi

  dnl if Perl is built with threading support, we need to link atheme against libpthread
  if test -n "`echo "$PERL_LIBS" | grep pthread`"; then
     LIBS="$LIBS -lpthread"
  fi
])

AS_IF([test "x$with_perl" = "xyes" && test "x$PERL_CFLAGS" = "x"],
[AC_MSG_ERROR([Perl support was requested but Perl could not be found.])])

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
AC_MSG_CHECKING(if you want FHS-like pathnames)
AC_ARG_ENABLE(fhs-paths,
AC_HELP_STRING([--enable-fhs-paths],[ Use more FHS-like pathnames (for packagers).]),
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
AC_MSG_RESULT($FHSPATHS)
AC_SUBST(DOCDIR)
AC_SUBST(MODDIR)
AC_SUBST(SHAREDIR)
AC_SUBST(DATADIR)
AC_SUBST(LOGDIR)
AC_SUBST(RUNDIR)
AC_SUBST(ABSPATHS)
AC_SUBST(SRCDIR)
AC_SUBST(BUILDDIR)
dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([MODDIR], "${MODDIR}", [Module root directory.])

dnl gettext
LOCALEDIR='${datadir}/locale'
AC_SUBST(LOCALEDIR)

LARGENET="no"
AC_MSG_CHECKING(if you want large network support)
AC_ARG_ENABLE(large-net,
AC_HELP_STRING([--enable-large-net],[ Enable large network support.]),
[
  case "$enableval" in
  yes)
    LARGENET="yes"
    AC_DEFINE([LARGE_NETWORK], 1, [Uncomment to enable large network support.])
    ;;
  no)
    LARGENET="no"
    ;;
  esac
])
AC_MSG_RESULT($LARGENET)

CONTRIB="no"
AC_MSG_CHECKING(if you want contrib modules)
AC_ARG_ENABLE(contrib,
AC_HELP_STRING([--enable-contrib],[ Enable contrib modules.]),
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
AC_MSG_RESULT($CONTRIB)

if test "$CONTRIB" = "yes"; then
	CONTRIB_ENABLE="contrib"
else
	CONTRIB_ENABLE=""
fi

AC_SUBST(CONTRIB_ENABLE)

AC_ARG_ENABLE(ssl,
	AC_HELP_STRING([--disable-ssl], [don't use OpenSSL to provide more SASL mechanisms]),
	,
	enable_ssl=yes)

SSL=no
if test "$enable_ssl" = "yes"; then
	AC_CHECK_LIB(ssl, SSL_read, [
		AC_CHECK_HEADERS(openssl/ssl.h openssl/err.h, [
			SSL_LIBS="-lssl -lcrypto"
			AC_SUBST(SSL_LIBS)
			SSL=yes
			],[SSL=no],[-])
		], [
			AC_CHECK_LIB(eay32, DH_generate_key, [
				AC_CHECK_HEADERS(openssl/ssl.h openssl/err.h, [
					# Ugh.
					SSL_LIBS="-leay32 /mingw/lib/ssleay32.a"
					AC_SUBST(SSL_LIBS)
					SSL=yes
					],[SSL=no],[-])
			], [], [])
		], -lcrypto)
	if test "$SSL" = "yes"; then
		AC_CHECK_HEADERS(openssl/ec.h, [], [], []) # Defines HAVE_OPENSSL_EC_H
		AC_DEFINE(HAVE_OPENSSL,, Define to 1 if openssl is available)
	fi
fi

case "$host" in
*-*-mingw32)
    CFLAGS="-I/mingw/include $CFLAGS"
    LIBS="$LIBS -lwsock32 -lws2_32 -lregex"
        ;;
esac

ATHEME_C_GCC_TRY_FLAGS([-std=gnu99], atheme_cv_c_gcc_std_gnu99)
ATHEME_C_GCC_TRY_FLAGS([-Werror-implicit-function-declaration], atheme_cv_c_gcc_w_error_implicit_function_declaration)

AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],[ Enable compiler warnings])],
	[], [enable_warnings=no])
AS_IF([test "x$enable_warnings" = "xyes"],
[
dnl See what warnings we can get away with
ATHEME_C_GCC_TRY_FLAGS([-Wall], atheme_cv_c_gcc_w_all)
ATHEME_C_GCC_TRY_FLAGS([-Wpointer-arith], atheme_cv_c_gcc_w_pointer_arith)
ATHEME_C_GCC_TRY_FLAGS([-Wimplicit -Wnested-externs], atheme_cv_c_gcc_w_implicit)
ATHEME_C_GCC_TRY_FLAGS([-Wcast-align], atheme_cv_c_gcc_w_cast_align)
ATHEME_C_GCC_TRY_FLAGS([-Wcast-qual], atheme_cv_c_gcc_w_cast_qual)
ATHEME_C_GCC_TRY_FLAGS([-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations], atheme_cv_c_gcc_prototypes)
ATHEME_C_GCC_TRY_FLAGS([-Wparenthesis], atheme_cv_c_gcc_parenthesis)
ATHEME_C_GCC_TRY_FLAGS([-W -Wno-unused], atheme_cv_c_gcc_w)
ATHEME_C_GCC_TRY_FLAGS([-Wextra], atheme_cv_c_gcc_w_extra)
ATHEME_C_GCC_TRY_FLAGS([-Wshadow], atheme_cv_c_gcc_w_shadow)
ATHEME_C_GCC_TRY_FLAGS([-Wundef], atheme_cv_c_gcc_w_undef)
ATHEME_C_GCC_TRY_FLAGS([-Wpacked], atheme_cv_c_gcc_w_packed)
ATHEME_C_GCC_TRY_FLAGS([-Wnested-externs], atheme_cv_c_gcc_w_nested_externs)
ATHEME_C_GCC_TRY_FLAGS([-Wunused-function -Wunused-label -Wunused-value -Wunused-variable], atheme_cv_c_gcc_w_unused)
ATHEME_C_GCC_TRY_FLAGS([-Wredundant-decls], atheme_cv_c_gcc_w_redundant_decls)
ATHEME_C_GCC_TRY_FLAGS([-Wfloat-equal], atheme_cv_c_gcc_w_float_equal)
dnl Added -fno-builtin-strftime to squelch a few (pointless) warnings from the time_format patch.
ATHEME_C_GCC_TRY_FLAGS([-Wformat=2 -fno-builtin-strftime], atheme_cv_c_gcc_w_format)
ATHEME_C_GCC_TRY_FLAGS([-Wdisabled-optimization], atheme_cv_c_gcc_w_disabled_optimization)
dnl ATHEME_C_GCC_TRY_FLAGS([-pedantic], atheme_cv_c_gcc_pedantic)

ATHEME_C_GCC_TRY_FLAGS([-Wunreachable-code], atheme_cv_c_gcc_w_unreachable_code)

dnl Maintainer flags.
dnl ATHEME_C_GCC_TRY_FLAGS([-Wconversion], atheme_cv_c_gcc_w_conversion)
dnl ATHEME_C_GCC_TRY_FLAGS([-Wwrite-strings], atheme_cv_c_gcc_w_strings)
dnl ATHEME_C_GCC_TRY_FLAGS([-Werror], atheme_cv_c_gcc_w_error)
])

AC_ARG_ENABLE([propolice],
	[AS_HELP_STRING([--disable-propolice],[ Disable propolice protections (for debugging.)])],
	[], [enable_propolice=no])
AS_IF([test "x$enable_propolice" = "xyes"],
[
ATHEME_C_GCC_TRY_FLAGS([-fno-stack-protector-all], atheme_cv_nspa)
ATHEME_C_GCC_TRY_FLAGS([-fno-pie], atheme_cv_npie)
ATHEME_C_GCC_TRY_FLAGS([-nonow], atheme_cv_nonow)
ATHEME_C_GCC_TRY_FLAGS([-norelro], atheme_cv_no_relro)
])

AC_ARG_ENABLE([profile],
	[AS_HELP_STRING([--enable-profile],[ Enable profiling extensions])],
	[], [enable_profile=no])
AS_IF([test "x$enable_profile" = "xyes"],
[
ATHEME_C_GCC_TRY_FLAGS([-pg], atheme_cv_pg)
ac_save_LDFLAGS="${LDFLAGS} -pg"
LDFLAGS=${ac_save_LDFLAGS}
])

ac_save_CPPFLAGS="${CFLAGS} ${orig_CFLAGS} ${CWARNS} -DPREFIX=\\\"\${prefix}\\\" -DLOCALEDIR=\\\"\${LOCALEDIR}\\\" -DMODDIR=\\\"\${MODDIR}\\\" -DSHAREDIR=\\\"${SHAREDIR}\\\" -DSYSCONFDIR=\\\"${sysconfdir}\\\" -DLOGDIR=\\\"${LOGDIR}\\\" -DRUNDIR=\\\"${RUNDIR}\\\" -DDATADIR=\\\"${DATADIR}\\\""
CPPFLAGS=${ac_save_CPPFLAGS}
AC_SUBST(CPPFLAGS)
dnl export CFLAGS

enablerpath="yes"
AC_ARG_ENABLE(rpath,
AC_HELP_STRING([--disable-rpath],[Disable use of -Wl,-rpath= during linking.]),
   [enablerpath="$enableval"], [enablerpath="yes"])

if test "x$enablerpath" != "xyes"; then
	LDFLAGS_RPATH=""
fi

PKG_CHECK_MODULES(JANSSON, [jansson >= 2.4],
	[JSON="yes"], [JSON="no"])

if test "$JSON" = "yes"; then
	AC_DEFINE(HAVE_JSON,, Define to 1 if JSON is available)
fi

PKG_CHECK_MODULES(LIBQRENCODE, [libqrencode], [LIBQRENCODE="yes"], [LIBQRENCODE="no"])

if test "$LIBQRENCODE" = "yes"; then
	AC_DEFINE(HAVE_LIBQRENCODE,, Define to 1 if libqrencode is available)
fi

AC_ARG_WITH(libmowgli,
  AC_HELP_STRING([--with-libmowgli@<:@=prefix@:>@],
    [ Specify location of system libmowgli install, or "no" to force use of internal libmowgli]),
  [], [with_libmowgli=yes])

if test "x$with_libmowgli" = "xyes"; then
  PKG_CHECK_MODULES(MOWGLI, [libmowgli-2 >= 2.0.0],
    [MOWGLI_SOURCE="system"],
    [with_libmowgli=no])
fi

AS_IF(
  [test "x$with_libmowgli" = "xyes"], [ ],
  [test "x$with_libmowgli" = "xno"], [
    MOWGLI_CFLAGS="-I../libmowgli-2/src/libmowgli -I../../libmowgli-2/src/libmowgli -I../../../libmowgli-2/src/libmowgli"
    MOWGLI_LIBS="-L../libmowgli-2/src/libmowgli -L../../libmowgli-2/src/libmowgli -L../../../libmowgli-2/src/libmowgli -lmowgli-2"
    LIBMOWGLI="libmowgli-2"
    MOWGLI_SOURCE="internal"
    AC_CONFIG_SUBDIRS([libmowgli-2])],
  [ MOWGLI_SOURCE="system"
    MOWGLI_CFLAGS="-I$with_libmowgli/include/libmowgli-2"
    MOWGLI_LIBS="-L$with_libmowgli/lib -lmowgli-2" ])

CPPFLAGS="$CPPFLAGS $MOWGLI_CFLAGS"
LIBS="$LIBS $MOWGLI_LIBS"

AC_SUBST(MOWGLI_CFLAGS)
AC_SUBST(MOWGLI_LIBS)
AC_SUBST(LIBMOWGLI)

AC_ARG_WITH(pcre,
AC_HELP_STRING([--with-pcre],[ Enable PCRE regular expression support]),
[],
[with_pcre=no])

if test "x$with_pcre" != xno; then
	PKG_CHECK_MODULES(PCRE, [libpcre], [], [
		AC_ERROR([PCRE requested, but not found.])])

	AC_SUBST(PCRE_CFLAGS)
	AC_SUBST(PCRE_LIBS)
	AC_DEFINE([HAVE_PCRE], [1], [Define if you want to use PCRE])
fi

BUILDROOT=`pwd`
AC_SUBST(BUILDROOT)

PODIR=""
if test "x$USE_NLS" = "xyes"; then
	PODIR="po"
fi
AC_SUBST(PODIR)

BUILDSYS_TOUCH_DEPS

AC_OUTPUT([
	buildsys.mk
	extra.mk
	atheme-services.pc
])

d=`eval echo ${MODDIR}`
d=`eval echo $d`
echo "
Configuration:
	Atheme version       : ${PACKAGE_VERSION}
	Installation prefix  : ${prefix}
	Module root directory: `eval echo $d`
	Config directory     : ${sysconfdir}
	Logfile directory    : ${LOGDIR}
	Data directory       : ${DATADIR}
	PID directory        : ${RUNDIR}
	Large network support: ${LARGENET}
	OpenSSL SASL support : ${SSL}
	Contrib modules      : ${CONTRIB}
	Mowgli installation  : ${MOWGLI_SOURCE}
	PCRE support         : ${with_pcre}
	Perl support         : ${with_perl}
	JSON support         : ${JSON}
	QR Code support      : ${LIBQRENCODE}
	CFLAGS               : ${CFLAGS}
	Internationalization : ${USE_NLS}

Type make to build Atheme, and make install to install it."
