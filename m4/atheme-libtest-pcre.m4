AC_DEFUN([ATHEME_LIBTEST_PCRE], [

	AS_IF([test "x${with_pcre}" != "xno"], [

		PKG_CHECK_MODULES([PCRE], [libpcre], [LIBPCRE="Yes"], [

			AS_IF([test "x${with_pcre}" != "xauto"], [
				AC_MSG_ERROR([--with-pcre was specified but the PCRE library could not be found])
			])
		])
	])

	AS_IF([test "x${LIBPCRE}" = "xYes"], [

		AC_DEFINE([HAVE_PCRE], [1], [Define to 1 if PCRE is available.])

		AC_SUBST([PCRE_CFLAGS])
		AC_SUBST([PCRE_LIBS])
	])
])
