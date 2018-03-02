/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 */

#include "atheme.h"

static void os_cmd_update(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_update = { "UPDATE", N_("Flushes services database to disk."), PRIV_ADMIN, 0, os_cmd_update, { .path = "oservice/update" } };

void
os_cmd_update(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	command_success_nodata(si, _("Updating database."));
	if (db_save)
		db_save(NULL, DB_SAVE_BG_IMPORTANT);
	/* db_save() will wallops/snoop/log the error */
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
        service_named_bind_command("operserv", &os_update);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_update);
}

SIMPLE_DECLARE_MODULE_V1("operserv/update", MODULE_UNLOAD_CAPABILITY_OK)
