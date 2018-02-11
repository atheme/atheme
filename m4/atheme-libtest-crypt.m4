AC_DEFUN([ATHEME_LIBTEST_CRYPT], [

	AS_IF([test "x${with_crypt}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([crypt], [crypt], [LIBCRYPT="Yes"], [
			AS_IF([test "x${with_crypt}" != "xauto"], [
				AC_MSG_ERROR([--with-crypt was specified but crypt(3) could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBCRYPT}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_crypt}" != "xnone required"], [
			LIBCRYPT_LIBS="${ac_cv_search_crypt}"
		])
		AC_DEFINE([HAVE_CRYPT], [1], [Define to 1 if crypt(3) is available])
		AC_SUBST([LIBCRYPT_LIBS])
	])
])
