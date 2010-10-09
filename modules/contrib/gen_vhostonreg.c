/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Sets usercloak metadata on register.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"misc/vhostonreg", false, _modinit, _moddeinit,
	"$Revision: 7785 $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* allow us-ascii letters, digits and the following characters */
#define VALID_SPECIALS "-"

static int counter;

static void handle_verify_register(hook_user_req_t *req);
static void hook_user_identify(user_t *u);

void _modinit(module_t *m)
{
	hook_add_event("user_verify_register");
	hook_add_user_verify_register(handle_verify_register);
	hook_add_event("user_identify");
	hook_add_user_identify(hook_user_identify);
	counter = (CURRTIME << 8) % 100000;
	if (counter < 0)
		counter += 100000;
}

void _moddeinit(void)
{
	hook_del_user_verify_register(handle_verify_register);
	hook_del_user_identify(hook_user_identify);
}

static void user_add_host(myuser_t *mu)
{
	int maxlen1, i;
	char newhost[HOSTLEN];
	const char *p;
	bool invalidchar = false;

	maxlen1 = HOSTLEN - 2 - strlen(me.hidehostsuffix);
	if (maxlen1 < 9)
		return;
	p = entity(mu)->name;
	i = 0;
	while (i < maxlen1 && *p != '\0')
	{
		if (isalnum(*p) || strchr(VALID_SPECIALS, *p))
			newhost[i++] = *p;
		else
			invalidchar = true;
		p++;
	}
	if (invalidchar || *p != '\0')
	{
		if (i > maxlen1 - 6)
			i = maxlen1 - 6;
		snprintf(newhost + i, sizeof newhost - i, "-%05d", counter);
		counter++;
		if (counter >= 100000)
			counter = 0;
		if (nicksvs.me != NULL)
		{
			myuser_notice(nicksvs.nick, mu, "Your account name cannot be used in a vhost directly. To ensure uniqueness, a number was added.");
			myuser_notice(nicksvs.nick, mu, "To avoid this, register an account name containing only letters, digits and %s.", VALID_SPECIALS);
			if (!nicksvs.no_nick_ownership && command_find(nicksvs.me->commands, "GROUP"))
				myuser_notice(nicksvs.nick, mu, "If you drop %s you can group it to your new account.", entity(mu)->name);
		}
	}
	else
		newhost[i] = '\0';
	strlcat(newhost, ".", sizeof newhost);
	strlcat(newhost, me.hidehostsuffix, sizeof newhost);

	metadata_add(mu, "private:usercloak", newhost);
}

static void handle_verify_register(hook_user_req_t *req)
{
	myuser_t *mu = req->mu;
	mowgli_node_t *n;
	user_t *u;

	if (me.hidehostsuffix == NULL)
		return;

	user_add_host(mu);

	MOWGLI_ITER_FOREACH(n, mu->logins.head)
	{
		u = n->data;
		hook_call_user_identify(u); /* XXX */
	}
}

static void hook_user_identify(user_t *u)
{
	/* if they have an existing cloak, don't do anything */
	if ((metadata_find(u->myuser, "private:usercloak")) || (me.hidehostsuffix == NULL))
		return;	

	/* they do not, add one. */
	user_add_host(u->myuser);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
