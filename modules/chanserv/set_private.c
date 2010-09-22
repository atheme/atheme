/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET PRIVATE command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_private", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_private = { "PRIVATE", N_("Hides information about a channel."), AC_NONE, 2, cs_cmd_set_private, { .path = "cservice/set_private" } };

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_private, *cs_set_cmdtree);

	use_channel_private++;
}

void _moddeinit()
{
	command_delete(&cs_set_private, *cs_set_cmdtree);

	use_channel_private--;
}

static void cs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PRIVATE");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_PRIVATE & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for \2%s\2."), "PRIVATE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PRIVATE:ON: \2%s\2", mc->name);

		mc->flags |= MC_PRIVATE;

		command_success_nodata(si, _("The \2%s\2 flag has been set for \2%s\2."), "PRIVATE", mc->name);

		return;
	}

	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_PRIVATE & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for \2%s\2."), "PRIVATE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PRIVATE:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_PRIVATE;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for \2%s\2."), "PRIVATE", mc->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PRIVATE");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
