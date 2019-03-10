/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Nettle frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>
#include <nettle/nettle-meta.h>

#ifdef HAVE_NETTLE_VERSION_H
#  include <nettle/version.h>
#endif /* HAVE_NETTLE_VERSION_H */

#define ATHEME_NETTLE_VERSION_PREFIX    "Nettle <https://www.lysator.liu.se/~nisse/nettle/>"

#define DIGEST_HMAC_INNER_XORVAL        0x36U
#define DIGEST_HMAC_OUTER_XORVAL        0x5CU

static bool _digest_oneshot(enum digest_algorithm, const void *, size_t, void *, size_t *);

const char *
digest_get_frontend_info(void)
{
#ifdef HAVE_NETTLE_VERSION_H

	static char result[BUFSIZE];

	const int vcmaj = NETTLE_VERSION_MAJOR;
	const int vcmin = NETTLE_VERSION_MINOR;
	const int vrmaj = nettle_version_major();
	const int vrmin = nettle_version_minor();

	(void) snprintf(result, sizeof result, "%s (compiled %d.%d, library %d.%d)",
	                ATHEME_NETTLE_VERSION_PREFIX, vcmaj, vcmin, vrmaj, vrmin);

	return result;

#else /* HAVE_NETTLE_VERSION_H */

	return ATHEME_NETTLE_VERSION_PREFIX " (unknown version)";

#endif /* ! HAVE_NETTLE_VERSION_H */
}

static bool
_digest_init(struct digest_context *const restrict ctx, const enum digest_algorithm alg)
{
	(void) memset(ctx, 0x00, sizeof *ctx);

	ctx->alg = alg;

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
	}

	if (! ctx->digsz)
	{
		(void) slog(LG_ERROR, "%s: unrecognised algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
		return false;
	}

	(void) ctx->init(&ctx->state);
	return true;
}

static bool
_digest_init_hmac(struct digest_context *const restrict ctx, const enum digest_algorithm alg,
                  const void *const restrict key, const size_t keyLen)
{
	(void) _digest_init(ctx, alg);

	ctx->hmac = true;

	if (key && keyLen)
	{
		if (keyLen > ctx->blksz)
		{
			(void) _digest_oneshot(alg, key, keyLen, ctx->ikey, NULL);
			(void) memcpy(ctx->okey, ctx->ikey, ctx->blksz);
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

	(void) ctx->update(&ctx->state, ctx->blksz, ctx->ikey);
	return true;
}

static bool
_digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (data && dataLen)
		(void) ctx->update(&ctx->state, dataLen, data);

	return true;
}

static bool
_digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	if (outLen)
		*outLen = ctx->digsz;

	if (ctx->hmac)
	{
		unsigned char inner_digest[DIGEST_MDLEN_MAX];

		(void) ctx->final(&ctx->state, ctx->digsz, inner_digest);
		(void) ctx->init(&ctx->state);
		(void) ctx->update(&ctx->state, ctx->blksz, ctx->okey);
		(void) ctx->update(&ctx->state, ctx->digsz, inner_digest);
		(void) smemzero(inner_digest, sizeof inner_digest);
	}

	(void) ctx->final(&ctx->state, ctx->digsz, out);
	return true;
}

static bool
_digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
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
	 *        dkLen = Desired length of DK; DerivedKey
	 *
	 * Internal Functions or Variables:
	 *
	 *     htonl(i) = representation of 'i' in big-endian (network) byte
	 *                order, where 'i' is a 32-bit integer input to T(i)
	 *
	 *   HMAC(k, d) = HMAC algorithm with 'alg' as underlying digest,
	 *                'k' as HMAC key, & 'd' as data to generate MAC for
	 *
	 *         hLen = Output length of HMAC()
	 *
	 * Algorithm:
	 *
	 *         U(i, j) = {
	 *             j = 0: HMAC(pass, salt || htonl(i))
	 *             j > 0: HMAC(pass, U(j - 1))
	 *         }
	 *
	 *         T(i) = U(i, 0) ^ U(i, 1) ^ U(i, 2) ^ ... ^ U(i, c)
	 *           DK = T(1) || T(2) || T(3) || ... || T(dkLen/hLen)
	 *
	 * This function contains 2 identical optimisations; one at the
	 * beginning of the inner loop that derives U(i, 1 ... c), and one
	 * at the end of the outer loop that derives U(i, 0) and T(i). The
	 * optimisation is as follows:
	 *
	 *   _digest_init_hmac() computes the inner and outer HMAC keys and
	 *   stores them in the digest context, initialises the underlying
	 *   digest state, and adds the inner key to the digest state (so it
	 *   is ready to receive data).
	 *
	 *   When the HMAC calculation is completed (by calling the
	 *   _digest_final() function on a context that was initialised by
	 *   _digest_init_hmac()), we simply reinitialise the underlying
	 *   digest state and add the inner key to it again.
	 *
	 *   This is what _digest_init_hmac() would do, but it is more
	 *   efficient; every time the loops make another pass, we're not
	 *   constantly re-validating the algorithm identifier, erasing
	 *   memory, setting function pointers, possibly performing a digest
	 *   operation (if the password is longer than the underlying digest
	 *   block size), and deriving the inner and outer HMAC keys again.
	 *
	 * Most invocations of this function will only ever get to i == 1;
	 * the outer loop will be executed once, for T(1) only. Such is the
	 * case when dkLen <= hLen.
	 */

	unsigned char tmp[DIGEST_MDLEN_MAX];
	struct digest_context ctx;

	(void) _digest_init_hmac(&ctx, alg, pass, passLen);

	const size_t hLen = ctx.digsz;
	unsigned char *out = dk;
	size_t rem = dkLen;

	for (uint32_t i = 1; /* No condition */ ; i++)
	{
		const size_t cpLen = (rem > hLen) ? hLen : rem;
		const uint32_t ibe = htonl(i);

		(void) ctx.update(&ctx.state, saltLen, salt);
		(void) ctx.update(&ctx.state, sizeof ibe, &ibe);
		(void) _digest_final(&ctx, tmp, NULL);
		(void) memcpy(out, tmp, cpLen);

		for (size_t j = 1; j < c; j++)
		{
			(void) ctx.init(&ctx.state);
			(void) ctx.update(&ctx.state, ctx.blksz, ctx.ikey);
			(void) ctx.update(&ctx.state, hLen, tmp);
			(void) _digest_final(&ctx, tmp, NULL);

			for (size_t k = 0; k < cpLen; k++)
				out[k] ^= tmp[k];
		}

		out += cpLen;
		rem -= cpLen;

		if (! rem)
			break;

		(void) ctx.init(&ctx.state);
		(void) ctx.update(&ctx.state, ctx.blksz, ctx.ikey);
	}

	(void) smemzero(&ctx, sizeof ctx);
	(void) smemzero(tmp, sizeof tmp);
	return true;
}
