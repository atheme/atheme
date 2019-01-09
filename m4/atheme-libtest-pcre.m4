AC_DEFUN([ATHEME_LIBTEST_PCRE], [

	LIBPCRE="No"

	AC_ARG_WITH([pcre],
		[AS_HELP_STRING([--without-pcre], [Do not attempt to detect libpcre])],
		[], [with_pcre="auto"])

	case "${with_pcre}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-pcre])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_pcre}" != "no"], [
		PKG_CHECK_MODULES([PCRE], [libpcre], [
			LIBPCRE="Yes"
			AC_DEFINE([HAVE_PCRE], [1], [Define to 1 if PCRE is available])
		], [
			LIBPCRE="No"
			AS_IF([test "${with_pcre}" = "yes"], [
				AC_MSG_ERROR([--with-pcre was specified but libpcre could not be found])
			])
		])
	])

	AS_IF([test "${LIBPCRE}" = "No"], [
		PCRE_CFLAGS=""
		PCRE_LIBS=""
	])

	AC_SUBST([PCRE_CFLAGS])
	AC_SUBST([PCRE_LIBS])

	LIBS="${LIBS_SAVED}"
])
