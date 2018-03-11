AC_DEFUN([ATHEME_LIBTEST_LDAP], [

	LIBLDAP="No"

	LDAP_CPPFLAGS=""
	LDAP_LDFLAGS=""
	LDAP_LIBS=""

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

		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([ldap_initialize], [ldap], [LIBLDAP="Yes"], [

			unset ac_cv_search_ldap_initialize

			CPPFLAGS_SAVED="${CPPFLAGS}"
			LDFLAGS_SAVED="${LDFLAGS}"

			CPPFLAGS="${CPPFLAGS} -I/usr/local/include"
			LDFLAGS="${LDFLAGS} -L/usr/local/lib"

			AC_SEARCH_LIBS([ldap_initialize], [ldap], [
				LIBLDAP="Yes"
				LDAP_CPPFLAGS="-I/usr/local/include"
				LDAP_LDFLAGS="-L/usr/local/lib"
			], [
				AS_IF([test "x${with_ldap}" != "xauto"], [
					AC_MSG_ERROR([--with-ldap was specified but LDAP library could not be found])
				])
			])

			CPPFLAGS="${CPPFLAGS_SAVED}"
			LDFLAGS="${LDFLAGS_SAVED}"
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBLDAP}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_ldap_initialize}" != "xnone required"], [
			LDAP_LIBS="${ac_cv_search_ldap_initialize}"
		])
		AC_DEFINE([HAVE_LDAP], [1], [Define to 1 if LDAP support is available])
		AC_SUBST([LDAP_CPPFLAGS])
		AC_SUBST([LDAP_LDFLAGS])
		AC_SUBST([LDAP_LIBS])
	])
])
