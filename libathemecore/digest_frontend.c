/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Frontend routines for the digest interface.
 */

#include <atheme.h>
#include "internal.h"

#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#define ATHEME_LAC_DIGEST_FRONTEND_C 1

#if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_OPENSSL)
#  include "digest_fe_openssl.c"
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_GCRYPT)
#  include "digest_fe_gcrypt.c"
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_MBEDTLS)
#  include "digest_fe_mbedtls.c"
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_INTERNAL)
#  include "digest_fe_internal.c"
#else
#  error "No Digest API frontend was selected by the build system"
#endif

static bool ATHEME_FATTR_WUR
_digest_update_vector(struct digest_context *const restrict ctx, const struct digest_vector *const restrict vec,
                      const size_t vecLen)
{
	for (size_t i = 0; i < vecLen; i++)
		if (! _digest_update(ctx, vec[i].ptr, vec[i].len))
			return false;

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot(const enum digest_algorithm alg, const void *const data, const size_t dataLen,
                void *const out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init(&ctx, alg))
		return false;

	if (! _digest_update(&ctx, data, dataLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_vector(const enum digest_algorithm alg, const struct digest_vector *const restrict vec,
                       const size_t vecLen, void *const restrict out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init(&ctx, alg))
		return false;

	if (! _digest_update_vector(&ctx, vec, vecLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_hmac(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                     const void *const data, const size_t dataLen, void *const out,
                     size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! _digest_update(&ctx, data, dataLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_hmac_vector(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                            const struct digest_vector *const restrict vec, const size_t vecLen,
                            void *const out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! _digest_update_vector(&ctx, vec, vecLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

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

size_t
digest_size_ctx(const struct digest_context *const restrict ctx)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return 0;
	}

	return digest_size_alg(ctx->alg);
}

bool ATHEME_FATTR_WUR
digest_init(struct digest_context *const restrict ctx, const enum digest_algorithm alg)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_init(ctx, alg);
}

bool ATHEME_FATTR_WUR
digest_init_hmac(struct digest_context *const restrict ctx, const enum digest_algorithm alg,
                 const void *const restrict key, const size_t keyLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! key && keyLen) || (key && ! keyLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched key parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_init_hmac(ctx, alg, key, keyLen);
}

bool ATHEME_FATTR_WUR
digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_update(ctx, data, dataLen);
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

	return _digest_update_vector(ctx, vec, vecLen);
}

bool ATHEME_FATTR_WUR
digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	const size_t hLen = digest_size_ctx(ctx);

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_final(ctx, out, outLen);
}

bool ATHEME_FATTR_WUR
digest_oneshot(const enum digest_algorithm alg, const void *const data, const size_t dataLen,
               void *const out, size_t *const outLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_oneshot(alg, data, dataLen, out, outLen);
}

bool ATHEME_FATTR_WUR
digest_oneshot_vector(const enum digest_algorithm alg, const struct digest_vector *const restrict vec,
                      const size_t vecLen, void *const restrict out, size_t *const outLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
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
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_oneshot_vector(alg, vec, vecLen, out, outLen);
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                    const void *const data, const size_t dataLen, void *const out,
                    size_t *const outLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
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
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_oneshot_hmac(alg, key, keyLen, data, dataLen, out, outLen);
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac_vector(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                           const struct digest_vector *const restrict vec, const size_t vecLen,
                           void *const out, size_t *const outLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
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
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_oneshot_hmac_vector(alg, key, keyLen, vec, vecLen, out, outLen);
}

bool ATHEME_FATTR_WUR
digest_hkdf_extract(const enum digest_algorithm alg, const void *const ikm, const size_t ikmLen,
                    const void *salt, size_t saltLen, void *const prk, const size_t prkLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! ikm)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ikm' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! ikmLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'ikmLen' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! salt && saltLen) || (salt && ! saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched salt parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! prk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'prk' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! prkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'prkLen' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (prkLen > hLen)
	{
		(void) slog(LG_ERROR, "%s: prkLen is too large (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	unsigned char zerobuf[DIGEST_MDLEN_MAX];
	unsigned char tmp[DIGEST_MDLEN_MAX];

	if (! salt)
	{
		(void) memset(zerobuf, 0x00, hLen);

		salt = zerobuf;
		saltLen = hLen;
	}
	if (! _digest_oneshot_hmac(alg, salt, saltLen, ikm, ikmLen, tmp, NULL))
	{
		(void) slog(LG_ERROR, "%s: _digest_oneshot_hmac() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) smemzero(tmp, sizeof tmp);
		return false;
	}

	(void) memcpy(prk, tmp, prkLen);
	(void) smemzero(tmp, sizeof tmp);
	return true;
}

bool ATHEME_FATTR_WUR
digest_hkdf_expand(const enum digest_algorithm alg, const void *const prk, const size_t prkLen,
                   const void *const info, const size_t infoLen, void *const okm, const size_t okmLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! prk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'prk' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! prkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'prkLen' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! info && infoLen) || (info && ! infoLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched info parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! okm)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'okm' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! okmLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'okmLen' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (okmLen > (hLen * 0xFFU))
	{
		(void) slog(LG_ERROR, "%s: called with unacceptable 'okmLen' (%zu) (BUG)", MOWGLI_FUNC_NAME, okmLen);
		return false;
	}

	unsigned char tmp[DIGEST_MDLEN_MAX];
	struct digest_context ctx;

	unsigned char *out = okm;
	size_t rem = okmLen;

	for (uint8_t t = 1; rem != 0; t++)
	{
		const size_t cpLen = (rem > hLen) ? hLen : rem;

		if (! digest_init_hmac(&ctx, alg, prk, prkLen))
			goto error;

		if (t > 1 && ! _digest_update(&ctx, tmp, hLen))
			goto error;

		if (! _digest_update(&ctx, info, infoLen))
			goto error;

		if (! _digest_update(&ctx, &t, sizeof t))
			goto error;

		if (! _digest_final(&ctx, tmp, NULL))
			goto error;

		(void) memcpy(out, tmp, cpLen);

		out += cpLen;
		rem -= cpLen;
	}

	(void) smemzero(&ctx, sizeof ctx);
	(void) smemzero(tmp, sizeof tmp);
	return true;

error:
	(void) smemzero(&ctx, sizeof ctx);
	(void) smemzero(tmp, sizeof tmp);
	return false;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hkdf(const enum digest_algorithm alg, const void *const ikm, const size_t ikmLen,
                    const void *const salt, const size_t saltLen, const void *const info, const size_t infoLen,
                    void *const okm, const size_t okmLen)
{
	unsigned char prk[DIGEST_MDLEN_MAX];

	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! digest_hkdf_extract(alg, ikm, ikmLen, salt, saltLen, prk, hLen))
	{
		(void) smemzero(prk, sizeof prk);
		return false;
	}
	if (! digest_hkdf_expand(alg, prk, hLen, info, infoLen, okm, okmLen))
	{
		(void) smemzero(prk, sizeof prk);
		return false;
	}

	(void) smemzero(prk, sizeof prk);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
                      const void *const restrict salt, const size_t saltLen, const size_t c,
                      void *const restrict dk, const size_t dkLen)
{
	const size_t hLen = digest_size_alg(alg);

	if (! hLen)
	{
		(void) slog(LG_ERROR, "%s: called with malformed/uninitialised 'alg' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! (pass && passLen))
	{
		(void) slog(LG_ERROR, "%s: called with no password (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! (salt && saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with no salt (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! c)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'c' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! dk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'dk' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! dkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'dkLen' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	return _digest_oneshot_pbkdf2(alg, pass, passLen, salt, saltLen, c, dk, dkLen);
}
