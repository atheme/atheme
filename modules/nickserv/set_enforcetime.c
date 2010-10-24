/*
 * Copyright (c) 2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ SET ENFORCETIMEOUT function.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set_enforcetime",false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_enforcetime(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_enforcetime = { "ENFORCETIME", N_("Amount of time it takes before nickname protection occurs."), AC_NONE, 1, ns_cmd_set_enforcetime, { .path = "nickserv/set_enforcetime" } };

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/enforce");
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_enforcetime, *ns_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_set_enforcetime, *ns_set_cmdtree);
}

static void ns_cmd_set_enforcetime(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCETIME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCETIME TIME|DEFAULT"));
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	int enforcetime = atoi(parv[0]);

	if (strcasecmp(setting, "DEFAULT") == 0)
	{
		if (metadata_find(si->smu, "private:doenforce"))
		{
			logcommand(si, CMDLOG_SET, "SET:ENFORCETIME:DEFAULT");
			metadata_delete(si->smu, "private:enforcetime");
			command_success_nodata(si, _("The \2%s\2 for account \2%s\2 has been reset to default, which is \2%d\2 seconds."), "ENFORCETIME", entity(si->smu)->name, nicksvs.enforce_delay);
		}
		else
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		}
	}
	else if (enforcetime > 0 && enforcetime <= 180)
	{
		if (metadata_find(si->smu, "private:doenforce"))
		{
			logcommand(si, CMDLOG_SET, "SET:ENFORCETIME: %d", enforcetime);
			metadata_add(si->smu, "private:enforcetime", setting);
			command_success_nodata(si, _("The \2%s\2 for account \2%s\2 has been set to \2%d\2 seconds."), "ENFORCETIME", entity(si->smu)->name, enforcetime);
		}
		else
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ENFORCETIME");
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
