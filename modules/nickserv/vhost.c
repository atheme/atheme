/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * VHost management! (ratbox only right now.)
 *
 * $Id: vhost.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/vhost", FALSE, _modinit, _moddeinit,
	"$Id: vhost.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree, *ns_helptree;

static void vhost_on_identify(void *vptr);
static void ns_cmd_vhost(char *origin);

command_t ns_vhost = { "VHOST", "Manages user virtualhosts.",
			PRIV_USER_VHOST, ns_cmd_vhost };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	hook_add_event("user_identify");
	hook_add_hook("user_identify", vhost_on_identify);
	command_add(&ns_vhost, ns_cmdtree);
	help_addentry(ns_helptree, "VHOST", "help/nickserv/vhost", NULL);
}

void _moddeinit(void)
{
	hook_del_hook("user_identify", vhost_on_identify);
	command_delete(&ns_vhost, ns_cmdtree);
	help_delentry(ns_helptree, "VHOST");
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;
	char luhost[BUFSIZE];

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		if (!strcmp(u->vhost, host))
			continue;
		strlcpy(u->vhost, host, HOSTLEN);
		notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
			u->vhost);
		sethost_sts(nicksvs.nick, u->nick, u->vhost);
		strlcpy(luhost, u->user, BUFSIZE);
		strlcat(luhost, "@", BUFSIZE);
		strlcat(luhost, u->vhost, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:vhost", luhost);
	}
}

static void do_sethost(user_t *u, char *host)
{
	char luhost[BUFSIZE];

	if (!strcmp(u->vhost, host))
		return;
	strlcpy(u->vhost, host, HOSTLEN);
	notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
		u->vhost);
	sethost_sts(nicksvs.nick, u->nick, u->vhost);
	strlcpy(luhost, u->user, BUFSIZE);
	strlcat(luhost, "@", BUFSIZE);
	strlcat(luhost, u->vhost, BUFSIZE);
	metadata_add(u->myuser, METADATA_USER, "private:host:vhost", luhost);
}

static void do_restorehost_all(myuser_t *mu)
{
	node_t *n;
	user_t *u;
	char luhost[BUFSIZE];

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		if (!strcmp(u->vhost, u->host))
			continue;
		strlcpy(u->vhost, u->host, HOSTLEN);
		notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
			u->host);
		sethost_sts(nicksvs.nick, u->nick, u->host);
		strlcpy(luhost, u->user, BUFSIZE);
		strlcat(luhost, "@", BUFSIZE);
		strlcat(luhost, u->host, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:vhost", luhost);
	}
}

static void do_restorehost(user_t *u)
{
	char luhost[BUFSIZE];

	if (!strcmp(u->vhost, u->host))
		return;
	strlcpy(u->vhost, u->host, HOSTLEN);
	notice(nicksvs.nick, u->nick, "Setting your host to \2%s\2.",
		u->host);
	sethost_sts(nicksvs.nick, u->nick, u->host);
	strlcpy(luhost, u->user, BUFSIZE);
	strlcat(luhost, "@", BUFSIZE);
	strlcat(luhost, u->host, BUFSIZE);
	metadata_add(u->myuser, METADATA_USER, "private:host:vhost", luhost);
}

/* VHOST <nick> [host] */
static void ns_cmd_vhost(char *origin)
{
	char *target = strtok(NULL, " ");
	char *host = strtok(NULL, " ");
	node_t *n;
	user_t *source = user_find_named(origin);
	myuser_t *mu;

	if (source == NULL)
		return;

	if (!target)
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "VHOST");
		notice(nicksvs.nick, origin, "Syntax: VHOST <nick> [vhost]");
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
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
		logcommand(nicksvs.me, source, CMDLOG_ADMIN, "VHOST REMOVE %s",
				target);
		do_restorehost_all(mu);
		return;
	}

	/* Never ever allow @!?* as they have special meaning in all ircds */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*'))
	{
		notice(nicksvs.nick, origin, "The vhost provided contains invalid characters.");
		return;
	}
	if (strlen(host) >= HOSTLEN)
	{
		notice(nicksvs.nick, origin, "The vhost provided is too long.");
		return;
	}
	/* XXX more checks here, perhaps as a configurable regexp? */

	metadata_add(mu, METADATA_USER, "private:usercloak", host);
	notice(nicksvs.nick, origin, "Assigned vhost \2%s\2 to \2%s\2.", 
		host, target);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, target, origin);
	logcommand(nicksvs.me, source, CMDLOG_ADMIN, "VHOST ASSIGN %s %s",
			target, host);
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
