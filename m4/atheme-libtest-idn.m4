AC_DEFUN([ATHEME_LIBTEST_IDN], [

	LIBIDN="No"

	AC_ARG_WITH([libidn],
		[AS_HELP_STRING([--without-libidn], [Do not attempt to detect GNU libidn (for modules/saslserv/scram-sha -- SASLprep normalization)])],
		[], [with_libidn="auto"])

	case "x${with_libidn}" in
		xno | xyes | xauto)
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
						#include <stringprep.h>
					]], [[
						(void) stringprep_locale_to_utf8(NULL);
						(void) stringprep(NULL, 0, (Stringprep_profile_flags) 0, stringprep_saslprep);
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
			], [])
		], [
			LIBIDN="No"
			AS_IF([test "${with_libidn}" = "yes"], [
				AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])
			])
		])
	])

	AS_IF([test "${LIBIDN}" = "No"], [
		LIBIDN_CFLAGS=""
		LIBIDN_LIBS=""
	])

	AC_SUBST([LIBIDN_CFLAGS])
	AC_SUBST([LIBIDN_LIBS])

	LIBS="${LIBS_SAVED}"
])
