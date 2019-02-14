/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock <nenolod@atheme.org>
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include "atheme.h"

static const struct sasl_core_functions *sasl_core_functions = NULL;

static unsigned int ATHEME_FATTR_WUR
mech_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
          struct sasl_output_buf ATHEME_VATTR_UNUSED *const restrict out)
{
	if (! (p && p->certfp))
		return ASASL_ERROR;

	struct mycertfp *const mcfp = mycertfp_find(p->certfp);

	if (! mcfp)
		return ASASL_ERROR;

	if (in && in->buf && in->len)
	{
		if (in->len > NICKLEN)
			return ASASL_ERROR;

		if (! sasl_core_functions->authzid_can_login(p, in->buf, NULL))
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
