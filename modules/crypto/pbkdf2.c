/*
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
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

#ifdef HAVE_OPENSSL

DECLARE_MODULE_V1("crypto/pbkdf2", false, _modinit, _moddeinit, PACKAGE_VERSION, "Atheme Development Group <http://www.atheme.org>");

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define ROUNDS		(128000)
#define SALTLEN		(16)

/* This is an implementation of PKCS#5 v2.0 password based encryption key
 * derivation function PBKDF2.
 * SHA1 version verified against test vectors posted by Peter Gutmann
 * <pgut001@cs.auckland.ac.nz> to the PKCS-TNG <pkcs-tng@rsa.com> mailing list.
 */
int PKCS5_PBKDF2_HMAC(const char *pass, int passlen,
			   const unsigned char *salt, int saltlen, int iter,
			   const EVP_MD *digest,
			   int keylen, unsigned char *out)
{
	unsigned char digtmp[EVP_MAX_MD_SIZE], *p, itmp[4];
	int cplen, j, k, tkeylen, mdlen;
	unsigned long i = 1;
	HMAC_CTX hctx;

	mdlen = EVP_MD_size(digest);

	HMAC_CTX_init(&hctx);
	p = out;
	tkeylen = keylen;
	if(!pass)
		passlen = 0;
	else if(passlen == -1)
		passlen = strlen(pass);
	while(tkeylen)
	{
		if(tkeylen > mdlen)
			cplen = mdlen;
		else
			cplen = tkeylen;
		/* We are unlikely to ever use more than 256 blocks (5120 bits!)
		 * but just in case...
		 */
		itmp[0] = (unsigned char)((i >> 24) & 0xff);
		itmp[1] = (unsigned char)((i >> 16) & 0xff);
		itmp[2] = (unsigned char)((i >> 8) & 0xff);
		itmp[3] = (unsigned char)(i & 0xff);
		HMAC_Init_ex(&hctx, pass, passlen, digest, NULL);
		HMAC_Update(&hctx, salt, saltlen);
		HMAC_Update(&hctx, itmp, 4);
		HMAC_Final(&hctx, digtmp, NULL);
		memcpy(p, digtmp, cplen);
		for(j = 1; j < iter; j++)
		{
			HMAC(digest, pass, passlen,
				 digtmp, mdlen, digtmp, NULL);
			for(k = 0; k < cplen; k++)
				p[k] ^= digtmp[k];
		}
		tkeylen-= cplen;
		i++;
		p+= cplen;
	}
	HMAC_CTX_cleanup(&hctx);
	return 1;
}

/*******************************************************************************************/

static const char *pbkdf2_salt(void)
{
	static char buf[SALTLEN + 1];
	char *randstr = random_string(SALTLEN);

	mowgli_strlcpy(buf, randstr, sizeof buf);

	free(randstr);

	return buf;
}

static const char *pbkdf2_crypt(const char *key, const char *salt)
{
	static char outbuf[PASSLEN];
	static unsigned char digestbuf[SHA512_DIGEST_LENGTH];
	int res, iter;

	if (strlen(salt) < SALTLEN)
		salt = pbkdf2_salt();

	memcpy(outbuf, salt, SALTLEN);

	res = PKCS5_PBKDF2_HMAC(key, strlen(key), (const unsigned char *)salt, SALTLEN, ROUNDS, EVP_sha512(), SHA512_DIGEST_LENGTH, digestbuf);

	for (iter = 0; iter < SHA512_DIGEST_LENGTH; iter++)
		sprintf(outbuf + SALTLEN + (iter * 2), "%02x", 255 & digestbuf[iter]);

	return outbuf;
}

static crypt_impl_t pbkdf2_crypt_impl = {
	.id = "pbkdf2",
	.crypt = &pbkdf2_crypt,
	.salt = &pbkdf2_salt
};

void _modinit(module_t *m)
{
	crypt_register(&pbkdf2_crypt_impl);
}

void _moddeinit(module_unload_intent_t intent)
{
	crypt_unregister(&pbkdf2_crypt_impl);
}

#endif
