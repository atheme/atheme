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
 * SHA2-512 backend for Atheme IRC Services.
 */

#include "atheme.h"
#include "digest_be_sha2.h"

#define DIGEST_SHORT_BKLEN (DIGEST_BKLEN_SHA2_512 - 0x10U)

#define SHR(b, x)       ((x) >> (b))
#define S64(b, x)       (((x) >> (b)) | ((x) << (0x40U - (b))))

#define Sigma0(x)       (S64(0x1CU, (x)) ^ S64(0x22U, (x)) ^ S64(0x27U, (x)))
#define Sigma1(x)       (S64(0x0EU, (x)) ^ S64(0x12U, (x)) ^ S64(0x29U, (x)))
#define sigma0(x)       (S64(0x01U, (x)) ^ S64(0x08U, (x)) ^ SHR(0x07U, (x)))
#define sigma1(x)       (S64(0x13U, (x)) ^ S64(0x3DU, (x)) ^ SHR(0x06U, (x)))

#define Ch(x, y, z)     (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x, y, z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define REVERSE64(w, x)                                                                                             \
    do {                                                                                                            \
        uint64_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x20U) | (tmp << 0x20U);                                                                      \
        tmp = ((tmp & UINT64_C(0xFF00FF00FF00FF00)) >> 0x08U) | ((tmp & UINT64_C(0x00FF00FF00FF00FF)) << 0x08U);    \
        (x) = ((tmp & UINT64_C(0xFFFF0000FFFF0000)) >> 0x10U) | ((tmp & UINT64_C(0x0000FFFF0000FFFF)) << 0x10U);    \
    } while (0)

#define ADDINC128(w, n)                                                                                             \
    do {                                                                                                            \
        (w)[0] += (uint64_t) (n);                                                                                   \
        if ((w)[0] < (n)) {                                                                                         \
            (w)[1]++;                                                                                               \
        }                                                                                                           \
    } while(0)

#define ROUND_0_TO_15(a, b, c, d, e, f, g, h)                                                                       \
    do {                                                                                                            \
        uint64_t t1 = s[h] + Sigma1(s[e]) + Ch(s[e], s[f], s[g]) + K[j] + W[j];                                     \
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
        REVERSE64(*data++, W[j]);                                                                                   \
        ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                                      \
    } while (0)

