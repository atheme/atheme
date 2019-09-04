/*
 * SPDX-License-Identifier: (Apache-2.0 OR CC0-1.0)
 * SPDX-URL: https://spdx.org/licenses/Apache-2.0.html
 * SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
 *
 * Copyright (C) 2015 Daniel Dinu
 * Copyright (C) 2015 Dmitry Khovratovich
 * Copyright (C) 2015 Jean-Philippe Aumasson
 * Copyright (C) 2015 Samuel Neves <sneves@dei.uc.pt>
 * Copyright (C) 2017-2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Argon2 reference C implementation (modified for Atheme IRC Services)
 */

#include <atheme.h>
#include "blake2b.h"

#define ARGON2D_MEMCOST_MIN     8
#define ARGON2D_MEMCOST_DEF     14
#define ARGON2D_MEMCOST_MAX     20

#define ARGON2D_TIMECOST_MIN    4
#define ARGON2D_TIMECOST_DEF    32
#define ARGON2D_TIMECOST_MAX    16384

// Format strings for (s)scanf(3)
#define ATHEME_ARGON2D_LOADSALT "$argon2d$v=19$m=%" SCNu32 ",t=%" SCNu32 ",p=1$%[" BASE64_ALPHABET_RFC4648 "]$"
#define ATHEME_ARGON2D_LOADHASH ATHEME_ARGON2D_LOADSALT "%[" BASE64_ALPHABET_RFC4648 "]"

// Format strings for (sn)printf(3)
#define ATHEME_ARGON2D_SAVESALT "$argon2d$v=19$m=%" PRIu32 ",t=%" PRIu32 ",p=1$%s$"
#define ATHEME_ARGON2D_SAVEHASH ATHEME_ARGON2D_SAVESALT "%s"

#define ATHEME_ARGON2D_AUTHLEN  0x00
#define ATHEME_ARGON2D_HASHLEN  0x40
#define ATHEME_ARGON2D_LANECNT  0x01
#define ATHEME_ARGON2D_PRIVLEN  0x00
#define ATHEME_ARGON2D_SALTLEN  0x20
#define ATHEME_ARGON2D_TYPEVAL  0x00
#define ATHEME_ARGON2D_VERSION  0x13

#define ARGON2_BLKSZ            0x400
#define ARGON2_PREHASH_LEN      0x40
#define ARGON2_PRESEED_LEN      0x48
#define ARGON2_BLK_QWORDS       (ARGON2_BLKSZ / 0x08)
#define ARGON2_SYNC_POINTS      0x04

struct argon2d_block
{
	uint64_t                v[ARGON2_BLK_QWORDS];
};

struct argon2d_context
{
	const uint8_t          *pass;
	uint8_t                 salt[ATHEME_ARGON2D_SALTLEN];
	uint8_t                 hash[ATHEME_ARGON2D_HASHLEN];
	uint32_t                passlen;
	uint32_t                m_cost;
	uint32_t                t_cost;
	uint32_t                lane_len;
	uint32_t                seg_len;
	uint32_t                index;
};

static const struct blake2b_functions *blake2bfn = NULL;
static mowgli_list_t **crypto_conf_table = NULL;

/*
 * The default memory and time cost variables
 * These can be adjusted in the configuration file
 */
static unsigned int atheme_argon2d_mcost = ARGON2D_MEMCOST_DEF;
static unsigned int atheme_argon2d_tcost = ARGON2D_TIMECOST_DEF;

/*
 * This is reallocated on-demand to save allocating and freeing every time we
 * digest a password. The mempoolsz variable tracks how large (in blocks) the
 * currently-allocated memory pool is.
 */
static struct argon2d_block *argon2d_mempool = NULL;
static uint32_t argon2d_mempoolsz = 0;

static inline void
atheme_argon2d_mempool_realloc(const uint32_t mem_blocks)
{
	if (argon2d_mempool != NULL && argon2d_mempoolsz >= mem_blocks)
		return;

	argon2d_mempool = sreallocarray(argon2d_mempool, mem_blocks, sizeof(struct argon2d_block));
	argon2d_mempoolsz = mem_blocks;
}

static inline void
atheme_argon2d_mempool_zero(void)
{
	if (! argon2d_mempool)
		return;

	const size_t mempool_sz_bytes = argon2d_mempoolsz * sizeof(struct argon2d_block);

	(void) smemzero(argon2d_mempool, mempool_sz_bytes);
}

