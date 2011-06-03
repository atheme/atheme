dnl Copyright (c) 2005 Atheme Development Group
dnl Rights to this code are documented in doc/LICENSE.
dnl
dnl This file is the autoconf stub.
dnl
dnl $Id: configure.ac 8407 2007-06-03 22:35:28Z jilles $

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(atheme, 7.0-alpha4, bugs@atheme.org)

dnl Sneaky way to get an id into the configure script.
AC_COPYRIGHT([$Id: configure.ac 8407 2007-06-03 22:35:28Z jilles $])

AC_CONFIG_AUX_DIR(autoconf)

AC_CONFIG_HEADER(include/sysconf.h)
AC_PREFIX_DEFAULT(~/atheme)

AC_GNU_SOURCE
AC_SYS_LARGEFILE

dnl Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])

dnl GNU gettext

dnl Uncomment this if we choose to provide our own libintl.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.16.1])
AM_PO_SUBDIRS

if test "$prefix" = "NONE"; then
	prefix="${ac_default_prefix}"
fi

dnl the following is wrong! -- jilles
dnl AC_DEFINE_UNQUOTED([PREFIX], "${prefix}", [Installation prefix.])

dnl Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_INSTALL
AC_EXEEXT
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(MV, mv)
AC_PATH_PROG(CP, cp)
AC_PATH_PROG(LN, ln)
AC_PATH_PROG(TAR, tar)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(RANLIB, ranlib)
AC_C_INLINE
AC_C_BIGENDIAN
AC_PROG_MAKE_SET

AC_MSG_CHECKING(how to generate dependancy info)
if test "$SunWorkShop" = "yes"; then
  AC_MSG_RESULT(Sun WorkShop/Forte using $CC -xM)
  MKDEP="$CC -xM"
fi
if test "$HPUX" = "yes"; then
  AC_MSG_RESULT(HP-UX C Compiler using makedepend)
  MKDEP="makedepend"
fi
if test "x$MKDEP" = "x"; then
  AC_MSG_RESULT(assuming $CC -MM)
  MKDEP="$CC -MM"
fi

AC_SUBST(MKDEP)

dnl Checks for header files.
AC_CHECK_HEADERS(link.h,,,[-])

dnl Buildsys module stuff
BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
LIBS="$LIBS $DYNAMIC_LD_LIBS"

dnl Checks for library functions.
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([strdup strlcpy strlcat inet_pton inet_ntop gettimeofday umask mmap arc4random getrlimit fork getpid execve])
AC_CHECK_FUNC(socket,, AC_CHECK_LIB(socket, socket))
AC_CHECK_FUNC(gethostbyname,, AC_CHECK_LIB(nsl, gethostbyname))

POSIX_CRYPTC=""
AC_SEARCH_LIBS(crypt, crypt, [POSIX_CRYPTC="posix.c"])

AC_SUBST(POSIX_CRYPTC)

CRACKLIB_C=""
AC_SEARCH_LIBS(FascistCheck, crack, [CRACKLIB_C="cracklib.c"])
AC_SUBST(CRACKLIB_C)

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

