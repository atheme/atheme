AC_DEFUN([ATHEME_LIBTEST_PCRE], [

	LIBPCRE="No"

	AC_ARG_WITH([pcre],
		[AS_HELP_STRING([--without-pcre], [Do not attempt to detect libpcre])],
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
				AC_MSG_CHECKING([if libpcre is usable])
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h>
						#include <pcre.h>
					]], [[
						(void) pcre_compile(NULL, 0, NULL, NULL, NULL);
						(void) pcre_exec(NULL, NULL, NULL, 0, 0, 0, NULL, 0);
						(void) pcre_free(NULL);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBPCRE="Yes"
					AC_DEFINE([HAVE_LIBPCRE], [1], [Define to 1 if PCRE is available])
				], [
					AC_MSG_RESULT([no])
					LIBPCRE="No"
					AS_IF([test "${with_pcre}" = "yes"], [
						AC_MSG_ERROR([--with-pcre was specified but libpcre does not appear to be usable])
					])
				])
			], [
				LIBPCRE="No"
				AS_IF([test "${with_pcre}" = "yes"], [
					AC_MSG_ERROR([--with-pcre was specified but a required header file is missing])
				])
			], [])
		], [
			LIBPCRE="No"
			AS_IF([test "${with_pcre}" = "yes"], [
				AC_MSG_ERROR([--with-pcre was specified but libpcre could not be found])
			])
		])
	])

	AS_IF([test "${LIBPCRE}" = "No"], [
		LIBPCRE_CFLAGS=""
		LIBPCRE_LIBS=""
	])

	AC_SUBST([LIBPCRE_CFLAGS])
	AC_SUBST([LIBPCRE_LIBS])

	LIBS="${LIBS_SAVED}"
])
