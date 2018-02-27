AC_DEFUN([ATHEME_LIBTEST_LDAP], [

	LIBLDAP="No"

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

		LDAP_AUTH_COND_C=""
		LDAP_CPPFLAGS=""
		LDAP_LDFLAGS=""
		LDAP_LIBS=""

		CPPFLAGS_SAVED="${CPPFLAGS}"
		LDFLAGS_SAVED="${LDFLAGS}"
		LIBS_SAVED="${LIBS}"

		AC_CHECK_LIB([ldap], [ldap_initialize], [
			LIBLDAP="Yes"
			LDAP_AUTH_COND_C="ldap.c"
			LDAP_LIBS="-lldap"
		], [
			unset ac_cv_lib_ldap_ldap_initialize

			CPPFLAGS="${CPPFLAGS} -I/usr/local/include"
			LDFLAGS="${LDFLAGS} -L/usr/local/lib"

			AC_CHECK_LIB([ldap], [ldap_initialize], [
				LIBLDAP="Yes"
				LDAP_AUTH_COND_C="ldap.c"
				LDAP_CPPFLAGS="-I/usr/local/include"
				LDAP_LDFLAGS="-L/usr/local/lib"
				LDAP_LIBS="-lldap"
			])
		])

		AC_SUBST([LDAP_AUTH_COND_C])
		AC_SUBST([LDAP_CPPFLAGS])
		AC_SUBST([LDAP_LDFLAGS])
		AC_SUBST([LDAP_LIBS])

		CPPFLAGS="${CPPFLAGS_SAVED}"
		LDFLAGS="${LDFLAGS_SAVED}"
		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${with_ldap}x${LDAP_AUTH_COND_C}" = "xyesx"], [
		AC_MSG_ERROR([LDAP support was explicitly requested but LDAP library could not be found])
	])
])
