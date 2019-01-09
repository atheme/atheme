AC_DEFUN([ATHEME_LIBTEST_IDN], [

	LIBIDN="No"

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

	AS_IF([test "${with_libidn}" != "no"], [
		PKG_CHECK_MODULES([LIBIDN], [libidn], [
			LIBS="${LIBIDN_LIBS} ${LIBS}"
			AC_CHECK_HEADERS([stringprep.h], [
				AC_MSG_CHECKING([if GNU libidn appears to be usable])
				AC_LINK_IFELSE([
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
						const int ret = stringprep(buf, sizeof buf, (Stringprep_profile_flags) 0, stringprep_saslprep);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBIDN="Yes"
					AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if we have GNU libidn available])
				], [
					AC_MSG_RESULT([no])
					LIBIDN="No"
					AS_IF([test "${with_libidn}" = "yes"], [
						AC_MSG_ERROR([--with-libidn was specified but GNU libidn appears to be unusable])
					])
				])
			], [
				LIBIDN="No"
				AS_IF([test "${with_libidn}" = "yes"], [
					AC_MSG_ERROR([--with-libidn was specified but a required header file is missing])
				])
			])
		], [
			LIBIDN="No"
			AS_IF([test "${with_libidn}" = "yes"], [
				AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
