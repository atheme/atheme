/*
 * Copyright (C) 2015-2017 Aaron M. D. Jones <aaronmdjones@gmail.com>
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

#ifdef HAVE_LIBIDN
#include <stringprep.h>
#endif /* HAVE_LIBIDN */

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "pbkdf2v2.h"

#define ATHEME_SASLPREP_MAXLEN (PASSLEN + 1)

static unsigned int pbkdf2v2_rounds = PBKDF2_ITERCNT_DEF;
static unsigned int pbkdf2v2_saltsz = PBKDF2_SALTLEN_DEF;

unsigned int pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;

static bool
atheme_pbkdf2v2_salt_is_b64(const unsigned int prf)
{
	switch (prf)
	{
		case PBKDF2_PRF_HMAC_SHA1_S64:
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
		case PBKDF2_PRF_SCRAM_SHA1_S64:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			return true;
	}

	return false;
}

static bool
atheme_pbkdf2v2_determine_params(struct pbkdf2v2_dbentry *const restrict dbe)
{
	switch (dbe->a)
	{
		case PBKDF2_PRF_SCRAM_SHA1:
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			dbe->scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA1:
		case PBKDF2_PRF_HMAC_SHA1_S64:
			dbe->md = EVP_sha1();
			dbe->dl = SHA_DIGEST_LENGTH;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_256:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			dbe->scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA2_256:
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
			dbe->md = EVP_sha256();
			dbe->dl = SHA256_DIGEST_LENGTH;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_512:
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			dbe->scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA2_512:
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
			dbe->md = EVP_sha512();
			dbe->dl = SHA512_DIGEST_LENGTH;
			break;

		default:
			(void) slog(LG_DEBUG, "%s: PRF ID '%u' unknown", __func__, dbe->a);
			return false;
	}

	if (! dbe->md)
	{
		(void) slog(LG_ERROR, "%s: dbe->md is NULL", __func__);
		return false;
	}

	return true;
}

static bool
atheme_pbkdf2v2_parse_dbentry(struct pbkdf2v2_dbentry *const restrict dbe, const char *const restrict parameters)
{
	char sdg64[0x1000];
	char ssk64[0x1000];
	char shk64[0x1000];

	(void) memset(dbe, 0x00, sizeof *dbe);

	if (sscanf(parameters, PBKDF2_FS_LOADHASH, &dbe->a, &dbe->c, dbe->salt64, ssk64, shk64) == 5)
	{
		(void) slog(LG_DEBUG, "%s: matched PBKDF2_FS_LOADHASH (SCRAM-SHA)", __func__);

		if (! atheme_pbkdf2v2_determine_params(dbe))
			// This function logs messages on failure
			return false;

		if (! dbe->scram)
		{
			(void) slog(LG_ERROR, "%s: is not a SCRAM-SHA PRF (BUG)", __func__);
			return false;
		}

		if (base64_decode(ssk64, dbe->ssk, sizeof dbe->ssk) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for ssk failed", __func__, ssk64);
			return false;
		}
		if (base64_decode(shk64, dbe->shk, sizeof dbe->shk) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for shk failed", __func__, shk64);
			return false;
		}

#ifndef HAVE_LIBIDN
		(void) slog(LG_INFO, "%s: encountered SCRAM format hash, but GNU libidn is unavailable", __func__);
		(void) slog(LG_INFO, "%s: user logins may fail if they have exotic password characters", __func__);
#endif /* !HAVE_LIBIDN */

		return true;
	}

	(void) memset(dbe, 0x00, sizeof *dbe);

	if (sscanf(parameters, PBKDF2_FN_LOADHASH, &dbe->a, &dbe->c, dbe->salt64, sdg64) == 4)
	{
		(void) slog(LG_DEBUG, "%s: matched PBKDF2_FN_LOADHASH (HMAC-SHA)", __func__);

		if (! atheme_pbkdf2v2_determine_params(dbe))
			// This function logs messages on failure
			return false;

		if (dbe->scram)
		{
			(void) slog(LG_ERROR, "%s: is a SCRAM-SHA PRF (BUG)", __func__);
			return false;
		}

		if (base64_decode(sdg64, dbe->sdg, sizeof dbe->sdg) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for sdg failed", __func__, sdg64);
			return false;
		}

		return true;
	}

	return false;
}

