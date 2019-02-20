AC_DEFUN([ATHEME_FEATURETEST_LEGACY_PWCRYPTO], [

	LEGACY_PWCRYPTO="No"

	AC_ARG_ENABLE([legacy-pwcrypto],
		[AS_HELP_STRING([--enable-legacy-pwcrypto], [Enable legacy password crypto modules])],
		[], [enable_legacy_pwcrypto="no"])

	case "x${enable_legacy_pwcrypto}" in
		xyes)
			LEGACY_PWCRYPTO="Yes"
			LEGACY_PWCRYPTO_COND_C="crypt3-des.c crypt3-md5.c ircservices.c rawmd5.c rawsha1.c"
			AC_DEFINE([ATHEME_ENABLE_LEGACY_PWCRYPTO], [1], [Define to 1 if --enable-legacy-pwcrypto was given to ./configure])
			AC_SUBST([LEGACY_PWCRYPTO_COND_C])
			;;
		xno)
			LEGACY_PWCRYPTO="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-legacy-pwcrypto])
			;;
	esac
])
