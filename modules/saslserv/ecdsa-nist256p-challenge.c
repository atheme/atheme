/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * ECDSA-NIST256P-CHALLENGE mechanism provider.
 */

#include "atheme.h"

#ifdef HAVE_LIBCRYPTO_ECDSA

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#define CHALLENGE_LENGTH	DIGEST_MDLEN_SHA2_256
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
	struct myuser *mu = NULL;

	const char *const end = memchr(in->buf, 0x00, in->len);

	if (! end)
	{
		if (in->len > NICKLEN)
			return ASASL_MRESULT_ERROR;

		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
			return ASASL_MRESULT_ERROR;
	}
	else
	{
		const char *const bufchrptr = in->buf;
		const size_t authcid_length = (size_t) (end - bufchrptr);
		const size_t authzid_length = in->len - 1 - authcid_length;

		if (! authcid_length || authcid_length > NICKLEN)
			return ASASL_MRESULT_ERROR;

		if (! authzid_length || authzid_length > NICKLEN)
			return ASASL_MRESULT_ERROR;

		if (! sasl_core_functions->authzid_can_login(p, end + 1, NULL))
			return ASASL_MRESULT_ERROR;

		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
			return ASASL_MRESULT_ERROR;
	}

	struct metadata *md;
	if (! (md = metadata_find(mu, "private:pubkey")) && ! (md = metadata_find(mu, "pubkey")))
		return ASASL_MRESULT_ERROR;

	unsigned char pubkey_raw[0x1000];
	const unsigned char *pubkey_raw_p = pubkey_raw;
	const size_t ret = base64_decode(md->value, pubkey_raw, sizeof pubkey_raw);
	if (ret == (size_t) -1)
		return ASASL_MRESULT_ERROR;

	EC_KEY *pubkey = EC_KEY_new_by_curve_name(CURVE_IDENTIFIER);
	(void) EC_KEY_set_conv_form(pubkey, POINT_CONVERSION_COMPRESSED);
	if (! o2i_ECPublicKey(&pubkey, &pubkey_raw_p, (long) ret))
		return ASASL_MRESULT_ERROR;

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

	const int ret = ECDSA_verify(0, s->challenge, CHALLENGE_LENGTH, in->buf, (int) in->len, s->pubkey);

	if (ret == 1)
		return ASASL_MRESULT_SUCCESS;
	else if (ret == 0)
		return ASASL_MRESULT_FAILURE;
	else
		return ASASL_MRESULT_ERROR;
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
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_pubkey");
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions");

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
