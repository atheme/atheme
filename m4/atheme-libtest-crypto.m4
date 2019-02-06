AC_DEFUN([ATHEME_LIBTEST_CRYPTO], [

	LIBCRYPTO="No"
	LIBCRYPTORNG="No"
	LIBCRYPTO_NAME=""

	AC_ARG_WITH([openssl],
		[AS_HELP_STRING([--without-openssl], [Do not attempt to detect libcrypto (for modules/saslserv/ecdsa-nist256p-challenge)])],
		[], [with_openssl="auto"])

	case "x${with_openssl}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-openssl])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_openssl}" != "no"], [
		PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [
			LIBS="${LIBCRYPTO_LIBS} ${LIBS}"
			LIBCRYPTO="Yes"
			AC_CHECK_HEADERS([openssl/ec.h openssl/ecdsa.h], [], [], [])
			AC_CHECK_HEADERS([openssl/evp.h openssl/hmac.h openssl/opensslv.h], [], [
				LIBCRYPTO="No"
				AS_IF([test "${with_openssl}" = "yes"], [
					AC_MSG_ERROR([--with-openssl was specified but a required header file is missing])
				])
			], [])
		], [
			LIBCRYPTO="No"
			AS_IF([test "${with_openssl}" = "yes"], [
				AC_MSG_ERROR([--with-openssl was specified but libcrypto could not be found])
			])
		])
	])

	AS_IF([test "${LIBCRYPTO}" = "Yes"], [
		AC_MSG_CHECKING([if libcrypto is usable])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <openssl/evp.h>
				#include <openssl/hmac.h>
			]], [[
				(void) EVP_MD_size(NULL);
				(void) EVP_DigestInit_ex(NULL, EVP_md5(), NULL);
				(void) EVP_DigestInit_ex(NULL, EVP_sha1(), NULL);
				(void) EVP_DigestInit_ex(NULL, EVP_sha256(), NULL);
				(void) EVP_DigestInit_ex(NULL, EVP_sha512(), NULL);
				(void) EVP_DigestUpdate(NULL, NULL, 0);
				(void) EVP_DigestFinal_ex(NULL, NULL, NULL);
				(void) HMAC_Init_ex(NULL, NULL, 0, EVP_md5(), NULL);
				(void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha1(), NULL);
				(void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha256(), NULL);
				(void) HMAC_Init_ex(NULL, NULL, 0, EVP_sha512(), NULL);
				(void) HMAC_Update(NULL, NULL, 0);
				(void) HMAC_Final(NULL, NULL, NULL);
				(void) PKCS5_PBKDF2_HMAC(NULL, 0, NULL, 0, 0, NULL, 0, NULL);
			]])
		], [
			AC_MSG_RESULT([yes])

			AC_MSG_CHECKING([if OpenSSL has a usable random number generator])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_STDDEF_H
					#  include <stddef.h>
					#endif
					#include <openssl/err.h>
					#include <openssl/rand.h>
				]], [[
					(void) ERR_get_error();
					(void) ERR_get_error_line_data(NULL, NULL, NULL, NULL);
					(void) RAND_add(NULL, 0, (double) 0);
					(void) RAND_bytes(NULL, 0);
					(void) RAND_status();
				]])
			], [
				AC_MSG_RESULT([yes])
				LIBCRYPTORNG="Yes"
			], [
				AC_MSG_RESULT([no])
				LIBCRYPTORNG="No"
			])

			AC_MSG_CHECKING([if OpenSSL provides HMAC_CTX_new()/HMAC_CTX_free()])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#ifdef HAVE_STDDEF_H
					#  include <stddef.h>
					#endif
					#include <openssl/hmac.h>
				]], [[
					(void) HMAC_CTX_new();
					(void) HMAC_CTX_free(NULL);
				]])
			], [
				AC_MSG_RESULT([yes])
				AC_DEFINE([HAVE_OPENSSL_HMAC_CTX_ALLOC], [1], [Define to 1 if HMAC_CTX_new()/_free() exist])
			], [
				AC_MSG_RESULT([no])
			])

			AC_MSG_CHECKING([if OpenSSL is really LibreSSL in disguise])
			AC_COMPILE_IFELSE([
				AC_LANG_PROGRAM([[
					#include <openssl/opensslv.h>
					#ifdef LIBRESSL_VERSION_NUMBER
					#  error "Detected LibreSSL"
					#endif
				]], [[
					return 0;
				]])
			], [
				AC_MSG_RESULT([no])
				LIBCRYPTO_NAME="OpenSSL"
			], [
				AC_MSG_RESULT([yes])
				LIBCRYPTO_NAME="LibreSSL"
			])

			AS_IF([test "x${ac_cv_header_openssl_ec_h}${ac_cv_header_openssl_ecdsa_h}" = "xyesyes"], [
				ECDSA_TOOLS_COND_D="ecdsadecode ecdsakeygen ecdsasign"
				AC_SUBST([ECDSA_TOOLS_COND_D])
			])

			AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if OpenSSL is available])
		], [
			LIBCRYPTO="No"
			AS_IF([test "${with_openssl}" = "yes"], [
				AC_MSG_FAILURE([--with-openssl was specified but libcrypto is unusable for this task])
			])
		])
	])

	LIBS="${LIBS_SAVED}"

	AS_IF([test "${LIBCRYPTO}" = "No"], [
		LIBCRYPTO_CFLAGS=""
		LIBCRYPTO_LIBS=""
	])

	AC_SUBST([LIBCRYPTO_CFLAGS])
	AC_SUBST([LIBCRYPTO_LIBS])
])
