AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_MBEDTLS], [

	DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_MBEDTLS"
	DIGEST_FRONTEND="ARM mbedTLS"
	DIGEST_API_CFLAGS=""
	DIGEST_API_LIBS="${LIBMBEDCRYPTO_LIBS}"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_NETTLE], [

	DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_NETTLE"
	DIGEST_FRONTEND="Nettle"
	DIGEST_API_CFLAGS="${LIBNETTLE_CFLAGS}"
	DIGEST_API_LIBS="${LIBNETTLE_LIBS}"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_OPENSSL], [

	DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_OPENSSL"
	DIGEST_FRONTEND="${LIBCRYPTO_NAME}"
	DIGEST_API_CFLAGS="${LIBCRYPTO_CFLAGS}"
	DIGEST_API_LIBS="${LIBCRYPTO_LIBS}"
])

AC_DEFUN([ATHEME_DIGEST_FRONTEND_USE_INTERNAL], [

	DIGEST_FRONTEND_VAL="ATHEME_API_DIGEST_FRONTEND_INTERNAL"
	DIGEST_FRONTEND="Internal"
	DIGEST_API_CFLAGS=""
	DIGEST_API_LIBS=""
])

AC_DEFUN([ATHEME_DECIDE_DIGEST_FRONTEND], [

	DIGEST_FRONTEND_VAL=""
	DIGEST_FRONTEND=""
	DIGEST_API_CFLAGS=""
	DIGEST_API_LIBS=""

	AC_ARG_WITH([digest-api-frontend],
		[AS_HELP_STRING([--with-digest-api-frontend=@<:@frontend@:>@], [Digest API frontend to use (auto, mbedtls, nettle, openssl, libressl, internal). Default: auto])],
		[], [with_digest_api_frontend="auto"])

	AC_MSG_CHECKING([which frontend to use for the Digest API])

	case "x${with_digest_api_frontend}" in

		xauto)
			AS_IF([test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_MBEDTLS
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen automatically)])
			], [test "${LIBNETTLE}${LIBNETTLE_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_NETTLE
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen automatically)])
			], [test "${LIBCRYPTO}${LIBCRYPTO_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_OPENSSL
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen automatically)])
			], [
				ATHEME_DIGEST_FRONTEND_USE_INTERNAL
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen automatically)])
			])
			;;

		xmbedtls)
			AS_IF([test "${LIBMBEDCRYPTO}${LIBMBEDCRYPTO_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_MBEDTLS
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen by user)])
			], [
				AC_MSG_RESULT([error])
				AC_MSG_ERROR([--with-digest-api-frontend=mbedtls requires --with-mbedtls and usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
			])
			;;

		xnettle)
			AS_IF([test "${LIBNETTLE}${LIBNETTLE_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_NETTLE
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen by user)])
			], [
				AC_MSG_RESULT([error])
				AC_MSG_ERROR([--with-digest-api-frontend=nettle requires --with-nettle and usable MD5/SHA1/SHA2 functions])
			])
			;;

		xopenssl | xlibressl)
			AS_IF([test "${LIBCRYPTO}${LIBCRYPTO_DIGEST}" = "YesYes"], [
				ATHEME_DIGEST_FRONTEND_USE_OPENSSL
				AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen by user)])
			], [
				AC_MSG_RESULT([error])
				AC_MSG_ERROR([--with-digest-api-frontend=openssl/libressl requires --with-openssl and usable MD5/SHA1/SHA2/HMAC/PBKDF2 functions])
			])
			;;

		internal)
			ATHEME_DIGEST_FRONTEND_USE_INTERNAL
			AC_MSG_RESULT([${DIGEST_FRONTEND} (chosen by user)])
			;;

		*)
			AC_MSG_RESULT([error])
			AC_MSG_ERROR([invalid option for --with-digest-api-frontend (auto, mbedtls, nettle, openssl, libressl, internal)])
			;;
	esac

	AC_DEFINE_UNQUOTED([ATHEME_API_DIGEST_FRONTEND], [${DIGEST_FRONTEND_VAL}], [Atheme Digest API Frontend])
])
