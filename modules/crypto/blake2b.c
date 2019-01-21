/*
 * SPDX-License-Identifier: (Apache-2.0 OR CC0-1.0 OR OpenSSL)
 * SPDX-URL: https://spdx.org/licenses/Apache-2.0.html
 * SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
 * SPDX-URL: https://spdx.org/licenses/OpenSSL.html
 *
 * Copyright (C) 2012 Samuel Neves <sneves@dei.uc.pt>
 * Copyright (C) 2017-2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * BLAKE2 reference C implementation (modified for Atheme IRC Services)
 */

#include "atheme.h"
#include "blake2b.h"
#include "blake2b-testvecs.h"

struct blake2b_param
{
	uint8_t         hash_len;
	uint8_t         key_len;
	uint8_t         fanout;
	uint8_t         depth;
	uint32_t        leaf_len;
	uint64_t        node_off;
	uint8_t         node_depth;
	uint8_t         inner_len;
	uint8_t         reserved[0x0E];
	uint8_t         salt[BLAKE2B_SALTLEN];
	uint8_t         personal[BLAKE2B_PERSLEN];
} ATHEME_SATTR_PACKED;

enum BLAKE2B_SZCHK_STATIC_ASSERT
{
	// Ensure `struct blake2b_param' has been properly packed & padded (a poor man's `static_assert')
	// This will generate a compile-time division-by-zero error if this is not the case
	// This is critical to the correct functioning of blake2b_init() below
	// INTEROPERABILITY WITH OTHER BLAKE2B IMPLMEMENTATIONS MAY BE JEOPARDISED IF THIS IS REMOVED
	BLAKE2B_SZCHK_0 = (0x01 / !!(CHAR_BIT == 0x08)),
	BLAKE2B_SZCHK_1 = (0x01 / !!(sizeof(struct blake2b_param) == (sizeof(uint64_t) * CHAR_BIT)))
};

static const uint64_t blake2b_iv[0x08] = {

	UINT64_C(0x6A09E667F3BCC908), UINT64_C(0xBB67AE8584CAA73B), UINT64_C(0x3C6EF372FE94F82B),
	UINT64_C(0xA54FF53A5F1D36F1), UINT64_C(0x510E527FADE682D1), UINT64_C(0x9B05688C2B3E6C1F),
	UINT64_C(0x1F83D9ABFB41BD6B), UINT64_C(0x5BE0CD19137E2179)
};

static const uint64_t blake2b_sigma[0x0C][0x10] = {

	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
	{ 0x0E, 0x0A, 0x04, 0x08, 0x09, 0x0F, 0x0D, 0x06, 0x01, 0x0C, 0x00, 0x02, 0x0B, 0x07, 0x05, 0x03 },
	{ 0x0B, 0x08, 0x0C, 0x00, 0x05, 0x02, 0x0F, 0x0D, 0x0A, 0x0E, 0x03, 0x06, 0x07, 0x01, 0x09, 0x04 },
	{ 0x07, 0x09, 0x03, 0x01, 0x0D, 0x0C, 0x0B, 0x0E, 0x02, 0x06, 0x05, 0x0A, 0x04, 0x00, 0x0F, 0x08 },
	{ 0x09, 0x00, 0x05, 0x07, 0x02, 0x04, 0x0A, 0x0F, 0x0E, 0x01, 0x0B, 0x0C, 0x06, 0x08, 0x03, 0x0D },
	{ 0x02, 0x0C, 0x06, 0x0A, 0x00, 0x0B, 0x08, 0x03, 0x04, 0x0D, 0x07, 0x05, 0x0F, 0x0E, 0x01, 0x09 },
	{ 0x0C, 0x05, 0x01, 0x0F, 0x0E, 0x0D, 0x04, 0x0A, 0x00, 0x07, 0x06, 0x03, 0x09, 0x02, 0x08, 0x0B },
	{ 0x0D, 0x0B, 0x07, 0x0E, 0x0C, 0x01, 0x03, 0x09, 0x05, 0x00, 0x0F, 0x04, 0x08, 0x06, 0x02, 0x0A },
	{ 0x06, 0x0F, 0x0E, 0x09, 0x0B, 0x03, 0x00, 0x08, 0x0C, 0x02, 0x0D, 0x07, 0x01, 0x04, 0x0A, 0x05 },
	{ 0x0A, 0x02, 0x08, 0x04, 0x07, 0x06, 0x01, 0x05, 0x0F, 0x0B, 0x09, 0x0E, 0x03, 0x0C, 0x0D, 0x00 },
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
	{ 0x0E, 0x0A, 0x04, 0x08, 0x09, 0x0F, 0x0D, 0x06, 0x01, 0x0C, 0x00, 0x02, 0x0B, 0x07, 0x05, 0x03 }
};

