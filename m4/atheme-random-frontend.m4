AC_DEFUN([ATHEME_DECIDE_RANDOM_FRONTEND], [

	RANDOM_FRONTEND_VAL="ATHEME_RANDOM_FRONTEND_INTERNAL"
	RANDOM_FRONTEND="Internal"
	RANDOM_API_CFLAGS=""
	RANDOM_API_LIBS=""

	AC_MSG_CHECKING([if we are building for OpenBSD])
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[
			#ifdef __OpenBSD__
			#  error "Detected OpenBSD"
			#endif
		]], [[
			return 0;
		]])
	], [
		AC_MSG_RESULT([no])
	], [
		AC_MSG_RESULT([yes])
		AC_MSG_CHECKING([if arc4random(3) is usable])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#ifdef HAVE_STDLIB_H
				#  include <stdlib.h>
				#endif
			]], [[
				(void) arc4random();
				(void) arc4random_uniform(0);
				(void) arc4random_buf(NULL, 0);
			]])
		], [
			AC_MSG_RESULT([yes])
			RANDOM_FRONTEND_VAL="ATHEME_RANDOM_FRONTEND_OPENBSD"
			RANDOM_FRONTEND="OpenBSD arc4random(3)"
			RANDOM_API_CFLAGS=""
			RANDOM_API_LIBS=""
		], [
			AC_MSG_RESULT([no])
		])
	])

	AS_IF([test "${RANDOM_FRONTEND}${LIBSODIUMRNG}" = "InternalYes"], [
		RANDOM_FRONTEND_VAL="ATHEME_RANDOM_FRONTEND_SODIUM"
		RANDOM_FRONTEND="Sodium"
		RANDOM_API_CFLAGS="${LIBSODIUM_CFLAGS}"
		RANDOM_API_LIBS="${LIBSODIUM_LIBS}"
	])

	AS_IF([test "${RANDOM_FRONTEND}${LIBMBEDCRYPTORNG}" = "InternalYes"], [
		RANDOM_FRONTEND_VAL="ATHEME_RANDOM_FRONTEND_MBEDTLS"
		RANDOM_FRONTEND="ARM mbedTLS"
		RANDOM_API_CFLAGS=""
		RANDOM_API_LIBS="${LIBMBEDCRYPTO_LIBS}"
	])

	AS_IF([test "${RANDOM_FRONTEND}${LIBCRYPTORNG}" = "InternalYes"], [
		RANDOM_FRONTEND_VAL="ATHEME_RANDOM_FRONTEND_OPENSSL"
		RANDOM_FRONTEND="${LIBCRYPTO_NAME}"
		RANDOM_API_CFLAGS="${LIBCRYPTO_CFLAGS}"
		RANDOM_API_LIBS="${LIBCRYPTO_LIBS}"
	])

	AS_IF([test "${RANDOM_FRONTEND}" = "Internal"], [
		AC_MSG_CHECKING([if getentropy(3) is available])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#ifdef HAVE_STDLIB_H
				#  include <stdlib.h>
				#endif
				#ifdef HAVE_UNISTD_H
				#  include <unistd.h>
				#endif
			]], [[
				(void) getentropy(NULL, 0);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_USABLE_GETENTROPY], [1], [Define to 1 if getentropy(3) is usable])
		], [
			AC_MSG_RESULT([no])
			AC_MSG_CHECKING([if getrandom(2) is available])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_STDDEF_H
					#  include <stddef.h>
					#endif
					#include <sys/random.h>
				]], [[
					(void) getrandom(NULL, 0, 0);
				]])
			], [
				AC_MSG_RESULT([yes])
				AC_DEFINE([HAVE_USABLE_GETRANDOM], [1], [Define to 1 if getrandom(2) is usable])
			], [
				AC_MSG_RESULT([no])
				AC_MSG_WARN([this program will need access to the urandom(4) device at run-time])
			])
		])
	])

	AC_DEFINE_UNQUOTED([ATHEME_RANDOM_FRONTEND], [${RANDOM_FRONTEND_VAL}], [Front-end for RNG interface])
])
