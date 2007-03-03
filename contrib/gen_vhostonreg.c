/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Sets usercloak metadata on register.
 *
 * $Id: gen_vhostonreg.c 7785 2007-03-03 15:54:32Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"misc/vhostonreg", FALSE, _modinit, _moddeinit,
	"$Revision: 7785 $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* allow us-ascii letters, digits and the following characters */
#define VALID_SPECIALS "-"

static int counter;

static void handle_register(void *vptr);

void _modinit(module_t *m)
{
	hook_add_event("user_register");
	hook_add_hook("user_register", handle_register);
	counter = (CURRTIME << 8) % 100000;
	if (counter < 0)
		counter += 100000;
}

void _moddeinit(void)
{
	hook_del_hook("user_register", handle_register);
}

static void handle_register(void *vptr)
{
	myuser_t *mu = vptr;
	node_t *n;
	user_t *u;
	char newhost[HOSTLEN];
	int maxlen1, i;
	boolean_t invalidchar = FALSE;
	const char *p;

	if (me.hidehostsuffix == NULL)
		return;

	maxlen1 = HOSTLEN - 2 - strlen(me.hidehostsuffix);
	if (maxlen1 < 9)
		return;
	p = mu->name;
	i = 0;
	while (i < maxlen1 && *p != '\0')
	{
		if (isalnum(*p) || strchr(VALID_SPECIALS, *p))
			newhost[i++] = *p;
		else
			invalidchar = TRUE;
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
			if (!nicksvs.no_nick_ownership && command_find(nicksvs.me->cmdtree, "GROUP"))
				myuser_notice(nicksvs.nick, mu, "If you drop %s you can group it to your new account.", mu->name);
		}
	}
	else
		newhost[i] = '\0';
	strlcat(newhost, ".", sizeof newhost);
	strlcat(newhost, me.hidehostsuffix, sizeof newhost);

	metadata_add(mu, METADATA_USER, "private:usercloak", newhost);

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;
		hook_call_event("user_identify", u); /* XXX */
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
