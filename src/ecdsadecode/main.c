/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"
#include "libathemecore.h"

#ifdef HAVE_LIBSODIUM
#  include <sodium/core.h>
#endif /* HAVE_LIBSODIUM */

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

int
main(int argc, const char **argv)
{
#ifdef HAVE_LIBSODIUM
	if (sodium_init() == -1)
	{
		(void) fprintf(stderr, "Error: sodium_init() failed!\n");
		return EXIT_FAILURE;
	}
#endif /* HAVE_LIBSODIUM */

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
