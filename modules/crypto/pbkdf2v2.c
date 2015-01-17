/*
 * Copyright (C) 2015 Aaron Jones <aaronmdjones@gmail.com>
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

DECLARE_MODULE_V1("crypto/pbkdf2v2", false, _modinit, _moddeinit,
                  PACKAGE_VERSION, "Aaron Jones <aaronmdjones@gmail.com>");

#include <openssl/evp.h>

/*
 * You can change the 2 values below without invalidating old hashes
 */

#define PBKDF2_PRF_DEF		6
#define PBKDF2_ITER_DEF		64000

/*
 * Do not change anything below this line unless you know what you are doing,
 * AND how it will (possibly) break backward-, forward-, or cross-compatibility
 */

#define PBKDF2_SALTLEN		16
#define PBKDF2_F_SCAN		"$z$%u$%u$%16[A-Za-z0-9]$"
#define PBKDF2_F_SALT		"$z$%u$%u$%s$"
#define PBKDF2_F_PRINT		"$z$%u$%u$%s$%s"

static const char salt_chars[62] =
	"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";

/*
 * This equivalent implementation provided incase the user doesn't
 * have a new enough OpenSSL library installed on their machine
 */
int PKCS5_PBKDF2_HMAC(const char *pass, int pl,
                      const unsigned char *salt, int sl,
                      int iter, const EVP_MD* PRF,
                      int dkLen, unsigned char *out)
{
	if (! pass)
		pl = 0;

	if (pass && pl < 0)
		pl = strlen(pass);

	int tkLen = dkLen;
	int mdLen = EVP_MD_size(PRF);
	unsigned char* p = out;
	unsigned long i = 1;

	HMAC_CTX hctx;
	HMAC_CTX_init(&hctx);

	while (tkLen) {

		unsigned char itmp[4];
		itmp[0] = (unsigned char) ((i >> 24) & 0xFF);
		itmp[1] = (unsigned char) ((i >> 16) & 0xFF);
		itmp[2] = (unsigned char) ((i >>  8) & 0xFF);
		itmp[3] = (unsigned char) ((i >>  0) & 0xFF);
		i++;

		unsigned char digtmp[EVP_MAX_MD_SIZE];
		HMAC_Init_ex(&hctx, pass, pl, PRF, NULL);
		HMAC_Update(&hctx, salt, sl);
		HMAC_Update(&hctx, itmp, 4);
		HMAC_Final(&hctx, digtmp, NULL);

		int cpLen = (tkLen > mdLen) ? mdLen : tkLen;
		memcpy(p, digtmp, cpLen);

		int j, k;
		for (j = 1; j < iter; j++) {
			HMAC(PRF, pass, pl, digtmp, mdLen, digtmp, NULL);
			for (k = 0; k < cpLen; k++)
				p[k] ^= digtmp[k];
		}

		tkLen -= cpLen;
		p += cpLen;
	}

	HMAC_CTX_cleanup(&hctx);

	return 1;
}

static const char* pbkdf2v2_make_salt(void)
{
	char		salt[PBKDF2_SALTLEN + 1];
	static char	result[PASSLEN];

	memset(salt, 0x00, sizeof salt);
	memset(result, 0x00, sizeof result);

	srand(time(NULL));
	for (int i = 0; i < PBKDF2_SALTLEN; i++)
		salt[i] = salt_chars[rand() % sizeof salt_chars];

	(void) snprintf(result, sizeof result, PBKDF2_F_SALT,
	                PBKDF2_PRF_DEF, PBKDF2_ITER_DEF, salt);

	return result;
}

static const char* pbkdf2v2_crypt(const char* pass, const char* crypt_str)
{
	unsigned int	prf = 0, iter = 0;
	char		salt[PBKDF2_SALTLEN + 1];

	const EVP_MD*	md = NULL;
	unsigned char	digest[EVP_MAX_MD_SIZE];
	char		digest_b64[(EVP_MAX_MD_SIZE * 2) + 5];
	static char	result[PASSLEN];

	/* Attempt to extract the PRF, iteration count and salt */
	if (sscanf(crypt_str, PBKDF2_F_SCAN, &prf, &iter, salt) < 3) {

		/*
		 * Didn't get all of the parameters we wanted, the crypt
		 * string must not be for a hash produced by this module.
		 * But we can't just return NULL or an empty string incase
		 * we're being asked to generate a new password hash for a
		 * new user registration (rather than for verification) or
		 * something along those lines. Therefore, generate params.
		 */
		(void) sscanf(pbkdf2v2_make_salt(), PBKDF2_F_SCAN,
		              &prf, &iter, salt);
	}

	/* Look up the digest method corresponding to the PRF */
	switch (prf) {

	case 5:
		md = EVP_sha256();
		break;

	case 6:
		md = EVP_sha512();
		break;

	default:
		/* This should match the default PRF */
		md = EVP_sha512();
		break;
	}

	/* Compute the PBKDF2 digest */
	size_t sl = strlen(salt);
	size_t pl = strlen(pass);
	(void) PKCS5_PBKDF2_HMAC(pass, pl, (unsigned char*) salt, sl,
	                         iter, md, EVP_MD_size(md), digest);

	/* Convert the digest to Base 64 */
	memset(digest_b64, 0x00, sizeof digest_b64);
	(void) base64_encode((const char*) digest, EVP_MD_size(md),
	                     digest_b64, sizeof digest_b64);

	/* Format the result */
	memset(result, 0x00, sizeof result);
	(void) snprintf(result, sizeof result, PBKDF2_F_PRINT,
	                prf, iter, salt, digest_b64);

	return result;
}

static crypt_impl_t pbkdf2_crypt_impl = {
	.id = "pbkdf2v2",
	.crypt = &pbkdf2v2_crypt,
	.salt = &pbkdf2v2_make_salt
};

void _modinit(module_t* m)
{
	crypt_register(&pbkdf2_crypt_impl);
}

void _moddeinit(module_unload_intent_t intent)
{
	crypt_unregister(&pbkdf2_crypt_impl);
}

#endif
