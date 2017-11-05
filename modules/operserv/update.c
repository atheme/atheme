/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 */

#include "atheme.h"

SIMPLE_DECLARE_MODULE_V1("operserv/update", MODULE_UNLOAD_CAPABILITY_OK,
                         _modinit, _moddeinit);

static void os_cmd_update(sourceinfo_t *si, int parc, char *parv[]);

command_t os_update = { "UPDATE", N_("Flushes services database to disk."), PRIV_ADMIN, 0, os_cmd_update, { .path = "oservice/update" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_update);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_update);
}

void os_cmd_update(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	command_success_nodata(si, _("Updating database."));
	if (db_save)
		db_save(NULL, DB_SAVE_BG_IMPORTANT);
	/* db_save() will wallops/snoop/log the error */
}
