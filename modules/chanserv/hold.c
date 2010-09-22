/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for channels.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/hold", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_hold(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_hold = { "HOLD", N_("Prevents a channel from expiring."),
			PRIV_HOLD, 2, cs_cmd_hold, { .path = "cservice/hold" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_hold);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_hold);
}

static void cs_cmd_hold(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	mychan_t *mc;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HOLD");
		command_fail(si, fault_needmoreparams, _("Usage: HOLD <#channel> <ON|OFF>"));
		return;
	}

	if (*target != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), target);
		return;
	}
	
	if (!strcasecmp(action, "ON"))
	{
		if (mc->flags & MC_HOLD)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already held."), target);
			return;
		}

		mc->flags |= MC_HOLD;

		wallops("%s set the HOLD option for the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("\2%s\2 is now held."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mc->flags & MC_HOLD))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not held."), target);
			return;
		}

		mc->flags &= ~MC_HOLD;

		wallops("%s removed the HOLD option on the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("\2%s\2 is no longer held."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		command_fail(si, fault_badparams, _("Usage: HOLD <#channel> <ON|OFF>"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
