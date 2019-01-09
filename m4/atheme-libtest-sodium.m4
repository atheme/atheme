AC_DEFUN([ATHEME_LIBTEST_SODIUM], [

	LIBSODIUM="No"
	LIBSODIUMMEMORY="No"
	LIBSODIUMMEMZERO="No"
	LIBSODIUMRNG="No"

	AC_ARG_WITH([sodium],
		[AS_HELP_STRING([--without-sodium], [Do not attempt to detect libsodium for secure memory ops])],
		[], [with_sodium="auto"])

	case "${with_sodium}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-sodium])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "x${with_sodium}" != "xno"], [
		PKG_CHECK_MODULES([LIBSODIUM], [libsodium], [
			LIBS="${LIBSODIUM_LIBS} ${LIBS}"
			LIBSODIUM="Yes"
			AC_CHECK_HEADERS([sodium/core.h sodium/utils.h sodium/version.h], [], [
				LIBSODIUM="No"
				AS_IF([test "x${with_sodium}" = "xyes"], [
					AC_MSG_ERROR([--with-sodium was specified but a required header file is missing])
				])
			], [])
		], [
			LIBSODIUM="No"
			AS_IF([test "x${with_sodium}" = "xyes"], [
				AC_MSG_ERROR([--with-sodium was specified but libsodium could not be found])
			])
		])
	])

	AS_IF([test "x${LIBSODIUM}" = "xYes"], [
		AC_MSG_CHECKING([if libsodium is usable as a whole])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <sodium/core.h>
			]], [[
				(void) sodium_init();
			]])
		], [
			AC_MSG_RESULT([yes])
		], [
			AC_MSG_RESULT([no])
			LIBSODIUM="No"
			AS_IF([test "x${with_sodium}" = "xyes"], [
				AC_MSG_ERROR([--with-sodium was specified but libsodium appears to be unusable])
			])
		])
	])

	AS_IF([test "x${LIBSODIUM}" = "xYes"], [
		AC_MSG_CHECKING([if libsodium has usable memory manipulation functions])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <stddef.h>
				#include <sodium/core.h>
				#include <sodium/utils.h>
			]], [[
				(void) sodium_malloc(0);
				(void) sodium_mlock(NULL, 0);
				(void) sodium_mprotect_noaccess(NULL);
				(void) sodium_mprotect_readonly(NULL);
				(void) sodium_mprotect_readwrite(NULL);
				(void) sodium_munlock(NULL, 0);
				(void) sodium_free(NULL);
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBSODIUMMEMORY="Yes"
		], [
			AC_MSG_RESULT([no])
			LIBSODIUMMEMORY="No"
		])

		AC_MSG_CHECKING([if libsodium has a usable memory zeroing function])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <stddef.h>
				#include <sodium/core.h>
				#include <sodium/utils.h>
			]], [[
				(void) sodium_memzero(NULL, 0);
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBSODIUMMEMZERO="Yes"
			AC_DEFINE([HAVE_LIBSODIUM_MEMZERO], [1], [libsodium memzero is available])
		], [
			AC_MSG_RESULT([no])
			LIBSODIUMMEMZERO="No"
		])

		AC_CHECK_HEADERS([sodium/randombytes.h], [
			AC_MSG_CHECKING([if libsodium has a usable random number generator])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#include <stddef.h>
					#include <sodium/core.h>
					#include <sodium/utils.h>
					#include <sodium/randombytes.h>
				]], [[
					(void) randombytes_random();
					(void) randombytes_uniform(0);
					(void) randombytes_buf(NULL, 0);
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBSODIUMRNG="Yes"
			], [
				AC_MSG_RESULT([no])
				LIBSODIUMRNG="No"
			])
		], [], [])

		AS_IF([test "x${LIBSODIUMMEMORY}x${LIBSODIUMMEMZERO}x${LIBSODIUMRNG}" = "xNoxNoxNo"], [
			LIBSODIUM="No"
			AS_IF([test "x${with_sodium}" = "xyes"], [
				AC_MSG_ERROR([--with-sodium was specified but libsodium appears to be unusable])
			])
		], [
			AC_DEFINE([HAVE_LIBSODIUM], [1], [Define to 1 if libsodium is available])
		])
	])

	LIBS="${LIBS_SAVED}"
])
