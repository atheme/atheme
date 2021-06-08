/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2015-2018 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

#ifdef HAVE_LIBIDN
#  include <stringprep.h>
#endif /* HAVE_LIBIDN */

#define CRYPTO_MODULE_NAME PBKDF2V2_CRYPTO_MODULE_NAME

static mowgli_list_t **crypto_conf_table = NULL;

static unsigned int pbkdf2v2_digest = 0;
static unsigned int pbkdf2v2_rounds = 0;
static unsigned int pbkdf2v2_saltsz = 0;

static pbkdf2v2_scram_confhook_fn pbkdf2v2_scram_confhook = NULL;

static inline void
atheme_pbkdf2v2_scram_confhook_dispatch(void)
{
	if (! pbkdf2v2_scram_confhook || ! pbkdf2v2_digest || ! pbkdf2v2_rounds || ! pbkdf2v2_saltsz)
		return;

	struct pbkdf2v2_scram_config pbkdf2v2_scram_config = {
		.a      = pbkdf2v2_digest,
		.c      = pbkdf2v2_rounds,
		.sl     = pbkdf2v2_saltsz,
	};

	(void) (*pbkdf2v2_scram_confhook)(&pbkdf2v2_scram_config);
}

static void
atheme_pbkdf2v2_config_ready(void ATHEME_VATTR_UNUSED *const restrict unused)
{
	if (! pbkdf2v2_digest)
		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;

	if (! pbkdf2v2_rounds)
		pbkdf2v2_rounds = PBKDF2_ITERCNT_DEF;

	if (! pbkdf2v2_saltsz)
		pbkdf2v2_saltsz = PBKDF2_SALTLEN_DEF;

	(void) atheme_pbkdf2v2_scram_confhook_dispatch();
}

static inline bool
atheme_pbkdf2v2_salt_is_b64(const unsigned int prf)
{
	switch (prf)
	{
		case PBKDF2_PRF_HMAC_MD5_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA1_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_MD5_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			ATHEME_FALLTHROUGH;
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
		case PBKDF2_PRF_SCRAM_MD5:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_MD5_S64:
			dbe->scram = true;
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_MD5:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_MD5_S64:
			dbe->md = DIGALG_MD5;
			dbe->dl = DIGEST_MDLEN_MD5;
			break;

		case PBKDF2_PRF_SCRAM_SHA1:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			dbe->scram = true;
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA1:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA1_S64:
			dbe->md = DIGALG_SHA1;
			dbe->dl = DIGEST_MDLEN_SHA1;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_256:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			dbe->scram = true;
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_256:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
			dbe->md = DIGALG_SHA2_256;
			dbe->dl = DIGEST_MDLEN_SHA2_256;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_512:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			dbe->scram = true;
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_512:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
			dbe->md = DIGALG_SHA2_512;
			dbe->dl = DIGEST_MDLEN_SHA2_512;
			break;

		default:
			(void) slog(LG_DEBUG, "%s: PRF ID '%u' unknown", MOWGLI_FUNC_NAME, dbe->a);
			return false;
	}

	return true;
}