static inline void
argon2d_copy_block(struct argon2d_block *const restrict dst, const struct argon2d_block *const restrict src)
{
	(void) memcpy(dst->v, src->v, (0x08 * ARGON2_BLK_QWORDS));
}

static inline void
argon2d_xor_block(struct argon2d_block *const restrict dst, const struct argon2d_block *const restrict src)
{
	for (size_t x = 0x00; x < ARGON2_BLK_QWORDS; x++)
		dst->v[x] ^= src->v[x];
}

static inline void
argon2d_load_block(struct argon2d_block *const restrict dst, const uint8_t *const restrict input)
{
	for (size_t x = 0x00; x < ARGON2_BLK_QWORDS; x++)
		dst->v[x] = blake2bfn->b2b_load64(&input[(x * sizeof dst->v[x])]);
}

static inline void
argon2d_store_block(uint8_t *const restrict output, const struct argon2d_block *const restrict src)
{
	for (size_t x = 0x00; x < ARGON2_BLK_QWORDS; x++)
		(void) blake2bfn->b2b_store64(&output[(x * sizeof src->v[x])], src->v[x]);
}

static inline bool ATHEME_FATTR_WUR
argon2d_hash_init(struct argon2d_context *const restrict ctx, uint8_t *const restrict bhash)
{
	struct blake2b_state state;
	uint8_t value[4];

	if (!blake2bfn->b2b_init(&state, ARGON2_PREHASH_LEN, NULL, 0))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_LANECNT);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_HASHLEN);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ctx->m_cost);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ctx->t_cost);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_VERSION);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_TYPEVAL);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ctx->passlen);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	if (!blake2bfn->b2b_update(&state, ctx->pass, ctx->passlen))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_SALTLEN);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	if (!blake2bfn->b2b_update(&state, ctx->salt, ATHEME_ARGON2D_SALTLEN))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_PRIVLEN);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	(void) blake2bfn->b2b_store32(value, ATHEME_ARGON2D_AUTHLEN);

	if (!blake2bfn->b2b_update(&state, value, sizeof value))
		return false;

	return blake2bfn->b2b_final(&state, bhash);
}

static uint32_t
argon2d_idx(const struct argon2d_context *const restrict ctx, const uint32_t pass, const uint8_t slice,
            const uint64_t rand_p)
{
	uint32_t ra_size = (ctx->index - 0x01);

	if (pass)
		ra_size += (ctx->lane_len - ctx->seg_len);
	else if (slice)
		ra_size += (slice * ctx->seg_len);

	uint64_t relative_pos = (uint64_t)(rand_p & 0xFFFFFFFF);
	relative_pos = ((relative_pos * relative_pos) >> 0x20U);
	relative_pos = (ra_size - 0x01 - ((ra_size * relative_pos) >> 0x20U));

	uint32_t start_pos = 0x00;
	if (pass && slice != (ARGON2_SYNC_POINTS - 0x01))
		start_pos = ((slice + 0x01) * ctx->seg_len);

	return ((start_pos + ((uint32_t) relative_pos)) % ctx->lane_len);
}

static void
argon2d_fill_block(const struct argon2d_block *const prev, const struct argon2d_block *const ref,
                   struct argon2d_block *const next, const uint32_t pass)
{
	struct argon2d_block block_r;
	struct argon2d_block block_x;

	(void) argon2d_copy_block(&block_r, ref);
	(void) argon2d_xor_block(&block_r, prev);
	(void) argon2d_copy_block(&block_x, &block_r);

	if (pass != 0x00)
		(void) argon2d_xor_block(&block_x, next);

