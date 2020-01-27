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

#include <atheme/digest/direct.h>       // self-declarations
#include <atheme/memory.h>              // smemzero()
#include <atheme/stdheaders.h>          // size_t, uint32_t, htonl(3), memcpy(3), memset(3)

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

union SHA1_B64Q16
{
	uint8_t         c[0x40U];
	uint32_t        l[0x10U];
};

static void
digest_transform_block_sha1(union digest_direct_ctx *const restrict state,
                            const unsigned char *const restrict in)
{
	const bool digest_is_big_endian = (htonl(UINT32_C(0x11223344)) == UINT32_C(0x11223344));

	union SHA1_B64Q16 tmp;
	union SHA1_B64Q16 *const block = &tmp;

	uint32_t s[0x05U];

	(void) memcpy(block, in, sizeof *block);
	(void) memcpy(s, state->sha1.state, sizeof s);

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
		state->sha1.state[i] += s[i];

	(void) smemzero(block, sizeof *block);
	(void) smemzero(s, sizeof s);
}

void
digest_direct_init_sha1(union digest_direct_ctx *const restrict state)
{
	static const uint32_t iv[DIGEST_IVLEN_SHA1] = {

		UINT32_C(0x67452301), UINT32_C(0xEFCDAB89), UINT32_C(0x98BADCFE), UINT32_C(0x10325476),
		UINT32_C(0xC3D2E1F0),
	};

	(void) memset(state, 0x00U, sizeof *state);
	(void) memcpy(state->sha1.state, iv, sizeof iv);
}

void
digest_direct_update_sha1(union digest_direct_ctx *const restrict state, const void *const restrict data, size_t len)
{
	if (! (data && len))
		return;

	const unsigned char *const ptr = data;

	uint32_t i = 0x00U;
	uint32_t j = (state->sha1.count[0x00U] >> 0x03U) & 0x3FU;

	state->sha1.count[1] += (len >> 0x1DU);

	if ((state->sha1.count[0x00U] += (len << 0x03U)) < (len << 0x03U))
		state->sha1.count[0x01U]++;

	if ((j + len) >= 0x40U)
	{
		i = 0x40U - j;

		(void) memcpy(state->sha1.buf + j, ptr, i);
		(void) digest_transform_block_sha1(state, state->sha1.buf);

		for ( ; (i + 0x3FU) < len; i += 0x40U)
			(void) digest_transform_block_sha1(state, ptr + i);

		j = 0x00U;
	}

	(void) memcpy(state->sha1.buf + j, ptr + i, len - i);
}

void
digest_direct_final_sha1(union digest_direct_ctx *const restrict state, void *const restrict out)
{
	static const unsigned char sep = 0x80U;
	static const unsigned char pad = 0x00U;

	unsigned char data[0x08U];

	for (uint32_t i = 0x00U; i < 0x04U; i++)
		data[i] = (unsigned char) ((state->sha1.count[0x01U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	for (uint32_t i = 0x04U; i < 0x08U; i++)
		data[i] = (unsigned char) ((state->sha1.count[0x00U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	(void) digest_direct_update_sha1(state, &sep, sizeof sep);

	while ((state->sha1.count[0] & 0x1F8U) != 0x1C0U)
		(void) digest_direct_update_sha1(state, &pad, sizeof pad);

	(void) digest_direct_update_sha1(state, data, sizeof data);

	unsigned char *const digest = out;

	for (uint32_t i = 0x00U; i < DIGEST_MDLEN_SHA1; i++)
		digest[i] = (unsigned char) ((state->sha1.state[i >> 0x02U] >> ((0x03U - (i & 0x03U)) * 0x08U)) & 0xFFU);

	(void) smemzero(data, sizeof data);
	(void) smemzero(state, sizeof *state);
}
