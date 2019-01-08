AC_DEFUN([ATHEME_LIBTEST_CRYPT], [

	LIBCRYPT="No"
	LIBCRYPT_LIBS=""

	AC_ARG_WITH([crypt],
		[AS_HELP_STRING([--without-crypt], [Do not attempt to detect crypt(3) (for modules/crypto/crypt3-*)])],
		[], [with_crypt="auto"])

	case "${with_crypt}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-crypt])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "x${with_crypt}" != "xno"], [
		AC_CHECK_HEADERS([crypt.h], [], [], [])
		AC_SEARCH_LIBS([crypt], [crypt], [
			AC_MSG_CHECKING([if crypt(3) appears to be usable])
			AC_COMPILE_IFELSE([
				AC_LANG_PROGRAM([[
					#include <unistd.h>
					#ifdef HAVE_CRYPT_H
					#  include <crypt.h>
					#endif
				]], [[
					const char *const result = crypt("test", "test");
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBCRYPT="Yes"
				AS_IF([test "x${ac_cv_search_crypt}" != "xnone required"], [
					LIBCRYPT_LIBS="${ac_cv_search_crypt}"
					AC_SUBST([LIBCRYPT_LIBS])
				])
				AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) is available])
			], [
				AC_MSG_RESULT([no])
				LIBCRYPT="No"
				AS_IF([test "x${with_crypt}" = "xyes"], [
					AC_MSG_ERROR([--with-crypt was specified but crypt(3) appears to be unusable])
				])
			])

		], [
			LIBCRYPT="No"
			AS_IF([test "x${with_crypt}" = "xyes"], [
				AC_MSG_ERROR([--with-crypt was specified but crypt(3) could not be found])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
