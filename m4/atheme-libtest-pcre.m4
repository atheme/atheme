AC_DEFUN([ATHEME_LIBTEST_PCRE], [

	LIBPCRE="No"

	AC_ARG_WITH([pcre],
		[AS_HELP_STRING([--without-pcre], [Do not attempt to detect Perl-Compatible Regular Expression library])],
		[], [with_pcre="auto"])

	case "${with_pcre}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-pcre])
			;;
	esac

	AS_IF([test "x${with_pcre}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		PKG_CHECK_MODULES([PCRE], [libpcre], [LIBPCRE="Yes"], [
			AS_IF([test "x${with_pcre}" != "xauto"], [
				AC_MSG_ERROR([--with-pcre was specified but the PCRE library could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBPCRE}" = "xYes"], [
		AC_DEFINE([HAVE_PCRE], [1], [Define to 1 if PCRE is available])
		AC_SUBST([PCRE_CFLAGS])
		AC_SUBST([PCRE_LIBS])
	])
])
