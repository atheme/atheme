/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-URL: https://spdx.org/licenses/BSD-3-Clause.html
 *
 * FILE:	sha2.c
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (C) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Heavily modified by Aaron M. D. Jones <aaronmdjones@gmail.com> (2018)
 * for general code cleanup and conformance to new algorithm-agnostic
 * digest API for Atheme IRC Services <https://github.com/atheme/atheme/>.
 *
 * SHA2-256 backend for Atheme IRC Services.
 */

#include "atheme.h"
#include "digest_be_sha2.h"

#define DIGEST_SHORT_BKLEN (DIGEST_BKLEN_SHA2_256 - 0x08U)

#define SHR(b, x)       ((x) >> (b))
#define S32(b, x)       (((x) >> (b)) | ((x) << (0x20U - (b))))

#define Sigma0(x)       (S32(0x02U, (x)) ^ S32(0x0DU, (x)) ^ S32(0x16U, (x)))
#define Sigma1(x)       (S32(0x06U, (x)) ^ S32(0x0BU, (x)) ^ S32(0x19U, (x)))
#define sigma0(x)       (S32(0x07U, (x)) ^ S32(0x12U, (x)) ^ SHR(0x03U, (x)))
#define sigma1(x)       (S32(0x11U, (x)) ^ S32(0x13U, (x)) ^ SHR(0x0AU, (x)))

#define Ch(x, y, z)     (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x, y, z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define REVERSE32(w, x)                                                                                             \
    do {                                                                                                            \
        uint32_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x10U) | (tmp << 0x10U);                                                                      \
        (x) = ((tmp & UINT32_C(0xFF00FF00)) >> 0x08U) | ((tmp & UINT32_C(0x00FF00FF)) << 0x08U);                    \
    } while (0)

#define REVERSE64(w, x)                                                                                             \
    do {                                                                                                            \
        uint64_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x20U) | (tmp << 0x20U);                                                                      \
        tmp = ((tmp & UINT64_C(0xFF00FF00FF00FF00)) >> 0x08U) | ((tmp & UINT64_C(0x00FF00FF00FF00FF)) << 0x08U);    \
        (x) = ((tmp & UINT64_C(0xFFFF0000FFFF0000)) >> 0x10U) | ((tmp & UINT64_C(0x0000FFFF0000FFFF)) << 0x10U);    \
    } while (0)

