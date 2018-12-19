/* OPENBSD ORIGINAL: lib/libc/crypto/arc4random.c */
/* $OpenBSD: arc4random.c,v 1.25 2013/10/01 18:34:57 markus Exp $ */

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "atheme.h"

#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG

#include <mbedtls/entropy.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/md.h>

#if defined(MBEDTLS_ERROR_C) && defined(HAVE_MBEDTLS_ERROR_H)
#  include <mbedtls/error.h>
#endif

static const char atheme_pers_str[] = "Atheme IRC Services v" PACKAGE_VERSION;

static mbedtls_entropy_context seed_ctx;
static mbedtls_hmac_drbg_context hmac_ctx;

static pid_t rs_stir_pid = (pid_t) -1;

static const char *
atheme_arc4random_mbedtls_strerror(int err)
{
	static char errbuf[384];

	if (err < 0)
		err = -err;

#if defined(MBEDTLS_ERROR_C) && defined(HAVE_MBEDTLS_ERROR_H)
	char mbed_errbuf[320];

	(void) mbedtls_strerror(err, mbed_errbuf, sizeof mbed_errbuf);
	(void) snprintf(errbuf, sizeof errbuf, "-0x%X: %s", (unsigned int) err, mbed_errbuf);
#else
	(void) snprintf(errbuf, sizeof errbuf, "-0x%X", (unsigned int) err);
#endif

	return errbuf;
}

void
atheme_arc4random_buf(void *const restrict out, const size_t len)
{
	if (rs_stir_pid == (pid_t) -1)
	{
		const mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
		const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(md_type);

		(void) mbedtls_entropy_init(&seed_ctx);
		(void) mbedtls_hmac_drbg_init(&hmac_ctx);

		const int ret = mbedtls_hmac_drbg_seed(&hmac_ctx, md_info, &mbedtls_entropy_func, &seed_ctx,
		                                       (const void *) atheme_pers_str, sizeof atheme_pers_str);

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_seed: error %s", __func__,
			                      atheme_arc4random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}
	else if (getpid() != rs_stir_pid)
	{
		const int ret = mbedtls_hmac_drbg_reseed(&hmac_ctx, NULL, 0);

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_reseed: error %s", __func__,
			                      atheme_arc4random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}

	const int ret = mbedtls_hmac_drbg_random(&hmac_ctx, out, len);

	if (ret != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_random: error %s", __func__,
		                      atheme_arc4random_mbedtls_strerror(ret));
		exit(EXIT_FAILURE);
	}
}

#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

/*
 * We don't have a library that provides a high-quality RNG.
 * Fall back to ChaCha20-based RNG from OpenBSD.
 */

#if !defined(HAVE_GETENTROPY) && defined(HAVE_GETRANDOM) && defined(HAVE_SYS_RANDOM_H)
#  include <sys/random.h>
#endif

#define CHACHA20_KEYSZ          0x20U
#define CHACHA20_IVSZ           0x08U
#define CHACHA20_BLOCKSZ        0x40U
#define CHACHA20_STATESZ        (0x10U * CHACHA20_BLOCKSZ)

#define CHACHA20_U8V(v)         (((uint8_t) (v)) & UINT8_C(0xFF))
#define CHACHA20_U32V(v)        (((uint32_t) (v)) & UINT32_C(0xFFFFFFFF))
#define CHACHA20_ROTL32(v, w)   (CHACHA20_U32V((v) << (w)) | ((v) >> (0x20U - (w))))
#define CHACHA20_ROTATE(v, w)   (CHACHA20_ROTL32((v), (w)))

#define CHACHA20_QUARTERROUND(x, a, b, c, d)                                    \
    do {                                                                        \
        x[a] += x[b];                                                           \
        x[d] = CHACHA20_ROTATE((x[d] ^ x[a]), 0x10U);                           \
        x[c] += x[d];                                                           \
        x[b] = CHACHA20_ROTATE((x[b] ^ x[c]), 0x0CU);                           \
        x[a] += x[b];                                                           \
        x[d] = CHACHA20_ROTATE((x[d] ^ x[a]), 0x08U);                           \
        x[c] += x[d];                                                           \
        x[b] = CHACHA20_ROTATE((x[b] ^ x[c]), 0x07U);                           \
    } while (0)

#define CHACHA20_U32TO8(p, v)                                                   \
    do {                                                                        \
        ((uint8_t *) (p))[0x00U] = CHACHA20_U8V((v) >> 0x00U);                  \
        ((uint8_t *) (p))[0x01U] = CHACHA20_U8V((v) >> 0x08U);                  \
        ((uint8_t *) (p))[0x02U] = CHACHA20_U8V((v) >> 0x10U);                  \
        ((uint8_t *) (p))[0x03U] = CHACHA20_U8V((v) >> 0x18U);                  \
    } while (0)

