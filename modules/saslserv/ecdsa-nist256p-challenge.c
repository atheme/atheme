/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * ECDSA-NIST256P-CHALLENGE mechanism provider.
 */

#include <atheme.h>

#ifdef HAVE_LIBCRYPTO_ECDSA

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#define CHALLENGE_LENGTH	32U
#define CURVE_IDENTIFIER	NID_X9_62_prime256v1

struct sasl_ecdsa_nist256p_challenge_session
{
	unsigned char    challenge[CHALLENGE_LENGTH];
	EC_KEY          *pubkey;
};

static const struct sasl_core_functions *sasl_core_functions = NULL;

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_ecdsa_step_account_names(struct sasl_session *const restrict p,
                                   const struct sasl_input_buf *const restrict in,
                                   struct sasl_output_buf *const restrict out)
{
	EC_KEY *pubkey = NULL;
	struct myuser *mu = NULL;

	const char *const end = memchr(in->buf, 0x00, in->len);

	if (! end)
	{
		if (in->len > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: in->len (%zu) is unacceptable", MOWGLI_FUNC_NAME, in->len);
			return ASASL_MRESULT_ERROR;
		}
		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
		{
			(void) slog(LG_DEBUG, "%s: authcid_can_login failed", MOWGLI_FUNC_NAME);
			return ASASL_MRESULT_ERROR;
		}
	}
	else
	{
		const char *const bufchrptr = in->buf;
		const size_t authcid_length = (size_t) (end - bufchrptr);
		const size_t authzid_length = in->len - 1 - authcid_length;

		if (! authcid_length || authcid_length > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: authcid_length (%zu) is unacceptable",
			                      MOWGLI_FUNC_NAME, authcid_length);
			return ASASL_MRESULT_ERROR;
		}
		if (! authzid_length || authzid_length > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: authzid_length (%zu) is unacceptable",
			                      MOWGLI_FUNC_NAME, authzid_length);
			return ASASL_MRESULT_ERROR;
		}
		if (! sasl_core_functions->authzid_can_login(p, end + 1, NULL))
		{
			(void) slog(LG_DEBUG, "%s: authzid_can_login failed", MOWGLI_FUNC_NAME);
			return ASASL_MRESULT_ERROR;
		}
		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
		{
			(void) slog(LG_DEBUG, "%s: authcid_can_login failed", MOWGLI_FUNC_NAME);
			return ASASL_MRESULT_ERROR;
		}
	}
	if (! mu)
	{
		(void) slog(LG_ERROR, "%s: authcid_can_login did not set 'mu' (BUG)", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	struct metadata *md;
	if (! (md = metadata_find(mu, "private:pubkey")) && ! (md = metadata_find(mu, "pubkey")))
	{
		(void) slog(LG_DEBUG, "%s: metadata_find() failed", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	unsigned char pubkey_raw[0x1000];
	const unsigned char *pubkey_raw_p = pubkey_raw;
	const size_t ret = base64_decode(md->value, pubkey_raw, sizeof pubkey_raw);
	if (ret == BASE64_FAIL)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode() failed", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	if (! (pubkey = EC_KEY_new_by_curve_name(CURVE_IDENTIFIER)))
	{
		(void) slog(LG_ERROR, "%s: EC_KEY_new_by_curve_name() failed", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	(void) EC_KEY_set_conv_form(pubkey, POINT_CONVERSION_COMPRESSED);

	if (! o2i_ECPublicKey(&pubkey, &pubkey_raw_p, (long) ret))
	{
		(void) slog(LG_DEBUG, "%s: o2i_ECPublicKey() failed", MOWGLI_FUNC_NAME);
		(void) EC_KEY_free(pubkey);
		return ASASL_MRESULT_ERROR;
	}

	struct sasl_ecdsa_nist256p_challenge_session *const s = smalloc(sizeof *s);
	(void) atheme_random_buf(s->challenge, sizeof s->challenge);
	s->pubkey = pubkey;

	out->buf = s->challenge;
	out->len = sizeof s->challenge;

	p->mechdata = s;

	return ASASL_MRESULT_CONTINUE;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_ecdsa_step_verify_signature(struct sasl_session *const restrict p,
                                      const struct sasl_input_buf *const restrict in)
{
	struct sasl_ecdsa_nist256p_challenge_session *const s = p->mechdata;

	const int retd = ECDSA_verify(0, s->challenge, sizeof s->challenge, in->buf, (int) in->len, s->pubkey);

	if (retd == 1)
		return ASASL_MRESULT_SUCCESS;
	if (retd != 0)
		return ASASL_MRESULT_ERROR;

	/* Some cryptographic signature abstractions *require* hashing, so make it easier for clients of all types
	 * to sign the challenge. If they are forced to hash it, hash it on this side too and try verifying it
	 * again. NIST FIPS186-4 calls for SHA2-256, which is why this module uses a 32-byte challenge anyway.
	 * We're not going to support any other digests.   <https://github.com/atheme/atheme/issues/684>    -- amdj
	 */
	unsigned char digestbuf[DIGEST_MDLEN_SHA2_256];

	if (! digest_oneshot(DIGALG_SHA2_256, s->challenge, sizeof s->challenge, digestbuf, NULL))
		return ASASL_MRESULT_ERROR;

	const int reth = ECDSA_verify(0, digestbuf, sizeof digestbuf, in->buf, (int) in->len, s->pubkey);

	if (reth == 1)
		return ASASL_MRESULT_SUCCESS;
	if (reth != 0)
		return ASASL_MRESULT_ERROR;

	// The signature is formatted correctly, but is not valid for the challenge or the hashed challenge
	return ASASL_MRESULT_FAILURE;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_ecdsa_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                     struct sasl_output_buf *const restrict out)
{
	if (! (p && in && in->buf && in->len))
		return ASASL_MRESULT_ERROR;

	if (p->mechdata == NULL)
		return sasl_mech_ecdsa_step_account_names(p, in, out);
	else
		return sasl_mech_ecdsa_step_verify_signature(p, in);
}

static void
sasl_mech_ecdsa_finish(struct sasl_session *const restrict p)
{
	if (! (p && p->mechdata))
		return;

	struct sasl_ecdsa_nist256p_challenge_session *const s = p->mechdata;

	if (s->pubkey)
		(void) EC_KEY_free(s->pubkey);

	(void) sfree(s);

	p->mechdata = NULL;
}

static const struct sasl_mechanism sasl_mech_ecdsa = {

	.name           = "ECDSA-NIST256P-CHALLENGE",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_ecdsa_step,
	.mech_finish    = &sasl_mech_ecdsa_finish,
	.password_based = false,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_pubkey")
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions")

	(void) sasl_core_functions->mech_register(&sasl_mech_ecdsa);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) sasl_core_functions->mech_unregister(&sasl_mech_ecdsa);
}

#else /* HAVE_LIBCRYPTO_ECDSA */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires OpenSSL ECDSA support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_LIBCRYPTO_ECDSA */

SIMPLE_DECLARE_MODULE_V1("saslserv/ecdsa-nist256p-challenge", MODULE_UNLOAD_CAPABILITY_OK)
