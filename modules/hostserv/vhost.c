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
	"hostserv/vhost", false, _modinit, _moddeinit,
	"$Id: vhost.c 8195 2007-04-25 16:27:08Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *hs_cmdtree, *hs_helptree;

static void hs_cmd_vhostnick(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_vhost(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_group(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_vhostnick = { "VHOSTNICK", N_("Manages per-nick virtual hosts."), PRIV_USER_VHOST, 2, hs_cmd_vhostnick };
command_t hs_vhost = { "VHOST", N_("Manages per-account virtual hosts."), PRIV_USER_VHOST, 2, hs_cmd_vhost };
command_t hs_listvhost = { "LISTVHOST", N_("Lists user virtual hosts."), PRIV_USER_AUSPEX, 1, hs_cmd_listvhost };
command_t hs_group = { "GROUP", N_("Syncs the vhost for all nicks in a group."), AC_NONE, 1, hs_cmd_group };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");

	command_add(&hs_vhostnick, hs_cmdtree);
	command_add(&hs_vhost, hs_cmdtree);
	command_add(&hs_listvhost, hs_cmdtree);
	command_add(&hs_group, hs_cmdtree);
	help_addentry(hs_helptree, "VHOSTNICK", "help/hostserv/vhostnick", NULL);
	help_addentry(hs_helptree, "VHOST", "help/hostserv/vhost", NULL);
	help_addentry(hs_helptree, "LISTVHOST", "help/hostserv/listvhost", NULL);
	help_addentry(hs_helptree, "GROUP", "help/hostserv/group", NULL);
}

void _moddeinit(void)
{
	command_delete(&hs_vhostnick, hs_cmdtree);
	command_delete(&hs_vhost, hs_cmdtree);
	command_delete(&hs_listvhost, hs_cmdtree);
	command_delete(&hs_group, hs_cmdtree);
	help_delentry(hs_helptree, "VHOSTNICK");
	help_delentry(hs_helptree, "VHOST");
	help_delentry(hs_helptree, "LISTVHOST");
	help_delentry(hs_helptree, "GROUP");
}


static void do_sethost(user_t *u, char *host)
{
	if (!strcmp(u->vhost, host ? host : u->host))
		return;
	strlcpy(u->vhost, host ? host : u->host, HOSTLEN);
	sethost_sts(hostsvs.me->me, u, u->vhost);
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		do_sethost(u, host);
	}
}

/* VHOSTNICK <nick> [host] */
static void hs_cmd_vhostnick(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;
	user_t *u;
	metadata_t *md;
	char *p;
	char buf[BUFSIZE];
	node_t *n;
	int found = 0;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOSTNICK");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOSTNICK <nick> [vhost]"));
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	LIST_FOREACH(n, mu->nicks.head)
	{
		if (!irccasecmp(((mynick_t *)(n->data))->nick, target))
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((mynick_t *)(n->data))->nick);
			found++;
		}
	}

	if (!found)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid target."), target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		metadata_delete(mu, buf);
		command_success_nodata(si, _("Deleted vhost for \2%s\2."), target);
		snoop("VHOSTNICK:REMOVE: \2%s\2 by \2%s\2", target, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "VHOSTNICK REMOVE %s", target);
		u = user_find_named(target);
		if (u != NULL)
		{
			/* Revert to account's vhost */
			md = metadata_find(mu, "private:usercloak");
			do_sethost(u, md ? md->value : NULL);
		}
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

	metadata_add(mu, buf, host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to \2%s\2."),
			host, target);
	snoop("VHOSTNICK:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, target, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOSTNICK ASSIGN %s %s",
			target, host);
	u = user_find_named(target);
	if (u != NULL)
		do_sethost(u, host);
	return;
}

static void hs_sethost_all(myuser_t *mu, const char *host)
{
	node_t *n;
	mynick_t *mn;
	char buf[BUFSIZE];

	LIST_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
		metadata_delete(mu, buf);
	}
	if (host != NULL)
		metadata_add(mu, "private:usercloak", host);
	else
		metadata_delete(mu, "private:usercloak");
}

/* VHOST <nick> [host] */
static void hs_cmd_vhost(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;
	char *p;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOST");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOST <nick> [vhost]"));
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
		hs_sethost_all(mu, NULL);
		command_success_nodata(si, _("Deleted all vhosts for \2%s\2."), mu->name);
		snoop("VHOST:REMOVE: \2%s\2 by \2%s\2", target, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "VHOST REMOVE %s", target);
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

	hs_sethost_all(mu, host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to all nicks in account \2%s\2."),
			host, mu->name);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, target, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOST ASSIGN %s %s",
			target, host);
	do_sethost_all(mu, host);
	return;
}

static void hs_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[])
{
	const char *pattern;
	mowgli_patricia_iteration_state_t state;
	myuser_t *mu;
	metadata_t *md;
	node_t *n;
	char buf[BUFSIZE];
	int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	snoop("LISTVHOST: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
	MOWGLI_PATRICIA_FOREACH(mu, &state, mulist)
	{
		md = metadata_find(mu, "private:usercloak");
		if (md != NULL && !match(pattern, md->value))
		{
			command_success_nodata(si, "- %-30s %s", mu->name, md->value);
			matches++;
		}
		LIST_FOREACH(n, mu->nicks.head)
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((mynick_t *)(n->data))->nick);
			md = metadata_find(mu, buf);
			if (md == NULL)
				continue;
			if (!match(pattern, md->value))
			{
				command_success_nodata(si, "- %-30s %s", ((mynick_t *)(n->data))->nick, md->value);
				matches++;
			}
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTVHOST %s (%d matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No vhosts matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

static void hs_cmd_group(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	metadata_t *md;
	char buf[BUFSIZE];

	/* This is only because we need a nick to copy from. */
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "GROUP");
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), si->su->nick);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("Please contact an Operator to get a vhost assigned to this nick."));
		return;
	}
	strlcpy(buf, md->value, sizeof buf);
	hs_sethost_all(si->smu, buf);
	do_sethost_all(si->smu, buf);
	command_success_nodata(si, _("All vhosts in the group \2%s\2 have been set to \2%s\2."), si->smu->name, buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
