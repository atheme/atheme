/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>
#include <atheme/libathemecore.h>

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

int
main(int argc, const char **argv)
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	BIO *out;
	EC_KEY *prv;
	unsigned char *workbuf, *workbuf_p;
	char encbuf[BUFSIZE];
	size_t len;

	memset(encbuf, '\0', sizeof encbuf);

	prv = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

	EC_KEY_set_conv_form(prv, POINT_CONVERSION_COMPRESSED);
	EC_KEY_generate_key(prv);

	len = i2o_ECPublicKey(prv, NULL);
	workbuf = mowgli_alloc(len);
	workbuf_p = workbuf;
	i2o_ECPublicKey(prv, &workbuf_p);
	workbuf_p = workbuf;

	if (base64_encode(workbuf_p, len, encbuf, sizeof encbuf) == (size_t) -1)
	{
		fprintf(stderr, "Failed to encode public key!\n");
		return EXIT_FAILURE;
	}

	printf("Keypair:\n");
	EC_KEY_print_fp(stdout, prv, 4);

	printf("Private key:\n");
	out = BIO_new_fp(stdout, 0);
	PEM_write_bio_ECPrivateKey(out, prv, NULL, NULL, 0, NULL, NULL);

	printf("Public key (atheme format):\n");
	printf("%s\n", encbuf);

	return EXIT_SUCCESS;
}
