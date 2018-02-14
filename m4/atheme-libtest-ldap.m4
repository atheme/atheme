AC_DEFUN([ATHEME_LIBTEST_LDAP], [

	AC_ARG_WITH([ldap],
		[AS_HELP_STRING([--without-ldap], [Do not attempt to detect LDAP for modules/auth/ldap])],
		[], [with_ldap="auto"])

	case "${with_ldap}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-ldap])
			;;
	esac

	AS_IF([test "x${with_ldap}" != "xno"], [

		LDAP_AUTHC=""
		LDAP_CFLAGS=""
		LDAP_LIBS=""

		AC_CHECK_LIB([ldap], [ldap_initialize], [
			LDAP_AUTHC="ldap.c"
			LDAP_CFLAGS=""
			LDAP_LIBS="-lldap"
		], [
			unset ac_cv_lib_ldap_ldap_initialize

			CFLAGS_SAVED="${CFLAGS}"
			LIBS_SAVED="${LIBS}"

			CFLAGS="${CFLAGS} -I/usr/local/include"
			LIBS="${LIBS} -L/usr/local/lib"

			AC_CHECK_LIB([ldap], [ldap_initialize], [
				LDAP_AUTHC="ldap.c"
				LDAP_CFLAGS="-I/usr/local/include"
				LDAP_LIBS="-L/usr/local/lib -lldap"
			])

			CFLAGS="${CFLAGS_SAVED}"
			LIBS="${LIBS_SAVED}"
		])

		AC_SUBST([LDAP_AUTHC])
		AC_SUBST([LDAP_CFLAGS])
		AC_SUBST([LDAP_LIBS])
	])

	AS_IF([test "x${with_ldap}x${LDAP_AUTHC}" = "xyesx"], [
		AC_MSG_ERROR([LDAP support was explicitly requested but could not be found])
	])
])
