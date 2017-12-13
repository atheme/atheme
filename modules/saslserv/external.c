/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include "atheme.h"
#include "authcookie.h"

static const struct sasl_core_functions *sasl_core_functions = NULL;

static int
mech_step(struct sasl_session *const restrict p, char *const restrict message, const size_t len,
          char __attribute__((unused)) **const restrict out,
          size_t __attribute__((unused)) *const restrict out_len)
{
	if (! p->certfp)
		return ASASL_FAIL;

	mycertfp_t *const mcfp = mycertfp_find(p->certfp);
	if (! mcfp)
		return ASASL_FAIL;

	if (message && len)
	{
		char authzid[NICKLEN];

		if (! len || len >= sizeof authzid)
			return ASASL_FAIL;

		(void) memset(authzid, 0x00, sizeof authzid);
		(void) memcpy(authzid, message, len);

		if (! sasl_core_functions->authzid_can_login(p, authzid, NULL))
			return ASASL_FAIL;
	}

	const char *const authcid = entity(mcfp->mu)->name;
	if (! sasl_core_functions->authcid_can_login(p, authcid, NULL))
		return ASASL_FAIL;

	return ASASL_DONE;
}

static struct sasl_mechanism mech = {

	.name           = "EXTERNAL",
	.mech_start     = NULL,
	.mech_step      = &mech_step,
	.mech_finish    = NULL,
};

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions");

	(void) sasl_core_functions->mech_register(&mech);
}

static void
mod_deinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) sasl_core_functions->mech_unregister(&mech);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/external", MODULE_UNLOAD_CAPABILITY_OK)