	uint64_t *const v = block_r.v;

#define F(a, b, c, d) do {                                      \
	    (a) = blake2bfn->b2b_fBlaMka((a), (b));             \
	    (d) = blake2bfn->b2b_rotr64(((d) ^ (a)), 0x20);     \
	    (c) = blake2bfn->b2b_fBlaMka((c), (d));             \
	    (b) = blake2bfn->b2b_rotr64(((b) ^ (c)), 0x18);     \
	    (a) = blake2bfn->b2b_fBlaMka((a), (b));             \
	    (d) = blake2bfn->b2b_rotr64(((d) ^ (a)), 0x10);     \
	    (c) = blake2bfn->b2b_fBlaMka((c), (d));             \
	    (b) = blake2bfn->b2b_rotr64(((b) ^ (c)), 0x3F);     \
	} while (0)

	for (size_t x = 0x00; x < 0x08; x++)
	{
		F(v[(0x10 * x) + 0x00], v[(0x10 * x) + 0x04], v[(0x10 * x) + 0x08], v[(0x10 * x) + 0x0C]);
		F(v[(0x10 * x) + 0x01], v[(0x10 * x) + 0x05], v[(0x10 * x) + 0x09], v[(0x10 * x) + 0x0D]);
		F(v[(0x10 * x) + 0x02], v[(0x10 * x) + 0x06], v[(0x10 * x) + 0x0A], v[(0x10 * x) + 0x0E]);
		F(v[(0x10 * x) + 0x03], v[(0x10 * x) + 0x07], v[(0x10 * x) + 0x0B], v[(0x10 * x) + 0x0F]);
		F(v[(0x10 * x) + 0x00], v[(0x10 * x) + 0x05], v[(0x10 * x) + 0x0A], v[(0x10 * x) + 0x0F]);
		F(v[(0x10 * x) + 0x01], v[(0x10 * x) + 0x06], v[(0x10 * x) + 0x0B], v[(0x10 * x) + 0x0C]);
		F(v[(0x10 * x) + 0x02], v[(0x10 * x) + 0x07], v[(0x10 * x) + 0x08], v[(0x10 * x) + 0x0D]);
		F(v[(0x10 * x) + 0x03], v[(0x10 * x) + 0x04], v[(0x10 * x) + 0x09], v[(0x10 * x) + 0x0E]);
	}
	for (size_t x = 0x00; x < 0x08; x++)
	{
		F(v[(0x02 * x) + 0x00], v[(0x02 * x) + 0x20], v[(0x02 * x) + 0x40], v[(0x02 * x) + 0x60]);
		F(v[(0x02 * x) + 0x01], v[(0x02 * x) + 0x21], v[(0x02 * x) + 0x41], v[(0x02 * x) + 0x61]);
		F(v[(0x02 * x) + 0x10], v[(0x02 * x) + 0x30], v[(0x02 * x) + 0x50], v[(0x02 * x) + 0x70]);
		F(v[(0x02 * x) + 0x11], v[(0x02 * x) + 0x31], v[(0x02 * x) + 0x51], v[(0x02 * x) + 0x71]);
		F(v[(0x02 * x) + 0x00], v[(0x02 * x) + 0x21], v[(0x02 * x) + 0x50], v[(0x02 * x) + 0x71]);
		F(v[(0x02 * x) + 0x01], v[(0x02 * x) + 0x30], v[(0x02 * x) + 0x51], v[(0x02 * x) + 0x60]);
		F(v[(0x02 * x) + 0x10], v[(0x02 * x) + 0x31], v[(0x02 * x) + 0x40], v[(0x02 * x) + 0x61]);
		F(v[(0x02 * x) + 0x11], v[(0x02 * x) + 0x20], v[(0x02 * x) + 0x41], v[(0x02 * x) + 0x70]);
	}

#undef F

	(void) argon2d_copy_block(next, &block_x);
	(void) argon2d_xor_block(next, &block_r);

	(void) smemzero(&block_r, sizeof block_r);
	(void) smemzero(&block_x, sizeof block_x);
}

static inline void
argon2d_segment_fill(struct argon2d_context *const restrict ctx, const uint32_t pass, const uint8_t slice)
{
	const uint32_t start_idx = ((!pass && !slice) ? 0x02 : 0x00);
	uint32_t cur_off = (slice * ctx->seg_len) + start_idx;
	uint32_t prv_off = (cur_off - 0x01);

	if ((cur_off % ctx->lane_len) == 0x00)
		prv_off += ctx->lane_len;

	for (uint32_t i = start_idx; i < ctx->seg_len; i++, cur_off++, prv_off++)
	{
		if ((cur_off % ctx->lane_len) == 0x01)
			prv_off = (cur_off - 0x01);

		ctx->index = i;

		const uint64_t rand_p = argon2d_mempool[prv_off].v[0x00];
		const uint32_t ref_idx = argon2d_idx(ctx, pass, slice, rand_p);

		const struct argon2d_block *const prv = &argon2d_mempool[prv_off];
		const struct argon2d_block *const ref = &argon2d_mempool[ref_idx];
		struct argon2d_block *const cur = &argon2d_mempool[cur_off];

		(void) argon2d_fill_block(prv, ref, cur, pass);
	}
}