static inline void
blake2b_store32(uint8_t *const restrict dst, const uint32_t w)
{
#if defined(NATIVE_LITTLE_ENDIAN)
	(void) memcpy(dst, &w, sizeof w);
#else
	for (size_t x = 0x00; x < 0x04; x++)
		dst[x] = (uint8_t)(w >> (x * 0x08));
#endif
}

static inline void
blake2b_store64(uint8_t *const restrict dst, const uint64_t w)
{
#if defined(NATIVE_LITTLE_ENDIAN)
	(void) memcpy(dst, &w, sizeof w);
#else
	for (size_t x = 0x00; x < 0x08; x++)
		dst[x] = (uint8_t)(w >> (x * 0x08));
#endif
}

static inline uint64_t
blake2b_load64(const uint8_t *const restrict src)
{
	uint64_t w = 0;
#if defined(NATIVE_LITTLE_ENDIAN)
	(void) memcpy(&w, src, sizeof w);
#else
	for (size_t x = 0x00; x < 0x08; x++)
		w |= (((uint64_t) src[x]) << (x * 0x08));
#endif
	return w;
}

static inline uint64_t
blake2b_rotr64(const uint64_t w, const unsigned c)
{
	return ((w >> c) | (w << (0x40 - c)));
}

static inline uint64_t
blake2b_fBlaMka(const uint64_t x, const uint64_t y)
{
	const uint64_t m = UINT64_C(0xFFFFFFFF);
	const uint64_t v = ((x & m) * (y & m));
	return (x + y + (v * 0x02));
}

static void
blake2b_compress(struct blake2b_state *const restrict state, const uint8_t *const restrict block)
{
	uint64_t m[0x10];
	uint64_t v[0x10];

	for (size_t x = 0x00; x < 0x10; x++)
		m[x] = blake2b_load64(block + (x * sizeof m[x]));

	for (size_t x = 0x00; x < 0x08; x++)
	{
		v[x] = state->h[x];
		v[x + 0x08] = blake2b_iv[x];
	}
	for (size_t x = 0x00; x < 0x02; x++)
	{
		v[x + 0x0C] ^= state->t[x];
		v[x + 0x0E] ^= state->f[x];
	}

#define G(r, i, a, b, c, d) do {                                      \
	    (a) = (a) + (b) + m[blake2b_sigma[r][(i * 0x02) + 0x00]]; \
	    (d) = blake2b_rotr64(((d) ^ (a)), 0x20); (c) += (d);      \
	    (b) = blake2b_rotr64(((b) ^ (c)), 0x18);                  \
	    (a) = (a) + (b) + m[blake2b_sigma[r][(i * 0x02) + 0x01]]; \
	    (d) = blake2b_rotr64(((d) ^ (a)), 0x10); (c) += (d);      \
	    (b) = blake2b_rotr64(((b) ^ (c)), 0x3F);                  \
	} while (0)

	for (size_t x = 0x00; x < 0x0C; x++)
	{
		G(x, 0x00, v[0x00], v[0x04], v[0x08], v[0x0C]);
		G(x, 0x01, v[0x01], v[0x05], v[0x09], v[0x0D]);
		G(x, 0x02, v[0x02], v[0x06], v[0x0A], v[0x0E]);
		G(x, 0x03, v[0x03], v[0x07], v[0x0B], v[0x0F]);
		G(x, 0x04, v[0x00], v[0x05], v[0x0A], v[0x0F]);
		G(x, 0x05, v[0x01], v[0x06], v[0x0B], v[0x0C]);
		G(x, 0x06, v[0x02], v[0x07], v[0x08], v[0x0D]);
		G(x, 0x07, v[0x03], v[0x04], v[0x09], v[0x0E]);
	}

#undef G

	for (size_t x = 0x00; x < 0x08; x++)
		state->h[x] ^= (v[x] ^ v[x + 0x08]);
}

static inline void
blake2b_counter_inc(struct blake2b_state *const restrict state, const uint64_t inc)
{
	state->t[0x00] += inc;
	state->t[0x01] += (state->t[0x00] < inc);
}

static inline void
blake2b_set_lastblock(struct blake2b_state *const restrict state)
{
	state->f[0x00] = (uint64_t) -1;

	if (state->last_node)
		state->f[0x01] = (uint64_t) -1;
}

