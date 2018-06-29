AC_DEFUN([ATHEME_FEATURETEST_CONTRIB], [

	CONTRIB_MODULES="No"

	CONTRIB_COND_D=""

	AC_ARG_ENABLE([contrib],
		[AS_HELP_STRING([--enable-contrib], [Enable contrib modules])],
		[], [enable_contrib="no"])

	case "${enable_contrib}" in
		yes)
			CONTRIB_MODULES="Yes"
			CONTRIB_COND_D="contrib"
			AC_SUBST([CONTRIB_COND_D])
			AC_DEFINE([ENABLE_CONTRIB_MODULES], [1], [Enable contrib modules])
			;;
		no)
			CONTRIB_MODULES="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-contrib])
			;;
	esac
])
