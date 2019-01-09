AC_DEFUN([ATHEME_LIBTEST_CRACK], [

	LIBCRACK="No"
	LIBCRACK_LIBS=""

	AC_ARG_WITH([cracklib],
		[AS_HELP_STRING([--without-cracklib], [Do not attempt to detect cracklib (for modules/nickserv/cracklib -- checking password strength)])],
		[], [with_cracklib="auto"])

	case "${with_cracklib}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-cracklib])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_cracklib}" != "no"], [
		AC_SEARCH_LIBS([FascistCheck], [crack], [
			AC_CHECK_HEADERS([crack.h], [
				AC_MSG_CHECKING([if cracklib appears to be usable])
				AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h>
						#include <crack.h>
					]], [[
						(void) FascistCheck(NULL, NULL);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBCRACK="Yes"
					AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"], [
						LIBCRACK_LIBS="${ac_cv_search_FascistCheck}"
						AC_SUBST([LIBCRACK_LIBS])
					])
					AC_DEFINE([HAVE_CRACKLIB], [1], [Define to 1 if cracklib is available])
				], [
					AC_MSG_RESULT([no])
					LIBCRACK="No"
					AS_IF([test "${with_cracklib}" = "yes"], [
						AC_MSG_ERROR([--with-cracklib was specified but cracklib appears to be unusable])
					])
				])
			], [
				LIBCRACK="No"
				AS_IF([test "${with_cracklib}" = "yes"], [
					AC_MSG_ERROR([--with-cracklib was specified but a required header file is missing])
				])
			], [])
		], [
			LIBCRACK="No"
			AS_IF([test "${with_cracklib}" = "yes"], [
				AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
