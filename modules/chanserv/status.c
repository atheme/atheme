/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService STATUS function.
 */

#include <atheme.h>

static void
cs_cmd_status(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];

	if (chan)
	{
		struct mychan *mc = mychan_find(chan);
		unsigned int flags;

		if (*chan != '#')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "STATUS");
			return;
		}

		if (!mc)
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
			return;
		}

		logcommand(si, CMDLOG_GET, "STATUS: \2%s\2", mc->name);

		if (metadata_find(mc, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
			return;
		}

		flags = chanacs_source_flags(mc, si);
		if (flags & CA_AKICK && !(flags & CA_EXEMPT))
			command_success_nodata(si, _("You are banned from \2%s\2."), mc->name);
		else if (flags != 0)
		{
			command_success_nodata(si, _("You have access flags \2%s\2 on \2%s\2."), bitmask_to_flags(flags), mc->name);
		}
		else
			command_success_nodata(si, _("You have no special access to \2%s\2."), mc->name);

		return;
	}

	logcommand(si, CMDLOG_GET, "STATUS");
	if (!si->smu)
		command_success_nodata(si, STR_NOT_LOGGED_IN);
	else
	{
		command_success_nodata(si, _("You are logged in as \2%s\2."), entity(si->smu)->name);

		if (is_soper(si->smu))
		{
			struct soper *soper = si->smu->soper;

			command_success_nodata(si, _("You are a services operator of class %s."), soper->operclass ? soper->operclass->name : soper->classname);
		}
	}

	if (si->su != NULL)
	{
		struct mynick *mn;

		mn = mynick_find(si->su->nick);
		if (mn != NULL && mn->owner != si->smu &&
				myuser_access_verify(si->su, mn->owner))
			command_success_nodata(si, _("You are recognized as \2%s\2."), entity(mn->owner)->name);
	}

	if (si->su != NULL && is_admin(si->su))
		command_success_nodata(si, _("You are a server administrator."));

	if (si->su != NULL && is_ircop(si->su))
		command_success_nodata(si, _("You are an IRC operator."));
}

static struct command cs_status = {
	.name           = "STATUS",
	.desc           = N_("Displays your status in services."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_status,
	.help           = { .path = "cservice/status" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_status);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_status);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/status", MODULE_UNLOAD_CAPABILITY_OK)
