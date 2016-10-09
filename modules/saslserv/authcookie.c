/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * AUTHCOOKIE mechanism provider
 *
 */

#include "atheme.h"
#include "authcookie.h"

DECLARE_MODULE_V1
(
	"saslserv/authcookie", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

sasl_mech_register_func_t *regfuncs;
static int mech_start(sasl_session_t *p, char **out, size_t *out_len);
static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"AUTHCOOKIE", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, regfuncs, "saslserv/main", "sasl_mech_register_funcs");
	regfuncs->mech_register(&mech);
}

void _moddeinit(module_unload_intent_t intent)
{
	regfuncs->mech_unregister(&mech);
}

static int mech_start(sasl_session_t *p, char **out, size_t *out_len)
{
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
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

static void mech_finish(sasl_session_t *p)
{
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
