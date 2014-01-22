/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * EXTERNAL IRCv3.1 SASL draft mechanism implementation.
 */

#include "atheme.h"
#include "authcookie.h"

DECLARE_MODULE_V1
(
	"saslserv/external", MODULE_UNLOAD_CAPABILITY_NEVER, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;
static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"EXTERNAL", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, mechanisms, "saslserv/main", "sasl_mechanisms");
	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_node_delete(mnode, mechanisms);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	mycertfp_t *mcfp;
	const char *name;
	int namelen;

	if(p->certfp == NULL)
		return ASASL_FAIL;

	mcfp = mycertfp_find(p->certfp);
	if(mcfp == NULL)
		return ASASL_FAIL;

	/* The client response is the authorization identity.
	 * We do not support authenticating as someone else,
	 * so the client response should be either empty or match the
	 * certfp's user.
	 */
	name = entity(mcfp->mu)->name;
	namelen = strlen(name);
	if(len > 0 && (len != namelen || memcmp(name, message, len)))
		return ASASL_FAIL;

	p->username = strdup(name);
	return ASASL_DONE;
}

static void mech_finish(sasl_session_t *p)
{
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