ABSPATHS="#"
AC_MSG_CHECKING(if you want to use absolute paths when building)
AC_ARG_ENABLE(absolute-paths,
AC_HELP_STRING([--enable-absolute-paths],[ Use absolute paths when building.]),
[
   case "$enableval" in
   yes)
      case "$srcdir" in
         /*)
         ;;
         *)
            oldsrcdir="$srcdir"
            srcdir="`cd \"$srcdir\";pwd`"
         ;;
      esac
      ABSPATHS=""
      ;;
   no)
      ABSPATHS="#"
      ;;
   esac
])
AC_MSG_RESULT($ABSPATHS)

SRCDIR="$srcdir"
BUILDDIR=`pwd`

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

balloc="no"
AC_MSG_CHECKING(if you want to enable the block allocator)
AC_ARG_ENABLE(balloc,
AC_HELP_STRING([--disable-balloc],[Disable the block allocator.]),
[balloc=$enableval], [balloc=yes])

if test "$balloc" = no; then
	AC_DEFINE([NOBALLOC], 1, [Define to 1 if you wish to disable the block allocator.])
fi
AC_MSG_RESULT($balloc)

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
			],,[-])
		],, -lcrypto)
	if test "$SSL" = "yes"; then
		AC_DEFINE(HAVE_OPENSSL,, Define to 1 if openssl is available)
	fi
fi

shared_modules="yes"

dnl Second stage: check for functions and headers.
if test "$shared_modules" = yes; then
	AC_CHECK_HEADERS(dlfcn.h,,,[-])
	AC_SEARCH_LIBS(dlopen, [dl c_r],
	[
	    AC_DEFINE(HAVE_DLOPEN, 1, [Define if the dlopen function is available.])
	    AC_CHECK_FUNC(dlsym, ,
	    [
	      AC_ERROR([shared modules unavailable, Atheme will not work on your system])
	    ])
	    AC_CHECK_FUNCS(dlfunc)
	    AC_CHECK_FUNCS(dlinfo)
	],
	[
	    AC_ERROR([shared modules unavailable, Atheme will not work on your system])
	])
fi

dnl Third stage - wrangling the linker.
if test "$shared_modules" = yes; then
	# The GNU linker requires the -export-dynamic option to make
	# all symbols visible in the dynamic symbol table.
	hold_ldflags=$LDFLAGS
	AC_MSG_CHECKING(for the ld -export-dynamic flag)
	LDFLAGS="${LDFLAGS} -Wl,-export-dynamic"
	AC_LINK_IFELSE(AC_LANG_PROGRAM([],[int i;]), found=yes, found=no)
	LDFLAGS=$hold_ldflags

	if expr "`uname -s`" : ^IRIX >/dev/null 2>&1; then
		found="no, IRIX ld uses -B,dynamic"
		LDFLAGS="${LDFLAGS} -Wl,-B,dynamic"
	fi

	AC_MSG_RESULT($found)

	if test "$found" = yes; then
		LDFLAGS="${LDFLAGS} -Wl,-export-dynamic"
	fi

	if test "$ac_cv_c_compiler_gnu" = yes; then
		AC_MSG_CHECKING(if $CC is Apple GCC)
		if expr "`$CC -v 2>&1 | tail -n 1`" : ".*Apple" >/dev/null; then
			AppleGCC=yes
		else
			AppleGCC=no
		fi
	
		AC_MSG_RESULT($AppleGCC)
	fi

	AC_MSG_CHECKING(for compiler option to produce PIC)

	if test "$SGS" = "yes"; then
		AC_MSG_RESULT([SVR4 SGS interfaces: -KPIC -DPIC -G])
		PICFLAGS="-KPIC -DPIC -G"
	fi
	
	if test "$AppleGCC" = "yes"; then
		AC_MSG_RESULT([Darwin Mach-O bundles: -fno-common -bundle -flat_namespace -undefined suppress])
		PICFLAGS="-fno-common -bundle -flat_namespace -undefined suppress"
	fi

	if test "$HPUX" = "yes" -a "$CC" != gcc; then
		AC_MSG_RESULT(HP-UX cc: +z -r -q -n)
		PICFLAGS="+z -r -q -n -c"
		AC_MSG_CHECKING([if +ESfic is required on this platform])
		
		if expr "`$CC +ESfic 2>&1`" : ".*neither supported.*" >/dev/null; then 
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			PICFLAGS="$PICFLAGS +ESfic"
		fi

		LDFLAGS="${LDFLAGS} -Wl,-E"
	fi

	if test "$Tru" = yes -a "$CC" != gcc; then
		AC_MSG_RESULT([Tru64: -shared -expect_unresolved '*'])
		PICFLAGS="-shared -expect_unresolved '*' "
		LDFLAGS="${hold_ldflags} -call_shared"
	fi

	if test "$CC" = tcc -a "$TenDRA" = no; then
		AC_MSG_RESULT([TinyCC: static only.])
	        AC_ERROR([your compiler is unable to compile Atheme's modules.])
		shared_modules=no
	fi

	if test "$CC" = icc -a "$ICC" = yes; then
		AC_MSG_RESULT(intel: -fPIC -DPIC -shared)
		PICFLAGS="-fPIC -DPIC -shared"
	fi

	if test -z "$PICFLAGS"; then
		if test "$ac_cv_c_compiler_gnu" = "yes"; then
			AC_MSG_RESULT(gcc: -fPIC -DPIC -shared)
			PICFLAGS="-fPIC -DPIC -shared"
		else
			AC_MSG_RESULT(no)
		        AC_ERROR([your compiler is unable to compile Atheme's modules.])
		fi
	fi
fi

if test "$shared_modules" = yes; then

	AC_CHECK_FUNC(nlist,,
		AC_CHECK_LIB(dl, nlist, nlist_lib="-ldl",
			AC_CHECK_LIB(elf, nlist, nlist_lib="-lelf",)
		)
	)

	dnl We need to find out whether underscores are appended to symbol
	dnl names in executable files.  First, though, we need to see
	dnl where nlist.h is hiding.
	AC_CHECK_HEADER(libelf/nlist.h, [ nlist_h="libelf/nlist.h" ], ,[-])
	AC_CHECK_HEADER(elf/nlist.h, [ nlist_h="elf/nlist.h" ], ,[-])
	AC_CHECK_HEADER(nlist.h, [ nlist_h="nlist.h" ], ,[-])
	if test x"$nlist_h" = "x"; then
		AC_DEFINE_UNQUOTED(SYMBOL_PREFIX, "",
			[String containing extra underscores prepended to symbols loaded from modules.])
	else
		AC_MSG_CHECKING(for extra underscores prepended to symbol names)
		AC_CACHE_VAL(atheme_cv_symbol_underscores,
		[
cat << EOF > conftest.c
#include <$nlist_h>
#include <stdio.h>
#include <stdlib.h>
void _modinit(void);
int main(int argc, char *argv[[]]) {
	int i;
	struct nlist nl[[5]];

	/* fill the names in this way, so it'll work almost everywhere */
	nl[[0]].n_name = "_modinit";
	nl[[1]].n_name = "__modinit";
	nl[[2]].n_name = "___modinit";
	nl[[3]].n_name = "____modinit";
	nl[[0]].n_value = nl[[1]].n_value = nl[[2]].n_value = nl[[3]].n_value = nl[[4]].n_name = NULL;

	if(argc < 2 || (nlist(argv[[1]], nl)) == -1) exit(-1);
	for(i = 0; i < 4; i++) {
		if(nl[[i]].n_value != NULL)
		{
			int j;
			for(j = 0; j < i; j++)
				printf("_");
			exit(i);
		}
	}
	exit(-1);
}
void _modinit(void) { return; }
EOF
		$CC $CPPFLAGS $CFLAGS -o conftest conftest.c $nlist_lib >/dev/null 2>&1
		symbol_underscores=`./conftest conftest`
		AC_MSG_RESULT($symbol_underscores)
		$RM -f conftest conftest.c
		])
		AC_DEFINE_UNQUOTED(SYMBOL_PREFIX, "${symbol_underscores}", 
			[String containing extra underscores prepended to symbols loaded from modules.])
	fi
