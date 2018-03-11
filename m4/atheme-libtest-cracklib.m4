AC_DEFUN([ATHEME_LIBTEST_CRACKLIB], [

	LIBCRACKLIB="No"

	CRACKLIB_LIBS=""

	AC_ARG_WITH([cracklib],
		[AS_HELP_STRING([--without-cracklib], [Do not attempt to detect CrackLib for modules/nickserv/cracklib (checking password strength)])],
		[], [with_cracklib="auto"])

	case "${with_cracklib}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-cracklib])
			;;
	esac

	AS_IF([test "x${with_cracklib}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([FascistCheck], [crack], [LIBCRACKLIB="Yes"], [
			AS_IF([test "x${with_cracklib}" != "xauto"], [
				AC_MSG_ERROR([--with-cracklib was specified but cracklib could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBCRACKLIB}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_FascistCheck}" != "xnone required"], [
			CRACKLIB_LIBS="${ac_cv_search_FascistCheck}"
		])
		AC_DEFINE([HAVE_CRACKLIB], [1], [Define to 1 if cracklib is available])
		AC_SUBST([CRACKLIB_LIBS])
	])
])
