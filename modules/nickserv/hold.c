/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for nicknames.
 *
 * $Id: hold.c 7185 2006-11-17 21:02:46Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 7185 2006-11-17 21:02:46Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_hold(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_hold = {
	"HOLD",
	"Prevents an account from expiring.",
	PRIV_HOLD,
	2,
	ns_cmd_hold
};

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_hold, ns_cmdtree);
	help_addentry(ns_helptree, "HOLD", "help/nickserv/hold", NULL);
}

void _moddeinit()
{
	command_delete(&ns_hold, ns_cmdtree);
	help_delentry(ns_helptree, "HOLD");
}

static void ns_cmd_hold(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	myuser_t *mu;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HOLD");
		command_fail(si, fault_needmoreparams, "Usage: HOLD <account> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mu->flags & MU_HOLD)
		{
			command_fail(si, fault_badparams, "\2%s\2 is already held.", target);
			return;
		}

		mu->flags |= MU_HOLD;

		wallops("%s set the HOLD option for the account \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD %s ON", target);
		command_success_nodata(si, "\2%s\2 is now held.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mu->flags & MU_HOLD))
		{
			command_fail(si, fault_badparams, "\2%s\2 is not held.", target);
			return;
		}

		mu->flags &= ~MU_HOLD;

		wallops("%s removed the HOLD option on the account \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD %s OFF", target);
		command_success_nodata(si, "\2%s\2 is no longer held.", target);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "HOLD");
		command_fail(si, fault_needmoreparams, "Usage: HOLD <account> <ON|OFF>");
	}
}
