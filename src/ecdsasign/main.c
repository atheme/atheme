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
main(int argc, char **argv)
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	BIO *in;
	EC_KEY *eckey;
	char challenge[BUFSIZE];
	const unsigned char *workbuf_p;
	unsigned char *sig_buf, *sig_buf_p;
	size_t len;
	unsigned int buf_len, i;

	if (argv[1] == NULL || argv[2] == NULL)
	{
		fprintf(stderr, "usage: %s [keyfile] [base64challenge]\n", argv[0]);
		return EXIT_FAILURE;
	}

	in = BIO_new(BIO_s_file());
	BIO_read_filename(in, argv[1]);
	eckey = PEM_read_bio_ECPrivateKey(in, NULL, NULL, NULL);
	BIO_free(in);

	if (!EC_KEY_check_key(eckey))
	{
		fprintf(stderr, "Key data for %s is inconsistent.\n", argv[1]);
		return EXIT_FAILURE;
	}

	memset(challenge, '\0', sizeof challenge);
	len = base64_decode(argv[2], challenge, sizeof challenge);
	workbuf_p = (unsigned char *) challenge;

	if (len == (size_t) -1)
	{
		fprintf(stderr, "Failed to decode challenge!\n");
		return EXIT_FAILURE;
	}

	buf_len = ECDSA_size(eckey);
	sig_buf = mowgli_alloc(buf_len);
	sig_buf_p = sig_buf;

	if (!ECDSA_sign(0, workbuf_p, len, sig_buf_p, &buf_len, eckey))
	{
		fprintf(stderr, "Failed to sign challenge!\n");
		return EXIT_FAILURE;
	}

	if (base64_encode(sig_buf, buf_len, challenge, sizeof challenge) == (size_t) -1)
	{
		fprintf(stderr, "Failed to encode signature!\n");
		return EXIT_FAILURE;
	}

	printf("%s\n", challenge);

	mowgli_free(sig_buf);

	return EXIT_SUCCESS;
}
