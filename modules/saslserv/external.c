/*
 * Copyright (C) 2006-2017 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include "atheme.h"

static const struct sasl_core_functions *sasl_core_functions = NULL;

static unsigned int
mech_step(struct sasl_session *const restrict p, const void *const restrict in, const size_t inlen,
          void ATHEME_VATTR_UNUSED **const restrict out, size_t ATHEME_VATTR_UNUSED *const restrict outlen)
{
	if (! (p && p->certfp))
		return ASASL_ERROR;

	struct mycertfp *const mcfp = mycertfp_find(p->certfp);
	if (! mcfp)
		return ASASL_ERROR;

	if (in && inlen)
	{
		if (inlen > NICKLEN)
			return ASASL_ERROR;

		if (! sasl_core_functions->authzid_can_login(p, in, NULL))
			return ASASL_ERROR;
	}

	const char *const authcid = entity(mcfp->mu)->name;
	if (! sasl_core_functions->authcid_can_login(p, authcid, NULL))
		return ASASL_ERROR;

	return ASASL_DONE;
}

static const struct sasl_mechanism mech = {

	.name           = "EXTERNAL",
	.mech_start     = NULL,
	.mech_step      = &mech_step,
	.mech_finish    = NULL,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions");

	(void) sasl_core_functions->mech_register(&mech);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) sasl_core_functions->mech_unregister(&mech);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/external", MODULE_UNLOAD_CAPABILITY_OK)
