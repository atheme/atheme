AC_DEFUN([ATHEME_LIBTEST_SODIUM], [

	LIBSODIUM="No"
	LIBSODIUMMEMORY="No"
	LIBSODIUMMEMZERO="No"
	LIBSODIUMRNG="No"
	LIBSODIUMSCRYPT="No"

	AC_ARG_WITH([sodium],
		[AS_HELP_STRING([--without-sodium], [Do not attempt to detect libsodium (cryptographic library)])],
		[], [with_sodium="auto"])

	case "x${with_sodium}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-sodium])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_sodium}" != "no"], [
		PKG_CHECK_MODULES([LIBSODIUM], [libsodium], [
			LIBS="${LIBSODIUM_LIBS} ${LIBS}"
			LIBSODIUM="Yes"
			AC_CHECK_HEADERS([sodium/core.h sodium/utils.h sodium/version.h], [], [
				LIBSODIUM="No"
				AS_IF([test "${with_sodium}" = "yes"], [
					AC_MSG_ERROR([--with-sodium was specified but a required header file is missing])
				])
			], [])
		], [
			LIBSODIUM="No"
			AS_IF([test "${with_sodium}" = "yes"], [
				AC_MSG_ERROR([--with-sodium was specified but libsodium could not be found])
			])
		])
	])

	AS_IF([test "${LIBSODIUM}" = "Yes"], [
		AC_MSG_CHECKING([if libsodium is usable as a whole])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <sodium/core.h>
			]], [[
				(void) sodium_init();
			]])
		], [
			AC_MSG_RESULT([yes])

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
			], [
				LIBSODIUMRNG="No"
			], [])

			AC_CHECK_HEADERS([sodium/crypto_pwhash_scryptsalsa208sha256.h], [
				AC_MSG_CHECKING([if libsodium has a usable scrypt password hash generator])
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h>
						#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
					]], [[
						const unsigned long long int opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_MIN;
						const size_t memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_MIN;
						(void) crypto_pwhash_scryptsalsa208sha256_str_needs_rehash(NULL, 0, 0);
						(void) crypto_pwhash_scryptsalsa208sha256_str_verify(NULL, NULL, 0);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBSODIUMSCRYPT="Yes"
				], [
					AC_MSG_RESULT([no])
					LIBSODIUMSCRYPT="No"
				])
			], [
				LIBSODIUMSCRYPT="No"
			], [])

			AS_IF([test "${LIBSODIUMMEMORY}${LIBSODIUMMEMZERO}${LIBSODIUMRNG}${LIBSODIUMSCRYPT}" = "NoNoNoNo"], [
				LIBSODIUM="No"
				AS_IF([test "${with_sodium}" = "yes"], [
					AC_MSG_FAILURE([--with-sodium was specified but libsodium appears to be unusable])
				])
			], [
				AC_DEFINE([HAVE_LIBSODIUM], [1], [Define to 1 if libsodium is available])
				AS_IF([test "${LIBSODIUMMEMZERO}" = "Yes"], [
					AC_DEFINE([HAVE_LIBSODIUM_MEMZERO], [1], [Define to 1 if libsodium memzero is available])
				])
				AS_IF([test "${LIBSODIUMSCRYPT}" = "Yes"], [
					AC_DEFINE([HAVE_LIBSODIUM_SCRYPT], [1], [Define to 1 if libsodium scrypt is available])
				])
			])
		], [
			AC_MSG_RESULT([no])
			LIBSODIUM="No"
			AS_IF([test "${with_sodium}" = "yes"], [
				AC_MSG_FAILURE([--with-sodium was specified but libsodium appears to be unusable])
			])
		])
	])

	AS_IF([test "${LIBSODIUM}" = "No"], [
		LIBSODIUM_CFLAGS=""
		LIBSODIUM_LIBS=""
	])

	AC_SUBST([LIBSODIUM_CFLAGS])
	AC_SUBST([LIBSODIUM_LIBS])

	LIBS="${LIBS_SAVED}"
])
