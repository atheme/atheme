/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_register_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGISTER");
		(void) command_fail(si, fault_badparams, _("Syntax: REGISTER <!group>"));
		return;
	}

	if (strlen(group) > GROUPLEN)
	{
		(void) command_fail(si, fault_badparams, _("The group name \2%s\2 is too long."), group);
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		(void) command_fail(si, fault_notverified, _("You need to verify your email address before you "
		                                             "may register groups."));
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		(void) command_fail(si, fault_noprivs, _("You have been restricted from registering groups by "
		                                         "network staff."));
		return;
	}

	const unsigned int founders = gcsyms->myentity_count_group_flag(entity(si->smu), GA_FOUNDER);

	if (founders > gcsyms->config->maxgroups && ! has_priv(si, PRIV_REG_NOLIMIT))
	{
		(void) command_fail(si, fault_toomany, _("You have too many groups registered."));
		return;
	}

	if (gcsyms->mygroup_find(group))
	{
		(void) command_fail(si, fault_alreadyexists, _("The group \2%s\2 already exists."), group);
		return;
	}

	struct mygroup *const mg = gcsyms->mygroup_add(group);

	(void) gcsyms->groupacs_add(mg, entity(si->smu), (GA_ALL | GA_FOUNDER));
	(void) hook_call_group_register(mg);

	(void) logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2", group);
	(void) command_success_nodata(si, _("The group \2%s\2 has been registered to \2%s\2."), group,
	                              entity(si->smu)->name);
}

static struct command gs_cmd_register = {
	.name           = "REGISTER",
	.desc           = N_("Registers a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_register_func,
	.help           = { .path = "groupserv/register" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_register);

	(void) hook_add_event("group_register");
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_register);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/register", MODULE_UNLOAD_CAPABILITY_OK)
