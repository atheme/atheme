AC_DEFUN([ATHEME_FEATURETEST_REPROBUILDS], [

	REPRODUCIBLE_BUILDS="No"

	AC_ARG_ENABLE([reproducible-builds],
		[AS_HELP_STRING([--enable-reproducible-builds], [Enable reproducible builds])],
		[], [enable_reproducible_builds="no"])

	case "x${enable_reproducible_builds}" in
		xyes)
			REPRODUCIBLE_BUILDS="Yes"
			AC_DEFINE([ATHEME_ENABLE_REPRODUCIBLE_BUILDS], [1], [Define to 1 if --enable-reproducible-builds was given to ./configure])
			AS_IF([test "x${SOURCE_DATE_EPOCH}" != "x"], [
				AC_DEFINE_UNQUOTED([SOURCE_DATE_EPOCH], [${SOURCE_DATE_EPOCH}], [Reproducible build timestamp])
			])
			;;
		xno)
			REPRODUCIBLE_BUILDS="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-reproducible-builds])
			;;
	esac
])