#define CHACHA20_U8TO32(p)                                                      \
    (((uint32_t) ((p)[0x00U]) << 0x00U) | ((uint32_t) ((p)[0x01U]) << 0x08U) |  \
     ((uint32_t) ((p)[0x02U]) << 0x10U) | ((uint32_t) ((p)[0x03U]) << 0x18U))

struct chacha20_context
{
	uint32_t state[0x10U];
};

static struct chacha20_context rs;

static const uint8_t sigma[] = {
	0x65, 0x78, 0x70, 0x61, 0x6E, 0x64, 0x20, 0x33, 0x32, 0x2D, 0x62, 0x79, 0x74, 0x65, 0x20, 0x6B
};

static const uint8_t tau[] = {
	0x65, 0x78, 0x70, 0x61, 0x6E, 0x64, 0x20, 0x31, 0x36, 0x2D, 0x62, 0x79, 0x74, 0x65, 0x20, 0x6B
};

static uint8_t rs_buf[CHACHA20_STATESZ];
static size_t rs_count = 0;
static size_t rs_have = 0;

static bool rs_initialized = false;
static pid_t rs_stir_pid = -1;

static void
_rs_get_seed_material(uint8_t *const restrict buf, const size_t len)
{
#ifdef HAVE_GETENTROPY

	if (getentropy(buf, len) != 0)
	{
		(void) slog(LG_ERROR, "%s: getentropy(2): %s", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}

#else /* HAVE_GETENTROPY */
#  if defined(HAVE_GETRANDOM) && defined(HAVE_SYS_RANDOM_H)

	size_t out = 0;

	while (out < len)
	{
		const ssize_t ret = getrandom(buf + out, len - out, 0);

		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;

			(void) slog(LG_ERROR, "%s: getrandom(2): %s", __func__, strerror(errno));
			exit(EXIT_FAILURE);
		}

		out += (size_t) ret;
	}

#  else /* HAVE_GETRANDOM && HAVE_SYS_RANDOM_H */

	static const char *const random_dev = "/dev/urandom";
	static int fd = -1;
	size_t out = 0;

	if (fd == -1 && (fd = open(random_dev, O_RDONLY)) == -1)
	{
		(void) slog(LG_ERROR, "%s: open('%s'): %s", __func__, random_dev, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (out < len)
	{
		const ssize_t ret = read(fd, buf + out, len - out);

		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;

			(void) slog(LG_ERROR, "%s: read('%s'): %s", __func__, random_dev, strerror(errno));
			exit(EXIT_FAILURE);
		}

		out += (size_t) ret;
	}

#  endif /* !HAVE_GETRANDOM || !HAVE_SYS_RANDOM_H */
#endif /* !HAVE_GETENTROPY */
}

static void
_rs_chacha_keysetup(struct chacha20_context *const restrict ctx, const uint8_t *restrict k)
{
	ctx->state[0x04U] = CHACHA20_U8TO32(k + 0x00U);
	ctx->state[0x05U] = CHACHA20_U8TO32(k + 0x04U);
	ctx->state[0x06U] = CHACHA20_U8TO32(k + 0x08U);
	ctx->state[0x07U] = CHACHA20_U8TO32(k + 0x0CU);

	k += 0x10U;

	ctx->state[0x08U] = CHACHA20_U8TO32(k + 0x00U);
	ctx->state[0x09U] = CHACHA20_U8TO32(k + 0x04U);
	ctx->state[0x0AU] = CHACHA20_U8TO32(k + 0x08U);
	ctx->state[0x0BU] = CHACHA20_U8TO32(k + 0x0CU);

	ctx->state[0x00U] = CHACHA20_U8TO32(sigma + 0x00U);
	ctx->state[0x01U] = CHACHA20_U8TO32(sigma + 0x04U);
	ctx->state[0x02U] = CHACHA20_U8TO32(sigma + 0x08U);
	ctx->state[0x03U] = CHACHA20_U8TO32(sigma + 0x0CU);
}

static void
_rs_chacha_ivsetup(struct chacha20_context *const restrict ctx, const uint8_t *const restrict iv)
{
	ctx->state[0x0CU] = 0x00U;
	ctx->state[0x0DU] = 0x00U;
	ctx->state[0x0EU] = CHACHA20_U8TO32(iv + 0x00U);
	ctx->state[0x0FU] = CHACHA20_U8TO32(iv + 0x04U);
}

static void
_rs_chacha_encrypt(struct chacha20_context *const restrict ctx, const uint8_t *m, uint8_t *c, uint32_t bytes)
{
	if (! bytes)
		return;

	uint32_t j[0x10U];
	uint32_t x[0x10U];

	uint8_t tmp[0x40U];
	uint8_t *ctarget = NULL;

	(void) memcpy(j, ctx->state, sizeof j);

	for (;;)
	{
		if (bytes < 0x40U)
		{
			for (size_t i = 0x00U; i < bytes; i++)
				tmp[i] = m[i];

			ctarget = c;
			c = tmp;
			m = tmp;
		}

		(void) memcpy(x, j, sizeof x);

		for (size_t i = 0x14U; i > 0x00U; i -= 0x02U)
		{
			CHACHA20_QUARTERROUND(x, 0x00U, 0x04U, 0x08U, 0x0CU);
			CHACHA20_QUARTERROUND(x, 0x01U, 0x05U, 0x09U, 0x0DU);
			CHACHA20_QUARTERROUND(x, 0x02U, 0x06U, 0x0AU, 0x0EU);
			CHACHA20_QUARTERROUND(x, 0x03U, 0x07U, 0x0BU, 0x0FU);
			CHACHA20_QUARTERROUND(x, 0x00U, 0x05U, 0x0AU, 0x0FU);
			CHACHA20_QUARTERROUND(x, 0x01U, 0x06U, 0x0BU, 0x0CU);
			CHACHA20_QUARTERROUND(x, 0x02U, 0x07U, 0x08U, 0x0DU);
			CHACHA20_QUARTERROUND(x, 0x03U, 0x04U, 0x09U, 0x0EU);
		}

		for (size_t i = 0x00U; i < 0x10U; i++)
			x[i] += j[i];

		j[0x0CU]++;

		if (! j[0x0CU])
			j[0x0DU]++;

		for (size_t i = 0x00U; i < 0x10U; i++)
			CHACHA20_U32TO8(c + (i * 0x04U), x[i]);

		if (bytes <= 0x40U)
		{
			if (bytes < 0x40U)
				for (size_t i = 0x00U; i < bytes; i++)
					ctarget[i] = c[i];

			ctx->state[0x0CU] = j[0x0CU];
			ctx->state[0x0DU] = j[0x0DU];
			return;
		}

		bytes -= 0x40U;
		c += 0x40U;
	}
}

static inline void
_rs_init(uint8_t *const restrict buf)
{
	(void) _rs_chacha_keysetup(&rs, buf);
	(void) _rs_chacha_ivsetup(&rs, (buf + CHACHA20_KEYSZ));
}

static void
_rs_rekey(uint8_t *const restrict buf)
{
	(void) _rs_chacha_encrypt(&rs, rs_buf, rs_buf, CHACHA20_STATESZ);

	for (size_t i = 0; buf != NULL && i < (CHACHA20_KEYSZ + CHACHA20_IVSZ); i++)
		rs_buf[i] ^= buf[i];

	(void) _rs_init(rs_buf);
	(void) memset(rs_buf, 0x00, (CHACHA20_KEYSZ + CHACHA20_IVSZ));

	rs_have = (CHACHA20_STATESZ - CHACHA20_KEYSZ - CHACHA20_IVSZ);
}

static void
_rs_stir_if_needed(const size_t len)
{
	pid_t pid = getpid();

	if (rs_count <= len || ! rs_initialized || rs_stir_pid != pid)
	{
		uint8_t tmp[CHACHA20_KEYSZ + CHACHA20_IVSZ];

		(void) _rs_get_seed_material(tmp, sizeof tmp);

		if (! rs_initialized)
		{
			(void) _rs_init(tmp);

			rs_initialized = true;
		}
		else
			(void) _rs_rekey(tmp);

		(void) smemzero(tmp, sizeof tmp);
		(void) memset(rs_buf, 0x00, sizeof rs_buf);

		rs_stir_pid = pid;
		rs_count = 1600000;
		rs_have = 0;
	}
	else
		rs_count -= len;
}

void
atheme_arc4random_buf(void *const restrict out, size_t len)
{
	uint8_t *buf = (uint8_t *) out;

	(void) _rs_stir_if_needed(len);

	while (len)
	{
		if (rs_have)
		{
			const size_t min = MIN(len, rs_have);

			(void) memcpy(buf, rs_buf + CHACHA20_STATESZ - rs_have, min);
			(void) memset(rs_buf + CHACHA20_STATESZ - rs_have, 0x00, min);

			rs_have -= min;
			buf += min;
			len -= min;
		}

		if (! rs_have)
			(void) _rs_rekey(NULL);
	}
}

#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

uint32_t
atheme_arc4random(void)
{
	uint32_t val;

	(void) atheme_arc4random_buf(&val, sizeof val);

	return val;
}

uint32_t
atheme_arc4random_uniform(const uint32_t bound)
{
	if (bound < 2)
		return 0;

	const uint32_t min = -bound % bound;

	for (;;)
	{
		uint32_t candidate;

		(void) atheme_arc4random_buf(&candidate, sizeof candidate);

		if (candidate >= min)
			return candidate % bound;
	}
}
