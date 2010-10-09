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
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;
static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"AUTHCOOKIE", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(mechanisms, "saslserv/main", "sasl_mechanisms");
	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit()
{
	mowgli_node_delete(mnode, mechanisms);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	char auth[256];
	char cookie[256];
	myuser_t *mu;

	/* Skip the authzid entirely */
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	message += strlen(message) + 1;

	/* Copy the authcid */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	strcpy(auth, message);
	message += strlen(message) + 1;

	/* Copy the authcookie */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	strlcpy(cookie, message, len + 1);

	/* Done dissecting, now check. */
	if(!(mu = myuser_find(auth)))
		return ASASL_FAIL;

	p->username = strdup(auth);
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