fi

AC_SUBST(PICFLAGS)

ATHEME_C_GCC_TRY_FLAGS([-std=gnu99], atheme_cv_c_gcc_std_gnu99)
ATHEME_C_GCC_TRY_FLAGS([-Werror-implicit-function-declaration], atheme_cv_c_gcc_w_error_implicit_function_declaration)

AC_ARG_ENABLE(warnings,
AC_HELP_STRING([--enable-warnings],[ Enable compiler warnings]),
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
ATHEME_C_GCC_TRY_FLAGS([-Wbad-function-cast], atheme_cv_c_gcc_w_bad_function_cast)
ATHEME_C_GCC_TRY_FLAGS([-Wunused-function -Wunused-label -Wunused-value -Wunused-variable], atheme_cv_c_gcc_w_unused)
ATHEME_C_GCC_TRY_FLAGS([-Wredundant-decls], atheme_cv_c_gcc_w_redundant_decls)
ATHEME_C_GCC_TRY_FLAGS([-Wfloat-equal], atheme_cv_c_gcc_w_float_equal)
dnl Added -fno-builtin-strftime to squelch a few (pointless) warnings from the time_format patch.
ATHEME_C_GCC_TRY_FLAGS([-Wformat=2 -fno-builtin-strftime], atheme_cv_c_gcc_w_format)
ATHEME_C_GCC_TRY_FLAGS([-Wdisabled-optimization], atheme_cv_c_gcc_w_disabled_optimization)
dnl ATHEME_C_GCC_TRY_FLAGS([-pedantic], atheme_cv_c_gcc_pedantic)

dnl Too unreliable to be useful at this time.
dnl ATHEME_C_GCC_TRY_FLAGS([-Wunreachable-code], atheme_cv_c_gcc_w_unreachable_code)

dnl Maintainer flags.
dnl ATHEME_C_GCC_TRY_FLAGS([-Wconversion], atheme_cv_c_gcc_w_conversion)
dnl ATHEME_C_GCC_TRY_FLAGS([-Wwrite-strings], atheme_cv_c_gcc_w_strings)
dnl ATHEME_C_GCC_TRY_FLAGS([-Werror], atheme_cv_c_gcc_w_error)
])

