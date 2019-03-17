AC_DEFUN([ATHEME_LIBTEST_PCRE], [

	LIBPCRE="No"

	AC_ARG_WITH([pcre],
		[AS_HELP_STRING([--without-pcre], [Do not attempt to detect libpcre (Perl-Compatible Regular Expressions)])],
		[], [with_pcre="auto"])

	case "x${with_pcre}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-pcre])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_pcre}" != "no"], [
		PKG_CHECK_MODULES([LIBPCRE], [libpcre], [
			LIBS="${LIBPCRE_LIBS} ${LIBS}"
			AC_CHECK_HEADERS([pcre.h], [
				AC_MSG_CHECKING([if libpcre appears to be usable])
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#ifdef HAVE_STDDEF_H
						#  include <stddef.h>
						#endif
						#include <pcre.h>
					]], [[
						(void) pcre_compile(NULL, 0, NULL, NULL, NULL);
						(void) pcre_exec(NULL, NULL, NULL, 0, 0, 0, NULL, 0);
						(void) pcre_free(NULL);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBPCRE="Yes"
					AC_DEFINE([HAVE_LIBPCRE], [1], [Define to 1 if libpcre appears to be usable])
				], [
					AC_MSG_RESULT([no])
					LIBPCRE="No"
					AS_IF([test "${with_pcre}" = "yes"], [
						AC_MSG_FAILURE([--with-pcre was given but libpcre does not appear to be usable])
					])
				])
			], [
				LIBPCRE="No"
				AS_IF([test "${with_pcre}" = "yes"], [
					AC_MSG_ERROR([--with-pcre was given but a required header file is missing])
				])
			], [])
		], [
			LIBPCRE="No"
			AS_IF([test "${with_pcre}" = "yes"], [
				AC_MSG_ERROR([--with-pcre was given but libpcre could not be found])
			])
		])
	], [
		LIBPCRE="No"
	])

	LIBS="${LIBS_SAVED}"

	AS_IF([test "${LIBPCRE}" = "No"], [
		LIBPCRE_CFLAGS=""
		LIBPCRE_LIBS=""
	])

	AC_SUBST([LIBPCRE_CFLAGS])
	AC_SUBST([LIBPCRE_LIBS])
])
