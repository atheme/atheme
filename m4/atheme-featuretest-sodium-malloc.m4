AC_DEFUN([ATHEME_FEATURETEST_SODIUM_MALLOC], [

	SODIUM_MALLOC="No"

	AC_ARG_ENABLE([sodium-malloc],
		[AS_HELP_STRING([--enable-sodium-malloc], [Enable libsodium secure memory allocator])],
		[], [enable_sodium_malloc="no"])

	case "x${enable_sodium_malloc}" in
		xyes)
			AS_IF([test "${LIBSODIUMMEMORY}" = "No"], [
				AC_MSG_ERROR([--enable-sodium-malloc requires usable libsodium memory manipulation functions])
			])

			SODIUM_MALLOC="Yes"
			AC_DEFINE([USE_LIBSODIUM_ALLOCATOR], [1], [Enable libsodium secure memory allocator])
			AC_MSG_WARN([The sodium hardened memory allocator is not intended for production usage])
			;;
		xno)
			SODIUM_MALLOC="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-sodium-malloc])
			;;
	esac
])
