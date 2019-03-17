AC_DEFUN([ATHEME_LIBTEST_NETTLE], [

	LIBNETTLE="No"
	LIBNETTLE_USABLE="No"
	LIBNETTLE_DIGEST="No"

	AC_ARG_WITH([nettle],
		[AS_HELP_STRING([--without-nettle], [Do not attempt to detect nettle (cryptographic library)])],
		[], [with_nettle="auto"])

	case "x${with_nettle}" in
		xno | xyes | xauto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-nettle])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "${with_nettle}" != "no"], [
		PKG_CHECK_MODULES([LIBNETTLE], [nettle], [
			LIBS="${LIBNETTLE_LIBS} ${LIBS}"
			LIBNETTLE="Yes"
		], [
			LIBNETTLE="No"
			AS_IF([test "${with_nettle}" = "yes"], [
				AC_MSG_ERROR([--with-nettle was given but libnettle could not be found])
			])
		])
	], [
		LIBNETTLE="No"
	])

	AS_IF([test "${LIBNETTLE}" = "Yes"], [
		AC_MSG_CHECKING([if libnettle has usable MD5/SHA1/SHA2 functions])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <nettle/md5.h>
				#include <nettle/sha1.h>
				#include <nettle/sha2.h>
				#include <nettle/nettle-meta.h>
			]], [[
				struct md5_ctx md5ctx;
				struct sha1_ctx sha1ctx;
				struct sha256_ctx sha256ctx;
				struct sha512_ctx sha512ctx;
				(void) nettle_md5_init(NULL);
				(void) nettle_md5_update(NULL, 0, NULL);
				(void) nettle_md5_digest(NULL, 0, NULL);
				(void) nettle_sha1_init(NULL);
				(void) nettle_sha1_update(NULL, 0, NULL);
				(void) nettle_sha1_digest(NULL, 0, NULL);
				(void) nettle_sha256_init(NULL);
				(void) nettle_sha256_update(NULL, 0, NULL);
				(void) nettle_sha256_digest(NULL, 0, NULL);
				(void) nettle_sha512_init(NULL);
				(void) nettle_sha512_update(NULL, 0, NULL);
				(void) nettle_sha512_digest(NULL, 0, NULL);
			]])
		], [
			AC_MSG_RESULT([yes])
			LIBNETTLE_USABLE="Yes"
			LIBNETTLE_DIGEST="Yes"
		], [
			AC_MSG_RESULT([no])
		])

		AC_MSG_CHECKING([if libnettle has a usable constant-time memory comparison function])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <nettle/memops.h>
			]], [[
				(void) nettle_memeql_sec(NULL, NULL, 0);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBNETTLE_MEMEQL], [1], [Define to 1 if libnettle has a usable constant-time memory comparison function])
			LIBNETTLE_USABLE="Yes"
		], [
			AC_MSG_RESULT([no])
		])

		AC_MSG_CHECKING([if libnettle can provide SASL ECDH-X25519-CHALLENGE])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_STDDEF_H
				#  include <stddef.h>
				#endif
				#include <nettle/curve25519.h>
				#ifndef NETTLE_CURVE25519_RFC7748
				#  error "NETTLE_CURVE25519_RFC7748 is not set"
				#endif
			]], [[
				(void) nettle_curve25519_mul_g(NULL, NULL);
				(void) nettle_curve25519_mul(NULL, NULL, NULL);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBNETTLE_ECDH_X25519], [1], [Define to 1 if libnettle can provide SASL ECDH-X25519-CHALLENGE])
			ATHEME_COND_ECDH_X25519_TOOL_ENABLE
			LIBNETTLE_USABLE="Yes"
		], [
			AC_MSG_RESULT([no])
		])
	])

	AS_IF([test "${LIBNETTLE_USABLE}" = "Yes"], [
		AC_DEFINE([HAVE_LIBNETTLE], [1], [Define to 1 if libnettle appears to be usable])
		AC_CHECK_HEADERS([nettle/version.h], [], [], [])
	], [
		LIBNETTLE="No"
		AS_IF([test "${with_nettle}" = "yes"], [
			AC_MSG_FAILURE([--with-nettle was given but libnettle appears to be unusable])
		])
	])

	LIBS="${LIBS_SAVED}"

	AS_IF([test "${LIBNETTLE}" = "No"], [
		LIBNETTLE_CFLAGS=""
		LIBNETTLE_LIBS=""
	])

	AC_SUBST([LIBNETTLE_CFLAGS])
	AC_SUBST([LIBNETTLE_LIBS])
])