#define ROUND(a, b, c, d, e, f, g, h)                                                                               \
    do {                                                                                                            \
        uint64_t s0 = W[(j + 0x01U) & 0x0FU];                                                                       \
        uint64_t s1 = W[(j + 0x0EU) & 0x0FU];                                                                       \
        s0 = sigma0(s0);                                                                                            \
        s1 = sigma1(s1);                                                                                            \
        W[j & 0x0FU] += s1 + W[(j + 0x09U) & 0x0FU] + s0;                                                           \
        uint64_t t1 = s[h] + Sigma1(s[e]) + Ch(s[e], s[f], s[g]) + K[j] + W[j & 0x0FU];                             \
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
digest_transform_block(struct digest_context_sha2_512 *const ctx, const uint64_t *data)
{
	static const uint64_t K[] = {

		UINT64_C(0x428A2F98D728AE22), UINT64_C(0x7137449123EF65CD),
		UINT64_C(0xB5C0FBCFEC4D3B2F), UINT64_C(0xE9B5DBA58189DBBC),
		UINT64_C(0x3956C25BF348B538), UINT64_C(0x59F111F1B605D019),
		UINT64_C(0x923F82A4AF194F9B), UINT64_C(0xAB1C5ED5DA6D8118),
		UINT64_C(0xD807AA98A3030242), UINT64_C(0x12835B0145706FBE),
		UINT64_C(0x243185BE4EE4B28C), UINT64_C(0x550C7DC3D5FFB4E2),
		UINT64_C(0x72BE5D74F27B896F), UINT64_C(0x80DEB1FE3B1696B1),
		UINT64_C(0x9BDC06A725C71235), UINT64_C(0xC19BF174CF692694),
		UINT64_C(0xE49B69C19EF14AD2), UINT64_C(0xEFBE4786384F25E3),
		UINT64_C(0x0FC19DC68B8CD5B5), UINT64_C(0x240CA1CC77AC9C65),
		UINT64_C(0x2DE92C6F592B0275), UINT64_C(0x4A7484AA6EA6E483),
		UINT64_C(0x5CB0A9DCBD41FBD4), UINT64_C(0x76F988DA831153B5),
		UINT64_C(0x983E5152EE66DFAB), UINT64_C(0xA831C66D2DB43210),
		UINT64_C(0xB00327C898FB213F), UINT64_C(0xBF597FC7BEEF0EE4),
		UINT64_C(0xC6E00BF33DA88FC2), UINT64_C(0xD5A79147930AA725),
		UINT64_C(0x06CA6351E003826F), UINT64_C(0x142929670A0E6E70),
		UINT64_C(0x27B70A8546D22FFC), UINT64_C(0x2E1B21385C26C926),
		UINT64_C(0x4D2C6DFC5AC42AED), UINT64_C(0x53380D139D95B3DF),
		UINT64_C(0x650A73548BAF63DE), UINT64_C(0x766A0ABB3C77B2A8),
		UINT64_C(0x81C2C92E47EDAEE6), UINT64_C(0x92722C851482353B),
		UINT64_C(0xA2BFE8A14CF10364), UINT64_C(0xA81A664BBC423001),
		UINT64_C(0xC24B8B70D0F89791), UINT64_C(0xC76C51A30654BE30),
		UINT64_C(0xD192E819D6EF5218), UINT64_C(0xD69906245565A910),
		UINT64_C(0xF40E35855771202A), UINT64_C(0x106AA07032BBD1B8),
		UINT64_C(0x19A4C116B8D2D0C8), UINT64_C(0x1E376C085141AB53),
		UINT64_C(0x2748774CDF8EEB99), UINT64_C(0x34B0BCB5E19B48A8),
		UINT64_C(0x391C0CB3C5C95A63), UINT64_C(0x4ED8AA4AE3418ACB),
		UINT64_C(0x5B9CCA4F7763E373), UINT64_C(0x682E6FF3D6B2B8A3),
		UINT64_C(0x748F82EE5DEFB2FC), UINT64_C(0x78A5636F43172F60),
		UINT64_C(0x84C87814A1F0AB72), UINT64_C(0x8CC702081A6439EC),
		UINT64_C(0x90BEFFFA23631E28), UINT64_C(0xA4506CEBDE82BDE9),
		UINT64_C(0xBEF9A3F7B2C67915), UINT64_C(0xC67178F2E372532B),
		UINT64_C(0xCA273ECEEA26619C), UINT64_C(0xD186B8C721C0C207),
		UINT64_C(0xEADA7DD6CDE0EB1E), UINT64_C(0xF57D4F7FEE6ED178),
		UINT64_C(0x06F067AA72176FBA), UINT64_C(0x0A637DC5A2C898A6),
		UINT64_C(0x113F9804BEF90DAE), UINT64_C(0x1B710B35131C471B),
		UINT64_C(0x28DB77F523047D84), UINT64_C(0x32CAAB7B40C72493),
		UINT64_C(0x3C9EBE0A15C9BEBC), UINT64_C(0x431D67C49C100D4C),
		UINT64_C(0x4CC5D4BECB3E42B6), UINT64_C(0x597F299CFC657E2A),
		UINT64_C(0x5FCB6FAB3AD6FAEC), UINT64_C(0x6C44198C4A475817),
	};

	uint64_t *const W = (uint64_t *) ctx->buf;
	uint64_t j = 0x00U;

	uint64_t s[DIGEST_STLEN_SHA2];

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

	} while (j < 0x50U);

	for (size_t x = 0x00U; x < DIGEST_STLEN_SHA2; x++)
		ctx->state[x] += s[x];

	(void) smemzero(s, sizeof s);
}

