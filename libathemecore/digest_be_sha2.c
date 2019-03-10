/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-URL: https://spdx.org/licenses/BSD-3-Clause.html
 *
 * FILE:       sha2.c
 * AUTHOR:     Aaron D. Gifford - http://www.aarongifford.com/
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
 * SHA2-256 & SHA2-512 backend for Atheme IRC Services.
 */

#ifndef ATHEME_LAC_DIGEST_FE_INTERNAL_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FE_INTERNAL_C */

#define DIGEST_SHORT_BKLEN_SHA2_256     (DIGEST_BKLEN_SHA2_256 - 0x08U)
#define DIGEST_SHORT_BKLEN_SHA2_512     (DIGEST_BKLEN_SHA2_512 - 0x10U)

#define SHA2_SHR(b, x)                  ((x) >> (b))
#define SHA2_256_S32(b, x)              (((x) >> (b)) | ((x) << (0x20U - (b))))
#define SHA2_512_S64(b, x)              (((x) >> (b)) | ((x) << (0x40U - (b))))

#define SHA2_256_Sigma0(x)              (SHA2_256_S32(0x02U, (x)) ^ SHA2_256_S32(0x0DU, (x)) ^ SHA2_256_S32(0x16U, (x)))
#define SHA2_256_Sigma1(x)              (SHA2_256_S32(0x06U, (x)) ^ SHA2_256_S32(0x0BU, (x)) ^ SHA2_256_S32(0x19U, (x)))
#define SHA2_256_sigma0(x)              (SHA2_256_S32(0x07U, (x)) ^ SHA2_256_S32(0x12U, (x)) ^ SHA2_SHR(0x03U, (x)))
#define SHA2_256_sigma1(x)              (SHA2_256_S32(0x11U, (x)) ^ SHA2_256_S32(0x13U, (x)) ^ SHA2_SHR(0x0AU, (x)))

#define SHA2_512_Sigma0(x)              (SHA2_512_S64(0x1CU, (x)) ^ SHA2_512_S64(0x22U, (x)) ^ SHA2_512_S64(0x27U, (x)))
#define SHA2_512_Sigma1(x)              (SHA2_512_S64(0x0EU, (x)) ^ SHA2_512_S64(0x12U, (x)) ^ SHA2_512_S64(0x29U, (x)))
#define SHA2_512_sigma0(x)              (SHA2_512_S64(0x01U, (x)) ^ SHA2_512_S64(0x08U, (x)) ^ SHA2_SHR(0x07U, (x)))
#define SHA2_512_sigma1(x)              (SHA2_512_S64(0x13U, (x)) ^ SHA2_512_S64(0x3DU, (x)) ^ SHA2_SHR(0x06U, (x)))

#define SHA2_Ch(x, y, z)                (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA2_Maj(x, y, z)               (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SHA2_256_REVERSE32(w, x)                                                                                    \
    do {                                                                                                            \
        uint32_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x10U) | (tmp << 0x10U);                                                                      \
        (x) = ((tmp & UINT32_C(0xFF00FF00)) >> 0x08U) | ((tmp & UINT32_C(0x00FF00FF)) << 0x08U);                    \
    } while (0)

#define SHA2_256_REVERSE64(w, x)                                                                                    \
    do {                                                                                                            \
        uint64_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x20U) | (tmp << 0x20U);                                                                      \
        tmp = ((tmp & UINT64_C(0xFF00FF00FF00FF00)) >> 0x08U) | ((tmp & UINT64_C(0x00FF00FF00FF00FF)) << 0x08U);    \
        (x) = ((tmp & UINT64_C(0xFFFF0000FFFF0000)) >> 0x10U) | ((tmp & UINT64_C(0x0000FFFF0000FFFF)) << 0x10U);    \
    } while (0)

