/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET GUARD command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_guard", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_set_guard_config_ready(void *unused);

static void cs_cmd_set_guard(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_guard = { "GUARD", N_("Sets whether or not services will inhabit the channel."), AC_NONE, 2, cs_cmd_set_guard, { .path = "cservice/set_guard" } };

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_guard, *cs_set_cmdtree);

	hook_add_event("config_ready");
	hook_add_config_ready(cs_set_guard_config_ready);
}

void _moddeinit()
{
	command_delete(&cs_set_guard, *cs_set_cmdtree);

	hook_del_config_ready(cs_set_guard_config_ready);
}

static void cs_set_guard_config_ready(void *unused)
{
	if (config_options.join_chans)
		cs_set_guard.access = NULL;
	else
		cs_set_guard.access = PRIV_ADMIN;
}

static void cs_cmd_set_guard(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET GUARD");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_GUARD & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "GUARD", mc->name);
			return;
		}
		if (metadata_find(mc, "private:botserv:bot-assigned") &&
				module_find_published("botserv/main"))
		{
			command_fail(si, fault_noprivs, _("Channel \2%s\2 already has a BotServ bot assigned to it.  You need to unassign it first."), mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:GUARD:ON: \2%s\2", mc->name);

		mc->flags |= MC_GUARD;

		if (!(mc->flags & MC_INHABIT))
			join(mc->name, chansvs.nick);

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "GUARD", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_GUARD & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "GUARD", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:GUARD:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_GUARD;

		if (mc->chan != NULL && !(mc->flags & MC_INHABIT) && (!config_options.chan || irccasecmp(config_options.chan, mc->name)) && !(mc->chan->flags & CHAN_LOG))
			part(mc->name, chansvs.nick);

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "GUARD", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "GUARD");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