static bool
atheme_pbkdf2v2_parse_dbentry(struct pbkdf2v2_dbentry *const restrict dbe, const char *const restrict parameters)
{
	char sdg64[BUFSIZE];
	char ssk64[BUFSIZE];
	char shk64[BUFSIZE];

	bool retval = true;

	(void) memset(dbe, 0x00, sizeof *dbe);

	if (sscanf(parameters, PBKDF2_FS_LOADHASH, &dbe->a, &dbe->c, dbe->salt64, ssk64, shk64) == 5)
	{
		(void) slog(LG_DEBUG, "%s: matched PBKDF2_FS_LOADHASH (SCRAM)", MOWGLI_FUNC_NAME);

		if (! atheme_pbkdf2v2_determine_params(dbe))
			// This function logs messages on failure
			goto err;

		if (! dbe->scram)
		{
			(void) slog(LG_ERROR, "%s: is not a SCRAM PRF (BUG)", MOWGLI_FUNC_NAME);
			goto err;
		}

		if (base64_decode(ssk64, dbe->ssk, sizeof dbe->ssk) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for ssk failed (BUG)", MOWGLI_FUNC_NAME, ssk64);
			goto err;
		}
		if (base64_decode(shk64, dbe->shk, sizeof dbe->shk) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for shk failed (BUG)", MOWGLI_FUNC_NAME, shk64);
			goto err;
		}

#ifndef HAVE_LIBIDN
		(void) slog(LG_INFO, "%s: encountered SCRAM format hash, but GNU libidn is unavailable",
		                     MOWGLI_FUNC_NAME);

		(void) slog(LG_INFO, "%s: user logins may fail if they have exotic password characters",
		                     MOWGLI_FUNC_NAME);
#endif /* !HAVE_LIBIDN */

		goto done;
	}

	(void) memset(dbe, 0x00, sizeof *dbe);

	if (sscanf(parameters, PBKDF2_FN_LOADHASH, &dbe->a, &dbe->c, dbe->salt64, sdg64) == 4)
	{
		(void) slog(LG_DEBUG, "%s: matched PBKDF2_FN_LOADHASH (Regular PBKDF2-HMAC)", MOWGLI_FUNC_NAME);

		if (! atheme_pbkdf2v2_determine_params(dbe))
			// This function logs messages on failure
			goto err;

		if (dbe->scram)
		{
			(void) slog(LG_ERROR, "%s: is a SCRAM PRF (BUG)", MOWGLI_FUNC_NAME);
			goto err;
		}

		if (base64_decode(sdg64, dbe->sdg, sizeof dbe->sdg) != dbe->dl)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for sdg failed (BUG)", MOWGLI_FUNC_NAME, sdg64);
			goto err;
		}

		goto done;
	}

	(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", MOWGLI_FUNC_NAME);

err:
	// No need to zero dbe, callers do that if this fails
	retval = false;

done:
	(void) smemzero(sdg64, sizeof sdg64);
	(void) smemzero(ssk64, sizeof ssk64);
	(void) smemzero(shk64, sizeof shk64);
	return retval;
}

static inline bool
atheme_pbkdf2v2_parameters_sane(const struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (dbe->sl < PBKDF2_SALTLEN_MIN || dbe->sl > PBKDF2_SALTLEN_MAX)
	{
		(void) slog(LG_ERROR, "%s: salt length '%zu' out of range", MOWGLI_FUNC_NAME, dbe->sl);
		return false;
	}
	if (! dbe->c)
	{
		(void) slog(LG_ERROR, "%s: iteration count '%u' invalid", MOWGLI_FUNC_NAME, dbe->c);
		return false;
	}

	return true;
}