static bool ATHEME_FATTR_WUR
blake2b_update(struct blake2b_state *const restrict state, const uint8_t *restrict in, size_t inlen)
{
	if (!inlen)
		return true;

	if (state->f[0x00] != 0x00)
		return false;

	if ((state->buflen + inlen) > BLAKE2B_BLOCKLEN)
	{
		const size_t left = state->buflen;
		const size_t fill = BLAKE2B_BLOCKLEN - left;

		(void) memcpy(&state->buf[left], in, fill);
		(void) blake2b_counter_inc(state, BLAKE2B_BLOCKLEN);
		(void) blake2b_compress(state, state->buf);

		in += fill;
		inlen -= fill;
		state->buflen = 0x00;

		while (inlen > BLAKE2B_BLOCKLEN)
		{
			(void) blake2b_counter_inc(state, BLAKE2B_BLOCKLEN);
			(void) blake2b_compress(state, in);

			in += BLAKE2B_BLOCKLEN;
			inlen -= BLAKE2B_BLOCKLEN;
		}
	}

	(void) memcpy(&state->buf[state->buflen], in, inlen);
	state->buflen += inlen;
	return true;
}

static bool ATHEME_FATTR_WUR
blake2b_init(struct blake2b_state *const restrict state, const size_t outlen,
             const void *const restrict key, const size_t keylen)
{
	if ((! outlen) || (outlen > BLAKE2B_HASHLEN))
		return false;

	if ((! key && keylen) || (key && ! keylen) || (keylen > BLAKE2B_BLOCKLEN))
		return false;

	struct blake2b_param params;
	const uint8_t *const params_byte = (const uint8_t *) &params;

	(void) memset(&params, 0x00, sizeof params);
	(void) memset(state, 0x00, sizeof *state);

	params.hash_len = (uint8_t) outlen;
	params.key_len = (uint8_t) keylen;
	params.fanout = 0x01;
	params.depth = 0x01;

	(void) memcpy(state->h, blake2b_iv, sizeof blake2b_iv);

	for (size_t x = 0x00; x < 0x08; x++)
		state->h[x] ^= blake2b_load64(&params_byte[(x * sizeof state->h[x])]);

	state->outlen = outlen;

	if (keylen)
	{
		static uint8_t block[BLAKE2B_BLOCKLEN];

		(void) memset(block, 0x00, sizeof block);
		(void) memcpy(block, key, keylen);

		const bool result = blake2b_update(state, block, sizeof block);

		(void) smemzero(block, sizeof block);

		return result;
	}

	return true;
}

static bool ATHEME_FATTR_WUR
blake2b_final(struct blake2b_state *const restrict state, uint8_t *const restrict out)
{
	if (state->f[0x00] != 0x00)
		return false;

	(void) blake2b_counter_inc(state, state->buflen);
	(void) blake2b_set_lastblock(state);
	(void) memset(&state->buf[state->buflen], 0x00, (BLAKE2B_BLOCKLEN - state->buflen));
	(void) blake2b_compress(state, state->buf);

	uint8_t buffer[BLAKE2B_HASHLEN];
	(void) memset(buffer, 0x00, sizeof buffer);

	for (size_t x = 0x00; x < 0x08; x++)
		(void) blake2b_store64(&buffer[(x * sizeof state->h[x])], state->h[x]);

	(void) memcpy(out, buffer, state->outlen);
	(void) smemzero(buffer, sizeof buffer);
	(void) smemzero(state, sizeof *state);

	return true;
}

static bool ATHEME_FATTR_WUR
blake2b_full(const uint8_t *const restrict in, const size_t inlen,
             const void *const restrict key, const size_t keylen,
             uint8_t *const restrict out, const size_t outlen)
{
	struct blake2b_state state;

	if (!blake2b_init(&state, outlen, key, keylen))
		return false;

	if (!blake2b_update(&state, in, inlen))
		return false;

	return blake2b_final(&state, out);
}

