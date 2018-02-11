/*
 * Copyright (C) 2018 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 */

#ifndef INC_CRYPT3_H
#define INC_CRYPT3_H

#include "atheme.h"

#ifdef HAVE_CRYPT

#define CRYPT3_AN_CHARS_RANGE                   "A-Za-z0-9"
#define CRYPT3_B64_CHARS_RANGE                  "./" CRYPT3_AN_CHARS_RANGE

#define CRYPT3_LOADHASH_FORMAT_DES              "%[" CRYPT3_B64_CHARS_RANGE "]"
#define CRYPT3_LOADHASH_FORMAT_MD5              "$1$%*[" CRYPT3_B64_CHARS_RANGE "]$%[" CRYPT3_B64_CHARS_RANGE "]"

#define CRYPT3_LOADHASH_LENGTH_DES              0x0DU
#define CRYPT3_LOADHASH_LENGTH_MD5              0x16U

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

#define CRYPT3_MODULE_WARNING                   "%s: this module relies upon platform-specific behaviour and " \
                                                "may stop working if you migrate services to another machine!"



/*
 * This takes care of resetting errno(3) and reporting the reason for
 * crypt(3) failing, if any. It also sets aside a static buffer and
 * does length checking on the result to avoid silent truncation
 * errors.
 */
static inline const char * __attribute__((warn_unused_result))
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

	static char result[PASSLEN];

	if (mowgli_strlcpy(result, encrypted, sizeof result) >= sizeof result)
	{
		(void) slog(LG_ERROR, "%s: mowgli_strlcpy() would have overflowed result buffer (BUG)", caller);
		return NULL;
	}

	return result;
}

#endif /* HAVE_CRYPT */
#endif /* !INC_CRYPT3_H */