static bool
atheme_pbkdf2v2_scram_derive(const struct pbkdf2v2_dbentry *const dbe, const unsigned char *const idg,
                             unsigned char *const restrict csk, unsigned char *const restrict chk)
{
	static const char ClientKeyConstant[] = "Client Key";
	static const char ServerKeyConstant[] = "Server Key";

	unsigned char cck[DIGEST_MDLEN_MAX];
	bool retval = false;

	if (csk && ! digest_oneshot_hmac(dbe->md, idg, dbe->dl, ServerKeyConstant, 10U, csk, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_hmac(idg) for csk failed (BUG)", MOWGLI_FUNC_NAME);
		goto end;
	}
	if (chk && ! digest_oneshot_hmac(dbe->md, idg, dbe->dl, ClientKeyConstant, 10U, cck, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_hmac(idg) for cck failed (BUG)", MOWGLI_FUNC_NAME);
		goto end;
	}
	if (chk && ! digest_oneshot(dbe->md, cck, dbe->dl, chk, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot(cck) for chk failed (BUG)", MOWGLI_FUNC_NAME);
		goto end;
	}

	retval = true;

end:
	(void) smemzero(cck, sizeof cck);
	return retval;
}

static bool
atheme_pbkdf2v2_recrypt(const struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (dbe->a != pbkdf2v2_digest)
	{
		(void) slog(LG_DEBUG, "%s: prf (%u) != default (%u)", MOWGLI_FUNC_NAME, dbe->a, pbkdf2v2_digest);
		return true;
	}

	if (dbe->c != pbkdf2v2_rounds)
	{
		(void) slog(LG_DEBUG, "%s: rounds (%u) != default (%u)", MOWGLI_FUNC_NAME, dbe->c, pbkdf2v2_rounds);
		return true;
	}

	if (atheme_pbkdf2v2_salt_is_b64(dbe->a))
	{
		if (dbe->sl != pbkdf2v2_saltsz)
		{
			(void) slog(LG_DEBUG, "%s: salt length (%zu) != default (%u)", MOWGLI_FUNC_NAME, dbe->sl,
			                      pbkdf2v2_saltsz);
			return true;
		}
	}
	else
	{
		(void) slog(LG_DEBUG, "%s: salt wasn't base64-encoded", MOWGLI_FUNC_NAME);
		return true;
	}

	return false;
}

#ifdef HAVE_LIBIDN

/* *************************************************************
 * These 3 functions are provided for modules/saslserv/scram   *
 * (RFC 5802, RFC 7677, RFC 4013). The second function is also *
 * used by *this* module for password normalization.           *
 *                                                             *
 * A structure containing pointers to them appears last,       *
 * so that it can be imported by the SASL SCRAM module.        *
 ************************************************************* */

static bool
atheme_pbkdf2v2_scram_dbextract(const char *const restrict parameters, struct pbkdf2v2_dbentry *const restrict dbe)
{
	if (! atheme_pbkdf2v2_parse_dbentry(dbe, parameters))
		// This function logs messages on failure
		return false;

	const bool salt_was_b64 = atheme_pbkdf2v2_salt_is_b64(dbe->a);

	// Ensure that the SCRAM module knows which one of 3 possible algorithms we're using
	switch (dbe->a)
	{
		case PBKDF2_PRF_HMAC_SHA1:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA1_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA1:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			dbe->a = PBKDF2_PRF_SCRAM_SHA1_S64;
			break;

		case PBKDF2_PRF_HMAC_SHA2_256:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_256:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			dbe->a = PBKDF2_PRF_SCRAM_SHA2_256_S64;
			break;

		case PBKDF2_PRF_HMAC_SHA2_512:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_512:
			ATHEME_FALLTHROUGH;
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			dbe->a = PBKDF2_PRF_SCRAM_SHA2_512_S64;
			break;

		default:
			(void) slog(LG_DEBUG, "%s: unsupported PRF '%u'", MOWGLI_FUNC_NAME, dbe->a);
			return false;
	}

	// Ensure that the SCRAM module has a base64-encoded salt if it wasn't already so
	if (! salt_was_b64)
	{
		dbe->sl = strlen(dbe->salt64);

		(void) memcpy(dbe->salt, dbe->salt64, dbe->sl);

		if (base64_encode(dbe->salt, dbe->sl, dbe->salt64, sizeof dbe->salt64) != BASE64_SIZE_RAW(dbe->sl))
		{
			(void) slog(LG_ERROR, "%s: base64_encode() for salt failed (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	/* If the password hash was not in SCRAM format, then we only have SaltedPassword,
	 * not ServerKey and StoredKey. This means that in order for the login to succeed,
	 * we need to derive the latter 2 from what we have (this is fast and easy).
	 */
	if (! dbe->scram)
	{
		if (! atheme_pbkdf2v2_scram_derive(dbe, dbe->sdg, dbe->ssk, dbe->shk))
			// This function logs messages on failure
			return false;

		/* If the database is breached in the future, SaltedPassword can be used to compute
		 * the ClientKey and impersonate the client to this service (without knowing the
		 * original password). If the login is successful, the SCRAM module will convert
		 * the database entry into a SCRAM format to avoid this. Log a message anyway.
		 */
		(void) slog(LG_INFO, "%s: attempting SASL SCRAM login with regular PBKDF2 credentials",
		                     MOWGLI_FUNC_NAME);
	}

	return true;
}

static bool
atheme_pbkdf2v2_scram_normalize(char *const restrict buf, const size_t buflen)
{
	const int ret = stringprep(buf, buflen, (Stringprep_profile_flags) 0, stringprep_saslprep);

	if (ret != STRINGPREP_OK)
	{
		(void) slog(LG_DEBUG, "%s: %s", MOWGLI_FUNC_NAME, stringprep_strerror((Stringprep_rc) ret));
		return false;
	}

	return true;
}

static void
atheme_pbkdf2v2_scram_confhook(const pbkdf2v2_scram_confhook_fn confhook)
{
	pbkdf2v2_scram_confhook = confhook;

	(void) atheme_pbkdf2v2_scram_confhook_dispatch();
}

extern const struct pbkdf2v2_scram_functions pbkdf2v2_scram_functions;
const struct pbkdf2v2_scram_functions pbkdf2v2_scram_functions = {

	.dbextract      = &atheme_pbkdf2v2_scram_dbextract,
	.normalize      = &atheme_pbkdf2v2_scram_normalize,
	.confhook       = &atheme_pbkdf2v2_scram_confhook,
};

/* *************************************************************
 * End SCRAM functions                                         *
 ************************************************************* */

#endif /* HAVE_LIBIDN */

static bool ATHEME_FATTR_WUR
atheme_pbkdf2v2_compute(const char *const restrict password, struct pbkdf2v2_dbentry *const restrict dbe)
{
	char key[PASSLEN + 1];
	(void) mowgli_strlcpy(key, password, sizeof key);

#ifdef HAVE_LIBIDN
	if (dbe->scram && ! atheme_pbkdf2v2_scram_normalize(key, sizeof key))
	{
		(void) slog(LG_DEBUG, "%s: SASLprep normalization of password failed", MOWGLI_FUNC_NAME);
		(void) smemzero(key, sizeof key);
		return false;
	}
#endif /* HAVE_LIBIDN */

	const size_t kl = strlen(key);

	if (! kl)
	{
		(void) slog(LG_DEBUG, "%s: password length == 0", MOWGLI_FUNC_NAME);
		(void) smemzero(key, sizeof key);
		return false;
	}

	if (! digest_oneshot_pbkdf2(dbe->md, key, kl, dbe->salt, dbe->sl, dbe->c, dbe->cdg, dbe->dl))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_pbkdf2() for cdg failed (BUG)", MOWGLI_FUNC_NAME);
		(void) smemzero(key, sizeof key);
		return false;
	}

	(void) smemzero(key, sizeof key);
	return true;
}

static const char * ATHEME_FATTR_WUR
atheme_pbkdf2v2_crypt(const char *const restrict password,
                      const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	char cdg64[BASE64_SIZE_STR(DIGEST_MDLEN_MAX)];
	char csk64[BASE64_SIZE_STR(DIGEST_MDLEN_MAX)];
	char chk64[BASE64_SIZE_STR(DIGEST_MDLEN_MAX)];
	struct pbkdf2v2_dbentry dbe;

	static char res[PASSLEN + 1];
	const char *retval = res;

	(void) memset(&dbe, 0x00, sizeof dbe);

	dbe.sl = (size_t) pbkdf2v2_saltsz;
	dbe.a = pbkdf2v2_digest;
	dbe.c = pbkdf2v2_rounds;

	(void) atheme_random_buf(dbe.salt, dbe.sl);

	if (base64_encode(dbe.salt, dbe.sl, dbe.salt64, sizeof dbe.salt64) != BASE64_SIZE_RAW(dbe.sl))
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for salt failed (BUG)", MOWGLI_FUNC_NAME);
		goto err;
	}

	if (! atheme_pbkdf2v2_determine_params(&dbe))
		// This function logs messages on failure
		goto err;

	if (! atheme_pbkdf2v2_compute(password, &dbe))
		// This function logs messages on failure
		goto err;

	if (dbe.scram)
	{
		if (! atheme_pbkdf2v2_scram_derive(&dbe, dbe.cdg, dbe.ssk, dbe.shk))
			// This function logs messages on failure
			goto err;

		if (base64_encode(dbe.ssk, dbe.dl, csk64, sizeof csk64) != BASE64_SIZE_RAW(dbe.dl))
		{
			(void) slog(LG_ERROR, "%s: base64_encode() for ServerKey failed (BUG)", MOWGLI_FUNC_NAME);
			goto err;
		}
		if (base64_encode(dbe.shk, dbe.dl, chk64, sizeof chk64) != BASE64_SIZE_RAW(dbe.dl))
		{
			(void) slog(LG_ERROR, "%s: base64_encode() for StoredKey failed (BUG)", MOWGLI_FUNC_NAME);
			goto err;
		}
		if (snprintf(res, sizeof res, PBKDF2_FS_SAVEHASH, dbe.a, dbe.c, dbe.salt64, csk64, chk64) > PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) output would have been too long (BUG)",
			                      MOWGLI_FUNC_NAME);
			goto err;
		}
	}
	else
	{
		if (base64_encode(dbe.cdg, dbe.dl, cdg64, sizeof cdg64) != BASE64_SIZE_RAW(dbe.dl))
		{
			(void) slog(LG_ERROR, "%s: base64_encode() for cdg failed (BUG)", MOWGLI_FUNC_NAME);
			goto err;
		}
		if (snprintf(res, sizeof res, PBKDF2_FN_SAVEHASH, dbe.a, dbe.c, dbe.salt64, cdg64) > PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) output would have been too long (BUG)",
			                      MOWGLI_FUNC_NAME);
			goto err;
		}
	}

	goto done;

err:
	retval = NULL;
	(void) smemzero(res, sizeof res);

done:
	(void) smemzero(cdg64, sizeof cdg64);
	(void) smemzero(csk64, sizeof csk64);
	(void) smemzero(chk64, sizeof chk64);
	(void) smemzero(&dbe, sizeof dbe);
	return retval;
}

