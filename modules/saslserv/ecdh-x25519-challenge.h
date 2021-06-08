/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * ECDH-X25519-CHALLENGE mechanism shared routines.
 */

#ifndef ATHEME_MOD_SASL_ECDH_X25519_CHALLENGE_H
#define ATHEME_MOD_SASL_ECDH_X25519_CHALLENGE_H 1

#define ATHEME_ECDH_X25519_XKEY_LEN     32U
#define ATHEME_ECDH_X25519_SALT_LEN     32U
#define ATHEME_ECDH_X25519_CHAL_LEN     32U

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
