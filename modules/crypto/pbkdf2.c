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

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define ATHEME_PBKDF2_ROUNDS    128000
#define ATHEME_PBKDF2_SALTLEN   16

static bool
atheme_pbkdf2_verify(const char *const restrict password, const char *const restrict parameters)
{
	if (strlen(parameters) != (ATHEME_PBKDF2_SALTLEN + (2 * SHA512_DIGEST_LENGTH)))
		return false;

	const EVP_MD *const md = EVP_sha512();
	if (!md)
		return false;

	unsigned char buf[SHA512_DIGEST_LENGTH];
	const int ret = PKCS5_PBKDF2_HMAC(password, (int) strlen(password), (const unsigned char *) parameters,
	                                  ATHEME_PBKDF2_SALTLEN, ATHEME_PBKDF2_ROUNDS, md,
	                                  SHA512_DIGEST_LENGTH, buf);
	if (!ret)
		return false;

	char result[(2 * SHA512_DIGEST_LENGTH) + 1];
	for (size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
		(void) sprintf(result + (i * 2), "%02x", 255 & buf[i]);

	if (strcmp(result, parameters + ATHEME_PBKDF2_SALTLEN) != 0)
		return false;

	return true;
}

static crypt_impl_t crypto_pbkdf2_impl = {

	.id         = "pbkdf2",
	.verify     = &atheme_pbkdf2_verify,
};

static void
crypto_pbkdf2_modinit(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_pbkdf2_impl);
}

static void
crypto_pbkdf2_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) crypt_unregister(&crypto_pbkdf2_impl);
}

DECLARE_MODULE_V1("crypto/pbkdf2", false, crypto_pbkdf2_modinit, crypto_pbkdf2_moddeinit,
                  PACKAGE_VERSION, VENDOR_STRING);

#endif
