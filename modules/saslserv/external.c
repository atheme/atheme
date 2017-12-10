/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include "atheme.h"
#include "authcookie.h"

sasl_mech_register_func_t *regfuncs;
static int mech_start(sasl_session_t *p, char **out, size_t *out_len);
static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"EXTERNAL", &mech_start, &mech_step, &mech_finish};

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, regfuncs, "saslserv/main", "sasl_mech_register_funcs");
	regfuncs->mech_register(&mech);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	regfuncs->mech_unregister(&mech);
}

static int mech_start(sasl_session_t *p, char **out, size_t *out_len)
{
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	mycertfp_t *mcfp;
	const char *name;
	int namelen;

	if(p->certfp == NULL)
		return ASASL_FAIL;

	mcfp = mycertfp_find(p->certfp);
	if(mcfp == NULL)
		return ASASL_FAIL;

	/* The client response is the authorization identity, which is verified
	 * by saslserv/main, therefore the mechanism need not check it. */

	name = entity(mcfp->mu)->name;
	p->username = sstrdup(name);
	p->authzid = sstrdup(message);

	return ASASL_DONE;
}

static void mech_finish(sasl_session_t *p)
{
}

SIMPLE_DECLARE_MODULE_V1("saslserv/external", MODULE_UNLOAD_CAPABILITY_OK)
