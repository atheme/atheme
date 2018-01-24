AC_DEFUN([ATHEME_LIBTEST_OPENSSL], [

	AS_IF([test "x${with_openssl}" != "xno"], [

		LIBS_SAVED="${LIBS}"

		AC_SEARCH_LIBS([EVP_DigestUpdate], [crypto], [LIBCRYPTO="Yes"], [

			AS_IF([test "x${with_openssl}" != "xauto"], [
				AC_MSG_ERROR([--with-openssl was specified but OpenSSL could not be found])
			])
		])

		LIBS="${LIBS_SAVED}"
	])

	AS_IF([test "x${LIBCRYPTO}" = "xYes"], [

		AC_CHECK_HEADERS([openssl/ec.h], [], [], [])

		AC_CHECK_HEADERS([openssl/evp.h openssl/hmac.h openssl/sha.h], [], [

			LIBCRYPTO="No"

			AS_IF([test "x${with_openssl}" = "xyes"], [AC_MSG_ERROR([required header file missing])])
		], [])
	])

	AS_IF([test "x${LIBCRYPTO}" = "xYes"], [

		AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if OpenSSL is available])

		AS_IF([test "x${ac_cv_search_EVP_DigestUpdate}" != "xnone required"],
			[LIBCRYPTO_LIBS="${ac_cv_search_EVP_DigestUpdate}"])

		AC_SUBST([LIBCRYPTO_LIBS])
	])
])
