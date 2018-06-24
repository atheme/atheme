AC_DEFUN([ATHEME_FEATURETEST_SODIUM_MALLOC], [

	SODIUM_MALLOC="No"

	AC_ARG_ENABLE([sodium-malloc],
		[AS_HELP_STRING([--enable-sodium-malloc], [Enable libsodium secure memory allocator])],
		[], [enable_sodium_malloc="no"])

	case "${enable_sodium_malloc}" in
		yes)
			AS_IF([test "x${CONTRIB_MODULES}" != "xNo"], [
				AC_MSG_ERROR([--enable-sodium-malloc requires --disable-contrib])
			])

			AS_IF([test "x${LIBSODIUM}" != "xYes"], [
				AC_MSG_ERROR([--enable-sodium-malloc requires --with-sodium])
			])

			SODIUM_MALLOC="Yes"
			AC_DEFINE([USE_LIBSODIUM_ALLOCATOR], [1], [Enable libsodium secure memory allocator])
			AC_MSG_WARN([The sodium hardened memory allocator is not intended for production usage])
			;;
		no)
			SODIUM_MALLOC="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-sodium-malloc])
			;;
	esac
])
