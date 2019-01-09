AC_DEFUN([ATHEME_LIBTEST_PERL], [

	LIBPERL="No"
	LIBPERL_CFLAGS=""
	LIBPERL_LIBS=""
	PERL_COND_D=""

	AC_ARG_WITH([perl],
	        [AS_HELP_STRING([--with-perl], [Enable Perl for modules/scripting/perl])],
	        [], [with_perl="no"])

	case "${with_perl}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-perl])
			;;
	esac

	AS_IF([test "${with_perl}" != "no"], [

		AC_PATH_PROG([perlpath], [perl])

		if test -n "${perlpath}"
		then
			LIBPERL="Yes"
			LIBPERL_CFLAGS="$(perl -MExtUtils::Embed -e ccopts)"
			LIBPERL_LIBS="$(perl -MExtUtils::Embed -e ldopts)"

			PERL_COND_D="perl"
			AC_SUBST([PERL_COND_D])
		fi

		dnl if Perl is built with threading support, we need to link atheme against libpthread
		AS_IF([echo "${LIBPERL_LIBS}" | grep -q pthread], [
			LIBS="-lpthread ${LIBS}"
		])
	])

	AS_IF([test "${with_perl}${LIBPERL_CFLAGS}" = "yes"], [
		AC_MSG_ERROR([Perl support was requested but Perl could not be found])
	])

	AC_SUBST([LIBPERL_CFLAGS])
	AC_SUBST([LIBPERL_LIBS])
])