AC_ARG_ENABLE(propolice,
AC_HELP_STRING([--disable-propolice],[ Disable propolice protections (for debugging.)]),
[
ATHEME_C_GCC_TRY_FLAGS([-fno-stack-protector-all], atheme_cv_nspa)
ATHEME_C_GCC_TRY_FLAGS([-fno-pie], atheme_cv_npie)
ATHEME_C_GCC_TRY_FLAGS([-nonow], atheme_cv_nonow)
ATHEME_C_GCC_TRY_FLAGS([-norelro], atheme_cv_no_relro)
])

AC_ARG_ENABLE(profile,
AC_HELP_STRING([--enable-profile],[ Enable profiling extensions]),
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
RPATH=""

AC_SUBST(RPATH)

PKG_CHECK_MODULES(MOWGLI, [libmowgli >= 0.9.50],
	[MOWGLI_SOURCE="system"], [
	MOWGLI_SOURCE="internal"
	MOWGLI_CFLAGS="-I../libmowgli/src/libmowgli -I../../libmowgli/src/libmowgli -I../../../libmowgli/src/libmowgli"
	MOWGLI_LIBS="-L../libmowgli/src/libmowgli -L../../libmowgli/src/libmowgli -L../../../libmowgli/src/libmowgli -lmowgli"
	LIBMOWGLI="libmowgli"
	if test "x$enablerpath" = "xyes"; then
		RPATH="-Wl,-rpath,${libdir}"
	fi
])
AS_IF([ test "x$MOWGLI_SOURCE" = "xinternal" ],
	[ AC_CONFIG_SUBDIRS([libmowgli]) ])

CPPFLAGS="$CPPFLAGS $MOWGLI_CFLAGS"
LIBS="$LIBS $MOWGLI_LIBS $RPATH"

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
	Block Allocator      : ${balloc}
	Mowgli installation  : ${MOWGLI_SOURCE}
	PCRE support         : ${with_pcre}
	Perl support         : ${with_perl}
	CFLAGS               : ${CFLAGS}
	Internationalization : ${USE_NLS}

Type make to build Atheme, and make install to install it."