static bool ATHEME_FATTR_WUR
atheme_pbkdf2v2_verify(const char *const restrict password, const char *const restrict parameters,
                       unsigned int *const restrict flags)
{
	unsigned char csk[DIGEST_MDLEN_MAX];
	struct pbkdf2v2_dbentry dbe;
	bool retval = false;

	if (! atheme_pbkdf2v2_parse_dbentry(&dbe, parameters))
		// This function logs messages on failure
		goto end;

	if (atheme_pbkdf2v2_salt_is_b64(dbe.a))
	{
		if ((dbe.sl = base64_decode(dbe.salt64, dbe.salt, sizeof dbe.salt)) == BASE64_FAIL)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for salt failed", MOWGLI_FUNC_NAME, dbe.salt64);
			goto end;
		}

		if (! atheme_pbkdf2v2_parameters_sane(&dbe))
			// This function logs messages on failure
			goto end;
	}
	else
	{
		dbe.sl = strlen(dbe.salt64);

		if (! atheme_pbkdf2v2_parameters_sane(&dbe))
			// This function logs messages on failure
			goto end;

		(void) memcpy(dbe.salt, dbe.salt64, dbe.sl);
	}

	*flags |= PWVERIFY_FLAG_MYMODULE;

	if (! atheme_pbkdf2v2_compute(password, &dbe))
		// This function logs messages on failure
		goto end;

	if (dbe.scram)
	{
		if (! atheme_pbkdf2v2_scram_derive(&dbe, dbe.cdg, csk, NULL))
			// This function logs messages on failure
			goto end;

		if (smemcmp(dbe.ssk, csk, dbe.dl) != 0)
		{
			(void) slog(LG_DEBUG, "%s: smemcmp() mismatch on ssk (invalid password?)", MOWGLI_FUNC_NAME);
			goto end;
		}
	}
	else
	{
		if (smemcmp(dbe.sdg, dbe.cdg, dbe.dl) != 0)
		{
			(void) slog(LG_DEBUG, "%s: smemcmp() mismatch on sdg (invalid password?)", MOWGLI_FUNC_NAME);
			goto end;
		}
	}

	if (atheme_pbkdf2v2_recrypt(&dbe))
		*flags |= PWVERIFY_FLAG_RECRYPT;

	retval = true;

