/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService REHASH command.
 *
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"operserv/rehash", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_rehash(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rehash = { "REHASH", N_("Reload the configuration data."), PRIV_ADMIN, 0, os_cmd_rehash, { .path = "oservice/rehash" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_rehash);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_rehash);
}

/* REHASH */
void os_cmd_rehash(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "UPDATE (due to REHASH): \2%s\2", get_oper_name(si));
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	if (db_save)
		db_save(NULL);

	logcommand(si, CMDLOG_ADMIN, "REHASH");
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, get_oper_name(si));

	if (conf_rehash())
		command_success_nodata(si, _("REHASH completed."));
	else
		command_fail(si, fault_nosuch_target, _("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
