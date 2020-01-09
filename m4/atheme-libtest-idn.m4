AC_DEFUN([ATHEME_LIBTEST_IDN], [

	LIBIDN="No"
	LIBIDN_PATH=""

	AC_ARG_WITH([libidn],
		[AS_HELP_STRING([--without-libidn], [Do not attempt to detect GNU libidn (for modules/saslserv/scram-sha -- SASLprep normalization)])],
		[], [with_libidn="auto"])

	case "x${with_libidn}" in
		xno | xyes | xauto)
			;;
		x/*)
			LIBIDN_PATH="${with_libidn}"
			with_libidn="yes"
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-libidn])
			;;
	esac

	CPPFLAGS_SAVED="${CPPFLAGS}"
	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_libidn}" != "no"], [
		AS_IF([test -n "${LIBIDN_PATH}"], [
			dnl Allow for user to provide custom installation directory
			AS_IF([test -d "${LIBIDN_PATH}/include" -a -d "${LIBIDN_PATH}/lib"], [
				LIBIDN_CFLAGS="-I${LIBIDN_PATH}/include"
				LIBIDN_LIBS="-L${LIBIDN_PATH}/lib"
			], [
				AC_MSG_ERROR([${LIBIDN_PATH} is not a suitable directory for GNU libidn])
			])
		], [test -n "${PKG_CONFIG}"], [
			dnl Allow for the user to "override" pkg-config without it being installed
			PKG_CHECK_MODULES([LIBIDN], [libidn], [], [])
		])
		AS_IF([test -n "${LIBIDN_CFLAGS+set}" -a -n "${LIBIDN_LIBS+set}"], [
			dnl Only proceed with library tests if custom paths were given or pkg-config succeeded
			LIBIDN="Yes"
		], [
			LIBIDN="No"
			AS_IF([test "${with_libidn}" != "auto"], [
				AC_MSG_FAILURE([--with-libidn was given but GNU libidn could not be found])
			])
		])
	])

	AS_IF([test "${LIBIDN}" = "Yes"], [
		CPPFLAGS="${LIBIDN_CFLAGS} ${CPPFLAGS}"
		LIBS="${LIBIDN_LIBS} ${LIBS}"

		AC_MSG_CHECKING([if GNU libidn appears to be usable])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <stringprep.h>
			]], [[
				(void) stringprep_locale_to_utf8(NULL);
				(void) stringprep(NULL, 0, (Stringprep_profile_flags) 0, stringprep_saslprep);
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBIDN="Yes"
			AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if libidn appears to be usable])
		], [
			AC_MSG_RESULT([no])
			LIBIDN="No"
			AS_IF([test "${with_libidn}" != "auto"], [
				AC_MSG_FAILURE([--with-libidn was given but GNU libidn appears to be unusable])
			])
		])
	])

	CPPFLAGS="${CPPFLAGS_SAVED}"
	LIBS="${LIBS_SAVED}"

	AS_IF([test "${LIBIDN}" = "No"], [
		LIBIDN_CFLAGS=""
		LIBIDN_LIBS=""
	])

	AC_SUBST([LIBIDN_CFLAGS])
	AC_SUBST([LIBIDN_LIBS])
])
