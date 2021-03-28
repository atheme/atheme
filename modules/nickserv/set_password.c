/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Changes the password associated with your account.
 */

#include <atheme.h>

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET PASSWORD <password>
static void
ns_cmd_set_password(struct sourceinfo *si, int parc, char *parv[])
{
	char *password = parv[0];

	if (auth_module_loaded)
	{
		command_fail(si, fault_noprivs, _("You must change the password in the external system."));
		return;
	}

	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PASSWORD");
		return;
	}

	if (strlen(password) > PASSLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PASSWORD");
		command_fail(si, fault_badparams, _("Registration passwords may not be longer than \2%u\2 characters."), PASSLEN);
		return;
	}

	if (!strcasecmp(password, entity(si->smu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SET PASSWORD <new password>"));
		return;
	}

	struct hook_user_change_password_check hdata = {
		.si         = si,
		.mu         = si->smu,
		.password   = password,
		.allowed    = true,
	};

	(void) hook_call_user_can_change_password(&hdata);

	if (! hdata.allowed)
		return;

	logcommand(si, CMDLOG_SET, "SET:PASSWORD");

	set_password(si->smu, password);

	command_success_nodata(si, _("The password for \2%s\2 has been successfully changed."), entity(si->smu)->name);
}

static struct command ns_set_password = {
	.name           = "PASSWORD",
	.desc           = N_("Changes the password associated with your account."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_password,
	.help           = { .path = "nickserv/set_password" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_password, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_password, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_password", MODULE_UNLOAD_CAPABILITY_OK)
