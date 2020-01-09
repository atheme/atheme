AC_DEFUN([ATHEME_LIBTEST_ARGON2], [

	LIBARGON2="No"
	LIBARGON2_PATH=""

	AC_ARG_WITH([libargon2],
		[AS_HELP_STRING([--without-libargon2], [Do not attempt to detect libargon2 (for modules/crypto/argon2)])],
		[], [with_libargon2="auto"])

	case "x${with_libargon2}" in
		xno | xyes | xauto)
			;;
		x/*)
			LIBARGON2_PATH="${with_libargon2}"
			with_libargon2="yes"
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-libargon2])
			;;
	esac

	CPPFLAGS_SAVED="${CPPFLAGS}"
	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_libargon2}" != "no"], [
		AS_IF([test -n "${LIBARGON2_PATH}"], [
			dnl Allow for user to provide custom installation directory
			AS_IF([test -d "${LIBARGON2_PATH}/include" -a -d "${LIBARGON2_PATH}/lib"], [
				LIBARGON2_CFLAGS="-I${LIBARGON2_PATH}/include"
				LIBARGON2_LIBS="-L${LIBARGON2_PATH}/lib"
			], [
				AC_MSG_ERROR([${LIBARGON2_PATH} is not a suitable directory for libargon2])
			])
		], [test -n "${PKG_CONFIG}"], [
			dnl Allow for the user to "override" pkg-config without it being installed
			PKG_CHECK_MODULES([LIBARGON2], [libargon2], [], [])
		])
		AS_IF([test -n "${LIBARGON2_CFLAGS+set}" -a -n "${LIBARGON2_LIBS+set}"], [
			dnl Only proceed with library tests if custom paths were given or pkg-config succeeded
			LIBARGON2="Yes"
		], [
			LIBARGON2="No"
			AS_IF([test "${with_libargon2}" != "auto"], [
				AC_MSG_FAILURE([--with-libargon2 was given but libargon2 could not be found])
			])
		])
	])

	AS_IF([test "${LIBARGON2}" = "Yes"], [
		CPPFLAGS="${LIBARGON2_CFLAGS} ${CPPFLAGS}"
		LIBS="${LIBARGON2_LIBS} ${LIBS}"

		AC_MSG_CHECKING([if libargon2 appears to be usable])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#ifdef HAVE_SIGNAL_H
				#  include <signal.h>
				#endif
				#include <argon2.h>
			]], [[
				// POSIX.1-2001 Signal-blocking functions are necessary for thread-safety
				sigset_t oldset;
				sigset_t newset;
				(void) sigfillset(&newset);
				(void) sigprocmask(SIG_BLOCK, &newset, &oldset);

				argon2_context ctx = {
					.out = NULL,
					.outlen = 0,
					.pwd = NULL,
					.pwdlen = 0,
					.salt = NULL,
					.saltlen = 0,
					.t_cost = ARGON2_MAX_TIME,
					.m_cost = ARGON2_MAX_MEMORY,
					.lanes = 1,
					.threads = 1,
					.version = ARGON2_VERSION_NUMBER,
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
			AS_IF([test "${with_libargon2}" != "auto"], [
				AC_MSG_FAILURE([--with-libargon2 was given but libargon2 appears to be unusable])
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