static bool ATHEME_FATTR_WUR
argon2d_hash_raw(struct argon2d_context *const restrict ctx)
{
	ctx->seg_len = (ctx->m_cost / ARGON2_SYNC_POINTS);

	const uint32_t mem_blocks = (ctx->seg_len * ARGON2_SYNC_POINTS);

	ctx->lane_len = mem_blocks;

	(void) atheme_argon2d_mempool_realloc(mem_blocks);

	uint8_t bhash_init[ARGON2_PRESEED_LEN];

	if (!argon2d_hash_init(ctx, bhash_init))
		return false;

	(void) blake2bfn->b2b_store32(bhash_init + ARGON2_PREHASH_LEN, 0x00);
	(void) blake2bfn->b2b_store32(bhash_init + ARGON2_PREHASH_LEN + 0x04, 0x00);

	uint8_t bhash_bytes[ARGON2_BLKSZ];

	if (!blake2bfn->b2b_long(bhash_init, ARGON2_PRESEED_LEN, NULL, 0, bhash_bytes, ARGON2_BLKSZ))
		return false;

	(void) argon2d_load_block(&argon2d_mempool[0x00], bhash_bytes);
	(void) blake2bfn->b2b_store32(bhash_init + ARGON2_PREHASH_LEN, 0x01);

	if (!blake2bfn->b2b_long(bhash_init, ARGON2_PRESEED_LEN, NULL, 0, bhash_bytes, ARGON2_BLKSZ))
		return false;

	(void) argon2d_load_block(&argon2d_mempool[0x01], bhash_bytes);

	for (uint32_t pass = 0x00; pass < ctx->t_cost; pass++)
	{
		for (uint8_t slice = 0x00; slice < ARGON2_SYNC_POINTS; slice++)
		{
			ctx->index = 0x00;
			(void) argon2d_segment_fill(ctx, pass, slice);
		}
	}

	struct argon2d_block bhash_final;

	(void) argon2d_copy_block(&bhash_final, &argon2d_mempool[ctx->lane_len - 0x01]);
	(void) argon2d_store_block(bhash_bytes, &bhash_final);

	const bool ret = blake2bfn->b2b_long(bhash_bytes, ARGON2_BLKSZ, NULL, 0, ctx->hash, ATHEME_ARGON2D_HASHLEN);

	(void) smemzero(bhash_init, sizeof bhash_init);
	(void) smemzero(bhash_bytes, sizeof bhash_bytes);
	(void) smemzero(&bhash_final, sizeof bhash_final);

	return ret;
}

static inline size_t ATHEME_FATTR_WUR
base64_encode_raw(const void *const restrict in, const size_t in_len, char *const restrict out, const size_t out_len)
{
	return base64_encode_table(in, in_len, out, out_len, BASE64_ALPHABET_RFC4648_NOPAD);
}

static const char * ATHEME_FATTR_WUR
atheme_argon2d_crypt(const char *const restrict password,
                     const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	struct argon2d_context ctx;
	(void) memset(&ctx, 0x00, sizeof ctx);
	(void) atheme_random_buf(ctx.salt, sizeof ctx.salt);

	ctx.pass = (const uint8_t *) password;
	ctx.passlen = (uint32_t) strlen(password);
	ctx.m_cost = (uint32_t) (0x01U << atheme_argon2d_mcost);
	ctx.t_cost = (uint32_t) atheme_argon2d_tcost;

	if (! argon2d_hash_raw(&ctx))
		return NULL;

	char salt_b64[BASE64_SIZE_STR(ATHEME_ARGON2D_SALTLEN)];
	if (base64_encode_raw(ctx.salt, sizeof ctx.salt, salt_b64, sizeof salt_b64) == BASE64_FAIL)
		return NULL;

	char hash_b64[BASE64_SIZE_STR(ATHEME_ARGON2D_HASHLEN)];
	if (base64_encode_raw(ctx.hash, sizeof ctx.hash, hash_b64, sizeof hash_b64) == BASE64_FAIL)
		return NULL;

	static char res[PASSLEN + 1];
	if (snprintf(res, sizeof res, ATHEME_ARGON2D_SAVEHASH, ctx.m_cost, ctx.t_cost, salt_b64, hash_b64) > PASSLEN)
		return NULL;

	(void) smemzero(&ctx, sizeof ctx);
	(void) atheme_argon2d_mempool_zero();

	return res;
}

