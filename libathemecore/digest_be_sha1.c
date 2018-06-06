/*
 * SPDX-License-Identifier: CC0-1.0
 * SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
 *
 * Originally witten by Steve Reid <steve@edmweb.com>
 *
 * Modified by Aaron D. Gifford <agifford@infowest.com>
 *
 * NO COPYRIGHT - THIS IS 100% IN THE PUBLIC DOMAIN
 *
 * The original unmodified version is available at:
 *    ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
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
 * SHA1 backend for Atheme IRC Services.
 */

#include "atheme.h"
#include "digest_be_sha1.h"

#define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (0x20U - (bits))))

#define SHA1_BLK0_BE(i) block->l[i]

#define SHA1_BLK0_LE(i) \
    (block->l[i] = (SHA1_ROL(block->l[i], 0x18U) & UINT32_C(0xFF00FF00)) | \
     (SHA1_ROL(block->l[i], 0x08U) & UINT32_C(0x00FF00FF)))

#define SHA1_BLK(i) \
	(block->l[i & 0x0FU] = SHA1_ROL(block->l[(i + 0x0DU) & 0x0FU] ^ block->l[(i + 0x08U) & 0x0FU] ^ \
	 block->l[(i + 0x02U) & 0x0FU] ^ block->l[i & 0x0FU], 0x01U))

#define SHA1_ROUND0(v, w, x, y, z, i)                                                                                  \
    do {                                                                                                               \
        if (digest_is_big_endian)                                                                                      \
            s[z] += ((s[w] & (s[x] ^ s[y])) ^ s[y]) + SHA1_BLK0_BE(i) + UINT32_C(0x5A827999) + SHA1_ROL(s[v], 0x05U);  \
        else                                                                                                           \
            s[z] += ((s[w] & (s[x] ^ s[y])) ^ s[y]) + SHA1_BLK0_LE(i) + UINT32_C(0x5A827999) + SHA1_ROL(s[v], 0x05U);  \
                                                                                                                       \
        s[w] = SHA1_ROL(s[w], 0x1EU);                                                                                  \
    } while (0)

#define SHA1_ROUND1(v, w, x, y, z, i)                                                                                  \
    do {                                                                                                               \
        s[z] += ((s[w] & (s[x] ^ s[y])) ^ s[y]) + SHA1_BLK(i) + UINT32_C(0x5A827999) + SHA1_ROL(s[v], 0x05U);          \
        s[w] = SHA1_ROL(s[w], 0x1EU);                                                                                  \
    } while (0)

#define SHA1_ROUND2(v, w, x, y, z, i)                                                                                  \
    do {                                                                                                               \
        s[z] += (s[w] ^ s[x] ^ s[y]) + SHA1_BLK(i) + UINT32_C(0x6ED9EBA1) + SHA1_ROL(s[v], 0x05U);                     \
        s[w] = SHA1_ROL(s[w], 0x1EU);                                                                                  \
    } while (0)

#define SHA1_ROUND3(v, w, x, y, z, i)                                                                                  \
    do {                                                                                                               \
        s[z] += (((s[w] | s[x]) & s[y]) | (s[w] & s[x])) + SHA1_BLK(i) + UINT32_C(0x8F1BBCDC) + SHA1_ROL(s[v], 0x05U); \
        s[w] = SHA1_ROL(s[w], 0x1EU);                                                                                  \
    } while (0)

#define SHA1_ROUND4(v, w, x, y, z, i)                                                                                  \
    do {                                                                                                               \
        s[z] += (s[w] ^ s[x] ^ s[y]) + SHA1_BLK(i) + UINT32_C(0xCA62C1D6) + SHA1_ROL(s[v], 0x05U);                     \
        s[w] = SHA1_ROL(s[w], 0x1EU);                                                                                  \
    } while (0)

union B64Q16
{
	uint8_t         c[0x40U];
	uint32_t        l[0x10U];
};