bool
digest_init_sha2_512(struct digest_context_sha2_512 *const restrict ctx)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	static const uint64_t iv[] = {

		UINT64_C(0x6A09E667F3BCC908), UINT64_C(0xBB67AE8584CAA73B),
		UINT64_C(0x3C6EF372FE94F82B), UINT64_C(0xA54FF53A5F1D36F1),
		UINT64_C(0x510E527FADE682D1), UINT64_C(0x9B05688C2B3E6C1F),
		UINT64_C(0x1F83D9ABFB41BD6B), UINT64_C(0x5BE0CD19137E2179),
	};

	(void) memset(ctx, 0x00U, sizeof *ctx);
	(void) memcpy(ctx->state, iv, sizeof iv);

	return true;
}

bool
digest_update_sha2_512(struct digest_context_sha2_512 *const restrict ctx,
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

	const uint64_t usedspace = (uint64_t) ((ctx->count[0] >> 0x03U) % DIGEST_BKLEN_SHA2_512);

	if (usedspace)
	{
		const uint64_t freespace = (DIGEST_BKLEN_SHA2_512 - usedspace);

		if (rem >= freespace)
		{
			(void) memcpy(ctx->buf + usedspace, ptr, (size_t) freespace);
			(void) digest_transform_block(ctx, (const void *) ctx->buf);

			ADDINC128(ctx->count, (freespace << 0x03U));

			ptr += freespace;
			rem -= freespace;
		}
		else
		{
			(void) memcpy(ctx->buf + usedspace, ptr, rem);

			ADDINC128(ctx->count, (rem << 0x03U));

			return true;
		}
	}

	while (rem >= DIGEST_BKLEN_SHA2_512)
	{
		(void) digest_transform_block(ctx, (const void *) ptr);

		ADDINC128(ctx->count, (DIGEST_BKLEN_SHA2_512 << 0x03U));

		ptr += DIGEST_BKLEN_SHA2_512;
		rem -= DIGEST_BKLEN_SHA2_512;
	}

	if (rem)
	{
		(void) memcpy(ctx->buf, ptr, rem);

		ADDINC128(ctx->count, (rem << 0x03U));
	}

	return true;
}

bool
digest_final_sha2_512(struct digest_context_sha2_512 *const restrict ctx,
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
		if (*len < DIGEST_MDLEN_SHA2_512)
		{
			(void) slog(LG_ERROR, "%s: output buffer length %zu is too small", __func__, *len);
			return false;
		}

		*len = DIGEST_MDLEN_SHA2_512;
	}

	uint64_t usedspace = ((ctx->count[0x00U] >> 0x03U) % DIGEST_BKLEN_SHA2_512);

	if (! digest_is_big_endian())
	{
		REVERSE64(ctx->count[0x00U], ctx->count[0x00U]);
		REVERSE64(ctx->count[0x01U], ctx->count[0x01U]);
	}

	if (usedspace)
	{
		ctx->buf[usedspace++] = 0x80U;

		if (usedspace <= DIGEST_SHORT_BKLEN)
		{
			(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_SHORT_BKLEN - usedspace));
		}
		else
		{
			if (usedspace < DIGEST_BKLEN_SHA2_512)
			{
				(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_BKLEN_SHA2_512 - usedspace));
			}

			(void) digest_transform_block(ctx, (const void *) ctx->buf);
			(void) memset(ctx->buf, 0x00U, (DIGEST_BKLEN_SHA2_512 - 0x02U));
		}
	}
	else
	{
		(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN);

		ctx->buf[0x00U] = 0x80U;
	}

	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN]) = ctx->count[0x01U];
	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN + 0x08U]) = ctx->count[0x00U];

	(void) digest_transform_block(ctx, (const void *) ctx->buf);

	uint64_t *d = (uint64_t *) out;

	if (digest_is_big_endian())
		(void) memcpy(d, ctx->state, sizeof ctx->state);
	else for (size_t i = 0x00U; i < DIGEST_STLEN_SHA2; i++)
		REVERSE64(ctx->state[i], *d++);

	(void) smemzero(ctx, sizeof *ctx);

	return true;
}
