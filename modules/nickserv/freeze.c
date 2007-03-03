/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Gives services the ability to freeze nicknames
 *
 * $Id: freeze.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/freeze", FALSE, _modinit, _moddeinit,
	"$Id: freeze.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_freeze(sourceinfo_t *si, int parc, char *parv[]);

/* FREEZE ON|OFF -- don't pollute the root with THAW */
command_t ns_freeze = { "FREEZE", "Freezes an account.", PRIV_USER_ADMIN, 3, ns_cmd_freeze };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_freeze, ns_cmdtree);
	help_addentry(ns_helptree, "FREEZE", "help/nickserv/freeze", NULL);
}

void _moddeinit()
{
	command_delete(&ns_freeze, ns_cmdtree);
	help_delentry(ns_helptree, "FREEZE");
}

static void ns_cmd_freeze(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *target = parv[0];
	char *action = parv[1];
	char *reason = parv[2];

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
		command_fail(si, fault_needmoreparams, "Usage: FREEZE <nickname> <ON|OFF> [reason]");
		return;
	}

	mu = myuser_find_ext(target);

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
			command_fail(si, fault_needmoreparams, "Usage: FREEZE <nickname> ON <reason>");
			return;
		}

		if (is_soper(mu))
		{
			command_fail(si, fault_badparams, "The account \2%s\2 belongs to a services operator; it cannot be frozen.", target);
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			command_fail(si, fault_badparams, "\2%s\2 is already frozen.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:freeze:freezer", get_oper_name(si));
		metadata_add(mu, METADATA_USER, "private:freeze:reason", reason);
		metadata_add(mu, METADATA_USER, "private:freeze:timestamp", itoa(CURRTIME));

		wallops("%s froze the account \2%s\2 (%s).", get_oper_name(si), target, reason);
		logcommand(si, CMDLOG_ADMIN, "FREEZE %s ON", target);
		command_success_nodata(si, "\2%s\2 is now frozen.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			command_fail(si, fault_badparams, "\2%s\2 is not frozen.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:freeze:freezer");
		metadata_delete(mu, METADATA_USER, "private:freeze:reason");
		metadata_delete(mu, METADATA_USER, "private:freeze:timestamp");

		wallops("%s thawed the account \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "FREEZE %s OFF", target);
		command_success_nodata(si, "\2%s\2 has been thawed", target);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
		command_fail(si, fault_needmoreparams, "Usage: FREEZE <account> <ON|OFF> [reason]");
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
