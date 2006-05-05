/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * PLAIN mechanism provider
 *
 * $Id: plain.c 5247 2006-05-05 01:41:08Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/plain", FALSE, _modinit, _moddeinit,
	"$Id: plain.c 5247 2006-05-05 01:41:08Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *mechanisms;
node_t *mnode;
static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"PLAIN", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	mnode = node_create();
	mechanisms = module_locate_symbol("saslserv/main", "sasl_mechanisms");
	node_add(&mech, mnode, mechanisms);
}

void _moddeinit()
{
	node_del(mnode, mechanisms);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	char auth[256];
	char pass[256];
	int n = 0, s = 0;
	myuser_t *mu;
	metadata_t *md;

	/* Skip the authzid entirely */
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	message += strlen(message) + 1;

	/* Copy the authcid */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	strcpy(auth, message);
	len -= strlen(message) + 1;
	if(len <= 0)
		return ASASL_FAIL;
	message += strlen(message) + 1;

	/* Copy the password */
	if(strlen(message) > 255)
		return ASASL_FAIL;
	strcpy(pass, message);

	/* Done dissecting, now check. */
	if(!(mu = myuser_find(auth)))
		return ASASL_FAIL;

	p->username = strdup(auth);
	return verify_password(mu, pass) ? ASASL_DONE : ASASL_FAIL;
}

static void mech_finish(sasl_session_t *p)
{
}

