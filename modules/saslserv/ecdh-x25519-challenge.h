/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * ECDH-X25519-CHALLENGE mechanism shared routines.
 */

#ifndef ATHEME_MOD_SASL_ECDH_X25519_CHALLENGE_H
#define ATHEME_MOD_SASL_ECDH_X25519_CHALLENGE_H 1

#define ATHEME_ECDH_X25519_KDF_INFO_CTX "ECDH-X25519-CHALLENGE"
#define ATHEME_ECDH_X25519_KDF_INFO_LEN 21U

#define ATHEME_ECDH_X25519_XKEY_LEN     32U
#define ATHEME_ECDH_X25519_SALT_LEN     32U
#define ATHEME_ECDH_X25519_CHAL_LEN     32U
#define ATHEME_ECDH_X25519_RAND_LEN     32U

#define ECDH_X25519_KDF(ikm, salt, okm)                                                     \
    digest_oneshot_hkdf(DIGALG_SHA2_256, ikm, DIGEST_MDLEN_SHA2_256,                        \
                        salt, ATHEME_ECDH_X25519_SALT_LEN,                                  \
                        ATHEME_ECDH_X25519_KDF_INFO_CTX, ATHEME_ECDH_X25519_KDF_INFO_LEN,   \
                        okm, ATHEME_ECDH_X25519_CHAL_LEN)

struct ecdh_x25519_server_response_fields
{
	unsigned char   pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char   salt[ATHEME_ECDH_X25519_SALT_LEN];
	unsigned char   challenge[ATHEME_ECDH_X25519_CHAL_LEN];
} ATHEME_SATTR_PACKED;

union ecdh_x25519_server_response
{
	unsigned char                               octets[sizeof(struct ecdh_x25519_server_response_fields)];
	struct ecdh_x25519_server_response_fields   field;
} ATHEME_SATTR_PACKED;

#endif /* !ATHEME_MOD_SASL_ECDH_X25519_CHALLENGE_H */
