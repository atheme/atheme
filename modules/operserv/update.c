/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the OService UPDATE command.
 */

#include <atheme.h>

static void
os_cmd_update(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	command_success_nodata(si, _("Updating database."));

	if (db_save)
		db_save(NULL, DB_SAVE_BG_IMPORTANT);

	// db_save() will wallops/snoop/log the error
}

static struct command os_update = {
	.name           = "UPDATE",
	.desc           = N_("Flushes services database to disk."),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_update,
	.help           = { .path = "oservice/update" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_update);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_update);
}

SIMPLE_DECLARE_MODULE_V1("operserv/update", MODULE_UNLOAD_CAPABILITY_OK)
