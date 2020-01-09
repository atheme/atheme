AC_DEFUN([ATHEME_LIBTEST_CRACK], [

	LIBCRACK="No"
	LIBCRACK_LIBS=""

	AC_ARG_WITH([cracklib],
		[AS_HELP_STRING([--without-cracklib], [Do not attempt to detect cracklib (for modules/nickserv/pwquality -- checking password strength)])],
		[], [with_cracklib="auto"])

	case "x${with_cracklib}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-cracklib])
			;;
	esac

	CFLAGS_SAVED="${CFLAGS}"
	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_cracklib}" != "no"], [
		dnl If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
		AC_SEARCH_LIBS([FascistCheck], [crack], [
			AC_MSG_CHECKING([if cracklib appears to be usable])
			AC_COMPILE_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_STDDEF_H
					#  include <stddef.h>
					#endif
					#include <crack.h>
				]], [[
					(void) FascistCheck(NULL, NULL);
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBCRACK="Yes"
				AC_DEFINE([HAVE_CRACKLIB], [1], [Define to 1 if cracklib appears to be usable])
				AC_DEFINE([HAVE_ANY_PASSWORD_QUALITY_LIBRARY], [1], [Define to 1 if any password quality library appears to be usable])
				AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"], [
					LIBCRACK_LIBS="${ac_cv_search_FascistCheck}"
					AC_SUBST([LIBCRACK_LIBS])
				])
			], [
				AC_MSG_RESULT([no])
				LIBCRACK="No"
				AS_IF([test "${with_cracklib}" = "yes"], [
					AC_MSG_FAILURE([--with-cracklib was given but cracklib appears to be unusable])
				])
			])
		], [
			LIBCRACK="No"
			AS_IF([test "${with_cracklib}" = "yes"], [
				AC_MSG_ERROR([--with-cracklib was given but cracklib could not be found])
			])
		])
	], [
		LIBCRACK="No"
	])

	CFLAGS="${CFLAGS_SAVED}"
	LIBS="${LIBS_SAVED}"
])
