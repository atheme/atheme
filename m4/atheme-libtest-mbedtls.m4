AC_DEFUN([ATHEME_LIBTEST_MBEDTLS], [

	LIBMBEDCRYPTO="No"
	LIBMBEDCRYPTORNG="No"
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

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_mbedtls}" != "no"], [
		AC_SEARCH_LIBS([mbedtls_md_setup], [mbedcrypto], [
			LIBMBEDCRYPTO="Yes"
			AC_CHECK_HEADERS([mbedtls/error.h mbedtls/version.h], [], [], [])
			AC_CHECK_HEADERS([mbedtls/md.h mbedtls/pkcs5.h], [], [
				LIBMBEDCRYPTO="No"
				AS_IF([test "${with_mbedtls}" = "yes"], [
					AC_MSG_ERROR([--with-mbedtls was specified but a required header file is missing])
				])
			], [])
		], [
			LIBMBEDCRYPTO="No"
			AS_IF([test "${with_mbedtls}" = "yes"], [
				AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS could not be found])
			])
		])
	])

	AS_IF([test "${LIBMBEDCRYPTO}" = "Yes"], [
		AC_MSG_CHECKING([if ARM mbedTLS has MD5/SHA1/SHA2/PBKDF2 support])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <stddef.h>
				#include <mbedtls/md.h>
				#include <mbedtls/pkcs5.h>
				#ifndef MBEDTLS_MD_C
				#  error "ARM mbedTLS support requires MBEDTLS_MD_C to be enabled"
				#endif
				#ifndef MBEDTLS_MD5_C
				#  error "ARM mbedTLS support requires MBEDTLS_MD5_C to be enabled"
				#endif
				#ifndef MBEDTLS_SHA1_C
				#  error "ARM mbedTLS support requires MBEDTLS_SHA1_C to be enabled"
				#endif
				#ifndef MBEDTLS_SHA256_C
				#  error "ARM mbedTLS support requires MBEDTLS_SHA256_C to be enabled"
				#endif
				#ifndef MBEDTLS_SHA512_C
				#  error "ARM mbedTLS support requires MBEDTLS_SHA512_C to be enabled"
				#endif
				#ifndef MBEDTLS_PKCS5_C
				#  error "ARM mbedTLS support requires MBEDTLS_PKCS5_C to be enabled"
				#endif
			]], [[
				(void) mbedtls_md_info_from_type(MBEDTLS_MD_NONE);
				(void) mbedtls_md_init(NULL);
				(void) mbedtls_md_setup(NULL, NULL, 0);
				(void) mbedtls_md_starts(NULL);
				(void) mbedtls_md_update(NULL, NULL, 0);
				(void) mbedtls_md_finish(NULL, NULL);
				(void) mbedtls_md_hmac_starts(NULL, NULL, 0);
				(void) mbedtls_md_hmac_update(NULL, NULL, 0);
				(void) mbedtls_md_hmac_finish(NULL, NULL);
				(void) mbedtls_md_free(NULL);
				(void) mbedtls_pkcs5_pbkdf2_hmac(NULL, NULL, 0, NULL, 0, 0, 0, NULL);
			]])
		], [
			AC_MSG_RESULT([yes])
			AS_IF([test "x${ac_cv_search_mbedtls_md_setup}" != "xnone required"], [
				LIBMBEDCRYPTO_LIBS="${ac_cv_search_mbedtls_md_setup}"
				AC_SUBST([LIBMBEDCRYPTO_LIBS])
			])
			AC_DEFINE([HAVE_LIBMBEDCRYPTO], [1], [Define to 1 if we have ARM mbedTLS available])

			AC_CHECK_HEADERS([mbedtls/entropy.h mbedtls/hmac_drbg.h], [], [], [])
			AC_MSG_CHECKING([if ARM mbedTLS has HMAC-DRBG support])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_MBEDTLS_ENTROPY_H
					#  include <mbedtls/entropy.h>
					#else
					#  error "HMAC-DRBG support requires <mbedtls/entropy.h>"
					#endif
					#ifdef HAVE_MBEDTLS_HMAC_DRBG_H
					#  include <mbedtls/hmac_drbg.h>
					#else
					#  error "HMAC-DRBG support requires <mbedtls/hmac_drbg.h>"
					#endif
					#ifndef MBEDTLS_ENTROPY_C
					#  error "HMAC-DRBG support requires MBEDTLS_ENTROPY_C"
					#endif
					#ifndef MBEDTLS_HMAC_DRBG_C
					#  error "HMAC-DRBG support requires MBEDTLS_HMAC_DRBG_C"
					#endif
				]], [[
					(void) mbedtls_entropy_init(NULL);
					(void) mbedtls_hmac_drbg_init(NULL);
					(void) mbedtls_hmac_drbg_seed(NULL, NULL, NULL, NULL, NULL, 0);
					(void) mbedtls_hmac_drbg_reseed(NULL, NULL, 0);
					(void) mbedtls_hmac_drbg_random(NULL, NULL, 0);
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBMBEDCRYPTORNG="Yes"
				AC_DEFINE([HAVE_LIBMBEDCRYPTO_HMAC_DRBG], [1], [Define to 1 if ARM mbedTLS has HMAC-DRBG support.])
			], [
				AC_MSG_RESULT([no])
				LIBMBEDCRYPTORNG="No"
			])
		], [
			AC_MSG_RESULT([no])
			LIBMBEDCRYPTO="No"
			AS_IF([test "${with_mbedtls}" = "yes"], [
				AC_MSG_ERROR([--with-mbedtls was specified but ARM mbedTLS appears to be unusable])
			])
		])
	])

	LIBS="${LIBS_SAVED}"
])
