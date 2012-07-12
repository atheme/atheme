/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#if defined(HAVE_CRYPT)

static const char *posixc_crypt_string(const char *key, const char *salt)
{
	const char *result;
	char salt2[3];
	static int warned;

	result = crypt(key, salt);
	if (!strncmp(salt, "$1$", 3) && strncmp(result, "$1$", 3))
	{
		if (!warned)
			slog(LG_INFO, "posixc_crypt_string(): broken crypt() detected, falling back to DES");
		warned = 1;
		salt2[0] = salt[3];
		salt2[1] = salt[4];
		salt2[2] = '\0';
		result = crypt(key, salt2);
	}
	return result;
}

static const char *(*crypt_impl_)(const char *key, const char *salt) = &posixc_crypt_string;

#elif defined(HAVE_OPENSSL)

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

/* Yanked out of OpenSSL-1.0.0g apps/passwd.c */
static unsigned const char cov_2char[64] = {
	/* from crypto/des/fcrypt.c */
	0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
	0x36,0x37,0x38,0x39,0x41,0x42,0x43,0x44,
	0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,
	0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,
	0x55,0x56,0x57,0x58,0x59,0x5A,0x61,0x62,
	0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
	0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,
	0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A
};

/* MD5-based password algorithm (should probably be available as a library
 * function; then the static buffer would not be acceptable).
 * For magic string "1", this should be compatible to the MD5-based BSD
 * password algorithm.
 * For 'magic' string "apr1", this is compatible to the MD5-based Apache
 * password algorithm.
 * (Apparently, the Apache password algorithm is identical except that the
 * 'magic' string was changed -- the laziest application of the NIH principle
 * I've ever encountered.)
 */
static const char *openssl_md5crypt(const char *passwd, const char *salt)
{
	const char *magic = "1";
	static char out_buf[6 + 9 + 24 + 2]; /* "$apr1$..salt..$.......md5hash..........\0" */
	unsigned char buf[MD5_DIGEST_LENGTH];
	char *salt_out;
	int n;
	unsigned int i;
	EVP_MD_CTX md,md2;
	size_t passwd_len, salt_len;

	passwd_len = strlen(passwd);
	out_buf[0] = '$';
	out_buf[1] = 0;
	soft_assert(strlen(magic) <= 4); /* "1" or "apr1" */
	strncat(out_buf, magic, 4);
	strncat(out_buf, "$", 1);
	strncat(out_buf, salt, 8);
	soft_assert(strlen(out_buf) <= 6 + 8); /* "$apr1$..salt.." */
	salt_out = out_buf + 2 + strlen(magic);
	salt_len = strlen(salt_out);
	soft_assert(salt_len <= 8);

	EVP_MD_CTX_init(&md);
	EVP_DigestInit_ex(&md,EVP_md5(), NULL);
	EVP_DigestUpdate(&md, passwd, passwd_len);
	EVP_DigestUpdate(&md, "$", 1);
	EVP_DigestUpdate(&md, magic, strlen(magic));
	EVP_DigestUpdate(&md, "$", 1);
	EVP_DigestUpdate(&md, salt_out, salt_len);

	EVP_MD_CTX_init(&md2);
	EVP_DigestInit_ex(&md2,EVP_md5(), NULL);
	EVP_DigestUpdate(&md2, passwd, passwd_len);
	EVP_DigestUpdate(&md2, salt_out, salt_len);
	EVP_DigestUpdate(&md2, passwd, passwd_len);
	EVP_DigestFinal_ex(&md2, buf, NULL);

	for (i = passwd_len; i > sizeof buf; i -= sizeof buf)
		EVP_DigestUpdate(&md, buf, sizeof buf);
	EVP_DigestUpdate(&md, buf, i);

	n = passwd_len;
	while (n)
	{
		EVP_DigestUpdate(&md, (n & 1) ? "\0" : passwd, 1);
		n >>= 1;
	}
	EVP_DigestFinal_ex(&md, buf, NULL);

	for (i = 0; i < 1000; i++)
	{
		EVP_DigestInit_ex(&md2,EVP_md5(), NULL);
		EVP_DigestUpdate(&md2, (i & 1) ? (unsigned const char *) passwd : buf,
		                       (i & 1) ? passwd_len : sizeof buf);
		if (i % 3)
			EVP_DigestUpdate(&md2, salt_out, salt_len);
		if (i % 7)
			EVP_DigestUpdate(&md2, passwd, passwd_len);
		EVP_DigestUpdate(&md2, (i & 1) ? buf : (unsigned const char *) passwd,
		                       (i & 1) ? sizeof buf : passwd_len);
		EVP_DigestFinal_ex(&md2, buf, NULL);
	}

	EVP_MD_CTX_cleanup(&md2);

	{
		/* transform buf into output string */
		unsigned char buf_perm[sizeof buf];
		int dest, source;
		char *output;

		/* silly output permutation */
		for (dest = 0, source = 0; dest < 14; dest++, source = (source + 6) % 17)
			buf_perm[dest] = buf[source];
		buf_perm[14] = buf[5];
		buf_perm[15] = buf[11];
#ifndef PEDANTIC /* Unfortunately, this generates a "no effect" warning */
		soft_assert(16 == sizeof buf_perm);
#endif

		output = salt_out + salt_len;
		soft_assert(output == out_buf + strlen(out_buf));

		*output++ = '$';

		for (i = 0; i < 15; i += 3)
			{
			*output++ = cov_2char[buf_perm[i+2] & 0x3f];
			*output++ = cov_2char[((buf_perm[i+1] & 0xf) << 2) |
				                  (buf_perm[i+2] >> 6)];
			*output++ = cov_2char[((buf_perm[i] & 3) << 4) |
				                  (buf_perm[i+1] >> 4)];
			*output++ = cov_2char[buf_perm[i] >> 2];
			}
		soft_assert(i == 15);
		*output++ = cov_2char[buf_perm[i] & 0x3f];
		*output++ = cov_2char[buf_perm[i] >> 6];
		*output = 0;
		soft_assert(strlen(out_buf) < sizeof(out_buf));
	 }
	EVP_MD_CTX_cleanup(&md);

	return out_buf;
}

static const char *openssl_crypt_string(const char *key, const char *salt)
{
	char real_salt[BUFSIZE];
	char *term;

	mowgli_strlcpy(real_salt, salt + 3, sizeof real_salt);

	term = strrchr(real_salt, '$');
	*term = '\0';

	return openssl_md5crypt(key, real_salt);
}

static const char *(*crypt_impl_)(const char *key, const char *salt) = &openssl_crypt_string;

#else

#warning could not find a crypt impl, sorry (this is stubbed)

static const char *stub_crypt_string(const char *key, const char *salt)
{
	return key;
}

static const char *(*crypt_impl_)(const char *key, const char *salt) = &stub_crypt_string;

#endif

static crypt_impl_t posix_crypt_impl = {
	.id = "posix",
};

void _modinit(module_t *m)
{
	posix_crypt_impl.crypt = crypt_impl_;

#if defined(HAVE_CRYPT) || defined(HAVE_OPENSSL)
	crypt_register(&posix_crypt_impl);
#endif
}

void _moddeinit(module_unload_intent_t intent)
{
#if defined(HAVE_CRYPT) || defined(HAVE_OPENSSL)
	crypt_unregister(&posix_crypt_impl);
#endif
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
