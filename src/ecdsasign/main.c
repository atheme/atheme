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

#ifdef HAVE_OPENSSL

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

int main(int argc, const char **argv)
{
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

	EC_KEY_print_fp(stdout, eckey, 4);

	memset(challenge, '\0', sizeof challenge);
	len = base64_decode(argv[2], challenge, BUFSIZE);
	workbuf_p = (unsigned char *) challenge;

	buf_len = ECDSA_size(eckey);
	sig_buf = mowgli_alloc(buf_len);
	sig_buf_p = sig_buf;

	if (!ECDSA_sign(0, challenge, len, sig_buf_p, &buf_len, eckey))
	{
		fprintf(stderr, "Failed to sign challenge!\n");
		return EXIT_FAILURE;
	}

	base64_encode(sig_buf, buf_len, challenge, BUFSIZE);
	printf("%s\n", challenge);

	mowgli_free(sig_buf);

	return EXIT_SUCCESS;
}

#else

int main(int argc, const char **argv)
{
	printf("I'm sorry, you didn't compile Atheme with OpenSSL support.\n");
	return EXIT_SUCCESS;
}

#endif
