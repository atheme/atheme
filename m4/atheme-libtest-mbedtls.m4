AC_DEFUN([ATHEME_LIBTEST_MBEDTLS], [

	LIBMBEDCRYPTO="No"

	LIBMBEDCRYPTO_LIBS=""

	AC_ARG_WITH([mbedtls],
		[AS_HELP_STRING([--without-mbedtls], [Do not attempt to detect ARM mbedTLS])],
		[], [with_mbedtls="auto"])

	case "${with_mbedtls}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-mbedtls])
			;;
	esac

	AS_IF([test "x${with_mbedtls}" != "xno"], [
		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([mbedtls_md_setup], [mbedcrypto], [LIBMBEDCRYPTO="Yes"], [
			AS_IF([test "x${with_mbedtls}" != "xauto"], [
				AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBMBEDCRYPTO}" = "xYes"], [
		AC_CHECK_HEADERS([mbedtls/error.h], [], [], [])
		AC_CHECK_HEADERS([mbedtls/md.h mbedtls/pkcs5.h], [], [
			LIBMBEDCRYPTO="No"
			AS_IF([test "x${with_mbedtls}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
		], [])
	])

	AS_IF([test "x${LIBMBEDCRYPTO}" = "xYes"], [
		AC_MSG_CHECKING([if ARM mbedTLS has MD5/SHA1/SHA2 and PBKDF2 support])

		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM([[

				#ifdef HAVE_MBEDTLS_MD_H
				#include <mbedtls/md.h>
				#else
				#error "ARM mbedTLS support requires <mbedtls/md.h>"
				#endif

				#ifdef HAVE_MBEDTLS_PKCS5_H
				#include <mbedtls/pkcs5.h>
				#else
				#error "ARM mbedTLS support requires <mbedtls/pkcs5.h>"
				#endif

				#ifndef MBEDTLS_MD_C
				#error "ARM mbedTLS support requires MBEDTLS_MD_C to be enabled"
				#endif

				#ifndef MBEDTLS_MD5_C
				#error "ARM mbedTLS support requires MBEDTLS_MD5_C to be enabled"
				#endif

				#ifndef MBEDTLS_SHA1_C
				#error "ARM mbedTLS support requires MBEDTLS_SHA1_C to be enabled"
				#endif

				#ifndef MBEDTLS_SHA256_C
				#error "ARM mbedTLS support requires MBEDTLS_SHA256_C to be enabled"
				#endif

				#ifndef MBEDTLS_SHA512_C
				#error "ARM mbedTLS support requires MBEDTLS_SHA512_C to be enabled"
				#endif

				#ifndef MBEDTLS_PKCS5_C
				#error "ARM mbedTLS support requires MBEDTLS_PKCS5_C to be enabled"
				#endif

			]], [[

				return 0;

			]])
		], [
			AC_MSG_RESULT([yes])
		], [
			AC_MSG_RESULT([no])
			LIBMBEDCRYPTO="No"

			AS_IF([test "x${with_mbedtls}" != "xauto"],
				[AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS is unusable for this task])])
		])
	])

	AS_IF([test "x${LIBMBEDCRYPTO}x${ARC4RANDOM_BUILDING}" = "xYesxYes"], [
		AC_CHECK_HEADERS([mbedtls/entropy.h mbedtls/hmac_drbg.h], [], [], [])
		AC_MSG_CHECKING([if ARM mbedTLS has HMAC-DRBG support])

		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM([[

				#ifdef HAVE_MBEDTLS_ENTROPY_H
				#include <mbedtls/entropy.h>
				#else
				#error "HMAC-DRBG support requires <mbedtls/entropy.h>"
				#endif

				#ifdef HAVE_MBEDTLS_HMAC_DRBG_H
				#include <mbedtls/hmac_drbg.h>
				#else
				#error "HMAC-DRBG support requires <mbedtls/hmac_drbg.h>"
				#endif

				#ifndef MBEDTLS_ENTROPY_C
				#error "HMAC-DRBG support requires MBEDTLS_ENTROPY_C"
				#endif

				#ifndef MBEDTLS_HMAC_DRBG_C
				#error "HMAC-DRBG support requires MBEDTLS_HMAC_DRBG_C"
				#endif

			]], [[

				return 0;

			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBMBEDCRYPTO_HMAC_DRBG], [1], [Define to 1 if ARM mbedTLS has HMAC-DRBG support.])

			ARC4RANDOM_BUILDING="Yes (ARM mbedTLS HMAC-DRBG)"
		], [
			AC_MSG_RESULT([no])
		])
	])

	AS_IF([test "x${LIBMBEDCRYPTO}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_mbedtls_md_setup}" != "xnone required"], [
			LIBMBEDCRYPTO_LIBS="${ac_cv_search_mbedtls_md_setup}"
		])
		AC_DEFINE([HAVE_LIBMBEDCRYPTO], [1], [Define to 1 if we have ARM mbedTLS available])
		AC_SUBST([LIBMBEDCRYPTO_LIBS])
	])
])
