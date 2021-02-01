/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService DROP function.
 */

#include <atheme.h>

static void
cmd_cs_drop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const name = parv[0];
	const char *const key = parv[1];

	if (! name)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <#channel>"));
		return;
	}

	if (*name != '#')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <#channel>"));
		return;
	}

	struct mychan *mc;

	if (! (mc = mychan_find(name)))
	{
		(void) command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (si->c)
	{
		(void) command_fail(si, fault_noprivs, _("For security reasons, you may not drop a channel "
		                                         "registration with a fantasy command."));
		return;
	}

	if (! is_founder(mc, entity(si->smu)))
	{
		(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2 failed to drop (closed)", mc->name);
		(void) command_fail(si, fault_noprivs, _("The channel \2%s\2 is closed; it cannot be dropped."),
		                    mc->name);
		return;
	}

	if (mc->flags & MC_HOLD)
	{
		(void) command_fail(si, fault_noprivs, _("The channel \2%s\2 is held; it cannot be dropped."),
		                    mc->name);
		return;
	}

	if (si->su)
	{
		const char *const challenge = create_weak_challenge(si, mc->name);

		if (! challenge)
		{
			(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
			return;
		}
		if (! key)
		{
			(void) command_success_nodata(si, _("This is a friendly reminder that you are about to "
			                                    "\2DESTROY\2 the channel \2%s\2."), mc->name);

			(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation "
			                                    "has to be confirmed. Please confirm by replying with "
			                                    "\2/msg %s DROP %s %s\2"), chansvs.me->disp, mc->name,
			                                    challenge);
			return;
		}
		if (strcmp(challenge, key) != 0)
		{
			(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
			return;
		}
	}

	(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", mc->name);

	(void) hook_call_channel_drop(mc);

	if (mc->chan && ! (mc->chan->flags & CHAN_LOG))
		(void) part(mc->name, chansvs.nick);

	(void) command_success_nodata(si, _("The channel \2%s\2 has been dropped."), mc->name);
	(void) atheme_object_unref(mc);
}

static void
cmd_cs_fdrop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const name = parv[0];

	if (! name)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FDROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: FDROP <#channel>"));
		return;
	}

	if (*name != '#')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FDROP");
		(void) command_fail(si, fault_badparams, _("Syntax: FDROP <#channel>"));
		return;
	}

	struct mychan *mc;

	if (! (mc = mychan_find(name)))
	{
		(void) command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (si->c)
	{
		(void) command_fail(si, fault_noprivs, _("For security reasons, you may not drop a channel "
		                                         "registration with a fantasy command."));
		return;
	}

	if (mc->flags & MC_HOLD)
	{
		(void) command_fail(si, fault_noprivs, _("The channel \2%s\2 is held; it cannot be dropped."),
		                    mc->name);
		return;
	}

	(void) logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", mc->name);
	(void) wallops("\2%s\2 dropped the channel \2%s\2", get_oper_name(si), mc->name);

	(void) hook_call_channel_drop(mc);

	if (mc->chan && ! (mc->chan->flags & CHAN_LOG))
		(void) part(mc->name, chansvs.nick);

	(void) command_success_nodata(si, _("The channel \2%s\2 has been dropped."), mc->name);
	(void) atheme_object_unref(mc);
}

static struct command cmd_cs_drop = {
	.name           = "DROP",
	.desc           = N_("Drops a channel registration."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cmd_cs_drop_func,
	.help           = { .path = "cservice/drop" },
};

static struct command cmd_cs_fdrop = {
	.name           = "FDROP",
	.desc           = N_("Forces dropping of a channel registration."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 1,
	.cmd            = &cmd_cs_fdrop_func,
	.help           = { .path = "cservice/fdrop" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	(void) service_named_bind_command("chanserv", &cmd_cs_drop);
	(void) service_named_bind_command("chanserv", &cmd_cs_fdrop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cmd_cs_drop);
	(void) service_named_unbind_command("chanserv", &cmd_cs_fdrop);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
