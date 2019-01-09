AC_DEFUN([ATHEME_LIBTEST_PERL], [

	LIBPERL="No"

	PERL_COND_D=""
	PERL_CFLAGS=""
	PERL_LIBS=""

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

			PERL_COND_D="perl"
			PERL_CFLAGS="$(perl -MExtUtils::Embed -e ccopts)"
			PERL_LIBS="$(perl -MExtUtils::Embed -e ldopts)"

			AC_SUBST([PERL_COND_D])
			AC_SUBST([PERL_CFLAGS])
			AC_SUBST([PERL_LIBS])
		fi

		dnl if Perl is built with threading support, we need to link atheme against libpthread
		if test -n "$(echo "${PERL_LIBS}" | grep pthread)"
		then
			LIBS="${LIBS} -lpthread"
		fi
	])

	AS_IF([test "${with_perl}${PERL_CFLAGS}" = "yes"], [
		AC_MSG_ERROR([Perl support was requested but Perl could not be found])
	])
])