static void
transform_block_sha1(struct digest_context_sha1 *const restrict ctx, const uint8_t *const restrict in)
{
	const bool digest_is_big_endian = (htonl(UINT32_C(0x11223344)) == UINT32_C(0x11223344));

	union B64Q16 tmp;
	union B64Q16 *const block = &tmp;

	uint32_t s[0x05U];

	(void) memcpy(block, in, sizeof *block);
	(void) memcpy(s, ctx->state, sizeof s);

	SHA1_ROUND0(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x00U);
	SHA1_ROUND0(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x01U);
	SHA1_ROUND0(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x02U);
	SHA1_ROUND0(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x03U);
	SHA1_ROUND0(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x04U);
	SHA1_ROUND0(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U);
	SHA1_ROUND0(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x06U);
	SHA1_ROUND0(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x07U);
	SHA1_ROUND0(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x08U);
	SHA1_ROUND0(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x09U);
	SHA1_ROUND0(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x0AU);
	SHA1_ROUND0(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x0BU);
	SHA1_ROUND0(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x0CU);
	SHA1_ROUND0(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x0DU);
	SHA1_ROUND0(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x0EU);
	SHA1_ROUND0(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x0FU);
	SHA1_ROUND1(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x10U);
	SHA1_ROUND1(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x11U);
	SHA1_ROUND1(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x12U);
	SHA1_ROUND1(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x13U);
	SHA1_ROUND2(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x14U);
	SHA1_ROUND2(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x15U);
	SHA1_ROUND2(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x16U);
	SHA1_ROUND2(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x17U);
	SHA1_ROUND2(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x18U);
	SHA1_ROUND2(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x19U);
	SHA1_ROUND2(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x1AU);
	SHA1_ROUND2(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x1BU);
	SHA1_ROUND2(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x1CU);
	SHA1_ROUND2(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x1DU);
	SHA1_ROUND2(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x1EU);
	SHA1_ROUND2(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x1FU);
	SHA1_ROUND2(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x20U);
	SHA1_ROUND2(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x21U);
	SHA1_ROUND2(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x22U);
	SHA1_ROUND2(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x23U);
	SHA1_ROUND2(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x24U);
	SHA1_ROUND2(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x25U);
	SHA1_ROUND2(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x26U);
	SHA1_ROUND2(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x27U);
	SHA1_ROUND3(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x28U);
	SHA1_ROUND3(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x29U);
	SHA1_ROUND3(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x2AU);
	SHA1_ROUND3(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x2BU);
	SHA1_ROUND3(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x2CU);
	SHA1_ROUND3(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x2DU);
	SHA1_ROUND3(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x2EU);
	SHA1_ROUND3(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x2FU);
	SHA1_ROUND3(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x30U);
	SHA1_ROUND3(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x31U);
	SHA1_ROUND3(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x32U);
	SHA1_ROUND3(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x33U);
	SHA1_ROUND3(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x34U);
	SHA1_ROUND3(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x35U);
	SHA1_ROUND3(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x36U);
	SHA1_ROUND3(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x37U);
	SHA1_ROUND3(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x38U);
	SHA1_ROUND3(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x39U);
	SHA1_ROUND3(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x3AU);
	SHA1_ROUND3(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x3BU);
	SHA1_ROUND4(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x3CU);
	SHA1_ROUND4(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x3DU);
	SHA1_ROUND4(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x3EU);
	SHA1_ROUND4(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x3FU);
	SHA1_ROUND4(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x40U);
	SHA1_ROUND4(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x41U);
	SHA1_ROUND4(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x42U);
	SHA1_ROUND4(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x43U);
	SHA1_ROUND4(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x44U);
	SHA1_ROUND4(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x45U);
	SHA1_ROUND4(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x46U);
	SHA1_ROUND4(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x47U);
	SHA1_ROUND4(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x48U);
	SHA1_ROUND4(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x49U);
	SHA1_ROUND4(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x4AU);
	SHA1_ROUND4(0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x4BU);
	SHA1_ROUND4(0x04U, 0x00U, 0x01U, 0x02U, 0x03U, 0x4CU);
	SHA1_ROUND4(0x03U, 0x04U, 0x00U, 0x01U, 0x02U, 0x4DU);
	SHA1_ROUND4(0x02U, 0x03U, 0x04U, 0x00U, 0x01U, 0x4EU);
	SHA1_ROUND4(0x01U, 0x02U, 0x03U, 0x04U, 0x00U, 0x4FU);

	for (size_t i = 0x00U; i < 0x05U; i++)
		ctx->state[i] += s[i];

	(void) explicit_bzero(block, sizeof *block);
	(void) explicit_bzero(s, sizeof s);
}

bool
digest_init_sha1(struct digest_context_sha1 *const restrict ctx)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	static const uint32_t iv[0x05U] = {

		UINT32_C(0x67452301), UINT32_C(0xEFCDAB89), UINT32_C(0x98BADCFE), UINT32_C(0x10325476),
		UINT32_C(0xC3D2E1F0),
	};

	(void) memset(ctx, 0x00U, sizeof *ctx);
	(void) memcpy(ctx->state, iv, sizeof iv);

	return true;
}

bool
digest_update_sha1(struct digest_context_sha1 *const restrict ctx, const void *const restrict data, size_t len)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	if (! (data && len))
		return true;

	const uint8_t *const ptr = data;

	uint32_t i = 0x00U;
	uint32_t j = (ctx->count[0x00U] >> 0x03U) & 0x3FU;

	ctx->count[1] += (len >> 0x1DU);

	if ((ctx->count[0x00U] += (len << 0x03U)) < (len << 0x03U))
		ctx->count[0x01U]++;

	if ((j + len) >= 0x40U)
	{
		i = 0x40U - j;

		(void) memcpy(ctx->buf + j, ptr, i);
		(void) transform_block_sha1(ctx, ctx->buf);

		for ( ; (i + 0x3FU) < len; i += 0x40U)
			(void) transform_block_sha1(ctx, ptr + i);

		j = 0x00U;
	}

	(void) memcpy(ctx->buf + j, ptr + i, len - i);
	return true;
}

bool
digest_final_sha1(struct digest_context_sha1 *const restrict ctx, void *const restrict out, size_t *const restrict len)
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
		if (*len < DIGEST_MDLEN_SHA1)
		{
			(void) slog(LG_ERROR, "%s: output buffer length %zu is too small", __func__, *len);
			return false;
		}

		*len = DIGEST_MDLEN_SHA1;
	}

	static const uint8_t sep = 0x80U;
	static const uint8_t pad = 0x00U;

	uint8_t	data[0x08U];

	for (uint32_t i = 0x00U; i < 0x04U; i++)
		data[i] = (uint8_t) ((ctx->count[0x01U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	for (uint32_t i = 0x04U; i < 0x08U; i++)
		data[i] = (uint8_t) ((ctx->count[0x00U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	(void) digest_update_sha1(ctx, &sep, sizeof sep);

	while ((ctx->count[0] & 0x1F8U) != 0x1C0U)
		(void) digest_update_sha1(ctx, &pad, sizeof pad);

	(void) digest_update_sha1(ctx, data, sizeof data);

	uint8_t *const digest = out;

	for (uint32_t i = 0x00U; i < DIGEST_MDLEN_SHA1; i++)
		digest[i] = (uint8_t) ((ctx->state[i >> 0x02U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	(void) explicit_bzero(ctx, sizeof *ctx);
	return true;
}