static bool ATHEME_FATTR_WUR
blake2b_long(const uint8_t *const restrict in, const size_t inlen,
             const void *const restrict key, const size_t keylen,
             uint8_t *restrict out, const size_t outlen)
{
	uint8_t outlen_buf[4] = { 0x00, 0x00, 0x00, 0x00 };
	struct blake2b_state blake_state;

	if ((sizeof(outlen) > sizeof(uint32_t)) && (outlen > UINT32_MAX))
		return false;

	(void) blake2b_store32(outlen_buf, (uint32_t) outlen);

	if (outlen <= BLAKE2B_HASHLEN)
	{
		if (!blake2b_init(&blake_state, outlen, key, keylen))
			return false;

		if (!blake2b_update(&blake_state, outlen_buf, sizeof outlen_buf))
			return false;

		if (!blake2b_update(&blake_state, in, inlen))
			return false;

		return blake2b_final(&blake_state, out);
	}

	uint8_t ibuf[BLAKE2B_HASHLEN];
	uint8_t obuf[BLAKE2B_HASHLEN];

	if (!blake2b_init(&blake_state, BLAKE2B_HASHLEN, key, keylen))
		return false;

	if (!blake2b_update(&blake_state, outlen_buf, sizeof outlen_buf))
		return false;

	if (!blake2b_update(&blake_state, in, inlen))
		return false;

	if (!blake2b_final(&blake_state, obuf))
		return false;

	(void) memcpy(out, obuf, BLAKE2B_HASHLEN_HALF);
	out += BLAKE2B_HASHLEN_HALF;

	uint32_t remain = (((uint32_t) outlen) - BLAKE2B_HASHLEN_HALF);

	while (remain > BLAKE2B_HASHLEN)
	{
		(void) memcpy(ibuf, obuf, BLAKE2B_HASHLEN);

		if (!blake2b_full(ibuf, BLAKE2B_HASHLEN, key, keylen, obuf, BLAKE2B_HASHLEN))
			return false;

		(void) memcpy(out, obuf, BLAKE2B_HASHLEN_HALF);

		out += BLAKE2B_HASHLEN_HALF;
		remain -= BLAKE2B_HASHLEN_HALF;
	}

	(void) memcpy(ibuf, obuf, BLAKE2B_HASHLEN);

	if (!blake2b_full(ibuf, BLAKE2B_HASHLEN, key, keylen, obuf, remain))
		return false;

	(void) memcpy(out, obuf, remain);
	(void) smemzero(ibuf, sizeof ibuf);
	(void) smemzero(obuf, sizeof obuf);

	return true;
}

// Imported by modules/crypto/argon2d
extern const struct blake2b_functions blake2b_functions;
const struct blake2b_functions blake2b_functions = {
	.b2b_store32    = &blake2b_store32,
	.b2b_store64    = &blake2b_store64,
	.b2b_load64     = &blake2b_load64,
	.b2b_rotr64     = &blake2b_rotr64,
	.b2b_fBlaMka    = &blake2b_fBlaMka,
	.b2b_init       = &blake2b_init,
	.b2b_update     = &blake2b_update,
	.b2b_final      = &blake2b_final,
	.b2b_full       = &blake2b_full,
	.b2b_long       = &blake2b_long,
};

static bool
blake2b_selftest(void)
{
	uint8_t key[BLAKE2B_HASHLEN];
	uint8_t buf[BLAKE2_KAT_LENGTH];

	for (size_t i = 0; i < sizeof key; i++)
		key[i] = (uint8_t) i;

	for (size_t i = 0; i < sizeof buf; i++)
		buf[i] = (uint8_t) i;

	for (size_t step = 1; step < BLAKE2B_BLOCKLEN; step++)
	{
		for (size_t i = 0; i < sizeof buf; i++)
		{
			uint8_t result[BLAKE2B_HASHLEN];
			struct blake2b_state state;
			const uint8_t *p = buf;
			size_t mlen = i;

			if (!blake2b_init(&state, sizeof result, key, sizeof key))
			{
				(void) slog(LG_ERROR, "%s: blake2b_init() failed", MOWGLI_FUNC_NAME);
				return false;
			}
			while (mlen >= step)
			{
				if (!blake2b_update(&state, p, step))
				{
					(void) slog(LG_ERROR, "%s: blake2b_update() [inner loop] failed",
					                      MOWGLI_FUNC_NAME);
					return false;
				}

				mlen -= step;
				p += step;
			}
			if (!blake2b_update(&state, p, mlen))
			{
				(void) slog(LG_ERROR, "%s: blake2b_update() [outer loop] failed", MOWGLI_FUNC_NAME);
				return false;
			}
			if (!blake2b_final(&state, result))
			{
				(void) slog(LG_ERROR, "%s: blake2b_final() failed", MOWGLI_FUNC_NAME);
				return false;
			}
			if (memcmp(blake2b_keyed_kat[i], result, sizeof result) != 0)
			{
				(void) slog(LG_ERROR, "%s: memcmp(3) mismatch", MOWGLI_FUNC_NAME);
				return false;
			}
		}
	}

	return true;
}

static void
mod_init(struct module *const restrict m)
{
	if (!blake2b_selftest())
	{
		(void) slog(LG_ERROR, "%s: self-test failed (BUG)", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) slog(LG_DEBUG, "warning: %s is not a password crypto provider; merely a dependency of others", m->name);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("crypto/blake2b", MODULE_UNLOAD_CAPABILITY_OK)
