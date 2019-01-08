AC_DEFUN([ATHEME_LIBTEST_IDN], [

	LIBIDN="No"
	LIBIDN_LIBS=""

	AC_ARG_WITH([libidn],
		[AS_HELP_STRING([--without-libidn], [Do not attempt to detect GNU libidn (for modules/saslserv/scram-sha -- SASLprep normalization)])],
		[], [with_libidn="auto"])

	case "${with_libidn}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-libidn])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "x${with_libidn}" != "xno"], [
		AC_CHECK_HEADERS([stringprep.h], [
			AC_SEARCH_LIBS([stringprep], [idn], [
				AC_MSG_CHECKING([if GNU libidn appears to be usable])
				AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h>
						#include <stdio.h>
						#include <stdlib.h>
						#include <stringprep.h>
					]], [[
						char buf[128];
						char *const tmp = stringprep_locale_to_utf8("test");
						(void) snprintf(buf, sizeof buf, "%s", tmp);
						(void) free(tmp);
						const int ret = stringprep(buf, sizeof buf, 0, stringprep_saslprep);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBIDN="Yes"
					AS_IF([test "x${ac_cv_search_stringprep}" != "xnone required"], [
						LIBIDN_LIBS="${ac_cv_search_stringprep}"
						AC_SUBST([LIBIDN_LIBS])
					])
					AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if we have GNU libidn available])
				], [
					AC_MSG_RESULT([no])
					LIBIDN="No"
					AS_IF([test "x${with_libidn}" = "xyes"], [
						AC_MSG_ERROR([--with-libidn was specified but GNU libidn appears to be unusable])
					])
				])
			], [
				LIBIDN="No"
				AS_IF([test "x${with_libidn}" = "xyes"], [
					AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])
				])
			])
		], [
			LIBIDN="No"
			AS_IF([test "x${with_libidn}" = "xyes"], [
				AC_MSG_ERROR([--with-libidn was specified but a required header file is missing])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
