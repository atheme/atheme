/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * AUTHCOOKIE mechanism provider
 */

#include "atheme.h"
#include "authcookie.h"

static const sasl_core_functions_t *sasl_core_functions = NULL;

static int
mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	char authz[256];
	char authc[256];
	char cookie[256];
	myuser_t *mu;

	/* Skip the authzid entirely */

	if(strlen(message) > 255)
		return ASASL_FAIL;
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	strcpy(authz, message);
	message += strlen(message) + 1;

	/* Copy the authcid */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	strcpy(authc, message);
	message += strlen(message) + 1;

	/* Copy the authcookie */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	mowgli_strlcpy(cookie, message, len + 1);

	/* Done dissecting, now check. */
	if(!(mu = myuser_find_by_nick(authc)))
		return ASASL_FAIL;

	p->username = sstrdup(authc);
	p->authzid = sstrdup(authz);
	return authcookie_find(cookie, mu) != NULL ? ASASL_DONE : ASASL_FAIL;
}

static sasl_mechanism_t mech = {

	.name           = "AUTHCOOKIE",
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
mod_deinit(const module_unload_intent_t intent)
{
	(void) sasl_core_functions->mech_unregister(&mech);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/authcookie", MODULE_UNLOAD_CAPABILITY_OK)
