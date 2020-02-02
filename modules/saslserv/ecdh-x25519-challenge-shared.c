/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * ECDH-X25519-CHALLENGE mechanism shared routines.
 */

#ifndef HAVE_ANYLIB_ECDH_X25519
#  error "Do not compile me directly; compile ecdh-x25519-challenge.c instead"
#endif

#include "ecdh-x25519-challenge.h"

#if defined(HAVE_LIBSODIUM_ECDH_X25519)
#  define ECDH_X25519_USE_LIBSODIUM     1
#elif defined(HAVE_LIBCRYPTO_ECDH_X25519)
#  define ECDH_X25519_USE_LIBNETTLE     1
#elif defined(HAVE_LIBMBEDCRYPTO_ECDH_X25519)
#  define ECDH_X25519_USE_LIBCRYPTO     1
#elif defined(HAVE_LIBNETTLE_ECDH_X25519)
#  define ECDH_X25519_USE_LIBMBEDCRYPTO 1
#else
#  error "No ECDH X25519 cryptographic library implementation is available"
#endif

static const unsigned char ecdh_x25519_zeroes[] = {
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};



#ifdef ECDH_X25519_LOG_TO_STDERR

static void ATHEME_FATTR_PRINTF(2, 3)
ecdh_x25519_log_error(const unsigned int ATHEME_VATTR_UNUSED loglevel, const char *const restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	(void) fprintf(stderr, "\n");
	va_end(ap);
}

#else /* ECDH_X25519_LOG_TO_STDERR */

static void ATHEME_FATTR_PRINTF(2, 3)
ecdh_x25519_log_error(const unsigned int loglevel, const char *const restrict format, ...)
{
	char message[BUFSIZE];
	va_list ap;
	va_start(ap, format);
	(void) vsnprintf(message, sizeof message, format, ap);
	(void) slog(loglevel, "%s", message);
	va_end(ap);
}

#endif /* !ECDH_X25519_LOG_TO_STDERR */



#ifdef ECDH_X25519_USE_LIBSODIUM

#include <sodium/crypto_scalarmult_curve25519.h>

static bool ATHEME_FATTR_WUR
ecdh_x25519_create_keypair(unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	(void) atheme_random_buf(seckey, ATHEME_ECDH_X25519_XKEY_LEN);

	// Clamp the secret key
	seckey[0x00U] &= 0xF8U;
	seckey[0x1FU] &= 0x7FU;
	seckey[0x1FU] |= 0x40U;

	if (crypto_scalarmult_curve25519_base(pubkey, seckey) != 0)
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: crypto_scalarmult_curve25519_base() failed (BUG?)",
		                                       MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}

