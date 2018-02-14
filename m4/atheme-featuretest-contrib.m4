AC_DEFUN([ATHEME_FEATURETEST_CONTRIB], [

	CONTRIB_MODULES="No"

	CONTRIB_ENABLE=""

	AC_ARG_ENABLE([contrib],
		[AS_HELP_STRING([--enable-contrib],[ Enable contrib modules.])],
		[], [enable_contrib="no"])

	case "${enable_contrib}" in
		yes)
			CONTRIB_MODULES="Yes"
			CONTRIB_ENABLE="contrib"
			AC_SUBST([CONTRIB_ENABLE])
			;;
		no)
			CONTRIB_MODULES="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-contrib])
			;;
	esac
])
