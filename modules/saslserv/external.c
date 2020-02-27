/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2017-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include <atheme.h>

static const struct sasl_core_functions *sasl_core_functions = NULL;

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_external_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                        struct sasl_output_buf ATHEME_VATTR_UNUSED *const restrict out)
{
	if (! (p && p->certfp))
		return ASASL_MRESULT_ERROR;

	struct mycertfp *const mcfp = mycertfp_find(p->certfp);

	if (! mcfp)
		return ASASL_MRESULT_ERROR;

	if (in && in->buf && in->len)
	{
		if (in->len > NICKLEN)
			return ASASL_MRESULT_ERROR;

		if (! sasl_core_functions->authzid_can_login(p, in->buf, NULL))
			return ASASL_MRESULT_ERROR;
	}

	const char *const authcid = entity(mcfp->mu)->name;

	if (! sasl_core_functions->authcid_can_login(p, authcid, NULL))
		return ASASL_MRESULT_ERROR;

	return ASASL_MRESULT_SUCCESS;
}

static const struct sasl_mechanism sasl_mech_external = {

	.name           = "EXTERNAL",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_external_step,
	.mech_finish    = NULL,
	.password_based = false,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions")

	(void) sasl_core_functions->mech_register(&sasl_mech_external);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) sasl_core_functions->mech_unregister(&sasl_mech_external);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/external", MODULE_UNLOAD_CAPABILITY_OK)
