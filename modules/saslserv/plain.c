/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * PLAIN mechanism provider
 */

#include <atheme.h>

static const struct sasl_core_functions *sasl_core_functions = NULL;

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_plain_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                     struct sasl_output_buf ATHEME_VATTR_UNUSED *const restrict out)
{
	if (! (p && in && in->buf && in->len))
		return ASASL_MRESULT_ERROR;

	// This buffer contains sensitive information
	*(in->flags) |= ASASL_INFLAG_WIPE_BUF;

	// Data format: [authzid] 0x00 authcid 0x00 password [0x00]
	if (in->len > (NICKLEN + 1 + NICKLEN + 1 + PASSLEN + 1))
		return ASASL_MRESULT_ERROR;

	const char *ptr = in->buf;
	const char *const end = ptr + in->len;

	const char *const authzid = ptr;
	if (strlen(authzid) > NICKLEN)
		return ASASL_MRESULT_ERROR;
	if ((ptr += strlen(authzid) + 1) >= end)
		return ASASL_MRESULT_ERROR;

	const char *const authcid = ptr;
	if (! *authcid)
		return ASASL_MRESULT_ERROR;
	if (strlen(authcid) > NICKLEN)
		return ASASL_MRESULT_ERROR;
	if ((ptr += strlen(authcid) + 1) >= end)
		return ASASL_MRESULT_ERROR;

	const char *const secret = ptr;
	if (! *secret)
		return ASASL_MRESULT_ERROR;
	if (strlen(secret) > PASSLEN)
		return ASASL_MRESULT_ERROR;

	if (*authzid && ! sasl_core_functions->authzid_can_login(p, authzid, NULL))
		return ASASL_MRESULT_ERROR;

	struct myuser *mu = NULL;
	if (! sasl_core_functions->authcid_can_login(p, authcid, &mu))
		return ASASL_MRESULT_ERROR;

	if (! verify_password(mu, secret))
		return ASASL_MRESULT_FAILURE;

	return ASASL_MRESULT_SUCCESS;
}

static const struct sasl_mechanism sasl_mech_plain = {

	.name           = "PLAIN",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_plain_step,
	.mech_finish    = NULL,
	.password_based = true,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions")

	(void) sasl_core_functions->mech_register(&sasl_mech_plain);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) sasl_core_functions->mech_unregister(&sasl_mech_plain);
}

SIMPLE_DECLARE_MODULE_V1("saslserv/plain", MODULE_UNLOAD_CAPABILITY_OK)
