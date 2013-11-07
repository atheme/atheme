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

#if defined HAVE_OPENSSL && defined HAVE_OPENSSL_EC_H

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

int main(int argc, const char **argv)
{
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
	base64_encode((const char *)workbuf_p, len, encbuf, BUFSIZE);

	printf("Keypair:\n");
	EC_KEY_print_fp(stdout, prv, 4);

	printf("Private key:\n");
	out = BIO_new_fp(stdout, 0);
	PEM_write_bio_ECPrivateKey(out, prv, NULL, NULL, 0, NULL, NULL);

	printf("Public key (atheme format):\n");
	printf("%s\n", encbuf);

	return EXIT_SUCCESS;
}

#else

int main(int argc, const char **argv)
{
	printf("I'm sorry, you didn't compile Atheme with OpenSSL support.\n");
	return EXIT_SUCCESS;
}

#endif
