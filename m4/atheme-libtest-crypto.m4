AC_DEFUN([ATHEME_LIBTEST_CRYPTO], [

	LIBCRYPTO="No"
	LIBCRYPTO_NAME=""
	LIBCRYPTO_USABLE="No"
	LIBCRYPTO_DIGEST="No"
	LIBCRYPTO_RANDOM="No"

	AC_ARG_WITH([openssl],
		[AS_HELP_STRING([--without-openssl], [Do not attempt to detect OpenSSL (cryptographic library)])],
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
			AC_CHECK_HEADERS([openssl/opensslv.h], [], [
				LIBCRYPTO="No"
				AS_IF([test "${with_openssl}" = "yes"], [
					AC_MSG_ERROR([--with-openssl was given but a required header file is missing])
				])
			], [])
		], [
			LIBCRYPTO="No"
			AS_IF([test "${with_openssl}" = "yes"], [
				AC_MSG_ERROR([--with-openssl was given but libcrypto could not be found])
			])
		])
	], [
		LIBCRYPTO="No"
	])

	AS_IF([test "${LIBCRYPTO}" = "Yes"], [
		AC_MSG_CHECKING([if libcrypto has usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
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
			LIBCRYPTO_USABLE="Yes"
			LIBCRYPTO_DIGEST="Yes"

			AC_MSG_CHECKING([if libcrypto has HMAC_CTX_new() & HMAC_CTX_free()])
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
				AC_DEFINE([HAVE_LIBCRYPTO_HMAC_CTX_DYNAMIC], [1], [Define to 1 if libcrypto has HMAC_CTX_new() & HMAC_CTX_free()])
			], [
				AC_MSG_RESULT([no])
			])
		], [
			AC_MSG_RESULT([no])
		])

		AC_MSG_CHECKING([if libcrypto has a usable random number generator])
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
			LIBCRYPTO_USABLE="Yes"
			LIBCRYPTO_RANDOM="Yes"
		], [
			AC_MSG_RESULT([no])
		])

		AC_MSG_CHECKING([if libcrypto has a usable constant-time memory comparison function])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <openssl/crypto.h>
			]], [[
				(void) CRYPTO_memcmp(NULL, NULL, 0);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBCRYPTO_MEMCMP], [1], [Define to 1 if libcrypto has a usable constant-time memory comparison function])
			LIBCRYPTO_USABLE="Yes"
		], [
			AC_MSG_RESULT([no])
		])

		AC_MSG_CHECKING([if libcrypto has usable elliptic curve key construction and signature verification functions])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <openssl/ec.h>
				#include <openssl/ecdsa.h>
				#include <openssl/evp.h>
			]], [[
				EC_KEY *pubkey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
				(void) EC_KEY_set_conv_form(pubkey, POINT_CONVERSION_COMPRESSED);
				(void) o2i_ECPublicKey(&pubkey, NULL, 0);
				(void) ECDSA_verify(0, NULL, 0, NULL, 0, pubkey);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBCRYPTO_ECDSA], [1], [Define to 1 if libcrypto has usable elliptic curve key construction and signature verification functions])
			LIBCRYPTO_USABLE="Yes"
			ECDSA_TOOLS_COND_D="ecdsadecode ecdsakeygen ecdsasign"
			AC_SUBST([ECDSA_TOOLS_COND_D])
		], [
			AC_MSG_RESULT([no])
		])
	])

	AS_IF([test "${LIBCRYPTO_USABLE}" = "Yes"], [
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

		AC_DEFINE([HAVE_LIBCRYPTO], [1], [Define to 1 if libcrypto appears to be usable])
	], [
		LIBCRYPTO="No"
		AS_IF([test "${with_openssl}" = "yes"], [
			AC_MSG_FAILURE([--with-openssl was given but libcrypto appears to be unusable])
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
