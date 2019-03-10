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

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

int
main(int argc, const char **argv)
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	EC_KEY *pub;
	char workbuf[BUFSIZE];
	const unsigned char *workbuf_p;
	size_t len, i;

	if (argv[1] == NULL)
	{
		fprintf(stderr, "usage: %s [base64key]\n", argv[0]);
		return EXIT_FAILURE;
	}

	memset(workbuf, '\0', sizeof workbuf);

	pub = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	EC_KEY_set_conv_form(pub, POINT_CONVERSION_COMPRESSED);

	len = base64_decode(argv[1], workbuf, sizeof workbuf);
	workbuf_p = (unsigned char *) workbuf;

	if (len == (size_t) -1)
	{
		fprintf(stderr, "Failed to decode key!\n");
		return EXIT_FAILURE;
	}

	o2i_ECPublicKey(&pub, &workbuf_p, len);
	if (!EC_KEY_check_key(pub))
	{
		fprintf(stderr, "Key data provided on commandline is inconsistent.\n");
		return EXIT_FAILURE;
	}

	printf("Public key (reassembled):\n");
	EC_KEY_print_fp(stdout, pub, 4);

	return EXIT_SUCCESS;
}