end:
	(void) smemzero(csk, sizeof csk);
	(void) smemzero(&dbe, sizeof dbe);
	return retval;
}

static int
c_ci_pbkdf2v2_digest(mowgli_config_file_entry_t *const restrict ce)
{
	if (! ce->vardata)
	{
		(void) conf_report_warning(ce, "no parameter for configuration option -- using default");

		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
		return 0;
	}

#ifndef HAVE_LIBIDN
	if (! strncasecmp(ce->vardata, "SCRAM-", 6))
	{
		(void) conf_report_warning(ce, "SCRAM mode is unavailable (GNU libidn is missing) -- using default");

		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
		return 0;
	}
#endif

	// Most of these are aliases, for compatibility. Having them around is harmless.   -- amdj

	if (! strcasecmp(ce->vardata, "SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SHA-1"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SHA2-256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SHA-256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SHA2-512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512_S64;
	else if (! strcasecmp(ce->vardata, "SHA-512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512_S64;
	else if (! strcasecmp(ce->vardata, "SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-1"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA2-256"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-256"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA2-512"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_512_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-512"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_512_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_512_S64;
	else
	{
		(void) conf_report_warning(ce, "invalid parameter for configuration option -- using default");

		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
	}

	return 0;
}

static const struct crypt_impl crypto_pbkdf2v2_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.crypt      = &atheme_pbkdf2v2_crypt,
	.verify     = &atheme_pbkdf2v2_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	(void) crypt_register(&crypto_pbkdf2v2_impl);

	(void) add_conf_item("pbkdf2v2_digest", *crypto_conf_table, &c_ci_pbkdf2v2_digest);

	(void) add_uint_conf_item("pbkdf2v2_rounds", *crypto_conf_table, 0, &pbkdf2v2_rounds,
	                          PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_MAX, PBKDF2_ITERCNT_DEF);

	(void) add_uint_conf_item("pbkdf2v2_saltlen", *crypto_conf_table, 0, &pbkdf2v2_saltsz,
	                          PBKDF2_SALTLEN_MIN, PBKDF2_SALTLEN_MAX, PBKDF2_SALTLEN_DEF);

	(void) hook_add_config_ready(&atheme_pbkdf2v2_config_ready);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_config_ready(&atheme_pbkdf2v2_config_ready);

	(void) del_conf_item("pbkdf2v2_digest", *crypto_conf_table);
	(void) del_conf_item("pbkdf2v2_rounds", *crypto_conf_table);
	(void) del_conf_item("pbkdf2v2_saltlen", *crypto_conf_table);

	(void) crypt_unregister(&crypto_pbkdf2v2_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
