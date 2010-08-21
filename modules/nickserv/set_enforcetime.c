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

list_t *ns_set_cmdtree, *ns_helptree; 

static void ns_cmd_set_enforcetime(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_enforcetime = { "ENFORCETIME", N_("Amount of time it takes before nickname protection occurs."), AC_NONE, 1, ns_cmd_set_enforcetime };

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/enforce");
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_set_enforcetime, ns_set_cmdtree);
	help_addentry(ns_helptree, "SET ENFORCETIME", "help/nickserv/set_enforcetime", NULL);
}

void _moddeinit(void)
{
	command_delete(&ns_set_enforcetime, ns_set_cmdtree);
	help_delentry(ns_helptree, "SET ENFORCETIME");
}

static void ns_cmd_set_enforcetime(sourceinfo_t *si, int parc, char *parv[])
{
	metadata_t *md;
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
		if ((md = metadata_find(si->smu, "private:doenforce")) != NULL)
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
		if ((md = metadata_find(si->smu, "private:doenforce")) != NULL)
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
