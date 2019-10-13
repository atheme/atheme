/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Stephen Bennett
 * Copyright (C) 2008 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the OService IDENTIFY command.
 */

#include <atheme.h>

static bool
verify_operserv_password(struct soper *so, char *password)
{
	if (so == NULL || password == NULL)
		return false;

	return crypt_verify_password(password, so->password, NULL) != NULL;
}

static void
os_cmd_identify(struct sourceinfo *si, int parc, char *parv[])
{
	// XXX use this with authcookie also
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "IDENTIFY");
		return;
	}

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IDENTIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: IDENTIFY <password>"));
		return;
	}

	if (!is_soper(si->smu))
	{
		command_fail(si, fault_noprivs, _("You do not have a services operator account."));
		return;
	}

	if (si->smu->soper->password == NULL)
	{
		command_fail(si, fault_nochange, _("Your services operator account does not have a password set."));
		return;
	}

	if (si->su->flags & UF_SOPER_PASS)
	{
		command_fail(si, fault_nochange, _("You are already identified to %s."), si->service->nick);
		return;
	}

	if (!verify_operserv_password(si->smu->soper, parv[0]))
	{
		command_fail(si, fault_authfail, _("Invalid password."));
		logcommand(si, CMDLOG_ADMIN, "failed IDENTIFY (bad password)");
		return;
	}

	si->su->flags |= UF_SOPER_PASS;
	command_success_nodata(si, _("You are now identified to %s."),
			si->service->nick);
	logcommand(si, CMDLOG_ADMIN, "IDENTIFY");
}

static struct command os_identify = {
	.name           = "IDENTIFY",
	.desc           = N_("Authenticate for services operator privileges."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &os_cmd_identify,
	.help           = { .path = "oservice/identify" },
};

static struct command os_id = {
	.name           = "ID",
	.desc           = N_("Alias for IDENTIFY"),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &os_cmd_identify,
	.help           = { .path = "oservice/identify" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_identify);
        service_named_bind_command("operserv", &os_id);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_identify);
	service_named_unbind_command("operserv", &os_id);
}

SIMPLE_DECLARE_MODULE_V1("operserv/identify", MODULE_UNLOAD_CAPABILITY_OK)
