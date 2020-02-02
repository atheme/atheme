/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-URL: https://spdx.org/licenses/BSD-3-Clause.html
 *
 * Copyright (C) 1996 David Mazieres <dm@uun.org>
 * Copyright (C) 2008 Damien Miller <djm@openbsd.org>
 * Copyright (C) 2013 Markus Friedl <markus@openbsd.org>
 * Copyright (C) 2017-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ATHEME_LAC_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_LAC_RANDOM_FRONTEND_C */

#ifdef HAVE_USABLE_GETRANDOM
#  include <sys/random.h>
#endif

#define RANDOM_DEV_PATH         "/dev/urandom"

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

static uint8_t rs_buf[CHACHA20_STATESZ];
static size_t rs_count = 0;
static size_t rs_have = 0;

static bool rs_slog_errors = false;
static bool rs_initialized = false;
static pid_t rs_stir_pid = (pid_t) -1;

static void ATHEME_FATTR_PRINTF(1, 2)
_rs_log_error(const char *const restrict format, ...)
{
	char buf[BUFSIZE];
	va_list argv;

	va_start(argv, format);
	(void) vsnprintf(buf, sizeof buf, format, argv);
	va_end(argv);

	if (rs_slog_errors)
	{
		(void) slog(LG_ERROR, "%s", buf);
		return;
	}

	(void) fprintf(stderr, "libathemecore: Early RNG initialization failed!\n");
	(void) fprintf(stderr, "Error: %s\n", buf);
}

static bool ATHEME_FATTR_WUR
_rs_get_seed_material(uint8_t *const restrict buf, const size_t len)
{
#if defined(HAVE_USABLE_GETENTROPY)

	if (getentropy(buf, len) != 0)
	{
		(void) _rs_log_error("%s: getentropy(2): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return false;
	}

#elif defined(HAVE_USABLE_GETRANDOM)

	size_t out = 0;

	while (out < len)
	{
		const ssize_t ret = getrandom(buf + out, len - out, 0);

		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;

			(void) _rs_log_error("%s: getrandom(2): %s", MOWGLI_FUNC_NAME, strerror(errno));
			return false;
		}
		if (ret == 0)
		{
			(void) _rs_log_error("%s: getrandom(2): no data returned", MOWGLI_FUNC_NAME);
			return false;
		}

		out += (size_t) ret;
	}

#else

	static int fd = -1;
	size_t out = 0;

	if (fd == -1 && (fd = open(RANDOM_DEV_PATH, O_RDONLY)) == -1)
	{
		(void) _rs_log_error("%s: open('%s'): %s", MOWGLI_FUNC_NAME, RANDOM_DEV_PATH, strerror(errno));
		return false;
	}

	while (out < len)
	{
		const ssize_t ret = read(fd, buf + out, len - out);

		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;

			(void) _rs_log_error("%s: read('%s'): %s", MOWGLI_FUNC_NAME, RANDOM_DEV_PATH, strerror(errno));
			return false;
		}
		if (ret == 0)
		{
			(void) _rs_log_error("%s: read('%s'): EOF", MOWGLI_FUNC_NAME, RANDOM_DEV_PATH);
			return false;
		}

		out += (size_t) ret;
	}

#endif

	return true;
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

static bool ATHEME_FATTR_WUR
_rs_stir_if_needed(const size_t len)
{
	pid_t pid = getpid();

	if (rs_count <= len || ! rs_initialized || rs_stir_pid != pid)
	{
		uint8_t tmp[CHACHA20_KEYSZ + CHACHA20_IVSZ];

		if (! _rs_get_seed_material(tmp, sizeof tmp))
			return false;

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

	return true;
}

uint32_t
atheme_random(void)
{
	uint32_t val;

	(void) atheme_random_buf(&val, sizeof val);

	return val;
}

uint32_t
atheme_random_uniform(const uint32_t bound)
{
	if (bound < 2)
		return 0;

	const uint32_t min = -bound % bound;

	for (;;)
	{
		uint32_t candidate;

		(void) atheme_random_buf(&candidate, sizeof candidate);

		if (candidate >= min)
			return candidate % bound;
	}
}

void
atheme_random_buf(void *const restrict out, size_t len)
{
	uint8_t *buf = (uint8_t *) out;

	if (! _rs_stir_if_needed(len))
		abort();

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

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	if (! _rs_stir_if_needed(0))
		return false;

	rs_slog_errors = true;
	return true;
}

const char *
random_get_frontend_info(void)
{
	return "Internal ChaCha20-based Fallback RNG";
}