#define ROUND_0_TO_15(a, b, c, d, e, f, g, h)                                                                       \
    do {                                                                                                            \
        uint32_t t1 = s[h] + Sigma1(s[e]) + Ch(s[e], s[f], s[g]) + K[j] + W[j];                                     \
        s[h] = t1 + Sigma0(s[a]) + Maj(s[a], s[b], s[c]);                                                           \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

#define ROUND_0_TO_15_BE(a, b, c, d, e, f, g, h)                                                                    \
    do {                                                                                                            \
        W[j] = *data++;                                                                                             \
        ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                                      \
    } while (0)

#define ROUND_0_TO_15_LE(a, b, c, d, e, f, g, h)                                                                    \
    do {                                                                                                            \
        REVERSE32(*data++, W[j]);                                                                                   \
        ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                                      \
    } while (0)

#define ROUND(a, b, c, d, e, f, g, h)                                                                               \
    do {                                                                                                            \
        uint32_t s0 = W[(j + 0x01U) & 0x0FU];                                                                       \
        uint32_t s1 = W[(j + 0x0EU) & 0x0FU];                                                                       \
        s0 = sigma0(s0);                                                                                            \
        s1 = sigma1(s1);                                                                                            \
        W[j & 0x0FU] += s1 + W[(j + 0x09U) & 0x0FU] + s0;                                                           \
        uint32_t t1 = s[h] + Sigma1(s[e]) + Ch(s[e], s[f], s[g]) + K[j] + W[j & 0x0FU];                             \
        s[h] = t1 + Sigma0(s[a]) + Maj(s[a], s[b], s[c]);                                                           \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

static inline bool
digest_is_big_endian(void)
{
	return (bool) (htonl(UINT32_C(0x11223344)) == UINT32_C(0x11223344));
}

static void
digest_transform_block(struct digest_context_sha2_256 *const ctx, const uint32_t *data)
{
	static const uint32_t K[] = {

		UINT32_C(0x428A2F98), UINT32_C(0x71374491), UINT32_C(0xB5C0FBCF), UINT32_C(0xE9B5DBA5),
		UINT32_C(0x3956C25B), UINT32_C(0x59F111F1), UINT32_C(0x923F82A4), UINT32_C(0xAB1C5ED5),
		UINT32_C(0xD807AA98), UINT32_C(0x12835B01), UINT32_C(0x243185BE), UINT32_C(0x550C7DC3),
		UINT32_C(0x72BE5D74), UINT32_C(0x80DEB1FE), UINT32_C(0x9BDC06A7), UINT32_C(0xC19BF174),
		UINT32_C(0xE49B69C1), UINT32_C(0xEFBE4786), UINT32_C(0x0FC19DC6), UINT32_C(0x240CA1CC),
		UINT32_C(0x2DE92C6F), UINT32_C(0x4A7484AA), UINT32_C(0x5CB0A9DC), UINT32_C(0x76F988DA),
		UINT32_C(0x983E5152), UINT32_C(0xA831C66D), UINT32_C(0xB00327C8), UINT32_C(0xBF597FC7),
		UINT32_C(0xC6E00BF3), UINT32_C(0xD5A79147), UINT32_C(0x06CA6351), UINT32_C(0x14292967),
		UINT32_C(0x27B70A85), UINT32_C(0x2E1B2138), UINT32_C(0x4D2C6DFC), UINT32_C(0x53380D13),
		UINT32_C(0x650A7354), UINT32_C(0x766A0ABB), UINT32_C(0x81C2C92E), UINT32_C(0x92722C85),
		UINT32_C(0xA2BFE8A1), UINT32_C(0xA81A664B), UINT32_C(0xC24B8B70), UINT32_C(0xC76C51A3),
		UINT32_C(0xD192E819), UINT32_C(0xD6990624), UINT32_C(0xF40E3585), UINT32_C(0x106AA070),
		UINT32_C(0x19A4C116), UINT32_C(0x1E376C08), UINT32_C(0x2748774C), UINT32_C(0x34B0BCB5),
		UINT32_C(0x391C0CB3), UINT32_C(0x4ED8AA4A), UINT32_C(0x5B9CCA4F), UINT32_C(0x682E6FF3),
		UINT32_C(0x748F82EE), UINT32_C(0x78A5636F), UINT32_C(0x84C87814), UINT32_C(0x8CC70208),
		UINT32_C(0x90BEFFFA), UINT32_C(0xA4506CEB), UINT32_C(0xBEF9A3F7), UINT32_C(0xC67178F2),
	};

	uint32_t *const W = (uint32_t *) ctx->buf;
	uint32_t j = 0x00U;

	uint32_t s[DIGEST_STLEN_SHA2];

	(void) memcpy(s, ctx->state, sizeof s);

	if (digest_is_big_endian())
	{
		do {
			ROUND_0_TO_15_BE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			ROUND_0_TO_15_BE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			ROUND_0_TO_15_BE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			ROUND_0_TO_15_BE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			ROUND_0_TO_15_BE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			ROUND_0_TO_15_BE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			ROUND_0_TO_15_BE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			ROUND_0_TO_15_BE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}
	else
	{
		do {
			ROUND_0_TO_15_LE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			ROUND_0_TO_15_LE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			ROUND_0_TO_15_LE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			ROUND_0_TO_15_LE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			ROUND_0_TO_15_LE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			ROUND_0_TO_15_LE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			ROUND_0_TO_15_LE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			ROUND_0_TO_15_LE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}

	do {
		ROUND(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
		ROUND(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
		ROUND(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
		ROUND(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
		ROUND(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
		ROUND(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
		ROUND(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
		ROUND(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

	} while (j < 0x40U);

	for (size_t x = 0x00U; x < DIGEST_STLEN_SHA2; x++)
		ctx->state[x] += s[x];

	(void) explicit_bzero(s, sizeof s);
}

bool
digest_init_sha2_256(struct digest_context_sha2_256 *const restrict ctx)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	static const uint32_t iv[] = {

		UINT32_C(0x6A09E667), UINT32_C(0xBB67AE85), UINT32_C(0x3C6EF372), UINT32_C(0xA54FF53A),
		UINT32_C(0x510E527F), UINT32_C(0x9B05688C), UINT32_C(0x1F83D9AB), UINT32_C(0x5BE0CD19),
	};

	(void) memset(ctx, 0x00U, sizeof *ctx);
	(void) memcpy(ctx->state, iv, sizeof iv);

	return true;
}

bool
digest_update_sha2_256(struct digest_context_sha2_256 *const restrict ctx,
                       const void *const restrict in, const size_t len)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	if (! (in && len))
		return true;

	const uint8_t *ptr = in;
	size_t rem = len;

	const uint64_t usedspace = (uint64_t) ((ctx->count >> 0x03U) % DIGEST_BKLEN_SHA2_256);

	if (usedspace)
	{
		const uint64_t freespace = (DIGEST_BKLEN_SHA2_256 - usedspace);

		if (rem >= freespace)
		{
			(void) memcpy(ctx->buf + usedspace, ptr, (size_t) freespace);
			(void) digest_transform_block(ctx, (const void *) ctx->buf);

			ctx->count += (freespace << 0x03U);

			ptr += freespace;
			rem -= freespace;
		}
		else
		{
			(void) memcpy(ctx->buf + usedspace, ptr, rem);

			ctx->count += (rem << 0x03U);

			return true;
		}
	}

	while (rem >= DIGEST_BKLEN_SHA2_256)
	{
		(void) digest_transform_block(ctx, (const void *) ptr);

		ctx->count += (DIGEST_BKLEN_SHA2_256 << 0x03U);

		ptr += DIGEST_BKLEN_SHA2_256;
		rem -= DIGEST_BKLEN_SHA2_256;
	}

	if (rem)
	{
		(void) memcpy(ctx->buf, ptr, rem);

		ctx->count += (rem << 0x03U);
	}

	return true;
}

bool
digest_final_sha2_256(struct digest_context_sha2_256 *const restrict ctx,
                      void *const restrict out, size_t *const restrict len)
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

	if (len)
	{
		if (*len < DIGEST_MDLEN_SHA2_256)
		{
			(void) slog(LG_ERROR, "%s: output buffer length %zu is too small", __func__, *len);
			return false;
		}

		*len = DIGEST_MDLEN_SHA2_256;
	}

	uint64_t usedspace = (ctx->count >> 0x03U) % DIGEST_BKLEN_SHA2_256;

	if (! digest_is_big_endian())
		REVERSE64(ctx->count, ctx->count);

	if (usedspace)
	{
		ctx->buf[usedspace++] = 0x80U;

		if (usedspace <= DIGEST_SHORT_BKLEN)
		{
			(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_SHORT_BKLEN - usedspace));
		}
		else
		{
			if (usedspace < DIGEST_BKLEN_SHA2_256)
			{
				(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_BKLEN_SHA2_256 - usedspace));
			}

			(void) digest_transform_block(ctx, (const void *) ctx->buf);
			(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN);
		}
	}
	else
	{
		(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN);

		ctx->buf[0x00U] = 0x80U;
	}

	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN]) = ctx->count;

	(void) digest_transform_block(ctx, (const void *) ctx->buf);

	uint32_t *d = (uint32_t *) out;

	if (digest_is_big_endian())
		(void) memcpy(d, ctx->state, sizeof ctx->state);
	else for (size_t i = 0x00U; i < DIGEST_STLEN_SHA2; i++)
		REVERSE32(ctx->state[i], *d++);

	(void) explicit_bzero(ctx, sizeof *ctx);
	return true;
}
