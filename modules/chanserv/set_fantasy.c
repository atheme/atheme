/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET FANTASY command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_fantasy", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_set_fantasy_config_ready(void *unused);

static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_fantasy = { "FANTASY", N_("Allows or disallows in-channel commands."), AC_NONE, 2, cs_cmd_set_fantasy, { .path = "cservice/set_fantasy" } };

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_fantasy, *cs_set_cmdtree);

	hook_add_event("config_ready");
	hook_add_config_ready(cs_set_fantasy_config_ready);
}

void _moddeinit()
{
	command_delete(&cs_set_fantasy, *cs_set_cmdtree);

	hook_del_config_ready(cs_set_fantasy_config_ready);
}

static void cs_set_fantasy_config_ready(void *unused)
{
	if (chansvs.fantasy)
		cs_set_fantasy.access = NULL;
	else
		cs_set_fantasy.access = AC_DISABLED;
}

static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FANTASY");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "disable_fantasy");

		if (!md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_delete(mc, "disable_fantasy");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "disable_fantasy");

		if (md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_add(mc, "disable_fantasy", "on");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FANTASY");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
