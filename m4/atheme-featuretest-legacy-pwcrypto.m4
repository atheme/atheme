AC_DEFUN([ATHEME_FEATURETEST_LEGACY_PWCRYPTO], [

	LEGACY_PWCRYPTO="No"

	AC_ARG_ENABLE([legacy-pwcrypto],
		[AS_HELP_STRING([--enable-legacy-pwcrypto], [Enable legacy password crypto modules])],
		[], [enable_legacy_pwcrypto="no"])

	case "${enable_legacy_pwcrypto}" in
		yes)
			LEGACY_PWCRYPTO="Yes"
			LEGACY_PWCRYPTO_COND_C="crypt3-des.c crypt3-md5.c ircservices.c rawmd5.c rawsha1.c"
			AC_SUBST([LEGACY_PWCRYPTO_COND_C])
			AC_DEFINE([ENABLE_LEGACY_CRYPTO_MODULES], [1], [Enable legacy crypto modules])
			;;
		no)
			LEGACY_PWCRYPTO="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-legacy-pwcrypto])
			;;
	esac
])
