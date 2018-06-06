/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Nettle frontend for the digest interface.
 */

#include "atheme.h"

#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>

#include <nettle/nettle-meta.h>

#define DIGEST_HMAC_INNER_XORVAL        0x36U
#define DIGEST_HMAC_OUTER_XORVAL        0x5CU

bool ATHEME_FATTR_WUR
digest_init(struct digest_context *const restrict ctx, const unsigned int alg)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	(void) memset(ctx, 0x00, sizeof *ctx);

	switch (alg)
	{
		case DIGALG_MD5:
			ctx->init   = (digest_init_fn)   &nettle_md5_init;
			ctx->update = (digest_update_fn) &nettle_md5_update;
			ctx->final  = (digest_final_fn)  &nettle_md5_digest;
			ctx->blksz  = DIGEST_BKLEN_MD5;
			ctx->digsz  = DIGEST_MDLEN_MD5;
			break;

		case DIGALG_SHA1:
			ctx->init   = (digest_init_fn)   &nettle_sha1_init;
			ctx->update = (digest_update_fn) &nettle_sha1_update;
			ctx->final  = (digest_final_fn)  &nettle_sha1_digest;
			ctx->blksz  = DIGEST_BKLEN_SHA1;
			ctx->digsz  = DIGEST_MDLEN_SHA1;
			break;

		case DIGALG_SHA2_256:
			ctx->init   = (digest_init_fn)   &nettle_sha256_init;
			ctx->update = (digest_update_fn) &nettle_sha256_update;
			ctx->final  = (digest_final_fn)  &nettle_sha256_digest;
			ctx->blksz  = DIGEST_BKLEN_SHA2_256;
			ctx->digsz  = DIGEST_MDLEN_SHA2_256;
			break;

		case DIGALG_SHA2_512:
			ctx->init   = (digest_init_fn)   &nettle_sha512_init;
			ctx->update = (digest_update_fn) &nettle_sha512_update;
			ctx->final  = (digest_final_fn)  &nettle_sha512_digest;
			ctx->blksz  = DIGEST_BKLEN_SHA2_512;
			ctx->digsz  = DIGEST_MDLEN_SHA2_512;
			break;

		default:
			(void) slog(LG_ERROR, "%s: called with unknown/unimplemented alg '%u' (BUG)",
			                      __func__, alg);
			return false;
	}

	(void) ctx->init(&ctx->state);
	return true;
}

bool ATHEME_FATTR_WUR
digest_init_hmac(struct digest_context *const restrict ctx, const unsigned int alg,
                 const void *const restrict key, const size_t keyLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	if (! digest_init(ctx, alg))
		return false;

	if (key && keyLen)
	{
		if (keyLen > ctx->blksz)
		{
			if (! digest_oneshot(alg, key, keyLen, ctx->ikey, NULL))
				return false;

			(void) memcpy(ctx->okey, ctx->ikey, ctx->digsz);
		}
		else
		{
			(void) memcpy(ctx->ikey, key, keyLen);
			(void) memcpy(ctx->okey, key, keyLen);
		}
	}

	for (size_t i = 0; i < ctx->blksz; i++)
	{
		ctx->ikey[i] ^= DIGEST_HMAC_INNER_XORVAL;
		ctx->okey[i] ^= DIGEST_HMAC_OUTER_XORVAL;
	}

	ctx->hmac = true;

	(void) ctx->update(&ctx->state, ctx->blksz, ctx->ikey);
	return true;
}

bool ATHEME_FATTR_WUR
digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}
	if (! ctx->update)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx->update' (BUG)", __func__);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", __func__);
		return false;
	}

	if (! (data && dataLen))
		return true;

	(void) ctx->update(&ctx->state, dataLen, data);
	return true;
}

bool ATHEME_FATTR_WUR
digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", __func__);
		return false;
	}
	if (! ctx->final)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx->final' (BUG)", __func__);
		return false;
	}
	if (outLen && *outLen < ctx->digsz)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", __func__);
		return false;
	}
	else if (outLen)
		*outLen = ctx->digsz;

	if (ctx->hmac)
	{
		uint8_t inner_digest[DIGEST_MDLEN_MAX];

		(void) ctx->final(&ctx->state, ctx->digsz, inner_digest);
		(void) ctx->init(&ctx->state);
		(void) ctx->update(&ctx->state, ctx->blksz, ctx->okey);
		(void) ctx->update(&ctx->state, ctx->digsz, inner_digest);
		(void) explicit_bzero(inner_digest, sizeof inner_digest);
	}

	(void) ctx->final(&ctx->state, ctx->digsz, out);
	return true;
}

