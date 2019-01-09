AC_DEFUN([ATHEME_FEATURETEST_REPROBUILDS], [

	REPRODUCIBLE_BUILDS="No"

	AC_ARG_ENABLE([reproducible-builds],
		[AS_HELP_STRING([--enable-reproducible-builds], [Enable reproducible builds])],
		[], [enable_reproducible_builds="no"])

	case "x${enable_reproducible_builds}" in
		xyes)
			REPRODUCIBLE_BUILDS="Yes"
			AC_DEFINE([REPRODUCIBLE_BUILDS], [1], [Enable reproducible builds])
			;;
		xno)
			REPRODUCIBLE_BUILDS="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-reproducible-builds])
			;;
	esac
])
