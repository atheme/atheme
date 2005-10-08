/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * VHost management! (ratbox only right now.)
 *
 * $Id: vhost.c 2773 2005-10-08 20:32:23Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/vhost", FALSE, _modinit, _moddeinit,
	"$Id: vhost.c 2773 2005-10-08 20:32:23Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree;

static void vhost_on_identify(void *vptr);
static void ns_cmd_vhost(char *origin);

command_t ns_vhost = { "VHOST", "Manages user virtualhosts.",
			AC_IRCOP, ns_cmd_vhost };

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	hook_add_event("user_identify");
	hook_add_hook("user_identify", vhost_on_identify);
	command_add(&ns_vhost, ns_cmdtree);
}

void _moddeinit(void)
{
	hook_del_hook("user_identify", vhost_on_identify);
	command_delete(&ns_vhost, ns_cmdtree);
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		strlcpy(u->vhost, host, HOSTLEN);
		notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
			host);
		sethost_sts(nicksvs.nick, u->nick, host);
	}
}

static void do_sethost(user_t *u, char *host)
{
	strlcpy(u->vhost, host, HOSTLEN);
	notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
		host);
	sethost_sts(nicksvs.nick, u->nick, host);
}

static void do_restorehost_all(myuser_t *mu)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		strlcpy(u->vhost, u->host, HOSTLEN);
		notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
			u->host);
		sethost_sts(nicksvs.nick, u->nick, u->host);
	}
}

static void do_restorehost(user_t *u)
{
	strlcpy(u->vhost, u->host, HOSTLEN);
	notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
		u->host);
	sethost_sts(nicksvs.nick, u->nick, u->host);
}

/* VHOST <nick> [host] */
static void ns_cmd_vhost(char *origin)
{
	char *target = strtok(NULL, " ");
	char *host = strtok(NULL, " ");
	node_t *n;
	user_t *u;
	myuser_t *mu;

	if (!target)
	{
		notice(nicksvs.nick, origin, "Invalid parameters for \2VHOST\2.");
		notice(nicksvs.nick, origin, "Syntax: VHOST <nick> [vhost]");
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		metadata_delete(mu, METADATA_USER, "private:usercloak");
		notice(nicksvs.nick, origin, "Deleted vhost for \2%s\2.", target);
		snoop("VHOST:REMOVE: \2%s\2 by \2%s\2", target, origin);
		do_restorehost_all(mu);
		return;
	}

	metadata_add(mu, METADATA_USER, "private:usercloak", host);
	notice(nicksvs.nick, origin, "Assigned vhost \2%s\2 to \2%s\2.", 
		host, target);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, target, origin);
	do_sethost_all(mu, host);
	return;
}

static void vhost_on_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	metadata_t *md;

	/* NO CLOAK?!*$*%*&&$(!& */
	if (!(md = metadata_find(mu, METADATA_USER, "private:usercloak")))
		return;

	do_sethost(u, md->value);
}
