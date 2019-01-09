ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT=""

AC_DEFUN([ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER], [

	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[

			#include <resolv.h>
		]], [[

			unsigned char nsbuf[4096];
			const int ret = res_query("", ns_c_any, ns_t_mx, nsbuf, sizeof nsbuf);
		]])
	], [
		ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT="yes"
	], [
		ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT="no"
	])
])

AC_DEFUN([ATHEME_LIBTEST_CONTRIB_RES_QUERY], [

	AC_MSG_CHECKING([whether compiling and linking a program using res_query(3) works])

	LIBS_SAVED="${LIBS}"

	ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER

	AS_IF([test "${ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT}" = "no"], [

		LIBS="-lresolv ${LIBS}"

		ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER

		AS_IF([test "${ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT}" = "no"], [

			AC_MSG_RESULT([no])
			AC_MSG_ERROR([--enable-contrib needs a functioning res_query(3)])
		])

		AS_IF([test "${ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT}" = "yes"], [

			CONTRIB_LIBS="-lresolv"
			AC_SUBST([CONTRIB_LIBS])
		])
	])

	LIBS="${LIBS_SAVED}"

	AC_MSG_RESULT([yes])
])

AC_DEFUN([ATHEME_FEATURETEST_CONTRIB], [

	CONTRIB_MODULES="No"

	CONTRIB_COND_D=""

	AC_ARG_ENABLE([contrib],
		[AS_HELP_STRING([--enable-contrib], [Enable contrib modules])],
		[], [enable_contrib="no"])

	case "x${enable_contrib}" in
		xyes)
			CONTRIB_MODULES="Yes"
			CONTRIB_COND_D="contrib"
			AC_SUBST([CONTRIB_COND_D])
			AC_DEFINE([ENABLE_CONTRIB_MODULES], [1], [Enable contrib modules])
			ATHEME_LIBTEST_CONTRIB_RES_QUERY
			;;
		xno)
			CONTRIB_MODULES="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-contrib])
			;;
	esac
])
