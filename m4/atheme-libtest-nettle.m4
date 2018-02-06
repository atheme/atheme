AC_DEFUN([ATHEME_LIBTEST_NETTLE], [

	AS_IF([test "x${with_nettle}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([nettle_md5_init], [nettle], [LIBNETTLE="Yes"], [
			AS_IF([test "x${with_nettle}" != "xauto"], [
				AC_MSG_ERROR([--with-nettle was specified but libnettle could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBNETTLE}" = "xYes"], [
		AC_CHECK_HEADERS([nettle/md5.h nettle/sha1.h nettle/sha2.h nettle/nettle-meta.h], [], [
			LIBNETTLE="No"
			AS_IF([test "x${with_nettle}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
		], [])
	])

	AS_IF([test "x${LIBNETTLE}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_nettle_md5_init}" != "xnone required"], [
			LIBNETTLE_LIBS="${ac_cv_search_nettle_md5_init}"
		])
		AC_DEFINE([HAVE_LIBNETTLE], [1], [Define to 1 if we have libnettle available])
		AC_SUBST([LIBNETTLE_LIBS])
	])
])
