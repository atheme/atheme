/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 */

#ifndef ATHEME_MOD_CRYPTO_CRYPT3_WRAPPER_H
#define ATHEME_MOD_CRYPTO_CRYPT3_WRAPPER_H 1

#include <atheme.h>

#ifdef HAVE_CRYPT

#ifdef HAVE_CRYPT_H
#  include <crypt.h>
#endif /* HAVE_CRYPT_H */

#define CRYPT3_LOADHASH_FORMAT_DES              "%[" BASE64_ALPHABET_CRYPT3 "]"
#define CRYPT3_LOADHASH_FORMAT_MD5              "$1$%*[" BASE64_ALPHABET_CRYPT3 "]$%[" BASE64_ALPHABET_CRYPT3 "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_256         "$5$%*[" BASE64_ALPHABET_CRYPT3 "]$%[" BASE64_ALPHABET_CRYPT3 "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_512         "$6$%*[" BASE64_ALPHABET_CRYPT3 "]$%[" BASE64_ALPHABET_CRYPT3 "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_256_EXT     "$5$rounds=%u$%*[" BASE64_ALPHABET_CRYPT3 "]$%[" BASE64_ALPHABET_CRYPT3 "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_512_EXT     "$6$rounds=%u$%*[" BASE64_ALPHABET_CRYPT3 "]$%[" BASE64_ALPHABET_CRYPT3 "]"

#define CRYPT3_SAVESALT_FORMAT_SHA2_256         "$5$%s$"
#define CRYPT3_SAVESALT_FORMAT_SHA2_512         "$6$%s$"
#define CRYPT3_SAVESALT_FORMAT_SHA2_256_EXT     "$5$rounds=%u$%s$"
#define CRYPT3_SAVESALT_FORMAT_SHA2_512_EXT     "$6$rounds=%u$%s$"

#define CRYPT3_LOADHASH_LENGTH_DES              0x0DU
#define CRYPT3_LOADHASH_LENGTH_MD5              0x16U
#define CRYPT3_LOADHASH_LENGTH_SHA2_256         0x2BU
#define CRYPT3_LOADHASH_LENGTH_SHA2_512         0x56U

#define CRYPT3_SHA2_ITERCNT_MIN                 5000U
#define CRYPT3_SHA2_ITERCNT_DEF                 5000U
#define CRYPT3_SHA2_ITERCNT_MAX                 1000000U
#define CRYPT3_SHA2_SALTLEN_RAW                 12U

#define CRYPT3_MODULE_TEST_PASSWORD             "YnqBeyzDmb4IHyXsGbasMBbmNiPM1E8GSAOsIwKhozJC0bZnHsaZBbT4x47iq4J7" \
                                                "PfyfpRnUpIS5TKuAcLGbXF1J8FaM10wQpwAqOlLRmyiOhNHN5o6NPfjgpebk3jqU" \
                                                "EtJebRswHdmg6TzKHU6RUbygbcPy6P11zvdYTCT02nMwMjMvSwtJqAq3Im5kYk2i" \
                                                "L09wI1CsqLElabrZOx8mmpnBKWQMSOBPOWlR0jBZdqZV7ZZyKpLHP99k9XuvWFax" \
                                                "xQDv4Qg82a1Pi4BN0IxSuwlHKgl2Kwqt"

#define CRYPT3_MODULE_TEST_VECTOR_DES           "JCaMNN9g7BnPQ"
#define CRYPT3_MODULE_TEST_VECTOR_MD5           "$1$xFIg9A$mXXbp3eKvgkOlChjAd2Eq1"
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_256      "$5$kcVpQifeRqqCjVKM$dhthFOcpznDllTsC2e3m5wGrZ9HIV50F9iRnUCrlLD9"
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_512      "$6$kcVpQifeRqqCjVKM$6a8TZKZazi58rwjsaBU8apGVcM6bHcTj7" \
                                                "djfYeaEuae8N3asrlUbh6LbGCXDcYEcFJTH7Ir5ToeSOciqO5363."
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_256_EXT  "$5$rounds=1000000$kcVpQifeRqqCjVKM$aXG6EdLpgwc3.RzodKaIORZd6.5GgSCSIf5iYgTy2N/"
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_512_EXT  "$6$rounds=1000000$kcVpQifeRqqCjVKM$Nqh0Pm5R4jepPPYOjwNHcppb." \
                                                "EOs62XBjIgvNiXBUXLyLkL6PyZXiLw58d0phVt9LD3GwWaW8i/s1bA9JTcs.0"

#define CRYPT3_MODULE_WARNING                   "%s: this module relies upon platform-specific behaviour and " \
                                                "may stop working if you migrate services to another machine!"



/*
 * This takes care of resetting errno(3) and reporting the reason for
 * crypt(3) failing, if any. It also sets aside a static buffer and
 * does length checking on the result to avoid silent truncation
 * errors.
 */
static inline const char * ATHEME_FATTR_WUR
atheme_crypt3_wrapper(const char *const restrict password, const char *const restrict parameters,
                      const char *const restrict caller)
{
	errno = 0;

	const char *const encrypted = crypt(password, parameters);

	if (! encrypted)
	{
		if (errno)
			(void) slog(LG_ERROR, "%s: crypt(3) failed: %s", caller, strerror(errno));
		else
			(void) slog(LG_ERROR, "%s: crypt(3) failed: <unknown reason>", caller);

		return NULL;
	}

	static char result[PASSLEN + 1];

	if (mowgli_strlcpy(result, encrypted, sizeof result) > PASSLEN)
	{
		(void) slog(LG_ERROR, "%s: mowgli_strlcpy() output would have been too long (BUG)", caller);
		return NULL;
	}

	return result;
}

static inline bool ATHEME_FATTR_WUR
atheme_crypt3_selftest(const bool log_errors, const char *const restrict parameters)
{
	static const char password[] = CRYPT3_MODULE_TEST_PASSWORD;

	const char *const result = atheme_crypt3_wrapper(password, parameters, MOWGLI_FUNC_NAME);

	if (! result)
		// That function logs messages on failure
		return false;

	if (strcmp(result, parameters) != 0)
	{
		if (log_errors)
		{
			(void) slog(LG_ERROR, "%s: crypt(3) returned an incorrect result", MOWGLI_FUNC_NAME);
			(void) slog(LG_ERROR, "%s: expected '%s', got '%s'", MOWGLI_FUNC_NAME, parameters, result);
		}

		return false;
	}

	return true;
}

#endif /* HAVE_CRYPT */

#endif /* !ATHEME_MOD_CRYPTO_CRYPT3_WRAPPER_H */