static inline bool
atheme_pbkdf2v2_parameters_sane(const struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (dbe->sl < PBKDF2_SALTLEN_MIN || dbe->sl > PBKDF2_SALTLEN_MAX)
	{
		(void) slog(LG_ERROR, "%s: salt length %zu out of range", __func__, dbe->sl);
		return false;
	}
	if (dbe->c < PBKDF2_ITERCNT_MIN || dbe->c > PBKDF2_ITERCNT_MAX)
	{
		(void) slog(LG_ERROR, "%s: iteration count '%u' out of range", __func__, dbe->c);
		return false;
	}

	return true;
}

static bool
atheme_pbkdf2v2_scram_derive(const struct pbkdf2v2_dbentry *const dbe, const unsigned char *const idg,
                             unsigned char *const restrict csk, unsigned char *const restrict chk)
{
	unsigned char cck[EVP_MAX_MD_SIZE];

	if (csk && ! HMAC(dbe->md, idg, (int) dbe->dl, ServerKeyStr, sizeof ServerKeyStr, csk, NULL))
	{
		(void) slog(LG_ERROR, "%s: HMAC() failed for csk", __func__);
		return false;
	}
	if (chk && ! HMAC(dbe->md, idg, (int) dbe->dl, ClientKeyStr, sizeof ClientKeyStr, cck, NULL))
	{
		(void) slog(LG_ERROR, "%s: HMAC() failed for cck", __func__);
		return false;
	}
	if (chk && EVP_Digest(cck, dbe->dl, chk, NULL, dbe->md, NULL) != 1)
	{
		(void) slog(LG_ERROR, "%s: EVP_Digest(cck) failed for chk", __func__);
		return false;
	}

	return true;
}

static bool
atheme_pbkdf2v2_recrypt(const struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (dbe->a != pbkdf2v2_digest)
	{
		(void) slog(LG_DEBUG, "%s: prf (%u) != default (%u)", __func__, dbe->a, pbkdf2v2_digest);
		return true;
	}

	if (dbe->c != pbkdf2v2_rounds)
	{
		(void) slog(LG_DEBUG, "%s: rounds (%u) != default (%u)", __func__, dbe->c, pbkdf2v2_rounds);
		return true;
	}

	if (atheme_pbkdf2v2_salt_is_b64(dbe->a))
	{
		if (dbe->sl != pbkdf2v2_saltsz)
		{
			(void) slog(LG_DEBUG, "%s: salt length (%zu) != default (%u)", __func__, dbe->sl,
			                      pbkdf2v2_saltsz);
			return true;
		}
	}
	else
	{
		(void) slog(LG_DEBUG, "%s: salt wasn't base64-encoded", __func__);
		return true;
	}

	return false;
}

#ifdef HAVE_LIBIDN

/* **********************************************************************************************
 * These 2 functions are provided for modules/saslserv/scram-sha (RFC 5802, RFC 7677, RFC 4013) *
 * The second function is also used by *this* module for password normalization (in SCRAM mode) *
 *                                                                                              *
 * A structure containing pointers to them appears last, so that it can be imported by the      *
 * SCRAM-SHA module.                                                                            *
 ********************************************************************************************** */

