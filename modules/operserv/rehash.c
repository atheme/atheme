/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the OService REHASH command.
 */

#include <atheme.h>

static void
os_cmd_rehash(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_INFO, "UPDATE (due to REHASH): \2%s\2", get_oper_name(si));
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	if (db_save)
		db_save(NULL, DB_SAVE_BG_IMPORTANT);

	logcommand(si, CMDLOG_ADMIN, "REHASH");
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, get_oper_name(si));

	if (conf_rehash())
		command_success_nodata(si, _("REHASH completed."));
	else
		command_fail(si, fault_nosuch_target, _("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);
}

static struct command os_rehash = {
	.name           = "REHASH",
	.desc           = N_("Reload the configuration data."),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_rehash,
	.help           = { .path = "oservice/rehash" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_rehash);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_rehash);
}

SIMPLE_DECLARE_MODULE_V1("operserv/rehash", MODULE_UNLOAD_CAPABILITY_OK)