static bool
atheme_argon2d_recrypt(const struct argon2d_context *const restrict ctx)
{
	const uint32_t m_cost_def = (uint32_t) (0x01U << atheme_argon2d_mcost);
	const uint32_t t_cost_def = (uint32_t) atheme_argon2d_tcost;

	if (ctx->m_cost != m_cost_def)
	{
		(void) slog(LG_DEBUG, "%s: memory cost (%" PRIu32 ") != default (%" PRIu32 ")", MOWGLI_FUNC_NAME,
		                      ctx->m_cost, m_cost_def);
		return true;
	}

	if (ctx->t_cost != t_cost_def)
	{
		(void) slog(LG_DEBUG, "%s: time cost (%" PRIu32 ") != default (%" PRIu32 ")", MOWGLI_FUNC_NAME,
		                      ctx->t_cost, t_cost_def);
		return true;
	}

	return false;
}

static bool ATHEME_FATTR_WUR
atheme_argon2d_verify(const char *const restrict password, const char *const restrict parameters,
                      unsigned int *const restrict flags)
{
	struct argon2d_context ctx;
	(void) memset(&ctx, 0x00, sizeof ctx);

	char salt_b64[0x1000];
	char hash_b64[0x1000];
	uint8_t dec_hash[ATHEME_ARGON2D_HASHLEN];

	if (sscanf(parameters, ATHEME_ARGON2D_LOADHASH, &ctx.m_cost, &ctx.t_cost, salt_b64, hash_b64) != 4)
		return false;

	if (ctx.m_cost < (0x01U << ARGON2D_MEMCOST_MIN) || ctx.m_cost > (0x01U << ARGON2D_MEMCOST_MAX))
		return false;

	if (ctx.t_cost < ARGON2D_TIMECOST_MIN || ctx.t_cost > ARGON2D_TIMECOST_MAX)
		return false;

	if (base64_decode(salt_b64, ctx.salt, sizeof ctx.salt) != sizeof ctx.salt)
		return false;

	if (base64_decode(hash_b64, dec_hash, sizeof dec_hash) != sizeof dec_hash)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	ctx.pass = (const uint8_t *) password;
	ctx.passlen = (uint32_t) strlen(password);

	if (! argon2d_hash_raw(&ctx))
		return false;

	if (smemcmp(ctx.hash, dec_hash, ATHEME_ARGON2D_HASHLEN) != 0)
		return false;

	if (atheme_argon2d_recrypt(&ctx))
		*flags |= PWVERIFY_FLAG_RECRYPT;

	(void) smemzero(&ctx, sizeof ctx);
	(void) atheme_argon2d_mempool_zero();

	return true;
}

static const struct crypt_impl crypto_argon2d_impl = {

	.id         = "argon2d",
	.crypt      = &atheme_argon2d_crypt,
	.verify     = &atheme_argon2d_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table");
	MODULE_TRY_REQUEST_SYMBOL(m, blake2bfn, "crypto/blake2b", "blake2b_functions");

	(void) crypt_register(&crypto_argon2d_impl);

	(void) add_uint_conf_item("argon2d_memory", *crypto_conf_table, 0, &atheme_argon2d_mcost,
	                          ARGON2D_MEMCOST_MIN, ARGON2D_MEMCOST_MAX, ARGON2D_MEMCOST_DEF);

	(void) add_uint_conf_item("argon2d_time", *crypto_conf_table, 0, &atheme_argon2d_tcost,
	                          ARGON2D_TIMECOST_MIN, ARGON2D_TIMECOST_MAX, ARGON2D_TIMECOST_DEF);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("argon2d_memory", *crypto_conf_table);
	(void) del_conf_item("argon2d_time", *crypto_conf_table);

	(void) crypt_unregister(&crypto_argon2d_impl);

	(void) sfree(argon2d_mempool);
}

SIMPLE_DECLARE_MODULE_V1("crypto/argon2d", MODULE_UNLOAD_CAPABILITY_OK)
