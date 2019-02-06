AC_DEFUN([ATHEME_LIBTEST_NETTLE], [

	LIBNETTLE="No"

	AC_ARG_WITH([nettle],
		[AS_HELP_STRING([--without-nettle], [Do not attempt to detect nettle (crypto library)])],
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
			AC_CHECK_HEADERS([nettle/version.h], [], [], [])
			AC_CHECK_HEADERS([nettle/md5.h nettle/sha1.h nettle/sha2.h nettle/nettle-meta.h], [], [
				LIBNETTLE="No"
				AS_IF([test "${with_nettle}" = "yes"], [
					AC_MSG_ERROR([--with-nettle was specified but a required header file is missing])
				])
			], [])
		], [
			LIBNETTLE="No"
			AS_IF([test "${with_nettle}" = "yes"], [
				AC_MSG_ERROR([--with-nettle was specified but libnettle could not be found])
			])
		])
	])

	AS_IF([test "${LIBNETTLE}" = "Yes"], [
		AC_MSG_CHECKING([if nettle is usable])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <nettle/md5.h>
				#include <nettle/sha1.h>
				#include <nettle/sha2.h>
			]], [[
				struct md5_ctx md5ctx;
				struct sha1_ctx sha1ctx;
				struct sha256_ctx sha256ctx;
				struct sha512_ctx sha512ctx;
				(void) md5_init(&md5ctx);
				(void) sha1_init(&sha1ctx);
				(void) sha256_init(&sha256ctx);
				(void) sha512_init(&sha512ctx);
			]])
		], [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_LIBNETTLE], [1], [Define to 1 if we have libnettle available])
		], [
			AC_MSG_RESULT([no])
			LIBNETTLE="No"
			AS_IF([test "${with_nettle}" = "yes"], [
				AC_MSG_FAILURE([--with-nettle was specified but libnettle is unusable for this task])
			])
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