bool ATHEME_FATTR_WUR
digest_pbkdf2_hmac(const unsigned int alg, const void *const restrict pass, const size_t passLen,
                   const void *const restrict salt, const size_t saltLen, const size_t c,
                   void *const restrict dk, const size_t dkLen)
{
	/*
	 * The PBKDF2-HMAC algorithm.
	 *
	 * Inputs:
	 *
	 *          alg = Message digest algorithm to use in HMAC mode
	 *         pass = Password
	 *         salt = Salt
	 *            c = Iteration Count
	 *        dkLen = Desired length of DerivedKey
	 *
	 * Internal Functions or Variables:
	 *
	 *     htonl(i) = representation of 'i' in big-endian (network) byte order,
	 *                where 'i' is a 32-bit integer as input to T(i)
	 *
	 *   HMAC(k, d) = HMAC algorithm with 'alg' as underlying message digest,
	 *                'k' as HMAC key, & 'd' as data to generate HMAC for
	 *
	 *         hLen = Output length of HMAC()
	 *
	 * Algorithm:
	 *
	 *         U(x) = {
	 *                  x = 1: HMAC(pass, salt || htonl(i))
	 *                  x > 1: HMAC(pass, U(x - 1))
	 *                }
	 *
	 *         T(i) = U(1) ^ U(2) ^ U(3) ^ ... ^ U(c)
	 *           DK = T(1) || T(2) || T(3) || ... || T(dkLen/hLen)
	 *
	 * This function contains 2 identical optimisations; one at the end of
	 * the outer loop that derives T(i), and one at the beginning of the
	 * inner loop that derives U(j + 1). The optimisation is as follows:
	 *
	 *   digest_init_hmac() computes the inner and outer HMAC keys and
	 *   stores them in the digest context, initialises the underlying
	 *   context, and adds the inner key to the digest state (so it is
	 *   ready to receive data).
	 *
	 *   When the HMAC calculation is complete (by calling digest_final()
	 *   on a context that was initialised by digest_init_hmac()), we
	 *   simply reinitialise the underlying state and add the inner key
	 *   to it again.
	 *
	 *   This is what digest_init_hmac() would do, but it is more
	 *   efficient; every time the loops make another pass, we're not
	 *   constantly validating the algorithm identifier, erasing memory,
	 *   setting function pointers, possibly performing a digest operation
	 *   (if the HMAC key is longer than the digest's block size), and
	 *   deriving the inner and outer keys with XOR operations again.
	 *
	 * Most invocations of this function will only ever get to i == 1; the
	 * outer loop will be executed once, for T(1) only. Such is the case
	 * when the requested output key length is not greater than the digest
	 * size of the underlying digest algorithm. However, the optimisation
	 * is still worth having.
	 *
	 * Similarly, we don't call digest_update(), which does some constraint
	 * checking before passing on the data to the underlying digest driver;
	 * we just call the underlying digest directly.
	 */

	uint8_t dtmp[DIGEST_MDLEN_MAX];
	struct digest_context ctx;

	if (! c)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'c' (BUG)", __func__);
		goto error;
	}
	if (! dk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'dk' (BUG)", __func__);
		goto error;
	}
	if (! dkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'dkLen' (BUG)", __func__);
		goto error;
	}
	if ((! pass && passLen) || (pass && ! passLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched pass parameters (BUG)", __func__);
		goto error;
	}
	if ((! salt && saltLen) || (salt && ! saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched salt parameters (BUG)", __func__);
		goto error;
	}

	if (! digest_init_hmac(&ctx, alg, pass, passLen))
		goto error;

	const size_t hLen = ctx.digsz;
	size_t odkLen = dkLen;
	uint8_t *odk = dk;

	for (uint32_t i = 1; /* No condition */ ; i++)
	{
		const uint32_t ibe = htonl(i);
		const size_t cpLen = (odkLen > hLen) ? hLen : odkLen;

		(void) ctx.update(&ctx.state, saltLen, salt);
		(void) ctx.update(&ctx.state, sizeof ibe, &ibe);

		if (! digest_final(&ctx, dtmp, NULL))
			goto error;

		(void) memcpy(odk, dtmp, cpLen);

		for (size_t j = 1; j < c; j++)
		{
			(void) ctx.init(&ctx.state);
			(void) ctx.update(&ctx.state, ctx.blksz, ctx.ikey);
			(void) ctx.update(&ctx.state, hLen, dtmp);

			if (! digest_final(&ctx, dtmp, NULL))
				goto error;

			for (size_t k = 0; k < cpLen; k++)
				odk[k] ^= dtmp[k];
		}

		odkLen -= cpLen;
		odk += cpLen;

		if (! odkLen)
			break;

		(void) ctx.init(&ctx.state);
		(void) ctx.update(&ctx.state, ctx.blksz, ctx.ikey);
	}

	(void) explicit_bzero(&ctx, sizeof ctx);
	(void) explicit_bzero(dtmp, sizeof dtmp);
	return true;

error:
	(void) explicit_bzero(&ctx, sizeof ctx);
	(void) explicit_bzero(dtmp, sizeof dtmp);
	(void) explicit_bzero(dk, dkLen);
	return false;
}
