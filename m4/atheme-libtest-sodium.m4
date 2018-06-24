AC_DEFUN([ATHEME_LIBTEST_SODIUM], [

	LIBSODIUM="No"

	LIBSODIUM_LIBS=""

	AC_ARG_WITH([sodium],
		[AS_HELP_STRING([--without-sodium], [Do not attempt to detect libsodium for secure memory ops])],
		[], [with_sodium="auto"])

	case "${with_sodium}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-sodium])
			;;
	esac

	AS_IF([test "x${with_sodium}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([sodium_memzero], [sodium], [LIBSODIUM="Yes"], [
			AS_IF([test "x${with_sodium}" != "xauto"], [
				AC_MSG_ERROR([--with-sodium was specified but libsodium could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBSODIUM}" = "xYes"], [
		AC_CHECK_HEADERS([sodium/core.h sodium/utils.h], [], [
			LIBSODIUM="No"
			AS_IF([test "x${with_sodium}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
		], [])
	])

	AS_IF([test "x${LIBSODIUM}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_sodium_memzero}" != "xnone required"], [
			LIBSODIUM_LIBS="${ac_cv_search_sodium_memzero}"
		])
		AC_DEFINE([HAVE_LIBSODIUM], [1], [Define to 1 if libsodium is available])
		AC_SUBST([LIBSODIUM_LIBS])
	])
])
