/*
 * Copyright (c) 2006 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GROUP command.
 *
 * $Id: group.c 7855 2007-03-06 00:43:08Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/group", FALSE, _modinit, _moddeinit,
	"$Id: group.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_group(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_ungroup(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_fungroup(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_group = { "GROUP", N_("Adds a nickname to your account."), AC_NONE, 0, ns_cmd_group };
command_t ns_ungroup = { "UNGROUP", N_("Removes a nickname from your account."), AC_NONE, 1, ns_cmd_ungroup };
command_t ns_fungroup = { "FUNGROUP", N_("Forces removal of a nickname from an account."), PRIV_USER_ADMIN, 1, ns_cmd_fungroup };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_group, ns_cmdtree);
	help_addentry(ns_helptree, "GROUP", "help/nickserv/group", NULL);
	command_add(&ns_ungroup, ns_cmdtree);
	help_addentry(ns_helptree, "UNGROUP", "help/nickserv/ungroup", NULL);
	command_add(&ns_fungroup, ns_cmdtree);
	help_addentry(ns_helptree, "FUNGROUP", "help/nickserv/fungroup", NULL);
}

void _moddeinit()
{
	command_delete(&ns_group, ns_cmdtree);
	help_delentry(ns_helptree, "GROUP");
	command_delete(&ns_ungroup, ns_cmdtree);
	help_delentry(ns_helptree, "UNGROUP");
	command_delete(&ns_fungroup, ns_cmdtree);
	help_delentry(ns_helptree, "FUNGROUP");
}

static void ns_cmd_group(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, "\2%s\2 can only be executed via IRC.", "GROUP");
		return;
	}

	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, "Nickname ownership is disabled.");
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (LIST_LENGTH(&si->smu->nicks) >= me.maxnicks && !has_priv(si, PRIV_REG_NOLIMIT))
	{
		command_fail(si, fault_noprivs, "You have too many nicks registered already.");
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn != NULL)
	{
		if (mn->owner == si->smu)
			command_fail(si, fault_nochange, "Nick \2%s\2 is already registered to your account.", mn->nick);
		else
			command_fail(si, fault_alreadyexists, "Nick \2%s\2 is already registered to \2%s\2.", mn->nick, mn->owner->name);
		return;
	}

	if (IsDigit(si->su->nick[0]))
	{
		command_fail(si, fault_badparams, "For security reasons, you can't register your UID.");
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "GROUP");
	snoop("GROUP: \2%s\2 to \2%s\2", si->su->nick, si->smu->name);
	mn = mynick_add(si->smu, si->su->nick);
	mn->registered = CURRTIME;
	mn->lastseen = CURRTIME;
	command_success_nodata(si, "Nick \2%s\2 is now registered to your account.", mn->nick);
}

static void ns_cmd_ungroup(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	char *target;

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (parc >= 1)
		target = parv[0];
	else if (si->su != NULL)
		target = si->su->nick;
	else
		target = "?";

	mn = mynick_find(target);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, "Nick \2%s\2 is not registered.", target);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, "Nick \2%s\2 is not registered to your account.", mn->nick);
		return;
	}
	if (!irccasecmp(mn->nick, si->smu->name))
	{
		command_fail(si, fault_noprivs, "Nick \2%s\2 is your account name; you may not remove it.", mn->nick);
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "UNGROUP %s", mn->nick);
	snoop("UNGROUP: \2%s\2 by \2%s\2", target, get_source_name(si));
	command_success_nodata(si, "Nick \2%s\2 has been removed from your account.", mn->nick);
	object_unref(mn);
}

static void ns_cmd_fungroup(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	myuser_t *mu;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FUNGROUP");
		command_fail(si, fault_needmoreparams, "Syntax: FUNGROUP <nickname>");
		return;
	}

	mn = mynick_find(parv[0]);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, "Nick \2%s\2 is not registered.", parv[0]);
		return;
	}
	mu = mn->owner;
	if (!irccasecmp(mn->nick, mu->name))
	{
		command_fail(si, fault_noprivs, "Nick \2%s\2 is an account name; you may not remove it.", mn->nick);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "FUNGROUP %s", mn->nick);
	wallops("%s dropped the nick \2%s\2 from %s",
			get_oper_name(si), mn->nick, mu->name);
	snoop("FUNGROUP: \2%s\2 from \2%s\2 by \2%s\2",
			mn->nick, mu->name, get_source_name(si));
	command_success_nodata(si, "Nick \2%s\2 has been removed from account \2%s\2.", mn->nick, mu->name);
	object_unref(mn);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
