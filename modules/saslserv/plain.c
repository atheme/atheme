/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * PLAIN mechanism provider
 */

#include "atheme.h"

static int mech_step(sasl_session_t *, char *, size_t, char **, size_t *);

static const sasl_core_functions_t *sasl_core_functions = NULL;

static sasl_mechanism_t mech = {

	.name           = "PLAIN",
	.mech_start     = NULL,
	.mech_step      = &mech_step,
	.mech_finish    = NULL,
};

static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	char authz[256];
	char authc[256];
	char pass[256];
	myuser_t *mu;
	char *end;

	/* Copy the authzid */
	end = memchr(message, '\0', len);
	if (end == NULL)
		return ASASL_FAIL;
	if (end - message > 255)
		return ASASL_FAIL;
	len -= end - message + 1;
	if (len <= 0)
		return ASASL_FAIL;
	memcpy(authz, message, end - message + 1);
	message = end + 1;

	/* Copy the authcid */
	end = memchr(message, '\0', len);
	if (end == NULL)
		return ASASL_FAIL;
	if (end - message > 255)
		return ASASL_FAIL;
	len -= end - message + 1;
	if (len <= 0)
		return ASASL_FAIL;
	memcpy(authc, message, end - message + 1);
	message = end + 1;

	/* Copy the password */
	end = memchr(message, '\0', len);
	if (end == NULL)
		end = message + len;
	if (end - message > 255)
		return ASASL_FAIL;
	memcpy(pass, message, end - message);
	pass[end - message] = '\0';

	/* Done dissecting, now check. */
	if(!(mu = myuser_find_by_nick(authc)))
		return ASASL_FAIL;

	/* Return ASASL_FAIL before p->username is set,
	   to prevent triggering bad_password(). */
	if (mu->flags & MU_NOPASSWORD)
		return ASASL_FAIL;

	p->username = sstrdup(authc);
	p->authzid = sstrdup(authz);
	return verify_password(mu, pass) ? ASASL_DONE : ASASL_FAIL;
}

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

SIMPLE_DECLARE_MODULE_V1("saslserv/plain", MODULE_UNLOAD_CAPABILITY_OK)
