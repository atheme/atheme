AC_DEFUN([ATHEME_LIBTEST_ARGON2], [

	LIBARGON2="No"

	AC_ARG_WITH([libargon2],
		[AS_HELP_STRING([--without-libargon2], [Do not attempt to detect libargon2 (for modules/crypto/argon2)])],
		[], [with_libargon2="auto"])

	case "x${with_libargon2}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-libargon2])
			;;
	esac

	CPPFLAGS_SAVED="${CPPFLAGS}"
	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_libargon2}" != "no"], [
		LIBARGON2="Yes"

		dnl Signal blocking functions are necessary for thread-safety    -- amdj
		AC_CHECK_FUNCS([sigfillset sigprocmask], [], [
			LIBARGON2="No"
			AS_IF([test "${with_libargon2}" = "yes"], [
				AC_MSG_FAILURE([--with-libargon2 was given but required signal functions are not available])
			])
		])
	], [
		LIBARGON2="No"
	])

	AS_IF([test "${LIBARGON2}" = "Yes"], [
		PKG_CHECK_MODULES([LIBARGON2], [libargon2], [
			CPPFLAGS="${LIBARGON2_CFLAGS} ${CPPFLAGS}"
			LIBS="${LIBARGON2_LIBS} ${LIBS}"
			AC_MSG_CHECKING([if libargon2 appears to be usable])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_STDDEF_H
					#  include <stddef.h>
					#endif
					#include <argon2.h>
				]], [[
					argon2_context ctx = {
						.out = NULL,
						.outlen = 0,
						.pwd = NULL,
						.pwdlen = 0,
						.salt = NULL,
						.saltlen = 0,
						.secret = NULL,
						.secretlen = 0,
						.ad = NULL,
						.adlen = 0,
						.t_cost = ARGON2_MAX_TIME,
						.m_cost = ARGON2_MAX_MEMORY,
						.lanes = 1,
						.threads = 1,
						.version = ARGON2_VERSION_13,
						.allocate_cbk = NULL,
						.free_cbk = NULL,
						.flags = 0,
					};
					(void) argon2_ctx(&ctx, Argon2_d);
					(void) argon2_ctx(&ctx, Argon2_i);
					(void) argon2_ctx(&ctx, Argon2_id);
					(void) argon2_error_message(0);
					(void) argon2_type2string(0, 0);
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBARGON2="Yes"
				AC_DEFINE([HAVE_LIBARGON2], [1], [Define to 1 if libargon2 appears to be usable])
			], [
				AC_MSG_RESULT([no])
				LIBARGON2="No"
				AS_IF([test "${with_libargon2}" = "yes"], [
					AC_MSG_FAILURE([--with-libargon2 was given but libargon2 appears to be unusable])
				])
			])
		], [
			LIBARGON2="No"
			AS_IF([test "${with_libargon2}" = "yes"], [
				AC_MSG_ERROR([--with-libargon2 was given but libargon2 could not be found])
			])
		])
	])

	CPPFLAGS="${CPPFLAGS_SAVED}"
	LIBS="${LIBS_SAVED}"

	AS_IF([test "${LIBARGON2}" = "No"], [
		LIBARGON2_CFLAGS=""
		LIBARGON2_LIBS=""
	])

	AC_SUBST([LIBARGON2_CFLAGS])
	AC_SUBST([LIBARGON2_LIBS])
])
