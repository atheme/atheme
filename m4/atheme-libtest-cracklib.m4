AC_DEFUN([ATHEME_LIBTEST_CRACKLIB], [

	LIBCRACKLIB="No"
	CRACKLIB_LIBS=""

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
		AC_CHECK_HEADERS([crack.h], [
			AC_SEARCH_LIBS([FascistCheck], [crack], [
				AC_MSG_CHECKING([if cracklib appears to be usable])
				AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h> /* <crack.h> uses size_t but doesn't define it */
						#include <crack.h>
					]], [[
						const char *const result = FascistCheck("test", "test");
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBCRACKLIB="Yes"
					AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"], [
						CRACKLIB_LIBS="${ac_cv_search_FascistCheck}"
						AC_SUBST([CRACKLIB_LIBS])
					])
					AC_DEFINE([HAVE_CRACKLIB], [1], [Define to 1 if cracklib is available])
				], [
					AC_MSG_RESULT([no])
					LIBCRACKLIB="No"
					AS_IF([test "${with_cracklib}" = "yes"], [
						AC_MSG_ERROR([--with-cracklib was specified but cracklib appears to be unusable])
					])
				])
			], [
				LIBCRACKLIB="No"
				AS_IF([test "${with_cracklib}" = "yes"], [
					AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found])
				])
			], [])
		], [
			LIBCRACKLIB="No"
			AS_IF([test "${with_cracklib}" = "yes"], [
				AC_MSG_ERROR([--with-cracklib was specified but a required header file is missing])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
