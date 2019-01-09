/*
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2017 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ECDSA-NIST256P-CHALLENGE mechanism provider.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#define CHALLENGE_LENGTH	DIGEST_MDLEN_SHA2_256
#define CURVE_IDENTIFIER	NID_X9_62_prime256v1

enum ecdsa_step
{
	ECDSA_ST_ACCNAME    = 0,
	ECDSA_ST_RESPONSE   = 1,
};

struct ecdsa_session
{
	EC_KEY          *pubkey;
	unsigned char    challenge[CHALLENGE_LENGTH];
	enum ecdsa_step  step;
};

static const struct sasl_core_functions *sasl_core_functions = NULL;

static unsigned int
mech_start(struct sasl_session *const restrict p, void ATHEME_VATTR_UNUSED **const restrict out,
           size_t ATHEME_VATTR_UNUSED *const restrict outlen)
{
	struct ecdsa_session *const s = smalloc(sizeof *s);

	p->mechdata = s;
	s->pubkey = EC_KEY_new_by_curve_name(CURVE_IDENTIFIER);
	s->step = ECDSA_ST_ACCNAME;

	(void) EC_KEY_set_conv_form(s->pubkey, POINT_CONVERSION_COMPRESSED);

	return ASASL_MORE;
}

static unsigned int
mech_step(struct sasl_session *const restrict p, const void *const restrict in, const size_t inlen,
          void **const restrict out, size_t *const restrict outlen)
{
	if (! (p && p->mechdata && in && inlen))
		return ASASL_ERROR;

	struct ecdsa_session *const s = p->mechdata;

	if (s->step == ECDSA_ST_RESPONSE)
	{
		if (ECDSA_verify(0, s->challenge, CHALLENGE_LENGTH, in, (int) inlen, s->pubkey) != 1)
			return ASASL_FAIL;

		return ASASL_DONE;
	}

	char authcid[NICKLEN + 1];
	(void) memset(authcid, 0x00, sizeof authcid);

	const char *const end = memchr(in, 0x00, inlen);
	if (! end)
	{
		if (inlen > NICKLEN)
			return ASASL_ERROR;

		(void) memcpy(authcid, in, inlen);
	}
	else
	{
		char authzid[NICKLEN + 1];
		(void) memset(authzid, 0x00, sizeof authzid);

		const char *const accnames = in;

		const size_t authcid_length = (size_t) (end - accnames);
		const size_t authzid_length = inlen - 1 - authcid_length;

		if (! authcid_length || authcid_length > NICKLEN)
			return ASASL_ERROR;

		if (! authzid_length || authzid_length > NICKLEN)
			return ASASL_ERROR;

		(void) memcpy(authcid, accnames, authcid_length);
		(void) memcpy(authzid, end + 1, authzid_length);

		if (! sasl_core_functions->authzid_can_login(p, authzid, NULL))
			return ASASL_ERROR;
	}

	struct myuser *mu = NULL;
	if (! sasl_core_functions->authcid_can_login(p, authcid, &mu))
		return ASASL_ERROR;

	struct metadata *md;
	if (! (md = metadata_find(mu, "private:pubkey")) && ! (md = metadata_find(mu, "pubkey")))
		return ASASL_ERROR;

	unsigned char pubkey_raw[0x1000];
	(void) memset(pubkey_raw, 0x00, sizeof pubkey_raw);

	const size_t ret = base64_decode(md->value, pubkey_raw, sizeof pubkey_raw);
	if (ret == (size_t) -1)
		return ASASL_ERROR;

	const unsigned char *pubkey_raw_p = pubkey_raw;
	if (! o2i_ECPublicKey(&s->pubkey, &pubkey_raw_p, (long) ret))
		return ASASL_ERROR;

	*out = smalloc(CHALLENGE_LENGTH);
	*outlen = CHALLENGE_LENGTH;

	(void) atheme_random_buf(s->challenge, CHALLENGE_LENGTH);
	(void) memcpy(*out, s->challenge, CHALLENGE_LENGTH);

	s->step = ECDSA_ST_RESPONSE;
	return ASASL_MORE;
}

static void
mech_finish(struct sasl_session *const restrict p)
{
	if (! (p && p->mechdata))
		return;

	struct ecdsa_session *const s = p->mechdata;

	if (s->pubkey)
		(void) EC_KEY_free(s->pubkey);

	(void) sfree(s);

	p->mechdata = NULL;
}

static const struct sasl_mechanism mech = {

	.name           = "ECDSA-NIST256P-CHALLENGE",
	.mech_start     = &mech_start,
	.mech_step      = &mech_step,
	.mech_finish    = &mech_finish,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_pubkey");
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions");

	(void) sasl_core_functions->mech_register(&mech);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) sasl_core_functions->mech_unregister(&mech);
}

#else /* HAVE_OPENSSL */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires OpenSSL support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_OPENSSL */

SIMPLE_DECLARE_MODULE_V1("saslserv/ecdsa-nist256p-challenge", MODULE_UNLOAD_CAPABILITY_OK)