#define SHA2_512_REVERSE64(w, x)                                                                                    \
    do {                                                                                                            \
        uint64_t tmp = (w);                                                                                         \
        tmp = (tmp >> 0x20U) | (tmp << 0x20U);                                                                      \
        tmp = ((tmp & UINT64_C(0xFF00FF00FF00FF00)) >> 0x08U) | ((tmp & UINT64_C(0x00FF00FF00FF00FF)) << 0x08U);    \
        (x) = ((tmp & UINT64_C(0xFFFF0000FFFF0000)) >> 0x10U) | ((tmp & UINT64_C(0x0000FFFF0000FFFF)) << 0x10U);    \
    } while (0)

#define SHA2_512_ADDINC128(w, n)                                                                                    \
    do {                                                                                                            \
        (w)[0] += (uint64_t) (n);                                                                                   \
        if ((w)[0] < (n)) {                                                                                         \
            (w)[1]++;                                                                                               \
        }                                                                                                           \
    } while(0)

#define SHA2_256_ROUND_0_TO_15(a, b, c, d, e, f, g, h)                                                              \
    do {                                                                                                            \
        uint32_t t1 = s[h] + SHA2_256_Sigma1(s[e]) + SHA2_Ch(s[e], s[f], s[g]) + K[j] + W[j];                       \
        s[h] = t1 + SHA2_256_Sigma0(s[a]) + SHA2_Maj(s[a], s[b], s[c]);                                             \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

#define SHA2_512_ROUND_0_TO_15(a, b, c, d, e, f, g, h)                                                              \
    do {                                                                                                            \
        uint64_t t1 = s[h] + SHA2_512_Sigma1(s[e]) + SHA2_Ch(s[e], s[f], s[g]) + K[j] + W[j];                       \
        s[h] = t1 + SHA2_512_Sigma0(s[a]) + SHA2_Maj(s[a], s[b], s[c]);                                             \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

#define SHA2_256_ROUND_0_TO_15_BE(a, b, c, d, e, f, g, h)                                                           \
    do {                                                                                                            \
        W[j] = *data++;                                                                                             \
        SHA2_256_ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                             \
    } while (0)

#define SHA2_512_ROUND_0_TO_15_BE(a, b, c, d, e, f, g, h)                                                           \
    do {                                                                                                            \
        W[j] = *data++;                                                                                             \
        SHA2_512_ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                             \
    } while (0)

#define SHA2_256_ROUND_0_TO_15_LE(a, b, c, d, e, f, g, h)                                                           \
    do {                                                                                                            \
        SHA2_256_REVERSE32(*data++, W[j]);                                                                          \
        SHA2_256_ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                             \
    } while (0)

#define SHA2_512_ROUND_0_TO_15_LE(a, b, c, d, e, f, g, h)                                                           \
    do {                                                                                                            \
        SHA2_512_REVERSE64(*data++, W[j]);                                                                          \
        SHA2_512_ROUND_0_TO_15(a, b, c, d, e, f, g, h);                                                             \
    } while (0)

#define SHA2_256_ROUND(a, b, c, d, e, f, g, h)                                                                      \
    do {                                                                                                            \
        uint32_t s0 = W[(j + 0x01U) & 0x0FU];                                                                       \
        uint32_t s1 = W[(j + 0x0EU) & 0x0FU];                                                                       \
        s0 = SHA2_256_sigma0(s0);                                                                                   \
        s1 = SHA2_256_sigma1(s1);                                                                                   \
        W[j & 0x0FU] += s1 + W[(j + 0x09U) & 0x0FU] + s0;                                                           \
        uint32_t t1 = s[h] + SHA2_256_Sigma1(s[e]) + SHA2_Ch(s[e], s[f], s[g]) + K[j] + W[j & 0x0FU];               \
        s[h] = t1 + SHA2_256_Sigma0(s[a]) + SHA2_Maj(s[a], s[b], s[c]);                                             \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

#define SHA2_512_ROUND(a, b, c, d, e, f, g, h)                                                                      \
    do {                                                                                                            \
        uint64_t s0 = W[(j + 0x01U) & 0x0FU];                                                                       \
        uint64_t s1 = W[(j + 0x0EU) & 0x0FU];                                                                       \
        s0 = SHA2_512_sigma0(s0);                                                                                   \
        s1 = SHA2_512_sigma1(s1);                                                                                   \
        W[j & 0x0FU] += s1 + W[(j + 0x09U) & 0x0FU] + s0;                                                           \
        uint64_t t1 = s[h] + SHA2_512_Sigma1(s[e]) + SHA2_Ch(s[e], s[f], s[g]) + K[j] + W[j & 0x0FU];               \
        s[h] = t1 + SHA2_512_Sigma0(s[a]) + SHA2_Maj(s[a], s[b], s[c]);                                             \
        s[d] += t1;                                                                                                 \
        j++;                                                                                                        \
    } while (0)

static inline bool
digest_is_big_endian_sha2(void)
{
	return (bool) (htonl(UINT32_C(0x11223344)) == UINT32_C(0x11223344));
}

static void
digest_transform_block_sha2_256(struct digest_context_sha2_256 *const ctx, const uint32_t *data)
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

	if (digest_is_big_endian_sha2())
	{
		do {
			SHA2_256_ROUND_0_TO_15_BE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			SHA2_256_ROUND_0_TO_15_BE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			SHA2_256_ROUND_0_TO_15_BE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			SHA2_256_ROUND_0_TO_15_BE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			SHA2_256_ROUND_0_TO_15_BE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			SHA2_256_ROUND_0_TO_15_BE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			SHA2_256_ROUND_0_TO_15_BE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			SHA2_256_ROUND_0_TO_15_BE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}
	else
	{
		do {
			SHA2_256_ROUND_0_TO_15_LE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			SHA2_256_ROUND_0_TO_15_LE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			SHA2_256_ROUND_0_TO_15_LE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			SHA2_256_ROUND_0_TO_15_LE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			SHA2_256_ROUND_0_TO_15_LE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			SHA2_256_ROUND_0_TO_15_LE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			SHA2_256_ROUND_0_TO_15_LE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			SHA2_256_ROUND_0_TO_15_LE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}

	do {
		SHA2_256_ROUND(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
		SHA2_256_ROUND(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
		SHA2_256_ROUND(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
		SHA2_256_ROUND(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
		SHA2_256_ROUND(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
		SHA2_256_ROUND(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
		SHA2_256_ROUND(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
		SHA2_256_ROUND(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

	} while (j < 0x40U);

	for (size_t x = 0x00U; x < DIGEST_STLEN_SHA2; x++)
		ctx->state[x] += s[x];

	(void) smemzero(s, sizeof s);
}

static void
digest_transform_block_sha2_512(struct digest_context_sha2_512 *const ctx, const uint64_t *data)
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

	if (digest_is_big_endian_sha2())
	{
		do {
			SHA2_512_ROUND_0_TO_15_BE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			SHA2_512_ROUND_0_TO_15_BE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			SHA2_512_ROUND_0_TO_15_BE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			SHA2_512_ROUND_0_TO_15_BE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			SHA2_512_ROUND_0_TO_15_BE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			SHA2_512_ROUND_0_TO_15_BE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			SHA2_512_ROUND_0_TO_15_BE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			SHA2_512_ROUND_0_TO_15_BE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}
	else
	{
		do {
			SHA2_512_ROUND_0_TO_15_LE(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
			SHA2_512_ROUND_0_TO_15_LE(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
			SHA2_512_ROUND_0_TO_15_LE(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
			SHA2_512_ROUND_0_TO_15_LE(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
			SHA2_512_ROUND_0_TO_15_LE(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
			SHA2_512_ROUND_0_TO_15_LE(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
			SHA2_512_ROUND_0_TO_15_LE(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
			SHA2_512_ROUND_0_TO_15_LE(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

		} while (j < 0x10U);
	}

	do {
		SHA2_512_ROUND(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U);
		SHA2_512_ROUND(0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U);
		SHA2_512_ROUND(0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
		SHA2_512_ROUND(0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U);
		SHA2_512_ROUND(0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U, 0x03U);
		SHA2_512_ROUND(0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U, 0x02U);
		SHA2_512_ROUND(0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U, 0x01U);
		SHA2_512_ROUND(0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x00U);

	} while (j < 0x50U);

	for (size_t x = 0x00U; x < DIGEST_STLEN_SHA2; x++)
		ctx->state[x] += s[x];

	(void) smemzero(s, sizeof s);
}

static bool ATHEME_FATTR_WUR
digest_init_sha2_256(union digest_state *const restrict state)
{
	struct digest_context_sha2_256 *const ctx = (struct digest_context_sha2_256 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
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

static bool ATHEME_FATTR_WUR
digest_init_sha2_512(union digest_state *const restrict state)
{
	struct digest_context_sha2_512 *const ctx = (struct digest_context_sha2_512 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
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

static bool ATHEME_FATTR_WUR
digest_update_sha2_256(union digest_state *const restrict state,
                       const void *const restrict in, const size_t len)
{
	struct digest_context_sha2_256 *const ctx = (struct digest_context_sha2_256 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	if (! (in && len))
		return true;

	const unsigned char *ptr = in;
	size_t rem = len;

	const uint64_t usedspace = (uint64_t) ((ctx->count >> 0x03U) % DIGEST_BKLEN_SHA2_256);

	if (usedspace)
	{
		const uint64_t freespace = (DIGEST_BKLEN_SHA2_256 - usedspace);

		if (rem >= freespace)
		{
			(void) memcpy(ctx->buf + usedspace, ptr, (size_t) freespace);
			(void) digest_transform_block_sha2_256(ctx, (const void *) ctx->buf);

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
		(void) digest_transform_block_sha2_256(ctx, (const void *) ptr);

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

static bool ATHEME_FATTR_WUR
digest_update_sha2_512(union digest_state *const restrict state,
                       const void *const restrict in, const size_t len)
{
	struct digest_context_sha2_512 *const ctx = (struct digest_context_sha2_512 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	if (! (in && len))
		return true;

	const unsigned char *ptr = in;
	size_t rem = len;

	const uint64_t usedspace = (uint64_t) ((ctx->count[0] >> 0x03U) % DIGEST_BKLEN_SHA2_512);

	if (usedspace)
	{
		const uint64_t freespace = (DIGEST_BKLEN_SHA2_512 - usedspace);

		if (rem >= freespace)
		{
			(void) memcpy(ctx->buf + usedspace, ptr, (size_t) freespace);
			(void) digest_transform_block_sha2_512(ctx, (const void *) ctx->buf);

			SHA2_512_ADDINC128(ctx->count, (freespace << 0x03U));

			ptr += freespace;
			rem -= freespace;
		}
		else
		{
			(void) memcpy(ctx->buf + usedspace, ptr, rem);

			SHA2_512_ADDINC128(ctx->count, (rem << 0x03U));

			return true;
		}
	}

	while (rem >= DIGEST_BKLEN_SHA2_512)
	{
		(void) digest_transform_block_sha2_512(ctx, (const void *) ptr);

		SHA2_512_ADDINC128(ctx->count, (DIGEST_BKLEN_SHA2_512 << 0x03U));

		ptr += DIGEST_BKLEN_SHA2_512;
		rem -= DIGEST_BKLEN_SHA2_512;
	}

	if (rem)
	{
		(void) memcpy(ctx->buf, ptr, rem);

		SHA2_512_ADDINC128(ctx->count, (rem << 0x03U));
	}

	return true;
}

static bool ATHEME_FATTR_WUR
digest_final_sha2_256(union digest_state *const restrict state, void *const restrict out)
{
	struct digest_context_sha2_256 *const ctx = (struct digest_context_sha2_256 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	uint64_t usedspace = (ctx->count >> 0x03U) % DIGEST_BKLEN_SHA2_256;

	if (! digest_is_big_endian_sha2())
		SHA2_256_REVERSE64(ctx->count, ctx->count);

	if (usedspace)
	{
		ctx->buf[usedspace++] = 0x80U;

		if (usedspace <= DIGEST_SHORT_BKLEN_SHA2_256)
		{
			(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_SHORT_BKLEN_SHA2_256 - usedspace));
		}
		else
		{
			if (usedspace < DIGEST_BKLEN_SHA2_256)
			{
				(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_BKLEN_SHA2_256 - usedspace));
			}

			(void) digest_transform_block_sha2_256(ctx, (const void *) ctx->buf);
			(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN_SHA2_256);
		}
	}
	else
	{
		(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN_SHA2_256);

		ctx->buf[0x00U] = 0x80U;
	}

	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN_SHA2_256]) = ctx->count;

	(void) digest_transform_block_sha2_256(ctx, (const void *) ctx->buf);

	uint32_t *d = (uint32_t *) out;

	if (digest_is_big_endian_sha2())
		(void) memcpy(d, ctx->state, sizeof ctx->state);
	else for (size_t i = 0x00U; i < DIGEST_STLEN_SHA2; i++)
		SHA2_256_REVERSE32(ctx->state[i], *d++);

	(void) smemzero(ctx, sizeof *ctx);

	return true;
}

static bool ATHEME_FATTR_WUR
digest_final_sha2_512(union digest_state *const restrict state, void *const restrict out)
{
	struct digest_context_sha2_512 *const ctx = (struct digest_context_sha2_512 *) state;

	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	uint64_t usedspace = ((ctx->count[0x00U] >> 0x03U) % DIGEST_BKLEN_SHA2_512);

	if (! digest_is_big_endian_sha2())
	{
		SHA2_512_REVERSE64(ctx->count[0x00U], ctx->count[0x00U]);
		SHA2_512_REVERSE64(ctx->count[0x01U], ctx->count[0x01U]);
	}

	if (usedspace)
	{
		ctx->buf[usedspace++] = 0x80U;

		if (usedspace <= DIGEST_SHORT_BKLEN_SHA2_512)
		{
			(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_SHORT_BKLEN_SHA2_512 - usedspace));
		}
		else
		{
			if (usedspace < DIGEST_BKLEN_SHA2_512)
			{
				(void) memset(ctx->buf + usedspace, 0x00U, (DIGEST_BKLEN_SHA2_512 - usedspace));
			}

			(void) digest_transform_block_sha2_512(ctx, (const void *) ctx->buf);
			(void) memset(ctx->buf, 0x00U, (DIGEST_BKLEN_SHA2_512 - 0x02U));
		}
	}
	else
	{
		(void) memset(ctx->buf, 0x00U, DIGEST_SHORT_BKLEN_SHA2_512);

		ctx->buf[0x00U] = 0x80U;
	}

	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN_SHA2_512]) = ctx->count[0x01U];
	*((uint64_t *) &ctx->buf[DIGEST_SHORT_BKLEN_SHA2_512 + 0x08U]) = ctx->count[0x00U];

	(void) digest_transform_block_sha2_512(ctx, (const void *) ctx->buf);

	uint64_t *d = (uint64_t *) out;

	if (digest_is_big_endian_sha2())
		(void) memcpy(d, ctx->state, sizeof ctx->state);
	else for (size_t i = 0x00U; i < DIGEST_STLEN_SHA2; i++)
		SHA2_512_REVERSE64(ctx->state[i], *d++);

	(void) smemzero(ctx, sizeof *ctx);

	return true;
}
