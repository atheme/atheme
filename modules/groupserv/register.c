/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_register(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if (*parv[0] != '!')
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	if (mygroup_find(parv[0]))
	{
		command_fail(si, fault_alreadyexists, _("The group \2%s\2 already exists."), parv[0]);
		return;
	}

	if (strlen(parv[0]) > GROUPLEN)
	{
		command_fail(si, fault_badparams, _("The group name \2%s\2 is invalid."), parv[0]);
		return;
	}

	if (myentity_count_group_flag(entity(si->smu), GA_FOUNDER) > gs_config->maxgroups &&
	    !has_priv(si, PRIV_EXCEED_LIMITS))
	{
		command_fail(si, fault_toomany, _("You have too many groups registered."));
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from registering groups by network staff."));
		return;
	}

	mg = mygroup_add(parv[0]);
	groupacs_add(mg, entity(si->smu), GA_ALL | GA_FOUNDER);
	hook_call_group_register(mg);

	logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2", entity(mg)->name);
	command_success_nodata(si, _("The group \2%s\2 has been registered to \2%s\2."), entity(mg)->name, entity(si->smu)->name);
}

static struct command gs_register = {
	.name           = "REGISTER",
	.desc           = N_("Registers a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_register,
	.help           = { .path = "groupserv/register" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_register);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_register);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/register", MODULE_UNLOAD_CAPABILITY_OK)
