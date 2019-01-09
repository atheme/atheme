AC_DEFUN([ATHEME_FEATURETEST_DEBUGGING], [

	DEBUGGING="No"

	AC_ARG_ENABLE([debugging],
		[AS_HELP_STRING([--enable-debugging], [Enable extensive debugging support])],
		[], [enable_debugging="no"])

	case "${enable_debugging}" in
		yes)
			ATHEME_CC_TEST_CFLAGS([-O0 -g])

			AS_IF([test "${ATHEME_CC_TEST_CFLAGS_RESULT}" = "no"], [

				AC_MSG_ERROR([--enable-debugging requires compiler support for -O0 -g])
			])

			AC_DEFINE([ATHEME_DEBUGGING], [1], [Enable extensive debugging support])
			DEBUGGING="Yes"
			;;
		no)
			ATHEME_CC_TEST_CFLAGS([-O2])
			DEBUGGING="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-debugging])
			;;
	esac
])
