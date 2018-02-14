AC_DEFUN([ATHEME_LIBTEST_CRYPT], [

	LIBCRYPT="No"

	LIBCRYPT_LIBS=""

	AC_ARG_WITH([crypt],
		[AS_HELP_STRING([--without-crypt], [Do not attempt to detect crypt(3) for modules/crypto/crypt3-*])],
		[], [with_crypt="auto"])

	case "${with_crypt}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-crypt])
			;;
	esac

	AS_IF([test "x${with_crypt}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_CHECK_HEADERS([crypt.h], [], [], [])

		AC_SEARCH_LIBS([crypt], [crypt], [LIBCRYPT="Yes"], [
			AS_IF([test "x${with_crypt}" != "xauto"], [
				AC_MSG_ERROR([--with-crypt was specified but crypt(3) could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBCRYPT}" = "xYes"], [

		AC_MSG_CHECKING([if compiling a program using crypt(3) works])

		AC_COMPILE_IFELSE([

			AC_LANG_PROGRAM([[

				#include <unistd.h>

				#ifdef HAVE_CRYPT_H
				#include <crypt.h>
				#endif

			]], [[

				const char *const result = crypt("test", "Aa");

				return 0;

			]])
		], [
			AC_MSG_RESULT([yes])
		], [
			AC_MSG_RESULT([no])
			LIBCRYPT="No"

			AS_IF([test "x${with_crypt}" != "xauto"],
				[AC_MSG_ERROR([--with-crypt was specified but cannot compile a program using crypt(3)])])
		])
	])

	AS_IF([test "x${LIBCRYPT}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_crypt}" != "xnone required"], [
			LIBCRYPT_LIBS="${ac_cv_search_crypt}"
		])
		AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) is available])
		AC_SUBST([LIBCRYPT_LIBS])
	])
])
