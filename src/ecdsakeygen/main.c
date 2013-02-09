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
	BIO *out;
	EC_KEY *prv, *pub;
	unsigned char *workbuf, *workbuf_p;
	char encbuf[BUFSIZE];
	size_t len;

	memset(encbuf, '\0', sizeof encbuf);

	prv = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	pub = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

	EC_KEY_generate_key(prv);

	len = i2o_ECPublicKey(prv, NULL);
	workbuf = mowgli_alloc(workbuf);
	workbuf_p = workbuf;
	i2o_ECPublicKey(prv, &workbuf_p);

	base64_encode(workbuf_p, len, encbuf, BUFSIZE);

	len = base64_decode(encbuf, workbuf, BUFSIZE);
	workbuf_p = workbuf;
	o2i_ECPublicKey(&pub, (const unsigned char **) &workbuf_p, len);

	out = BIO_new(BIO_s_file());
	BIO_set_fp(out, stdout, BIO_NOCLOSE);

	printf("Keypair:\n");
	EC_KEY_print_fp(stdout, pub, 4);

	printf("Private key:\n");
	PEM_write_bio_ECPrivateKey(out, prv, NULL, NULL, 0, NULL, NULL);

	printf("Public key (unserialized):\n");
	PEM_write_bio_EC_PUBKEY(out, prv);

	printf("Public key (reserialized, PEM):\n");
	PEM_write_bio_EC_PUBKEY(out, pub);

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
