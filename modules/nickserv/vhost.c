/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting a vhost on an account
 *
 * $Id: vhost.c 8195 2007-04-25 16:27:08Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/vhost", false, _modinit, _moddeinit,
	"$Id: vhost.c 8195 2007-04-25 16:27:08Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree, *ns_helptree;

static void vhost_on_identify(void *vptr);
static void ns_cmd_vhost(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_vhost = { "VHOST", N_("Manages user virtualhosts."), PRIV_USER_VHOST, 2, ns_cmd_vhost };
command_t ns_listvhost = { "LISTVHOST", N_("Lists user virtualhosts."), PRIV_USER_AUSPEX, 1, ns_cmd_listvhost };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	hook_add_event("user_identify");
	hook_add_hook("user_identify", vhost_on_identify);
	command_add(&ns_vhost, ns_cmdtree);
	command_add(&ns_listvhost, ns_cmdtree);
	help_addentry(ns_helptree, "VHOST", "help/nickserv/vhost", NULL);
	help_addentry(ns_helptree, "LISTVHOST", "help/nickserv/listvhost", NULL);
}

void _moddeinit(void)
{
	hook_del_hook("user_identify", vhost_on_identify);
	command_delete(&ns_vhost, ns_cmdtree);
	command_delete(&ns_listvhost, ns_cmdtree);
	help_delentry(ns_helptree, "VHOST");
	help_delentry(ns_helptree, "LISTVHOST");
}


static void do_sethost(user_t *u, char *host)
{
	if (!strcmp(u->vhost, host))
		return;
	strlcpy(u->vhost, host, HOSTLEN);
	sethost_sts(nicksvs.me->me, u, u->vhost);
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		do_sethost(u, host ? host : u->host);
	}
}

/* VHOST <account> [host] */
static void ns_cmd_vhost(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;
	metadata_t *md;
	char *p;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VHOST");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOST <account> [vhost]"));
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		if (!metadata_find(mu, "private:usercloak"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 does not have a vhost set."), mu->name);
			return;
		}
		metadata_delete(mu, "private:usercloak");
		command_success_nodata(si, _("Deleted vhost for \2%s\2."), mu->name);
		snoop("VHOST:REMOVE: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "VHOST REMOVE %s", mu->name);
		do_sethost_all(mu, NULL); // restore user vhost from user host
		return;
	}

	/* Never ever allow @!?* as they have special meaning in all ircds */
	/* Empty, space anywhere and colon at the start break the protocol */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*') || strchr(host, ' ') ||
			*host == ':' || *host == '\0')
	{
		command_fail(si, fault_badparams, _("The vhost provided contains invalid characters."));
		return;
	}
	if (strlen(host) >= HOSTLEN)
	{
		command_fail(si, fault_badparams, _("The vhost provided is too long."));
		return;
	}
	p = strrchr(host, '/');
	if (p != NULL && isdigit(p[1]))
	{
		command_fail(si, fault_badparams, _("The vhost provided looks like a CIDR mask."));
		return;
	}
	if (!is_valid_host(host))
	{
		/* This can be stuff like missing dots too. */
		command_fail(si, fault_badparams, _("The vhost provided is invalid."));
		return;
	}

	md = metadata_find(mu, "private:usercloak");
	if (md != NULL && !strcmp(md->value, host))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has the given vhost set."), mu->name);
		return;
	}

	metadata_add(mu, "private:usercloak", host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to \2%s\2."),
			host, mu->name);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, mu->name, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOST ASSIGN %s %s",
			mu->name, host);
	do_sethost_all(mu, host);
	return;
}

static void ns_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[])
{
	const char *pattern;
	mowgli_patricia_iteration_state_t state;
	myuser_t *mu;
	metadata_t *md;
	int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	snoop("LISTVHOST: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
	MOWGLI_PATRICIA_FOREACH(mu, &state, mulist)
	{
		md = metadata_find(mu, "private:usercloak");
		if (md == NULL)
			continue;
		if (!match(pattern, md->value))
		{
			command_success_nodata(si, "- %-30s %s", mu->name, md->value);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTVHOST %s (%d matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No vhosts matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

static void vhost_on_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	metadata_t *md;

	/* NO CLOAK?!*$*%*&&$(!& */
	if (!(md = metadata_find(mu, "private:usercloak")))
		return;

	do_sethost(u, md->value);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
