/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the digest interface.
 */

#include "atheme.h"

#define ATHEME_LAC_DIGEST_FRONTEND_C    1

#if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_INTERNAL)
#  include "digest_fe_internal.c"
#else
#  if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_MBEDTLS)
#    include "digest_fe_mbedtls.c"
#  else
#    if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_NETTLE)
#      include "digest_fe_nettle.c"
#    else
#      if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_OPENSSL)
#        include "digest_fe_openssl.c"
#      else
#        error "No Digest API frontend was selected by the build system"
#      endif
#    endif
#  endif
#endif

size_t
digest_size_alg(const enum digest_algorithm alg)
{
	switch (alg)
	{
		case DIGALG_MD5:
			return DIGEST_MDLEN_MD5;

		case DIGALG_SHA1:
			return DIGEST_MDLEN_SHA1;

		case DIGALG_SHA2_256:
			return DIGEST_MDLEN_SHA2_256;

		case DIGALG_SHA2_512:
			return DIGEST_MDLEN_SHA2_512;
	}

	return 0;
}

bool ATHEME_FATTR_WUR
digest_update_vector(struct digest_context *const restrict ctx, const struct digest_vector *const restrict vec,
                     const size_t vecLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(ctx, vec[i].ptr, vec[i].len))
			return false;

	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot(const enum digest_algorithm alg, const void *const data, const size_t dataLen,
               void *const out, size_t *const outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	struct digest_context ctx;

	if (! digest_init(&ctx, alg))
		return false;

	if (! digest_update(&ctx, data, dataLen))
		return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_vector(const enum digest_algorithm alg, const struct digest_vector *const restrict vec,
                      const size_t vecLen, void *const restrict out, size_t *const outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	struct digest_context ctx;

	if (! digest_init(&ctx, alg))
		return false;

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(&ctx, vec[i].ptr, vec[i].len))
			return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                    const void *const data, const size_t dataLen, void *const out,
                    size_t *const outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! key && keyLen) || (key && ! keyLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched key parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	struct digest_context ctx;

	if (! digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! digest_update(&ctx, data, dataLen))
		return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac_vector(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                           const struct digest_vector *const restrict vec, const size_t vecLen,
                           void *const out, size_t *const outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! key && keyLen) || (key && ! keyLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched key parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	struct digest_context ctx;

	if (! digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(&ctx, vec[i].ptr, vec[i].len))
			return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_hkdf_extract(const enum digest_algorithm alg, const void *const ikm, const size_t ikmLen,
                    const void *salt, size_t saltLen, void *const prk, const size_t prkLen)
{
	unsigned char zeroes[DIGEST_MDLEN_MAX];
	unsigned char tmp[DIGEST_MDLEN_MAX];
	bool retval;

	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: digest_size_alg() failed (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! ikm)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ikm' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! ikmLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'ikmLen' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if ((! salt && saltLen) || (salt && ! saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched salt parameters (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! prk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'prk' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! prkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'prkLen' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (prkLen > hLen)
	{
		(void) slog(LG_ERROR, "%s: prkLen is too large (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! salt)
	{
		(void) memset(zeroes, 0x00, hLen);

		salt = zeroes;
		saltLen = hLen;
	}
	if (! digest_oneshot_hmac(alg, salt, saltLen, ikm, ikmLen, tmp, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_hmac() failed (BUG?)", MOWGLI_FUNC_NAME);
		goto error;
	}

	(void) memcpy(prk, tmp, prkLen);
	retval = true;
	goto cleanup;

error:
	(void) smemzero(prk, prkLen);
	retval = false;
	goto cleanup;

cleanup:
	(void) smemzero(tmp, sizeof tmp);
	return retval;
}

bool ATHEME_FATTR_WUR
digest_hkdf_expand(const enum digest_algorithm alg, const void *const prk, const size_t prkLen,
                   const void *const info, const size_t infoLen, void *const okm, const size_t okmLen)
{
	unsigned char tmp[DIGEST_MDLEN_MAX];
	struct digest_context ctx;
	size_t rem = okmLen;
	unsigned char *out = okm;
	bool retval;

	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: digest_size_alg() failed (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! prk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'prk' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! prkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'prkLen' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if ((! info && infoLen) || (info && ! infoLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched info parameters (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! okm)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'okm' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! okmLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'okmLen' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (okmLen > (hLen * 0xFFU))
	{
		(void) slog(LG_ERROR, "%s: called with unacceptable 'okmLen' (%zu) (BUG)", MOWGLI_FUNC_NAME, okmLen);
		goto error;
	}

	for (uint8_t t = 1; rem != 0; t++)
	{
		if (! digest_init_hmac(&ctx, alg, prk, prkLen))
		{
			(void) slog(LG_ERROR, "%s: digest_init_hmac(prk) failed (BUG?)", MOWGLI_FUNC_NAME);
			goto error;
		}
		if (t > 1 && ! digest_update(&ctx, tmp, hLen))
		{
			(void) slog(LG_ERROR, "%s: digest_update(tmp) failed (BUG?)", MOWGLI_FUNC_NAME);
			goto error;
		}
		if (! digest_update(&ctx, info, infoLen))
		{
			(void) slog(LG_ERROR, "%s: digest_update(info) failed (BUG?)", MOWGLI_FUNC_NAME);
			goto error;
		}
		if (! digest_update(&ctx, &t, sizeof t))
		{
			(void) slog(LG_ERROR, "%s: digest_update(t) failed (BUG?)", MOWGLI_FUNC_NAME);
			goto error;
		}
		if (! digest_final(&ctx, tmp, NULL))
		{
			(void) slog(LG_ERROR, "%s: digest_final(tmp) failed (BUG?)", MOWGLI_FUNC_NAME);
			goto error;
		}

		const size_t cpLen = (rem > hLen) ? hLen : rem;

		(void) memcpy(out, tmp, cpLen);

		out += cpLen;
		rem -= cpLen;
	}

	retval = true;
	goto cleanup;

error:
	(void) smemzero(okm, okmLen);
	retval = false;
	goto cleanup;

cleanup:
	(void) smemzero(tmp, sizeof tmp);
	(void) smemzero(&ctx, sizeof ctx);
	return retval;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hkdf(const enum digest_algorithm alg, const void *const ikm, const size_t ikmLen,
                    const void *const salt, const size_t saltLen, const void *const info, const size_t infoLen,
                    void *const okm, const size_t okmLen)
{
	unsigned char prk[DIGEST_MDLEN_MAX];
	bool retval;

	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: digest_size_alg() failed (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}

	if (! digest_hkdf_extract(alg, ikm, ikmLen, salt, saltLen, prk, hLen))
		goto error;

	if (! digest_hkdf_expand(alg, prk, hLen, info, infoLen, okm, okmLen))
		goto error;

	retval = true;
	goto cleanup;

error:
	(void) smemzero(okm, okmLen);
	retval = false;
	goto cleanup;

cleanup:
	(void) smemzero(prk, sizeof prk);
	return retval;
}
