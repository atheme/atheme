AC_DEFUN([ATHEME_LIBTEST_IDN], [

	AS_IF([test "x${with_libidn}" != "xno"], [

		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([stringprep], [idn], [LIBIDN="Yes"], [

			AS_IF([test "x${with_libidn}" != "xauto"], [
				AC_MSG_ERROR([--with-libidn was specified but GNU libidn could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBIDN}" = "xYes"], [

		AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if we have GNU libidn available])

		AS_IF([test "x${ac_cv_search_stringprep}" != "xnone required"],
			[LIBIDN_LIBS="${ac_cv_search_stringprep}"])

		AC_SUBST([LIBIDN_LIBS])
	])
])
