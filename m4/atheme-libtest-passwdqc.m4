AC_DEFUN([ATHEME_LIBTEST_PASSWDQC], [

	LIBPASSWDQC="No"
	LIBPASSWDQC_LIBS=""

	AC_ARG_WITH([passwdqc],
		[AS_HELP_STRING([--without-passwdqc], [Do not attempt to detect libpasswdqc (for modules/nickserv/pwquality -- checking password strength)])],
		[], [with_passwdqc="auto"])

	case "x${with_passwdqc}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-passwdqc])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_passwdqc}" != "no"], [
		AC_SEARCH_LIBS([passwdqc_check], [passwdqc], [
			AC_CHECK_HEADERS([passwdqc.h], [
				AC_MSG_CHECKING([if libpasswdqc appears to be usable])
				AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([[
						#ifdef HAVE_STDDEF_H
						#  include <stddef.h>
						#endif
						#include <passwdqc.h>
					]], [[
						passwdqc_params_qc_t qc_config = {
							.min                    = { 32, 24, 11, 8, 7 },
							.max                    = 288,
							.passphrase_words       = 3,
							.match_length           = 4,
						};
						(void) passwdqc_check(&qc_config, NULL, NULL, NULL);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBPASSWDQC="Yes"
					AS_IF([test "x${ac_cv_search_passwdqc_check}" != "xnone required"], [
						LIBPASSWDQC_LIBS="${ac_cv_search_passwdqc_check}"
						AC_SUBST([LIBPASSWDQC_LIBS])
					])
					AC_DEFINE([HAVE_LIBPASSWDQC], [1], [Define to 1 if libpasswdqc is available])
					AC_DEFINE([HAVE_ANY_PASSWORD_QUALITY_LIBRARY], [1], [Define to 1 if any password quality library is available])
				], [
					AC_MSG_RESULT([no])
					LIBPASSWDQC="No"
					AS_IF([test "${with_passwdqc}" = "yes"], [
						AC_MSG_FAILURE([--with-passwdqc was specified but passwdqc appears to be unusable])
					])
				])
			], [
				LIBPASSWDQC="No"
				AS_IF([test "${with_passwdqc}" = "yes"], [
					AC_MSG_ERROR([--with-passwdqc was specified but a required header file is missing])
				])
			], [])
		], [
			LIBPASSWDQC="No"
			AS_IF([test "${with_passwdqc}" = "yes"], [
				AC_MSG_ERROR([--with-passwdqc was specified but passwdqc could not be found])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