static bool ATHEME_FATTR_WUR
ecdh_x25519_compute_shared(const unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           const unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char shared_secret[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	if (crypto_scalarmult_curve25519(shared_secret, seckey, pubkey) != 0)
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: crypto_scalarmult_curve25519() failed (BUG?)",
		                                       MOWGLI_FUNC_NAME);
		return false;
	}
	if (smemcmp(shared_secret, ecdh_x25519_zeroes, sizeof ecdh_x25519_zeroes) == 0)
	{
		(void) ecdh_x25519_log_error(LG_DEBUG, "%s: secret is all zeroes (bad pubkey?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}

#endif /* ECDH_X25519_USE_LIBSODIUM */



#ifdef ECDH_X25519_USE_LIBNETTLE

#include <nettle/curve25519.h>

static bool ATHEME_FATTR_WUR
ecdh_x25519_create_keypair(unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	(void) atheme_random_buf(seckey, ATHEME_ECDH_X25519_XKEY_LEN);

	// Clamp the secret key
	seckey[0x00U] &= 0xF8U;
	seckey[0x1FU] &= 0x7FU;
	seckey[0x1FU] |= 0x40U;

	(void) nettle_curve25519_mul_g(pubkey, seckey);
	return true;
}

static bool ATHEME_FATTR_WUR
ecdh_x25519_compute_shared(const unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           const unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char shared_secret[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	(void) nettle_curve25519_mul(shared_secret, seckey, pubkey);

	if (smemcmp(shared_secret, ecdh_x25519_zeroes, sizeof ecdh_x25519_zeroes) == 0)
	{
		(void) ecdh_x25519_log_error(LG_DEBUG, "%s: secret is all zeroes (bad pubkey?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}

#endif /* ECDH_X25519_USE_LIBNETTLE */



#ifdef ECDH_X25519_USE_LIBCRYPTO

#include <openssl/curve25519.h>

static bool ATHEME_FATTR_WUR
ecdh_x25519_create_keypair(unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	(void) X25519_keypair(pubkey, seckey);
	return true;
}

static bool ATHEME_FATTR_WUR
ecdh_x25519_compute_shared(const unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           const unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char shared_secret[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	if (X25519(shared_secret, seckey, pubkey) != 1)
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: X25519() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (smemcmp(shared_secret, ecdh_x25519_zeroes, sizeof ecdh_x25519_zeroes) == 0)
	{
		(void) ecdh_x25519_log_error(LG_DEBUG, "%s: secret is all zeroes (bad pubkey?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}

#endif /* ECDH_X25519_USE_LIBCRYPTO */



#ifdef ECDH_X25519_USE_LIBMBEDCRYPTO

#include <mbedtls/bignum.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>

#define X25519_MBEDTLS_CHK(loglevel, cond)                                                                           \
    do {                                                                                                             \
        if ((ret = (cond)) != 0) {                                                                                   \
            (void) ecdh_x25519_log_error(loglevel, "%s: %s: -0x%08X", MOWGLI_FUNC_NAME, #cond, (unsigned int) -ret); \
            goto cleanup;                                                                                            \
        }                                                                                                            \
    } while (0)

#define X25519_MBEDTLS_CHK_D(cond) X25519_MBEDTLS_CHK(LG_DEBUG, cond)
#define X25519_MBEDTLS_CHK_E(cond) X25519_MBEDTLS_CHK(LG_ERROR, cond)

// This provides a random number generator function with a function signature that ARM mbedTLS requires
static int
ecdh_x25519_rng_func(void ATHEME_VATTR_UNUSED *const restrict p_rng,
                     unsigned char *const restrict buf, const size_t len)
{
	(void) atheme_random_buf(buf, len);
	return 0;
}

/* The canonical representation for Curve25519 private keys, public keys, and shared secrets,
 * is little-endian; but libmbedcrypto's MPI subsystem reads and writes them in big-endian...
 */
static void
ecdh_x25519_array_reverse(unsigned char *const restrict ptr, const size_t len)
{
	for (size_t i = 0; i < (len / 2); i++)
	{
		unsigned char tmpval = ptr[i];
		ptr[i] = ptr[len - i - 1];
		ptr[len - i - 1] = tmpval;
	}
}

static bool ATHEME_FATTR_WUR
ecdh_x25519_create_keypair(unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	bool retval = false;

	mbedtls_ecp_group G;
	mbedtls_ecp_point Q;
	mbedtls_mpi d;
	int ret;

	(void) mbedtls_ecp_group_init(&G);
	(void) mbedtls_ecp_point_init(&Q);
	(void) mbedtls_mpi_init(&d);

	X25519_MBEDTLS_CHK_E(mbedtls_ecp_group_load(&G, MBEDTLS_ECP_DP_CURVE25519));
	X25519_MBEDTLS_CHK_E(mbedtls_ecdh_gen_public(&G, &d, &Q, &ecdh_x25519_rng_func, NULL));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_write_binary(&d, seckey, ATHEME_ECDH_X25519_XKEY_LEN));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_write_binary(&Q.X, pubkey, ATHEME_ECDH_X25519_XKEY_LEN));

	(void) ecdh_x25519_array_reverse(seckey, ATHEME_ECDH_X25519_XKEY_LEN);
	(void) ecdh_x25519_array_reverse(pubkey, ATHEME_ECDH_X25519_XKEY_LEN);

	retval = true;

cleanup:
	(void) mbedtls_mpi_free(&d);
	(void) mbedtls_ecp_point_free(&Q);
	(void) mbedtls_ecp_group_free(&G);
	return retval;
}

static bool ATHEME_FATTR_WUR
ecdh_x25519_compute_shared(const unsigned char seckey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           const unsigned char pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                           unsigned char shared_secret[const restrict static ATHEME_ECDH_X25519_XKEY_LEN])
{
	bool retval = false;

	unsigned char seckey_be[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char pubkey_be[ATHEME_ECDH_X25519_XKEY_LEN];

	mbedtls_ecp_group G;
	mbedtls_ecp_point Q;
	mbedtls_mpi d;
	mbedtls_mpi z;
	int ret;

	(void) memcpy(seckey_be, seckey, sizeof seckey_be);
	(void) memcpy(pubkey_be, pubkey, sizeof pubkey_be);
	(void) ecdh_x25519_array_reverse(seckey_be, sizeof seckey_be);
	(void) ecdh_x25519_array_reverse(pubkey_be, sizeof pubkey_be);

	(void) mbedtls_ecp_group_init(&G);
	(void) mbedtls_ecp_point_init(&Q);
	(void) mbedtls_mpi_init(&d);
	(void) mbedtls_mpi_init(&z);

	X25519_MBEDTLS_CHK_E(mbedtls_ecp_group_load(&G, MBEDTLS_ECP_DP_CURVE25519));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_read_binary(&d, seckey_be, sizeof seckey_be));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_read_binary(&Q.X, pubkey_be, sizeof pubkey_be));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_lset(&Q.Z, 1));
	X25519_MBEDTLS_CHK_D(mbedtls_ecp_check_pubkey(&G, &Q));
	X25519_MBEDTLS_CHK_D(mbedtls_ecdh_compute_shared(&G, &z, &Q, &d, &ecdh_x25519_rng_func, NULL));
	X25519_MBEDTLS_CHK_E(mbedtls_mpi_write_binary(&z, shared_secret, ATHEME_ECDH_X25519_XKEY_LEN));

	(void) ecdh_x25519_array_reverse(shared_secret, ATHEME_ECDH_X25519_XKEY_LEN);

	if (smemcmp(shared_secret, ecdh_x25519_zeroes, sizeof ecdh_x25519_zeroes) == 0)
	{
		(void) ecdh_x25519_log_error(LG_DEBUG, "%s: secret is all zeroes (bad pubkey?)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	retval = true;

cleanup:
	(void) mbedtls_mpi_free(&z);
	(void) mbedtls_mpi_free(&d);
	(void) mbedtls_ecp_point_free(&Q);
	(void) mbedtls_ecp_group_free(&G);
	(void) smemzero(seckey_be, sizeof seckey_be);
	return retval;
}

#endif /* ECDH_X25519_USE_LIBMBEDCRYPTO */



#define ECDH_X25519_KDF(ikm, salt, okm)                                 \
    digest_oneshot_hkdf(DIGALG_SHA2_256, ikm, DIGEST_MDLEN_SHA2_256,    \
                        salt, ATHEME_ECDH_X25519_SALT_LEN,              \
                        "ECDH-X25519-CHALLENGE", 21U,                   \
                        okm, ATHEME_ECDH_X25519_CHAL_LEN)

static bool ATHEME_FATTR_WUR
ecdh_x25519_kdf(const unsigned char shared_secret[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                const unsigned char client_pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                const unsigned char server_pubkey[const restrict static ATHEME_ECDH_X25519_XKEY_LEN],
                const unsigned char session_salt[const restrict static ATHEME_ECDH_X25519_SALT_LEN],
                unsigned char better_secret[const restrict static ATHEME_ECDH_X25519_CHAL_LEN])
{
	const struct digest_vector secret_vec[] = {
		{ shared_secret, ATHEME_ECDH_X25519_XKEY_LEN },
		{ client_pubkey, ATHEME_ECDH_X25519_XKEY_LEN },
		{ server_pubkey, ATHEME_ECDH_X25519_XKEY_LEN },
	};

	const size_t secret_vec_len = (sizeof secret_vec) / (sizeof secret_vec[0]);

	unsigned char ikm[DIGEST_MDLEN_SHA2_256];

	if (! digest_oneshot_vector(DIGALG_SHA2_256, secret_vec, secret_vec_len, ikm, NULL))
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: digest_oneshot_vector() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) smemzero(ikm, sizeof ikm);
		return false;
	}
	if (! ECDH_X25519_KDF(ikm, session_salt, better_secret))
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: digest_oneshot_hkdf() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) smemzero(ikm, sizeof ikm);
		return false;
	}

	(void) smemzero(ikm, sizeof ikm);
	return true;
}



#include "ecdh-x25519-challenge-vectors.h"

static bool ATHEME_FATTR_WUR
ecdh_x25519_selftest(void)
{
	unsigned char ecdh_x25519_secret_ab[sizeof ecdh_x25519_secret];
	unsigned char ecdh_x25519_secret_ba[sizeof ecdh_x25519_secret];

	if (! ecdh_x25519_compute_shared(ecdh_x25519_alice_sk, ecdh_x25519_bob_pk, ecdh_x25519_secret_ab))
		// This function logs messages on failure
		return false;

	if (! ecdh_x25519_compute_shared(ecdh_x25519_bob_sk, ecdh_x25519_alice_pk, ecdh_x25519_secret_ba))
		// This function logs messages on failure
		return false;

	if (memcmp(ecdh_x25519_secret_ab, ecdh_x25519_secret, sizeof ecdh_x25519_secret) != 0)
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: memcmp(3) mismatch (alice_sk, bob_pk) (BUG?)",
		                                       MOWGLI_FUNC_NAME);
		return false;
	}
	if (memcmp(ecdh_x25519_secret_ba, ecdh_x25519_secret, sizeof ecdh_x25519_secret) != 0)
	{
		(void) ecdh_x25519_log_error(LG_ERROR, "%s: memcmp(3) mismatch (bob_sk, alice_pk) (BUG?)",
		                                       MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}
