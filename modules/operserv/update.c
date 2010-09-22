/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/update", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_update(sourceinfo_t *si, int parc, char *parv[]);

command_t os_update = { "UPDATE", N_("Flushes services database to disk."), PRIV_ADMIN, 0, os_cmd_update, { .path = "oservice/update" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_update);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_update);
}

void os_cmd_update(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	if (db_save)
		db_save(NULL);
	/* db_save() will wallops/snoop/log the error */
	command_success_nodata(si, _("UPDATE completed."));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
