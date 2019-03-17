/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 */

#ifndef ATHEME_MOD_CRYPTO_CRYPT3_H
#define ATHEME_MOD_CRYPTO_CRYPT3_H 1

#include <atheme.h>

#ifdef HAVE_CRYPT

#ifdef HAVE_CRYPT_H
#  include <crypt.h>
#endif /* HAVE_CRYPT_H */

#define CRYPT3_AN_CHARS_RANGE                   "A-Za-z0-9"
#define CRYPT3_B64_CHARS_RANGE                  "./" CRYPT3_AN_CHARS_RANGE

#define CRYPT3_LOADHASH_FORMAT_DES              "%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_MD5              "$1$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_256         "$5$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_512         "$6$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_256_EXT     "$5$rounds=%*u$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_SHA2_512_EXT     "$6$rounds=%*u$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"

#define CRYPT3_SAVESALT_FORMAT_SHA2_256         "$5$%s$"
#define CRYPT3_SAVESALT_FORMAT_SHA2_512         "$6$%s$"

#define CRYPT3_LOADHASH_LENGTH_DES              0x0DU
#define CRYPT3_LOADHASH_LENGTH_MD5              0x16U
#define CRYPT3_LOADHASH_LENGTH_SHA2_256         0x2BU
#define CRYPT3_LOADHASH_LENGTH_SHA2_512         0x56U

#define CRYPT3_SHA2_SALTLENGTH                  0x10U
#define CRYPT3_SHA2_SALTCHARS                   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#define CRYPT3_SHA2_SALTCHARS_LENGTH            0x3EU

#define CRYPT3_MODULE_TEST_PASSWORD             "YnqBeyzDmb4IHyXsGbasMBbmNiPM1E8G" \
                                                "SAOsIwKhozJC0bZnHsaZBbT4x47iq4J7" \
                                                "PfyfpRnUpIS5TKuAcLGbXF1J8FaM10wQ" \
                                                "pwAqOlLRmyiOhNHN5o6NPfjgpebk3jqU" \
                                                "EtJebRswHdmg6TzKHU6RUbygbcPy6P11" \
                                                "zvdYTCT02nMwMjMvSwtJqAq3Im5kYk2i" \
                                                "L09wI1CsqLElabrZOx8mmpnBKWQMSOBP" \
                                                "OWlR0jBZdqZV7ZZyKpLHP99k9XuvWFax" \
                                                "xQDv4Qg82a1Pi4BN0IxSuwlHKgl2Kwqt"

#define CRYPT3_MODULE_TEST_VECTOR_DES           "JCaMNN9g7BnPQ"
#define CRYPT3_MODULE_TEST_VECTOR_MD5           "$1$xFIg9A$mXXbp3eKvgkOlChjAd2Eq1"
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_256      "$5$vishj8N7EZ05xejG$xz/ipkwwSKMJUEjF5gT.E7UWHQh9KI9ld0ornIVG0S1"
#define CRYPT3_MODULE_TEST_VECTOR_SHA2_512      "$6$vishj8N7EZ05xejG$yjsXj1aO1Vh.ZuhLrAPG1ch8NA.2HaaND" \
                                                "im5hixDtnNLr7i2c0fO7kM7ZGGkk0VBpqtaRzmnD7ob60m6JREb2/"

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

#endif /* HAVE_CRYPT */

#endif /* !ATHEME_MOD_CRYPTO_CRYPT3_H */