static bool
atheme_pbkdf2v2_scram_dbextract(const char *const restrict parameters, struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (! atheme_pbkdf2v2_parse_dbentry(dbe, parameters))
	{
		(void) slog(LG_DEBUG, "%s: could not extract necessary information from database", __func__);
		return false;
	}

	const bool salt_was_b64 = atheme_pbkdf2v2_salt_is_b64(dbe->a);

	// Ensure that the SCRAM-SHA module knows which one of 2 possible algorithms we're using
	switch (dbe->a)
	{
		case PBKDF2_PRF_HMAC_SHA1:
		case PBKDF2_PRF_HMAC_SHA1_S64:
		case PBKDF2_PRF_SCRAM_SHA1:
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			dbe->a = PBKDF2_PRF_SCRAM_SHA1;
			break;

		case PBKDF2_PRF_HMAC_SHA2_256:
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
		case PBKDF2_PRF_SCRAM_SHA2_256:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			dbe->a = PBKDF2_PRF_SCRAM_SHA2_256;
			break;

		default:
			(void) slog(LG_DEBUG, "%s: unsupported PRF '%u'", __func__, dbe->a);
			return false;
	}

	// Ensure that the SCRAM-SHA module has a base64-encoded salt if it wasn't already so
	if (! salt_was_b64)
	{
		dbe->sl = strlen(dbe->salt64);

		(void) memcpy(dbe->salt, dbe->salt64, dbe->sl);

		if (base64_encode(dbe->salt, dbe->sl, dbe->salt64, sizeof dbe->salt64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for salt", __func__);
			return false;
		}
	}

	if (! dbe->scram)
	{
		if (! atheme_pbkdf2v2_scram_derive(dbe, dbe->sdg, dbe->ssk, dbe->shk))
			// This function logs messages on failure
			return false;

		(void) slog(LG_INFO, "%s: attempting SCRAM-SHA login with regular PBKDF2 credentials", __func__);
	}

	return true;
}

static const char *
atheme_pbkdf2v2_scram_normalize(const char *const restrict input)
{
	static char buf[ATHEME_SASLPREP_MAXLEN];

	(void) memset(buf, 0x00, ATHEME_SASLPREP_MAXLEN);

	if (snprintf(buf, ATHEME_SASLPREP_MAXLEN, "%s", input) >= ATHEME_SASLPREP_MAXLEN)
	{
		(void) slog(LG_DEBUG, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
		return NULL;
	}

	const int ret = stringprep(buf, ATHEME_SASLPREP_MAXLEN, (Stringprep_profile_flags) 0, stringprep_saslprep);

	if (ret != STRINGPREP_OK)
	{
		(void) slog(LG_DEBUG, "%s: %s", __func__, stringprep_strerror((Stringprep_rc) ret));
		return NULL;
	}

	return buf;
}

const struct pbkdf2v2_scram_functions pbkdf2v2_scram_functions = {

	.dbextract      = &atheme_pbkdf2v2_scram_dbextract,
	.normalize      = &atheme_pbkdf2v2_scram_normalize,
};

/* **********************************************************************************************
 * End SCRAM-SHA functions                                                                      *
 ********************************************************************************************** */

#endif /* HAVE_LIBIDN */

static bool
atheme_pbkdf2v2_compute(const char *restrict password, struct pbkdf2v2_dbentry *const restrict dbe)
{
#ifdef HAVE_LIBIDN
	if (dbe->scram && ! (password = atheme_pbkdf2v2_scram_normalize(password)))
		// This function logs messages on failure
		return false;
#endif /* HAVE_LIBIDN */

	const size_t pl = strlen(password);

	if (! pl)
	{
		(void) slog(LG_DEBUG, "%s: password length == 0", __func__);
		return false;
	}

	const int ret = PKCS5_PBKDF2_HMAC(password, (int) pl, dbe->salt, (int) dbe->sl, (int) dbe->c,
	                                  dbe->md, (int) dbe->dl, dbe->cdg);
	if (ret != 1)
	{
		(void) slog(LG_ERROR, "%s: PKCS5_PBKDF2_HMAC() failed", __func__);
		return false;
	}

	return true;
}

static const char *
atheme_pbkdf2v2_crypt(const char *const restrict password,
                      const char __attribute__((unused)) *const restrict parameters)
{
	struct pbkdf2v2_dbentry dbe;
	(void) memset(&dbe, 0x00, sizeof dbe);
	(void) arc4random_buf(dbe.salt, (size_t) pbkdf2v2_saltsz);

	if (base64_encode(dbe.salt, (size_t) pbkdf2v2_saltsz, dbe.salt64, sizeof dbe.salt64) == (size_t) -1)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() failed (BUG)", __func__);
		return NULL;
	}

	dbe.a = pbkdf2v2_digest;
	dbe.c = pbkdf2v2_rounds;

	if (! atheme_pbkdf2v2_determine_params(&dbe))
		// This function logs messages on failure
		return NULL;

	if (! atheme_pbkdf2v2_compute(password, &dbe))
		// This function logs messages on failure
		return NULL;

	static char res[PASSLEN];

	if (dbe.scram)
	{
		unsigned char csk[EVP_MAX_MD_SIZE];
		unsigned char chk[EVP_MAX_MD_SIZE];
		char csk64[EVP_MAX_MD_SIZE * 3];
		char chk64[EVP_MAX_MD_SIZE * 3];

		if (! atheme_pbkdf2v2_scram_derive(&dbe, dbe.cdg, csk, chk))
			// This function logs messages on failure
			return NULL;

		if (base64_encode(csk, dbe.dl, csk64, sizeof csk64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for csk", __func__);
			return NULL;
		}
		if (base64_encode(chk, dbe.dl, chk64, sizeof chk64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for chk", __func__);
			return NULL;
		}
		if (snprintf(res, PASSLEN, PBKDF2_FS_SAVEHASH, dbe.a, dbe.c, dbe.salt64, csk64, chk64) >= PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
			return NULL;
		}
	}
	else
	{
		char cdg64[EVP_MAX_MD_SIZE * 3];

		if (base64_encode(dbe.cdg, dbe.dl, cdg64, sizeof cdg64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for cdg", __func__);
			return NULL;
		}
		if (snprintf(res, PASSLEN, PBKDF2_FN_SAVEHASH, dbe.a, dbe.c, dbe.salt64, cdg64) >= PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
			return NULL;
		}
	}

	return res;
}

static bool
atheme_pbkdf2v2_verify(const char *const restrict password, const char *const restrict parameters,
                       unsigned int *const restrict flags)
{
	struct pbkdf2v2_dbentry dbe;

	if (! atheme_pbkdf2v2_parse_dbentry(&dbe, parameters))
	{
		(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", __func__);
		return false;
	}

	if (atheme_pbkdf2v2_salt_is_b64(dbe.a))
	{
		if ((dbe.sl = base64_decode(dbe.salt64, dbe.salt, sizeof dbe.salt)) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for salt failed", __func__, dbe.salt64);
			return false;
		}

		if (! atheme_pbkdf2v2_parameters_sane(&dbe))
			// This function logs messages on failure
			return false;
	}
	else
	{
		dbe.sl = strlen(dbe.salt64);

		if (! atheme_pbkdf2v2_parameters_sane(&dbe))
			// This function logs messages on failure
			return false;

		(void) memcpy(dbe.salt, dbe.salt64, dbe.sl);
	}

	*flags |= PWVERIFY_FLAG_MYMODULE;

	if (! atheme_pbkdf2v2_compute(password, &dbe))
		// This function logs messages on failure
		return false;

	if (dbe.scram)
	{
		unsigned char csk[EVP_MAX_MD_SIZE];

		if (! atheme_pbkdf2v2_scram_derive(&dbe, dbe.cdg, csk, NULL))
			// This function logs messages on failure
			return false;

		if (memcmp(dbe.ssk, csk, dbe.dl) != 0)
		{
			(void) slog(LG_DEBUG, "%s: memcmp(3) mismatch on ssk (invalid password?)", __func__);
			return false;
		}
	}
	else
	{
		if (memcmp(dbe.sdg, dbe.cdg, dbe.dl) != 0)
		{
			(void) slog(LG_DEBUG, "%s: memcmp(3) mismatch on sdg (invalid password?)", __func__);
			return false;
		}
	}

	if (atheme_pbkdf2v2_recrypt(&dbe))
		*flags |= PWVERIFY_FLAG_RECRYPT;

	return true;
}

static int
c_ci_pbkdf2v2_digest(mowgli_config_file_entry_t *const restrict ce)
{
	if (! ce->vardata)
	{
		(void) conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (! strcasecmp(ce->vardata, "SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512_S64;
#ifdef HAVE_LIBIDN
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_256_S64;
/*	// No specification
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_512_S64;
*/
#endif /* HAVE_LIBIDN */
	else
	{
		(void) conf_report_warning(ce, "invalid parameter for configuration option -- using default");
		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
	}

	return 0;
}

static crypt_impl_t crypto_pbkdf2v2_impl = {

	.id         = "pbkdf2v2",
	.crypt      = &atheme_pbkdf2v2_crypt,
	.verify     = &atheme_pbkdf2v2_verify,
};

static mowgli_list_t pbkdf2v2_conf_table;

static void
mod_init(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_pbkdf2v2_impl);

	(void) add_subblock_top_conf("PBKDF2V2", &pbkdf2v2_conf_table);
	(void) add_conf_item("DIGEST", &pbkdf2v2_conf_table, c_ci_pbkdf2v2_digest);
	(void) add_uint_conf_item("ROUNDS", &pbkdf2v2_conf_table, 0, &pbkdf2v2_rounds,
	                          PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_MAX, PBKDF2_ITERCNT_DEF);
	(void) add_uint_conf_item("SALTLEN", &pbkdf2v2_conf_table, 0, &pbkdf2v2_saltsz,
	                          PBKDF2_SALTLEN_MIN, PBKDF2_SALTLEN_MAX, PBKDF2_SALTLEN_DEF);
}

static void
mod_deinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) del_conf_item("DIGEST", &pbkdf2v2_conf_table);
	(void) del_conf_item("ROUNDS", &pbkdf2v2_conf_table);
	(void) del_conf_item("SALTLEN", &pbkdf2v2_conf_table);
	(void) del_top_conf("PBKDF2V2");

	(void) crypt_unregister(&crypto_pbkdf2v2_impl);
}

SIMPLE_DECLARE_MODULE_V1(PBKDF2V2_CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)

#endif /* HAVE_OPENSSL */
