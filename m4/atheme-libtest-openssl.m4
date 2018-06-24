AC_DEFUN([ATHEME_LIBTEST_OPENSSL], [

	LIBCRYPTO="No"

	LIBCRYPTO_LIBS=""

	AC_ARG_WITH([openssl],
		[AS_HELP_STRING([--without-openssl], [Do not attempt to detect OpenSSL for modules/saslserv/ecdsa-nist256p-challenge])],
		[], [with_openssl="auto"])

	case "${with_openssl}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-openssl])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "x${with_openssl}" != "xno"], [

		AC_SEARCH_LIBS([EVP_DigestUpdate], [crypto], [LIBCRYPTO="Yes"], [
			AS_IF([test "x${with_openssl}" != "xauto"], [
				AC_MSG_ERROR([--with-openssl was specified but OpenSSL could not be found])
			])
		])
	])

	AS_IF([test "x${LIBCRYPTO}" = "xYes"], [
		AC_CHECK_HEADERS([openssl/evp.h openssl/hmac.h openssl/opensslv.h], [], [
			LIBCRYPTO="No"
			AS_IF([test "x${with_openssl}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
		], [])
	])

	AS_IF([test "x${LIBCRYPTO}" = "xYes"], [
		AC_CHECK_HEADERS([openssl/crypto.h], [
			AC_CHECK_LIB([crypto], [CRYPTO_set_mem_functions], [
				AC_DEFINE([HAVE_OPENSSL_MEMORY_API], [1],
					[Define to 1 if CRYPTO_set_mem_functions is available])
			], [], [])
		], [], [])
	])

	AS_IF([test "x${LIBCRYPTO}" = "xYes"], [
		AS_IF([test "x${ac_cv_search_EVP_DigestUpdate}" != "xnone required"], [
			LIBCRYPTO_LIBS="${ac_cv_search_EVP_DigestUpdate}"
		])
		AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if OpenSSL is available])
		AC_SUBST([LIBCRYPTO_LIBS])

		AC_CHECK_HEADERS([openssl/ec.h openssl/ecdsa.h], [], [], [])
		AS_IF([test "x${ac_cv_header_openssl_ec_h}x${ac_cv_header_openssl_ecdsa_h}" = "xyesxyes"], [
			ECDSA_TOOLS_COND_D="ecdsadecode ecdsakeygen ecdsasign"
			AC_SUBST([ECDSA_TOOLS_COND_D])
		])
	])

	LIBS="${LIBS_SAVED}"
])
