/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting a vhost on an account
 *
 * $Id: vhost.c 6955 2006-10-26 11:37:10Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/vhost", FALSE, _modinit, _moddeinit,
	"$Id: vhost.c 6955 2006-10-26 11:37:10Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree, *ns_helptree;

static void vhost_on_identify(void *vptr);
static void ns_cmd_vhost(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_vhost = { "VHOST", "Manages user virtualhosts.", PRIV_USER_VHOST, 2, ns_cmd_vhost };

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

/* VHOST <nick> [host] */
static void ns_cmd_vhost(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;
	char *p;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOST");
		command_fail(si, fault_needmoreparams, "Syntax: VHOST <nick> [vhost]");
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		metadata_delete(mu, METADATA_USER, "private:usercloak");
		command_success_nodata(si, "Deleted vhost for \2%s\2.", mu->name);
		snoop("VHOST:REMOVE: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "VHOST REMOVE %s", mu->name);
		do_restorehost_all(mu);
		return;
	}

	/* Never ever allow @!?* as they have special meaning in all ircds */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*'))
	{
		command_fail(si, fault_badparams, "The vhost provided contains invalid characters.");
		return;
	}
	if (strlen(host) >= HOSTLEN)
	{
		command_fail(si, fault_badparams, "The vhost provided is too long.");
		return;
	}
	p = strrchr(host, '/');
	if (p != NULL && isdigit(p[1]))
	{
		command_fail(si, fault_badparams, "The vhost provided looks like a CIDR mask.");
		return;
	}
	/* XXX more checks here, perhaps as a configurable regexp? */

	metadata_add(mu, METADATA_USER, "private:usercloak", host);
	command_success_nodata(si, "Assigned vhost \2%s\2 to \2%s\2.",
			host, mu->name);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, mu->name, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOST ASSIGN %s %s",
			mu->name, host);
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
