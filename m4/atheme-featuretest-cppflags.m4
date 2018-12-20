ATHEME_CPP_TEST_CPPFLAGS_RESULT="no"

AC_DEFUN([ATHEME_CPP_TEST_CPPFLAGS],
	[
		AC_MSG_CHECKING([for C preprocessor flag(s) $1 ])

		CPPFLAGS_SAVED="${CPPFLAGS}"
		CPPFLAGS="${CPPFLAGS} $1"

		AC_COMPILE_IFELSE(
			[
				AC_LANG_PROGRAM([[]], [[]])
			], [
				ATHEME_CPP_TEST_CPPFLAGS_RESULT='yes'

				AC_MSG_RESULT([yes])
			], [
				ATHEME_CPP_TEST_CPPFLAGS_RESULT='no'
				CPPFLAGS="${CPPFLAGS_SAVED}"

				AC_MSG_RESULT([no])
			], [
				ATHEME_CPP_TEST_CPPFLAGS_RESULT='no'
				CPPFLAGS="${CPPFLAGS_SAVED}"

				AC_MSG_RESULT([skipped as we are cross-compiling])
			]
		)

		unset CPPFLAGS_SAVED
	]
)

AC_DEFUN([ATHEME_FEATURETEST_CPPFLAGS], [

	AC_ARG_ENABLE([fortify-source],
		[AS_HELP_STRING([--disable-fortify-source], [Disable -D_FORTIFY_SOURCE=2 (hardening for many C library function invocations)])],
		[], [enable_fortify_source="yes"])

	case "${enable_fortify_source}" in
		yes)
			ATHEME_CPP_TEST_CPPFLAGS([-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-fortify-source])
			;;
	esac
])
