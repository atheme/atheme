AC_DEFUN([ATHEME_LIBTEST_QRENCODE], [

	LIBQRENCODE="No"
	QRCODE_COND_C=""

	AC_ARG_WITH([qrencode],
		[AS_HELP_STRING([--without-qrencode], [Do not attempt to detect libqrencode for generating QR codes])],
		[], [with_qrencode="auto"])

	case "${with_qrencode}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-qrencode])
			;;
	esac

	LIBS_SAVED="${LIBS}"

	AS_IF([test "x${with_qrencode}" != "xno"], [
		AC_CHECK_HEADERS([qrencode.h], [
			PKG_CHECK_MODULES([LIBQRENCODE], [libqrencode], [
				LIBS="${LIBQRENCODE_LIBS} ${LIBS}"
				AC_MSG_CHECKING([if libqrencode is usable])
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <stddef.h>
						#include <qrencode.h>
					]], [[
						(void) QRcode_encodeData(0, NULL, 0, (QRecLevel) 0);
						(void) QRcode_free(NULL);
					]])
				], [
					AC_MSG_RESULT([yes])
					LIBQRENCODE="Yes"
					QRCODE_COND_C="qrcode.c"
					AC_SUBST([QRCODE_COND_C])
					AC_DEFINE([HAVE_LIBQRENCODE], [1], [Define to 1 if libqrencode is available])
				], [
					AC_MSG_RESULT([no])
					LIBQRENCODE="No"
					AS_IF([test "x${with_qrencode}" = "xyes"], [
						AC_MSG_ERROR([--with-qrencode was specified but libqrencode does not appear to be usable])
					])
				])
			], [
				LIBQRENCODE="No"
				AS_IF([test "x${with_qrencode}" = "xyes"], [
					AC_MSG_ERROR([--with-qrencode was specified but libqrencode could not be found])
				])
			])
		], [
			LIBQRENCODE="No"
			AS_IF([test "x${with_qrencode}" = "xyes"], [
				AC_MSG_ERROR([--with-qrencode was specified but a required header file is missing])
			])
		], [])
	])

	LIBS="${LIBS_SAVED}"
])
