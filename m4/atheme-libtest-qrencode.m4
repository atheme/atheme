AC_DEFUN([ATHEME_LIBTEST_QRENCODE], [

	LIBQRENCODE="No"

	QRCODE_C=""

	AC_ARG_WITH([qrencode],
		[AS_HELP_STRING([--with-qrencode], [Compile with libqrencode for generating QR codes])],
		[], [with_qrencode="auto"])

	case "${with_qrencode}" in
		no | yes | auto)
			;;
		*)
			AC_MSG_ERROR([invalid option for --with-qrencode])
			;;
	esac

	AS_IF([test "x${with_qrencode}" != "xno"], [
		PKG_CHECK_MODULES([LIBQRENCODE], [libqrencode], [
			LIBQRENCODE="Yes"
			QRCODE_C="qrcode.c"
			AC_SUBST([QRCODE_C])
			AC_DEFINE([HAVE_LIBQRENCODE], [1], [Define to 1 if libqrencode is available])
		], [
			LIBQRENCODE="No"
			AS_IF([test "x${with_qrencode}" = "xyes"], [
				AC_MSG_ERROR([--with-qrencode was specified but libqrencode could not be found])
			])
		])
	])
])
